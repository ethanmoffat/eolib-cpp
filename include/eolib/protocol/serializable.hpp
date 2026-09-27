#pragma once

#include "eolib/data/eo_reader.hpp"
#include "eolib/data/eo_writer.hpp"
#include "eolib/export.hpp"

#include <iosfwd>
#include <string>

namespace eolib::protocol
{

/// An object that can be serialized to and deserialized from EO data.
class EOLIB_API Serializable
{
public:
    virtual ~Serializable() = default;

    /// Serializes this object to the provided writer.
    ///
    /// @param writer the writer that the data will be written to.
    /// @throws SerializationError if the object's data is invalid for serialization.
    virtual void Serialize(data::EoWriter& writer) const = 0;

    /// Deserializes this object from the provided reader, replacing its current data.
    ///
    /// @param reader the reader that the data will be read from.
    virtual void Deserialize(data::EoReader& reader) = 0;

    /// Gets the size of the data that this object was deserialized from.
    ///
    /// @return the deserialized size in bytes, or 0 for an object that was not deserialized.
    virtual int ByteSize() const noexcept = 0;

    /// Gets a human-readable representation of this object, for debugging purposes.
    ///
    /// @return the object's fields and values, as a string.
    virtual std::string ToString() const = 0;

protected:
    Serializable() = default;
    Serializable(const Serializable&) = default;
    Serializable(Serializable&&) = default;
    Serializable& operator=(const Serializable&) = default;
    Serializable& operator=(Serializable&&) = default;
};

/// Writes a human-readable representation of an object to a stream, for debugging purposes.
///
/// @param stream the stream to write to.
/// @param value the object to write.
/// @return the stream.
/// @see Serializable::ToString
EOLIB_API std::ostream& operator<<(std::ostream& stream, const Serializable& value);

} // namespace eolib::protocol
