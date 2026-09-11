// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Individual headers

#include <boost/int256/int256.hpp>
#include <boost/int256/numeric.hpp>
#include <boost/int256/iostream.hpp>
#include <boost/int256/limits.hpp>

// Or you can do a single header

// #include <boost/int256.hpp>

#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

int main()
{
    using boost::int256::uint256;

    // std::numeric_limits is specialized for the type
    constexpr auto max_value {(std::numeric_limits<uint256>::max)()};
    static_assert(std::is_same<decltype(max_value), const uint256>::value, "Types should match");

    std::cout << "=== Saturating Arithmetic ===" << std::endl;
    std::cout << "uint256 max = " << max_value << std::endl;

    // Saturating arithmetic clamps to max on overflow and to zero on underflow
    // rather than wrapping the way the operators do
    std::cout << "\n=== Saturating Addition and Subtraction ===" << std::endl;
    std::cout << "saturating_add(max, max) = " << boost::int256::saturating_add(max_value, max_value)
              << "\n  (saturates to max; max + max would wrap to max - 1)" << std::endl;
    std::cout << "saturating_sub(0, max)   = " << boost::int256::saturating_sub(uint256{0U}, max_value)
              << "\n  (saturates to 0; 0 - max would wrap to 1)" << std::endl;

    std::cout << "\n=== Saturating Multiplication ===" << std::endl;

    // A 256-bit type is easiest to overflow through multiplication: any two values
    // at or above 2^128 have a product that does not fit
    constexpr uint256 two_to_128 {uint256{1U} << 128U};
    std::cout << "saturating_mul(2^128, 2^128) = " << boost::int256::saturating_mul(two_to_128, two_to_128)
              << "\n  (saturates to max; the operator wraps to 0)" << std::endl;
    std::cout << "saturating_mul(max, 2)       = " << boost::int256::saturating_mul(max_value, uint256{2U})
              << std::endl;
    std::cout << "saturating_mul(2^100, 2^100) = "
              << boost::int256::saturating_mul(uint256{1U} << 100U, uint256{1U} << 100U)
              << "\n  (fits exactly, so nothing saturates)" << std::endl;

    std::cout << "\n=== Saturating Division ===" << std::endl;

    // An unsigned quotient is never larger than the dividend, so saturating_div is
    // provided for interface parity and simply divides
    std::cout << "saturating_div(max, 3) = " << boost::int256::saturating_div(max_value, uint256{3U}) << std::endl;

    std::cout << "\n=== Saturating Casts ===" << std::endl;

    // saturating_cast converts to a narrower type by clamping instead of truncating
    std::cout << "saturating_cast<std::uint64_t>(max) = "
              << boost::int256::saturating_cast<std::uint64_t>(max_value)
              << " (saturates to UINT64_MAX)" << std::endl;
    std::cout << "saturating_cast<std::int64_t>(max)  = "
              << boost::int256::saturating_cast<std::int64_t>(max_value)
              << " (saturates to INT64_MAX)" << std::endl;
    std::cout << "saturating_cast<std::int32_t>(max)  = "
              << boost::int256::saturating_cast<std::int32_t>(max_value)
              << " (saturates to INT32_MAX)" << std::endl;
    std::cout << "saturating_cast<std::uint64_t>(42)  = "
              << boost::int256::saturating_cast<std::uint64_t>(uint256{42U})
              << " (fits, so it is exact)" << std::endl;

    return 0;
}
