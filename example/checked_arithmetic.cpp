// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Individual headers

#include <boost/int256/utilities.hpp>
#include <boost/int256/iostream.hpp>
#include <boost/int256/limits.hpp>

// Or you can do a single header

// #include <boost/int256.hpp>

#include <cstdint>
#include <iostream>
#include <limits>

int main()
{
    using boost::int256::uint256;
    using boost::int256::ckd_add;
    using boost::int256::ckd_sub;
    using boost::int256::ckd_mul;

    std::cout << std::boolalpha;

    // ckd_add, ckd_sub, and ckd_mul implement the C23 stdckdint.h contract: the
    // operation is evaluated as if both operands had infinite range, the result
    // is written to *result wrapped to that type's width, and the function
    // returns true when the exact result did not fit.
    constexpr auto max_value {(std::numeric_limits<uint256>::max)()};

    // A result that fits returns false and holds the exact value.
    std::cout << "=== Results That Fit ===" << std::endl;
    uint256 r {};
    bool overflow {ckd_add(&r, uint256{20U}, uint256{22U})};
    std::cout << "ckd_add(20, 22): overflow=" << overflow << ", result=" << r << std::endl;

    // Addition that exceeds the type wraps modulo 2^256 and reports overflow.
    std::cout << "\n=== Addition Overflow ===" << std::endl;
    overflow = ckd_add(&r, max_value, uint256{1U});
    std::cout << "ckd_add(max, 1): overflow=" << overflow << ", wrapped=" << r << std::endl;

    // Subtracting below zero in an unsigned type wraps to the top of the range.
    std::cout << "\n=== Subtraction Underflow ===" << std::endl;
    overflow = ckd_sub(&r, uint256{0U}, uint256{1U});
    std::cout << "ckd_sub(0, 1): overflow=" << overflow << ", wrapped=" << r << std::endl;

    // Multiplication detects the overflow that operator* silently wraps. This is the
    // case a 256-bit type runs into most often: the square of anything at or above
    // 2^128 does not fit.
    std::cout << "\n=== Multiplication Overflow ===" << std::endl;
    constexpr uint256 two_to_128 {uint256{1U} << 128U};
    overflow = ckd_mul(&r, two_to_128, two_to_128);
    std::cout << "ckd_mul(2^128, 2^128): overflow=" << overflow << ", wrapped=" << r << std::endl;

    overflow = ckd_mul(&r, two_to_128 - 1U, two_to_128 - 1U);
    std::cout << "ckd_mul(2^128 - 1, 2^128 - 1): overflow=" << overflow << ", result=" << r
              << "\n  (the largest product that still fits)" << std::endl;

    // The result type and the two operand types are independent: they may differ
    // in width and signedness, and the exact mathematical value is always used.
    std::cout << "\n=== Mixed Types ===" << std::endl;
    std::uint64_t narrow {};
    overflow = ckd_add(&narrow, uint256{5U}, 3);
    std::cout << "ckd_add<uint64_t>(uint256{5}, 3): overflow=" << overflow
              << ", result=" << narrow << std::endl;

    overflow = ckd_add(&narrow, max_value, uint256{0U});
    std::cout << "ckd_add<uint64_t>(max, 0): overflow=" << overflow
              << ", wrapped=" << narrow << std::endl;

    // A wide result from narrow operands cannot overflow, which is a convenient way
    // to get the exact product of two 128-bit values
    overflow = ckd_mul(&r, UINT64_MAX, UINT64_MAX);
    std::cout << "ckd_mul<uint256>(UINT64_MAX, UINT64_MAX): overflow=" << overflow
              << ", result=" << r << std::endl;

    // Narrow targets make the wrap-around easy to see (400 modulo 256 is 144).
    std::uint8_t byte {};
    overflow = ckd_mul(&byte, std::uint8_t{20}, std::uint8_t{20});
    std::cout << "ckd_mul<uint8_t>(20, 20): overflow=" << overflow
              << ", wrapped=" << static_cast<int>(byte) << std::endl;

    return 0;
}
