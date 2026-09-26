#pragma once

#include <stdexcept>
#include <string>

namespace eolib::generator
{

/// An error in the protocol specification, reported to the user with context.
class GeneratorError : public std::runtime_error
{
public:
    explicit GeneratorError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

} // namespace eolib::generator
