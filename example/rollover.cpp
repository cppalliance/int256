// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// This example demonstrates the rollover behavior of uint256

#include <boost/int256.hpp>
#include <iostream>
#include <limits>

int main()
{
    using boost::int256::uint256;
    using boost::int256::int256;

    constexpr uint256 max_value {(std::numeric_limits<uint256>::max)()};
    constexpr uint256 min_value {(std::numeric_limits<uint256>::min)()};

    std::cout << "=== uint256 behavior ===" << std::endl;

    // uint256 is a modulo type, exactly like the built-in unsigned integers: every
    // operation is reduced modulo 2^256 and nothing is undefined at the boundary.
    std::cout << "is_modulo: " << std::boolalpha
              << std::numeric_limits<uint256>::is_modulo << "\n\n";

    std::cout << "Max of uint256: " << max_value << '\n'
              << "Max + 1U: " << max_value + 1U << "\n\n";

    std::cout << "Min of uint256: " << min_value << '\n'
              << "Min - 1U: " << min_value - 1U << "\n\n";

    // The same holds for multiplication, which is where a 256-bit type is easiest to
    // overflow: the square of anything at or above 2^128 does not fit.
    constexpr uint256 two_to_the_128 {uint256{1U} << 128U};

    std::cout << "2^128: " << two_to_the_128 << '\n'
              << "2^128 * 2^128 (wraps to zero): " << two_to_the_128 * two_to_the_128 << '\n' << std::endl;

    std::cout << "=== int256 behavior ===" << std::endl;

    // int256 is also a modulo type: the arithmetic operators wrap in the
    // unsigned domain, so there is no signed-overflow undefined behavior the
    // way there is for a built-in signed integer.
    std::cout << "is_modulo: " << std::boolalpha
              << std::numeric_limits<int256>::is_modulo << "\n\n";

    constexpr int256 max_signed_value {(std::numeric_limits<int256>::max)()};
    constexpr int256 min_signed_value {(std::numeric_limits<int256>::min)()};

    std::cout << "Max of int256: " << max_signed_value << '\n'
              << "Max + 1: " << max_signed_value + 1 << "\n";

    std::cout << "\nMin of int256: " << min_signed_value << '\n'
              << "Min - 1: " << min_signed_value - 1 << '\n' << std::endl;

    // MIN / -1 is the one signed overflow this library defines rather than
    // leaving undefined: it wraps back to MIN, matching every other
    // overflowing signed operation such as MIN * -1.
    std::cout << "MIN / -1: " << min_signed_value / int256{-1} << '\n'
              << "MIN * -1: " << min_signed_value * int256{-1} << '\n'
              << "-MIN:     " << -min_signed_value << std::endl;

    return 0;
}
