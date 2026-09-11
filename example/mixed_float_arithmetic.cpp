// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256.hpp>
#include <iomanip>
#include <iostream>

int main()
{
    // Operations between uint256 and the built-in floating point types follow the C++
    // usual arithmetic conversions, identical to the built-in unsigned __int128: the
    // 256-bit operand is converted to the floating point type first, and the result is
    // that floating point type.

    using boost::int256::uint256;

    std::cout << "=== Arithmetic ===" << std::endl;

    constexpr uint256 value {5U};

    std::cout << "value           = " << value << std::endl;
    std::cout << "value + 1.0     = " << (value + 1.0) << std::endl;
    std::cout << "1.0 - value     = " << (1.0 - value) << std::endl;

    // The floating point operand is not truncated first, so this is 2.5 and not 0
    std::cout << "value * 0.5     = " << (value * 0.5) << std::endl;
    std::cout << "value / 2.0     = " << (value / 2.0) << std::endl;
    std::cout << "value * 1.5F    = " << (value * 1.5F) << " (a float operand gives a float)" << std::endl;

    std::cout << "\n=== Comparisons ===" << std::endl;

    std::cout << std::boolalpha;
    std::cout << "value < 5.5     = " << (value < 5.5) << std::endl;
    std::cout << "value == 5.0    = " << (value == 5.0) << std::endl;
    std::cout << "value > 4.999   = " << (value > 4.999) << std::endl;

    std::cout << "\n=== Compound Assignment ===" << std::endl;

    // The value is converted to double, the operation is applied, and the result is
    // converted back, truncating toward zero
    uint256 accumulator {4U};
    accumulator *= 2.5;
    std::cout << "uint256{4} *= 2.5         = " << accumulator << std::endl;

    // A floating point left operand keeps its own type
    double total {1.0};
    total += uint256{2U};
    std::cout << "double{1.0} += uint256{2} = " << total << std::endl;

    std::cout << "\n=== Conversions to floating point ===" << std::endl;

    // The conversion is correctly rounded, to nearest with ties to even, on every
    // platform and in both builtin configurations
    std::cout << std::setprecision(17);
    std::cout << "double(2^200)  = " << static_cast<double>(uint256{1U} << 200U) << std::endl;
    std::cout << "double(max)    = " << static_cast<double>(~uint256{}) << std::endl;
    // A float tops out around 3.4e38, so 2^200 is above its range and becomes infinity
    std::cout << "float(2^200)   = " << static_cast<float>(uint256{1U} << 200U)
              << " (above the range of a float)" << std::endl;
    std::cout << "float(2^100)   = " << static_cast<float>(uint256{1U} << 100U) << std::endl;

    // A double has 53 significand bits, so anything wider is rounded. The max rounds up
    // to exactly 2^256, which is the first double above the range of the type. The
    // comparison below is the library's mixed operator, which converts the 256-bit
    // operand to double first.
    std::cout << "max == 2^256 as a double: " << (~uint256{} == 1.157920892373162e77) << std::endl;

    std::cout << "\n=== Precision ===" << std::endl;

    // A comparison converts the 256-bit value to double first, so it is only as precise
    // as a double. This is what the built-in types do as well.
    constexpr uint256 two_200 {uint256{1U} << 200U};
    constexpr uint256 two_200_plus_one {two_200 + uint256{1U}};

    std::cout << "2^200 + 1                  = " << two_200_plus_one << std::endl;
    std::cout << "2^200 + 1 == double(2^200) = "
              << (two_200_plus_one == 1.6069380442589903e60) << std::endl;
    std::cout << "2^200 + 1 == 2^200 exactly = " << (two_200_plus_one == two_200) << std::endl;

    return 0;
}
