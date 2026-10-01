#pragma once

#include "code_writer.hpp"
#include "errors.hpp"
#include "model.hpp"
#include "object_generator.hpp"
#include "types.hpp"

#include <string>
#include <vector>

namespace eolib::generator
{

/// Generates a switch instruction in a class: a nested class for each case that has data, a variant member that holds
/// the case data, and the switch statements that serialize and deserialize it. Follows the semantics of eolib-java's
/// SwitchCodeGenerator.
class SwitchGenerator
{
public:
    /// Creates the generator for a switch instruction in the class being generated. context and code belong to the
    /// enclosing class, and are updated as the switch is generated.
    SwitchGenerator(TypeRegistry& types, const ProtocolFile& file, const std::string& qualified_name,
                    const Instruction& instruction, Context& context, ObjectCode& code);

    /// Generates the switch.
    /// @throws GeneratorError if the switch or its cases are invalid.
    void Generate();

private:
    /// The code generated for the cases of the switch.
    struct Cases
    {
        std::vector<std::string> alternatives = {"std::monostate"};
        CodeWriter serialize;
        CodeWriter deserialize;
        bool reached_optional = false;
        bool reached_dummy = false;
        bool reached_unsized_array = false;
    };

    TypeRegistry& types_;
    const ProtocolFile& file_;
    const std::string& qualified_name_;
    const Instruction& instruction_;
    Context& context_;
    ObjectCode& code_;

    GeneratorError Error(const std::string& message) const;

    /// Gets the field that the switch is on, which must be an accessible integer or enum field.
    const FieldData& GetSwitchField() const;

    /// Validates case ordering and uniqueness. Returns true if the switch has a default case.
    bool ValidateCases(const FieldData& field, bool integer_switch) const;

    void GenerateEmptyCase(const FieldData& field, const std::string& data_field_name, const std::string& label,
                           Cases& cases) const;
    void GenerateDataCase(const ProtocolCase& protocol_case, const FieldData& field, const std::string& data_field_name,
                          const std::string& label, Cases& cases);

    std::string CaseValueExpression(const FieldData& field, const std::string& value, bool integer_switch) const;
    static std::string CaseValueDescription(const FieldData& field, const std::string& value);
};

} // namespace eolib::generator
