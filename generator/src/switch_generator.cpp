#include "switch_generator.hpp"

#include "doc_comment.hpp"
#include "names.hpp"

#include <algorithm>
#include <set>

namespace eolib::generator
{

SwitchGenerator::SwitchGenerator(TypeRegistry& types, const ProtocolFile& file, const std::string& qualified_name,
                                 const Instruction& instruction, Context& context, ObjectCode& code)
    : types_(types),
      file_(file),
      qualified_name_(qualified_name),
      instruction_(instruction),
      context_(context),
      code_(code)
{
}

void SwitchGenerator::Generate()
{
    const FieldData& field = GetSwitchField();

    const std::string data_type_name = SwitchDataTypeName(instruction_.switch_field);
    const std::string data_name = SwitchDataName(instruction_.switch_field);
    const std::string data_field_name = MemberIdentifier(data_name);
    if (context_.accessible_fields.count(data_name) != 0 || context_.scope_identifiers.count(data_field_name) != 0)
    {
        throw Error("Switch data field " + data_field_name + " conflicts with an existing field.");
    }

    // Switching on an enum with values that are not enumerators triggers -Wswitch, so switch on the integer value
    // instead in that case.
    const bool integer_switch = field.type->kind == TypeKind::Enum &&
                                std::any_of(instruction_.cases.begin(), instruction_.cases.end(),
                                            [](const ProtocolCase& x) { return !x.is_default && IsInteger(*x.value); });
    const std::string switch_expression =
        integer_switch ? "static_cast<int>(" + field.identifier + ")" : field.identifier;

    const bool has_default = ValidateCases(field, integer_switch);

    Cases cases;
    cases.reached_optional = context_.reached_optional;
    cases.reached_dummy = context_.reached_dummy;
    cases.reached_unsized_array = context_.reached_unsized_array;

    for (const auto& protocol_case : instruction_.cases)
    {
        const std::string label =
            protocol_case.is_default ? "default:"
                                     : "case " + CaseValueExpression(field, *protocol_case.value, integer_switch) + ":";

        if (protocol_case.instructions.empty())
        {
            GenerateEmptyCase(field, data_field_name, label, cases);
        }
        else
        {
            GenerateDataCase(protocol_case, field, data_field_name, label, cases);
        }
    }

    if (!has_default)
    {
        for (CodeWriter* writer : {&cases.serialize, &cases.deserialize})
        {
            writer->Line("default:");
            writer->Indent();
            writer->Line("break;");
            writer->Dedent();
        }
    }

    DocComment data_docs;
    data_docs.AddParagraph("Data associated with the `" + field.identifier + "` field.");
    data_docs.AddParagraph(instruction_.comment);
    data_docs.AddEmptyCaseParagraphs(instruction_, field.identifier);
    code_.members.DocComment("Data associated with different values of the `" + field.identifier + "` field.");
    code_.members.Line("using " + data_type_name + " = std::variant<" + Join(cases.alternatives, ", ") + ">;");
    code_.members.Line();
    code_.members.DocComment(data_docs.Text());
    code_.members.Line(data_type_name + " " + data_field_name + "{};");
    code_.members.Line();

    code_.serialize.Open("switch (" + switch_expression + ")");
    code_.serialize.Append(cases.serialize);
    code_.serialize.Close();

    code_.deserialize.Open("switch (" + switch_expression + ")");
    code_.deserialize.Append(cases.deserialize);
    code_.deserialize.Close();

    context_.reached_optional = cases.reached_optional;
    context_.reached_dummy = cases.reached_dummy;
    context_.reached_unsized_array = cases.reached_unsized_array;

    code_.value_members.push_back(data_field_name);
}

GeneratorError SwitchGenerator::Error(const std::string& message) const
{
    return GeneratorError(file_.path.string() + ": " + qualified_name_ + ": " + message);
}

const FieldData& SwitchGenerator::GetSwitchField() const
{
    const std::string& field_name = instruction_.switch_field;
    const auto field_it = context_.accessible_fields.find(field_name);
    if (field_it == context_.accessible_fields.end())
    {
        throw Error("Referenced " + field_name + " field is not accessible.");
    }

    const FieldData& field = field_it->second;
    if (field.array)
    {
        throw Error("\"" + field_name + "\" field referenced by switch must not be an array.");
    }
    if (field.optional || field.length_field || field.hardcoded)
    {
        throw Error("\"" + field_name +
                    "\" field referenced by switch must be unconditionally present (not optional, a length field or "
                    "hardcoded).");
    }
    if (field.type->kind != TypeKind::Integer && field.type->kind != TypeKind::Enum)
    {
        throw Error(field_name + " field referenced by switch must be a numeric or enumeration type.");
    }
    return field;
}

bool SwitchGenerator::ValidateCases(const FieldData& field, bool integer_switch) const
{
    bool reached_default = false;
    std::set<std::string> case_values;
    for (const auto& protocol_case : instruction_.cases)
    {
        if (reached_default)
        {
            throw Error("Only the last case in a switch can be the default case.");
        }
        if (protocol_case.is_default)
        {
            reached_default = true;
        }
        else if (!case_values.insert(CaseValueExpression(field, *protocol_case.value, integer_switch)).second)
        {
            throw Error("Duplicate case value " + *protocol_case.value + " in switch on " + instruction_.switch_field +
                        ".");
        }
    }
    return reached_default;
}

void SwitchGenerator::GenerateEmptyCase(const FieldData& field, const std::string& data_field_name,
                                        const std::string& label, Cases& cases) const
{
    cases.serialize.Line(label);
    cases.serialize.Indent();
    cases.serialize.Open("if (!std::holds_alternative<std::monostate>(" + data_field_name + "))");
    cases.serialize.Line("throw SerializationError(\"Expected " + data_field_name + " to be empty for " +
                         field.identifier + " \" + detail::FormatValue(" + field.identifier + ") + \".\");");
    cases.serialize.Close();
    cases.serialize.Line("break;");
    cases.serialize.Dedent();

    cases.deserialize.Line(label);
    cases.deserialize.Indent();
    cases.deserialize.Line(data_field_name + " = std::monostate();");
    cases.deserialize.Line("break;");
    cases.deserialize.Dedent();
}

void SwitchGenerator::GenerateDataCase(const ProtocolCase& protocol_case, const FieldData& field,
                                       const std::string& data_field_name, const std::string& label, Cases& cases)
{
    const std::string case_type_name = SwitchCaseTypeName(instruction_.switch_field, protocol_case);
    const std::string description =
        protocol_case.is_default ? "default" : "value " + CaseValueDescription(field, *protocol_case.value);

    Context case_context;
    case_context.chunked = context_.chunked;
    case_context.reached_optional = context_.reached_optional;
    case_context.reached_dummy = context_.reached_dummy;
    case_context.reached_unsized_array = context_.reached_unsized_array;

    ObjectGenerator case_generator(types_, file_, case_type_name, qualified_name_ + "::" + case_type_name,
                                   case_context);
    case_generator.GenerateInstructions(protocol_case.instructions);

    std::string docs = "Data associated with " + field.identifier + " " + description + ".";
    if (protocol_case.comment)
    {
        docs += "\n\n" + *protocol_case.comment;
    }

    CodeWriter case_declaration;
    case_generator.Finish(docs, std::nullopt, case_declaration, code_.nested_definitions);
    code_.members.Append(case_declaration);
    code_.members.Line();
    cases.alternatives.push_back(case_type_name);

    const Context& case_result = case_generator.GetContext();
    cases.reached_optional = cases.reached_optional || case_result.reached_optional;
    cases.reached_dummy = cases.reached_dummy || case_result.reached_dummy;
    cases.reached_unsized_array = cases.reached_unsized_array || case_result.reached_unsized_array;

    cases.serialize.Line(label);
    cases.serialize.Line("{");
    cases.serialize.Indent();
    cases.serialize.Line("const auto* case_data = std::get_if<" + case_type_name + ">(&" + data_field_name + ");");
    cases.serialize.Open("if (case_data == nullptr)");
    cases.serialize.Line("throw SerializationError(\"Expected " + data_field_name + " to be type " + case_type_name +
                         " for " + field.identifier + " \" + detail::FormatValue(" + field.identifier + ") + \".\");");
    cases.serialize.Close();
    cases.serialize.Line("case_data->Serialize(writer);");
    cases.serialize.Line("break;");
    cases.serialize.Close();

    cases.deserialize.Line(label);
    cases.deserialize.Indent();
    cases.deserialize.Line(data_field_name + ".emplace<" + case_type_name + ">().Deserialize(reader);");
    cases.deserialize.Line("break;");
    cases.deserialize.Dedent();
}

std::string SwitchGenerator::CaseValueExpression(const FieldData& field, const std::string& value,
                                                 bool integer_switch) const
{
    if (field.type->kind == TypeKind::Integer)
    {
        if (!IsInteger(value))
        {
            throw Error("\"" + value + "\" is not a valid integer value.");
        }
        return value;
    }

    const auto& protocol_enum = *field.type->enum_definition;
    const std::string enum_name = CppTypeName(*field.type, &file_);
    if (IsInteger(value))
    {
        const long long ordinal = std::stoll(value);
        for (const auto& enum_value : protocol_enum.values)
        {
            if (enum_value.ordinal == ordinal)
            {
                throw Error(protocol_enum.name + " value " + value + " must be referred to by name (" +
                            enum_value.name + ")");
            }
        }
        return integer_switch ? value : "static_cast<" + enum_name + ">(" + value + ")";
    }

    for (const auto& enum_value : protocol_enum.values)
    {
        if (enum_value.name == value)
        {
            return integer_switch ? "static_cast<int>(" + enum_name + "::" + value + ")" : enum_name + "::" + value;
        }
    }
    throw Error("\"" + value + "\" is not a valid value for enum type " + protocol_enum.name);
}

std::string SwitchGenerator::CaseValueDescription(const FieldData& field, const std::string& value)
{
    if (field.type->kind == TypeKind::Enum && IsInteger(value))
    {
        return "Unrecognized(" + value + ")";
    }
    return value;
}

} // namespace eolib::generator
