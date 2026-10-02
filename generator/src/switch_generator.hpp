#pragma once

#include "code_writer.hpp"
#include "errors.hpp"
#include "model.hpp"
#include "object_generator.hpp"
#include "types.hpp"

#include <optional>
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

    /// The result of generating a case, used for the accessors and factories.
    struct CaseInfo
    {
        const ProtocolCase* protocol_case = nullptr;
        /// The name of the case class, or nullopt if the case has no data.
        std::optional<std::string> type_name;
        /// The data members of the case class.
        std::vector<std::string> value_members;
        /// The factories of the switches in the case class, relative to the case class.
        std::vector<SwitchFactory> factories;
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

    CaseInfo GenerateEmptyCase(const ProtocolCase& protocol_case, const FieldData& field,
                               const std::string& data_field_name, const std::string& label, Cases& cases) const;
    CaseInfo GenerateDataCase(const ProtocolCase& protocol_case, const FieldData& field,
                              const std::string& data_field_name, const std::string& label, Cases& cases);

    /// Gets the name used for a case in the names of its accessor and factories: the enum value name, the case class
    /// name for an integer value, or "<Field>Default" for the default case.
    std::string CaseName(const FieldData& field, const CaseInfo& info) const;

    /// Declares an As<Case> accessor for each case that has a case class.
    void GenerateAccessors(const FieldData& field, const std::string& data_field_name,
                           const std::vector<CaseInfo>& infos);

    /// Adds a factory for each value of the switch field to the enclosing class, lifting the factories of nested
    /// switches.
    void GenerateFactories(const FieldData& field, const std::string& data_field_name, bool integer_switch,
                           const std::vector<CaseInfo>& infos);

    /// Adds the factories for a value of the switch field. info is the case that handles the value, or nullptr if
    /// there is none.
    void AddFactories(SwitchFactory factory, const CaseInfo* info);

    std::string CaseValueExpression(const FieldData& field, const std::string& value, bool integer_switch) const;
    static std::string CaseValueDescription(const FieldData& field, const std::string& value);

    /// Gets whether a case is for an integer value: any value of an integer field, or an unrecognized enum value.
    static bool IsIntegerCase(const FieldData& field, const ProtocolCase& protocol_case);

    /// Gets the expression that is switched on for a value of the switch field.
    static std::string SwitchExpression(const std::string& value, bool integer_switch);
};

} // namespace eolib::generator
