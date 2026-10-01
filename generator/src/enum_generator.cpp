#include "enum_generator.hpp"

#include "errors.hpp"
#include "names.hpp"

namespace eolib::generator
{

EnumGenerator::EnumGenerator(TypeRegistry& types)
    : types_(types)
{
}

void EnumGenerator::Generate(const ProtocolFile& file, std::vector<OutputFile>& result)
{
    GeneratedFile header("include/" + file.IncludePath("enums.hpp"), GeneratedFile::Kind::Header);
    header.Includes({"eolib/export.hpp"}, {"iosfwd", "string"});
    header.BeginNamespace(file.Namespace());

    GeneratedFile source("src/" + file.IncludePath("enums.cpp"), GeneratedFile::Kind::Source);
    source.Includes({file.IncludePath("enums.hpp")}, {"ostream", "string"});
    source.BeginNamespace(file.Namespace());

    for (const auto& protocol_enum : file.enums)
    {
        // Validates the enum and its underlying type.
        types_.Get(protocol_enum.name);

        if (protocol_enum.comment)
        {
            header.DocComment(DocText(*protocol_enum.comment));
        }
        header.Line("enum class " + protocol_enum.name + " : int");
        header.Line("{");
        header.Indent();
        for (const auto& value : protocol_enum.values)
        {
            if (value.ordinal < -2147483648LL || value.ordinal > 2147483647LL)
            {
                throw GeneratorError(file.path.string() + ": " + protocol_enum.name + "." + value.name +
                                     " ordinal value is out of range.");
            }
            if (value.comment)
            {
                header.DocComment(DocText(*value.comment));
            }
            header.Line(value.name + " = " + std::to_string(value.ordinal) + ",");
        }
        header.Dedent();
        header.Line("};");
        header.Line();
        header.DocComment("Gets the name of a " + protocol_enum.name +
                          " value.\n\n"
                          "@param value the value to get the name of.\n"
                          "@return the name of the value, or \"Unrecognized(N)\" for unrecognized values.");
        header.Line("EOLIB_API std::string ToString(" + protocol_enum.name + " value);");
        header.Line();
        header.DocComment("Writes the name of a " + protocol_enum.name +
                          " value to a stream.\n\n"
                          "@param stream the stream to write to.\n"
                          "@param value the value to write the name of.\n"
                          "@return the stream.\n"
                          "@see ToString(" +
                          protocol_enum.name + ")");
        header.Line("EOLIB_API std::ostream& operator<<(std::ostream& stream, " + protocol_enum.name + " value);");
        header.Line();

        source.Open("std::string ToString(" + protocol_enum.name + " value)");
        source.Open("switch (value)");
        for (const auto& value : protocol_enum.values)
        {
            source.Line("case " + protocol_enum.name + "::" + value.name + ":");
            source.Indent();
            source.Line("return \"" + value.name + "\";");
            source.Dedent();
        }
        source.Line("default:");
        source.Indent();
        source.Line("return \"Unrecognized(\" + std::to_string(static_cast<int>(value)) + \")\";");
        source.Dedent();
        source.Close();
        source.Close();
        source.Line();
        source.Open("std::ostream& operator<<(std::ostream& stream, " + protocol_enum.name + " value)");
        source.Line("return stream << ToString(value);");
        source.Close();
        source.Line();
    }

    header.EndNamespace(file.Namespace());
    source.EndNamespace(file.Namespace());
    result.push_back(header.ToOutput());
    result.push_back(source.ToOutput());
}

} // namespace eolib::generator
