#include "eolib/protocol/serializable.hpp"

#include <ostream>

namespace eolib::protocol
{

std::ostream& operator<<(std::ostream& stream, const Serializable& value)
{
    return stream << value.ToString();
}

} // namespace eolib::protocol
