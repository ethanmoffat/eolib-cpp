#pragma once

namespace eolib::data
{

/// Maximum values (exclusive) of the EO encoded numeric types.
/// The largest valid value for each type is <c>TypeMax - 1</c>.
struct EoNumericLimits
{
    /// The maximum value of an EO char (1-byte encoded integer).
    static constexpr unsigned int CharMax = 253;

    /// The maximum value of an EO short (2-byte encoded integer).
    static constexpr unsigned int ShortMax = CharMax * CharMax;

    /// The maximum value of an EO three (3-byte encoded integer).
    static constexpr unsigned int ThreeMax = CharMax * CharMax * CharMax;

    /// The maximum value of an EO int (4-byte encoded integer).
    static constexpr unsigned int IntMax = ShortMax * ShortMax;
};

} // namespace eolib::data
