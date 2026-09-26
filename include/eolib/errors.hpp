#pragma once

#include "eolib/export.hpp"

#include <stdexcept>
#include <string>

namespace eolib
{

/// Base class for errors raised by eolib.
class EOLIB_API EolibError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/// Raised when a protocol object cannot be serialized, e.g. a field value violates a length constraint or a switch
/// case payload does not match the value of its switch field.
class EOLIB_API SerializationError : public EolibError
{
public:
    using EolibError::EolibError;
};

/// Raised when data cannot be deserialized into a protocol object.
class EOLIB_API DeserializationError : public EolibError
{
public:
    using EolibError::EolibError;
};

} // namespace eolib
