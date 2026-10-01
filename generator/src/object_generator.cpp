#include "object_generator.hpp"

#include "names.hpp"
#include "switch_generator.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace eolib::generator
{

ObjectGenerator::ObjectGenerator(TypeRegistry& types, const ProtocolFile& file, std::string class_name,
                                 std::string qualified_name, Context context)
    : types_(types),
      file_(file),
      class_name_(std::move(class_name)),
      qualified_name_(std::move(qualified_name)),
      context_(std::move(context)),
      serialization_(file, qualified_name_)
{
}

void ObjectGenerator::GenerateInstructions(const std::vector<Instruction>& instructions)
{
    PrepareScope(instructions);
    GenerateInstructionList(instructions);
    docs_.AddInstructionNotes(instructions);
}

const Context& ObjectGenerator::GetContext() const
{
    return context_;
}

void ObjectGenerator::Finish(const std::optional<std::string>& comment, const std::optional<PacketInfo>& packet,
                             CodeWriter& declaration, CodeWriter& definitions)
{
    for (const auto& [name, referenced] : context_.length_field_referenced)
    {
        if (!referenced)
        {
            throw Error("Length field \"" + name + "\" must be referenced by another field.");
        }
    }

    WriteDeclaration(comment, packet, declaration);

    definitions.Append(code_.nested_definitions);
    WriteSerializationDefinitions(definitions);
    WriteToStringDefinition(definitions);
    WriteEqualityDefinitions(definitions);
}

GeneratorError ObjectGenerator::Error(const std::string& message) const
{
    return GeneratorError(file_.path.string() + ": " + qualified_name_ + ": " + message);
}

void ObjectGenerator::PrepareScope(const std::vector<Instruction>& instructions)
{
    for (const auto* instruction : FlattenChunked(instructions))
    {
        if (instruction->name)
        {
            context_.scope_identifiers.insert(MemberIdentifier(*instruction->name));
        }
        if ((instruction->kind == InstructionKind::Field || instruction->kind == InstructionKind::Array) &&
            instruction->length && !IsInteger(*instruction->length))
        {
            if (context_.length_references.count(*instruction->length) != 0)
            {
                throw Error("Length field \"" + *instruction->length + "\" must not be referenced by multiple fields.");
            }
            context_.length_references[*instruction->length] = instruction;
        }
    }
}

std::string ObjectGenerator::LocalName(const std::string& base) const
{
    std::string result = base;
    while (context_.scope_identifiers.count(result) != 0 || IsReservedIdentifier(result))
    {
        result += "_";
    }
    return result;
}

void ObjectGenerator::GenerateInstructionList(const std::vector<Instruction>& instructions)
{
    for (std::size_t i = 0; i < instructions.size(); ++i)
    {
        dummy_follows_ = std::any_of(instructions.begin() + static_cast<std::ptrdiff_t>(i) + 1, instructions.end(),
                                     [](const Instruction& x) { return x.kind == InstructionKind::Dummy; });
        GenerateInstruction(instructions[i]);
    }
}

void ObjectGenerator::GenerateInstruction(const Instruction& instruction)
{
    if (context_.reached_dummy)
    {
        throw Error("<dummy> elements must not be followed by any other elements.");
    }
    if (context_.reached_unsized_array && instruction.kind != InstructionKind::Break)
    {
        throw Error("Non-delimited arrays without a length must be the final element (or the final element in the "
                    "chunk, if chunked reading is enabled).");
    }

    switch (instruction.kind)
    {
        case InstructionKind::Field:
            GenerateField(instruction);
            break;
        case InstructionKind::Array:
            GenerateArray(instruction);
            break;
        case InstructionKind::Length:
            GenerateLength(instruction);
            break;
        case InstructionKind::Dummy:
            GenerateDummy(instruction);
            break;
        case InstructionKind::Switch:
            GenerateSwitch(instruction);
            break;
        case InstructionKind::Chunked:
            GenerateChunked(instruction);
            break;
        case InstructionKind::Break:
            GenerateBreak();
            break;
    }
}

void ObjectGenerator::CheckOptionalField(bool optional) const
{
    if (context_.reached_optional && !optional)
    {
        throw Error("Optional fields may not be followed by non-optional fields.");
    }
}

void ObjectGenerator::CheckUniqueName(const std::string& name) const
{
    if (context_.accessible_fields.count(name) != 0)
    {
        throw Error("Cannot redefine " + name + " field.");
    }
}

void ObjectGenerator::DeclareMember(const std::string& cpp_type, const FieldData& field)
{
    const std::string declared_type = field.optional ? "std::optional<" + cpp_type + ">" : cpp_type;
    code_.members.Line(declared_type + " " + field.identifier + "{};");
    code_.members.Line();
    code_.value_members.push_back(field.identifier);
}

void ObjectGenerator::AddMemberDocs(const Instruction& instruction, const Type& type, bool array,
                                    const std::optional<std::string>& remarks)
{
    DocComment docs;
    docs.AddParagraph(instruction.comment);
    docs.AddParagraph(remarks);
    docs.AddConstraintNotes(instruction, type, array, LengthFieldMax(instruction));
    if (!docs.Empty())
    {
        code_.members.DocComment(docs.Text());
    }
}

std::optional<long long> ObjectGenerator::LengthFieldMax(const Instruction& instruction) const
{
    if (!instruction.length)
    {
        return std::nullopt;
    }

    const auto length_field = context_.accessible_fields.find(*instruction.length);
    if (length_field == context_.accessible_fields.end())
    {
        return std::nullopt;
    }

    return MaxValueOf(*length_field->second.type) + length_field->second.offset;
}

void ObjectGenerator::BeginSerializeOptional(const FieldData& field)
{
    const std::string null_check = "!" + field.identifier + ".has_value()";
    if (context_.reached_optional && declared_reached_null_optional_)
    {
        code_.serialize.Line("reached_null_optional = reached_null_optional || " + null_check + ";");
    }
    else if (declared_reached_null_optional_)
    {
        code_.serialize.Line("reached_null_optional = " + null_check + ";");
    }
    else
    {
        code_.serialize.Line("bool reached_null_optional = " + null_check + ";");
        declared_reached_null_optional_ = true;
    }
    code_.serialize.Open("if (!reached_null_optional)");
}

void ObjectGenerator::BeginDeserializeOptional(const Type& type, bool array)
{
    if (dummy_follows_ && !array && type.fixed_size)
    {
        code_.deserialize.Open("if (reader.Remaining() >= " + std::to_string(*type.fixed_size) + ")");
    }
    else
    {
        code_.deserialize.Open("if (reader.Remaining() > 0)");
    }
}

std::optional<std::string> ObjectGenerator::DeserializeLengthExpression(const std::optional<std::string>& length) const
{
    if (!length)
    {
        return std::nullopt;
    }
    if (IsInteger(*length))
    {
        return length;
    }
    const auto it = context_.accessible_fields.find(*length);
    if (it == context_.accessible_fields.end() || !it->second.length_field)
    {
        throw Error("Referenced " + *length + " field is not accessible.");
    }
    return it->second.identifier;
}

void ObjectGenerator::ValidateLengthAttribute(const std::optional<std::string>& length)
{
    if (!length || IsInteger(*length))
    {
        return;
    }
    const auto it = context_.length_field_referenced.find(*length);
    if (it == context_.length_field_referenced.end())
    {
        throw Error("Length attribute \"" + *length + "\" must be a numeric literal, or refer to a length field.");
    }
    if (it->second)
    {
        throw Error("Length field \"" + *length + "\" must not be referenced by multiple fields.");
    }
    it->second = true;
}

void ObjectGenerator::GenerateSerializeLengthCheck(const std::string& identifier, const std::string& size_expression,
                                                   const std::string& limit, bool allow_shorter)
{
    const std::string op = allow_shorter ? ">" : "!=";
    const std::string expected = allow_shorter ? limit + " or less" : "exactly " + limit;
    code_.serialize.Open("if (" + size_expression + " " + op + " " + limit + ")");
    code_.serialize.Line("throw SerializationError(\"Expected " + identifier + ".size() to be " + expected +
                         ", got \" + std::to_string(" + size_expression + ") + \".\");");
    code_.serialize.Close();
}

void ObjectGenerator::GenerateField(const Instruction& instruction)
{
    CheckOptionalField(instruction.optional);

    const Type& type = types_.Get(instruction.type, instruction.length);
    ValidateField(instruction, type);

    const FieldData field = DeclareField(instruction, type);
    GenerateSerializeField(instruction, type, field);
    GenerateDeserializeField(instruction, type, field);

    if (instruction.optional)
    {
        context_.reached_optional = true;
    }
}

void ObjectGenerator::ValidateField(const Instruction& instruction, const Type& type)
{
    if (!instruction.name)
    {
        if (!instruction.value)
        {
            throw Error("Unnamed fields must specify a hardcoded field value.");
        }
        if (instruction.optional)
        {
            throw Error("Unnamed fields may not be optional.");
        }
    }
    if (instruction.value)
    {
        if (!type.IsBasic())
        {
            throw Error("Hardcoded field values are not allowed for " + type.name + " fields (must be a basic type).");
        }
        if (type.kind == TypeKind::String && type.fixed_size &&
            static_cast<std::size_t>(*type.fixed_size) != instruction.value->size())
        {
            throw Error("Expected length of " + std::to_string(*type.fixed_size) + " for hardcoded string value \"" +
                        *instruction.value + "\".");
        }
        if (instruction.length && !IsInteger(*instruction.length))
        {
            throw Error("Hardcoded fields must not reference a length field.");
        }
    }
    if ((instruction.length || instruction.padded) && type.kind != TypeKind::String)
    {
        throw Error("length and padded attributes are only allowed for string types.");
    }
    if (instruction.padded && !instruction.length)
    {
        throw Error("Padded fields must specify a length.");
    }
    ValidateLengthAttribute(instruction.length);
}

FieldData ObjectGenerator::DeclareField(const Instruction& instruction, const Type& type)
{
    FieldData field;
    field.type = &type;
    field.optional = instruction.optional;
    field.hardcoded = instruction.value.has_value();

    if (!instruction.name)
    {
        return field;
    }

    CheckUniqueName(*instruction.name);
    field.identifier = MemberIdentifier(*instruction.name);

    std::optional<std::string> remarks;
    if (field.hardcoded)
    {
        const std::string default_name = DefaultConstantName(*instruction.name);
        const std::string constant_type =
            type.kind == TypeKind::String ? "std::string_view" : CppTypeName(type, &file_);
        code_.members.DocComment(instruction.comment ? DocText(*instruction.comment)
                                                     : "The default value of the `" + field.identifier + "` field.");
        code_.members.Line("static constexpr " + constant_type + " " + default_name + " = " +
                           serialization_.HardcodedValueExpression(type, *instruction.value) + ";");
        code_.members.Line();

        if (SerializationCode::HasNonZeroDefault(type, *instruction.value))
        {
            remarks = "A zero value is serialized as `" + default_name + "` unless this object was deserialized.";
        }
    }

    AddMemberDocs(instruction, type, false, remarks);
    DeclareMember(CppTypeName(type, &file_), field);
    context_.accessible_fields[*instruction.name] = field;
    return field;
}

void ObjectGenerator::GenerateSerializeField(const Instruction& instruction, const Type& type, const FieldData& field)
{
    if (instruction.optional)
    {
        BeginSerializeOptional(field);
    }

    std::string value;
    if (!instruction.name)
    {
        value = serialization_.HardcodedValueExpression(type, *instruction.value);
    }
    else
    {
        value = instruction.optional ? "(*" + field.identifier + ")" : field.identifier;
        if (field.hardcoded)
        {
            value = SerializationCode::HardcodedFieldValueExpression(instruction, type, value);
        }
    }

    std::optional<std::string> serialize_length;
    if (instruction.length)
    {
        if (IsInteger(*instruction.length))
        {
            serialize_length = *instruction.length;
            if (instruction.name)
            {
                GenerateSerializeLengthCheck(field.identifier, value + ".size()", *instruction.length,
                                             instruction.padded);
            }
        }
        else
        {
            serialize_length = "static_cast<int>(" + value + ".size())";
        }
    }
    code_.serialize.Line(serialization_.WriteStatement(type, value, serialize_length, instruction.padded));

    if (instruction.optional)
    {
        code_.serialize.Close();
    }
}

void ObjectGenerator::GenerateDeserializeField(const Instruction& instruction, const Type& type, const FieldData& field)
{
    if (instruction.optional)
    {
        BeginDeserializeOptional(type, false);
    }

    const auto deserialize_length = DeserializeLengthExpression(instruction.length);
    if (!instruction.name)
    {
        code_.deserialize.Line(
            serialization_.ReadExpression(type.SerializationType(), deserialize_length, instruction.padded) + ";");
    }
    else if (type.kind == TypeKind::Struct)
    {
        const std::string target = instruction.optional ? field.identifier + ".emplace()" : field.identifier;
        code_.deserialize.Line(target + ".Deserialize(reader);");
    }
    else
    {
        code_.deserialize.Line(field.identifier + " = " +
                               serialization_.ReadExpression(type, deserialize_length, instruction.padded) + ";");
    }

    if (instruction.optional)
    {
        code_.deserialize.Close();
    }
}

void ObjectGenerator::GenerateArray(const Instruction& instruction)
{
    CheckOptionalField(instruction.optional);

    if (instruction.delimited && !context_.chunked)
    {
        throw Error("Cannot generate a delimited array instruction unless chunked reading is enabled. (All "
                    "delimited <array> elements must be within <chunked> sections.)");
    }

    const Type& type = types_.Get(instruction.type);
    ValidateArray(instruction, type);

    const FieldData field = DeclareArray(instruction, type);
    const std::string container = instruction.optional ? "(*" + field.identifier + ")" : field.identifier;
    const bool trailing_delimiter = instruction.delimited && instruction.trailing_delimiter.value_or(true);
    GenerateSerializeArray(instruction, type, field, container, trailing_delimiter);
    GenerateDeserializeArray(instruction, type, field, container, trailing_delimiter);

    if (instruction.optional)
    {
        context_.reached_optional = true;
    }
    if (!instruction.delimited && !instruction.length)
    {
        context_.reached_unsized_array = true;
    }
}

void ObjectGenerator::ValidateArray(const Instruction& instruction, const Type& type)
{
    if (!instruction.delimited && !type.bounded)
    {
        throw Error("Unbounded element type (" + instruction.type + ") forbidden in non-delimited array.");
    }
    if (!instruction.delimited && instruction.trailing_delimiter)
    {
        throw Error("Only delimited arrays can have a trailing delimiter.");
    }
    ValidateLengthAttribute(instruction.length);
    CheckUniqueName(*instruction.name);
}

FieldData ObjectGenerator::DeclareArray(const Instruction& instruction, const Type& type)
{
    FieldData field;
    field.identifier = MemberIdentifier(*instruction.name);
    field.type = &type;
    field.array = true;
    field.optional = instruction.optional;

    AddMemberDocs(instruction, type, true);
    DeclareMember("std::vector<" + CppTypeName(type, &file_) + ">", field);
    context_.accessible_fields[*instruction.name] = field;
    return field;
}

void ObjectGenerator::GenerateSerializeArray(const Instruction& instruction, const Type& type, const FieldData& field,
                                             const std::string& container, bool trailing_delimiter)
{
    if (instruction.optional)
    {
        BeginSerializeOptional(field);
    }

    if (instruction.length && IsInteger(*instruction.length))
    {
        GenerateSerializeLengthCheck(field.identifier, container + ".size()", *instruction.length, false);
    }

    code_.serialize.Open("for (std::size_t i = 0; i < " + container + ".size(); ++i)");
    if (instruction.delimited && !trailing_delimiter)
    {
        code_.serialize.Open("if (i > 0)");
        code_.serialize.Line("writer.AddByte(0xFF);");
        code_.serialize.Close();
    }
    code_.serialize.Line(serialization_.WriteStatement(type, container + "[i]", std::nullopt, false));
    if (trailing_delimiter)
    {
        code_.serialize.Line("writer.AddByte(0xFF);");
    }
    code_.serialize.Close();

    if (instruction.optional)
    {
        code_.serialize.Close();
    }
}

void ObjectGenerator::GenerateDeserializeArray(const Instruction& instruction, const Type& type, const FieldData& field,
                                               const std::string& container, bool trailing_delimiter)
{
    if (instruction.optional)
    {
        BeginDeserializeOptional(type, true);
        code_.deserialize.Line(field.identifier + ".emplace();");
    }

    std::optional<std::string> length = DeserializeLengthExpression(instruction.length);
    if (!length && !instruction.delimited && type.fixed_size)
    {
        const std::string size_variable = LocalName(field.identifier + "_size");
        code_.deserialize.Line("const int " + size_variable + " = reader.Remaining() / " +
                               std::to_string(*type.fixed_size) + ";");
        length = size_variable;
    }

    code_.deserialize.Open(length ? "for (int i = 0; i < " + *length + "; ++i)" : "while (reader.Remaining() > 0)");

    if (type.kind == TypeKind::Struct)
    {
        code_.deserialize.Line(container + ".emplace_back().Deserialize(reader);");
    }
    else
    {
        code_.deserialize.Line(container + ".push_back(" + serialization_.ReadExpression(type, std::nullopt, false) +
                               ");");
    }

    if (instruction.delimited)
    {
        const bool needs_guard = !trailing_delimiter && length;
        if (needs_guard)
        {
            code_.deserialize.Open("if (i + 1 < " + *length + ")");
        }
        code_.deserialize.Line("reader.NextChunk();");
        if (needs_guard)
        {
            code_.deserialize.Close();
        }
    }
    code_.deserialize.Close();

    if (instruction.optional)
    {
        code_.deserialize.Close();
    }
}

void ObjectGenerator::GenerateLength(const Instruction& instruction)
{
    CheckOptionalField(instruction.optional);

    const Type& type = types_.Get(instruction.type);
    const std::string& name = *instruction.name;
    if (type.kind != TypeKind::Integer)
    {
        throw Error(type.name + " is not a numeric type, so it is not allowed for a length field.");
    }
    CheckUniqueName(name);

    const auto reference = context_.length_references.find(name);
    if (reference == context_.length_references.end())
    {
        throw Error("Length field \"" + name + "\" must be referenced by another field.");
    }
    const Instruction& referencing = *reference->second;
    if (referencing.value || !referencing.name)
    {
        throw Error("Hardcoded fields must not reference a length field.");
    }
    const std::string referencing_identifier = MemberIdentifier(*referencing.name);

    FieldData field;
    field.identifier = MemberIdentifier(name);
    field.type = &type;
    field.offset = instruction.offset;
    field.length_field = true;
    field.optional = instruction.optional;
    context_.accessible_fields[name] = field;
    context_.length_field_referenced[name] = false;

    // Serialize: the value is computed from the size of the referencing field.
    if (instruction.optional)
    {
        BeginSerializeOptional(FieldData{referencing_identifier, &type, 0, false, true, false, false});
    }

    const std::string size_expression = referencing.optional
                                            ? "(" + referencing_identifier + ".has_value() ? static_cast<int>(" +
                                                  referencing_identifier + "->size()) : 0)"
                                            : "static_cast<int>(" + referencing_identifier + ".size())";

    const long long max_size = MaxValueOf(type) + instruction.offset;
    if (max_size < 0x7FFFFFFFLL)
    {
        GenerateSerializeLengthCheck(referencing_identifier, size_expression, std::to_string(max_size), true);
    }
    code_.serialize.Line(serialization_.WriteStatement(
        type, size_expression + SerializationCode::OffsetExpression(-instruction.offset), std::nullopt, false));

    if (instruction.optional)
    {
        code_.serialize.Close();
    }

    // Deserialize: the value is stored in a local variable, referenced by the field that follows.
    const std::string read = serialization_.ReadExpression(type, std::nullopt, false, instruction.offset);
    if (instruction.optional)
    {
        code_.deserialize.Line("int " + field.identifier + " = 0;");
        BeginDeserializeOptional(type, false);
        code_.deserialize.Line(field.identifier + " = " + read + ";");
        code_.deserialize.Close();
        context_.reached_optional = true;
    }
    else
    {
        code_.deserialize.Line("const int " + field.identifier + " = " + read + ";");
    }
}

void ObjectGenerator::GenerateDummy(const Instruction& instruction)
{
    const Type& type = types_.Get(instruction.type);
    if (!type.IsBasic())
    {
        throw Error("Dummy fields must be a basic type.");
    }

    const bool needs_guards = !code_.serialize.Empty() || !code_.deserialize.Empty();
    if (needs_guards)
    {
        code_.serialize.Open("if (writer.Length() == old_writer_length)");
        code_.deserialize.Open("if (reader.Position() == reader_start_position)");
    }

    code_.serialize.Line(serialization_.WriteStatement(
        type, serialization_.HardcodedValueExpression(type, *instruction.value), std::nullopt, false));
    code_.deserialize.Line(serialization_.ReadExpression(type.SerializationType(), std::nullopt, false) + ";");

    if (needs_guards)
    {
        code_.serialize.Close();
        code_.deserialize.Close();
        needs_old_writer_length_ = true;
    }

    context_.reached_dummy = true;
}

void ObjectGenerator::GenerateSwitch(const Instruction& instruction)
{
    SwitchGenerator(types_, file_, qualified_name_, instruction, context_, code_).Generate();
}

void ObjectGenerator::GenerateChunked(const Instruction& instruction)
{
    const bool was_already_enabled = context_.chunked;
    if (!was_already_enabled)
    {
        context_.chunked = true;
        uses_chunked_ = true;
        code_.deserialize.Line("reader.SetChunkedReadingMode(true);");
        code_.serialize.Line("writer.SetStringSanitization(true);");
    }

    const bool outer_dummy_follows = dummy_follows_;
    GenerateInstructionList(instruction.instructions);
    dummy_follows_ = outer_dummy_follows;

    if (!was_already_enabled)
    {
        context_.chunked = false;
        code_.deserialize.Line("reader.SetChunkedReadingMode(false);");
        code_.serialize.Line("writer.SetStringSanitization(false);");
    }
}

void ObjectGenerator::GenerateBreak()
{
    if (!context_.chunked)
    {
        throw Error("Cannot generate a break instruction unless chunked reading is enabled. (All <break> "
                    "elements must be within <chunked> sections.)");
    }

    context_.reached_optional = false;
    context_.reached_dummy = false;
    context_.reached_unsized_array = false;

    code_.deserialize.Line("reader.NextChunk();");
    code_.serialize.Line("writer.AddByte(0xFF);");
}

void ObjectGenerator::WriteDeclaration(const std::optional<std::string>& comment,
                                       const std::optional<PacketInfo>& packet, CodeWriter& declaration)
{
    docs_.AddParagraph(comment);
    if (!docs_.Empty())
    {
        declaration.DocComment(docs_.Text());
    }
    const std::string base = packet ? "net::Packet" : "Serializable";
    declaration.Line("class EOLIB_API " + class_name_ + " final : public " + base);
    declaration.Line("{");
    declaration.Line("public:");
    declaration.Indent();

    if (packet)
    {
        declaration.DocComment("The packet family associated with this packet.");
        declaration.Line("static constexpr PacketFamily FAMILY = PacketFamily::" + packet->family + ";");
        declaration.Line();
        declaration.DocComment("The packet action associated with this packet.");
        declaration.Line("static constexpr PacketAction ACTION = PacketAction::" + packet->action + ";");
        declaration.Line();
    }

    if (!code_.members.Empty())
    {
        declaration.Append(code_.members);
        declaration.TrimTrailingBlankLine();
        declaration.Line();
    }

    if (packet)
    {
        declaration.DocComment("Gets the packet family associated with this packet.\n\n@return the packet family.");
        declaration.Open("PacketFamily Family() const noexcept override");
        declaration.Line("return FAMILY;");
        declaration.Close();
        declaration.Line();
        declaration.DocComment("Gets the packet action associated with this packet.\n\n@return the packet action.");
        declaration.Open("PacketAction Action() const noexcept override");
        declaration.Line("return ACTION;");
        declaration.Close();
        declaration.Line();
    }

    declaration.DocComment("Gets the size of the data that this object was deserialized from.\n\n"
                           "@return the deserialized size in bytes, or 0 for an object that was not deserialized.");
    declaration.Line("int ByteSize() const noexcept override;");
    declaration.Line();
    declaration.DocComment("Serializes this object to the provided writer.\n\n"
                           "@param writer the writer that the data will be written to.\n"
                           "@throws SerializationError if the object's data is invalid for serialization.");
    declaration.Line("void Serialize(data::EoWriter& writer) const override;");
    declaration.Line();
    declaration.DocComment("Deserializes this object from the provided reader, replacing its current data.\n\n"
                           "@param reader the reader that the data will be read from.");
    declaration.Line("void Deserialize(data::EoReader& reader) override;");
    declaration.Line();
    declaration.DocComment("Gets a human-readable representation of this object, for debugging purposes.\n\n"
                           "@return the object's fields and values, as a string.");
    declaration.Line("std::string ToString() const override;");
    declaration.Line();
    declaration.DocComment("Compares this object with another for equality. The deserialized byte size is not "
                           "compared.\n\n"
                           "@param other the object to compare with.\n"
                           "@return true if all fields are equal, otherwise false.");
    declaration.Line("bool operator==(const " + class_name_ + "& other) const;");
    declaration.Line();
    declaration.DocComment("Compares this object with another for inequality. The deserialized byte size is not "
                           "compared.\n\n"
                           "@param other the object to compare with.\n"
                           "@return true if any field differs, otherwise false.");
    declaration.Line("bool operator!=(const " + class_name_ + "& other) const;");
    declaration.Dedent();
    declaration.Line();
    declaration.Line("private:");
    declaration.Indent();
    declaration.Line("int byte_size_ = 0;");
    declaration.Dedent();
    declaration.Line("};");
}

void ObjectGenerator::WriteSerializationDefinitions(CodeWriter& definitions) const
{
    definitions.Open("int " + qualified_name_ + "::ByteSize() const noexcept");
    definitions.Line("return byte_size_;");
    definitions.Close();
    definitions.Line();

    definitions.Open("void " + qualified_name_ + "::Serialize(data::EoWriter& writer) const");
    if (uses_chunked_)
    {
        definitions.Line("detail::StringSanitizationGuard sanitization_guard(writer);");
    }
    if (needs_old_writer_length_)
    {
        definitions.Line("const int old_writer_length = writer.Length();");
    }
    if (code_.serialize.Empty())
    {
        definitions.Line("(void)writer;");
    }
    definitions.Append(code_.serialize);
    definitions.Close();
    definitions.Line();

    definitions.Open("void " + qualified_name_ + "::Deserialize(data::EoReader& reader)");
    if (uses_chunked_)
    {
        definitions.Line("detail::ChunkedReadingModeGuard chunked_guard(reader);");
    }
    definitions.Line("*this = " + qualified_name_ + "();");
    definitions.Line("const int reader_start_position = reader.Position();");
    definitions.Append(code_.deserialize);
    definitions.Line("byte_size_ = reader.Position() - reader_start_position;");
    definitions.Close();
    definitions.Line();
}

void ObjectGenerator::WriteToStringDefinition(CodeWriter& definitions) const
{
    definitions.Open("std::string " + qualified_name_ + "::ToString() const");
    if (code_.value_members.empty())
    {
        definitions.Line("return \"" + class_name_ + "{}\";");
    }
    else
    {
        definitions.Line("std::string result = \"" + class_name_ + "{\";");
        for (std::size_t i = 0; i < code_.value_members.size(); ++i)
        {
            const auto& member = code_.value_members[i];
            const std::string separator = i == 0 ? "" : ", ";
            definitions.Line("result += \"" + separator + member + "=\" + detail::FormatValue(" + member + ");");
        }
        definitions.Line("result += \"}\";");
        definitions.Line("return result;");
    }
    definitions.Close();
    definitions.Line();
}

void ObjectGenerator::WriteEqualityDefinitions(CodeWriter& definitions) const
{
    if (code_.value_members.empty())
    {
        definitions.Open("bool " + qualified_name_ + "::operator==(const " + qualified_name_ + "&) const");
        definitions.Line("return true;");
    }
    else
    {
        definitions.Open("bool " + qualified_name_ + "::operator==(const " + qualified_name_ + "& other) const");
        for (std::size_t i = 0; i < code_.value_members.size(); ++i)
        {
            const auto& member = code_.value_members[i];
            const std::string prefix = i == 0 ? "return " : "       ";
            const std::string suffix = i + 1 == code_.value_members.size() ? ";" : " &&";
            definitions.Line(prefix + member + " == other." + member + suffix);
        }
    }
    definitions.Close();
    definitions.Line();

    definitions.Open("bool " + qualified_name_ + "::operator!=(const " + qualified_name_ + "& other) const");
    definitions.Line("return !(*this == other);");
    definitions.Close();
    definitions.Line();
}

} // namespace eolib::generator
