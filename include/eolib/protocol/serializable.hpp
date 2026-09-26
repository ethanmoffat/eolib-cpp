#pragma once

#include "eolib/data/eo_reader.hpp"
#include "eolib/data/eo_writer.hpp"
#include "eolib/export.hpp"

#include <string>

namespace eolib::protocol
{

/// An object that can be serialized to and deserialized from EO data.
class EOLIB_API Serializable
{
public:
    virtual ~Serializable() = default;

    /// Serializes this object to the provided writer.
    /// @throws SerializationError if the object's data is invalid for serialization.
    virtual void Serialize(data::EoWriter& writer) const = 0;

    /// Deserializes this object from the provided reader, replacing its current data.
    virtual void Deserialize(data::EoReader& reader) = 0;

    /// Gets the size of the data that this object was deserialized from.
    ///
    /// For objects that were not deserialized, this value is 0.
    virtual int ByteSize() const = 0;

    /// Gets a human-readable representation of this object, for debugging purposes.
    virtual std::string ToString() const = 0;

protected:
    Serializable() = default;
    Serializable(const Serializable&) = default;
    Serializable(Serializable&&) = default;
    Serializable& operator=(const Serializable&) = default;
    Serializable& operator=(Serializable&&) = default;
};

} // namespace eolib::protocol
