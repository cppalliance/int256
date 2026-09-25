// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>
#include <boost/int256/numeric.hpp>
#include <boost/int256/iostream.hpp>
#include <boost/int256/limits.hpp>
#include <iostream>
#include <limits>

int main()
{
    using boost::int256::uint256;

    std::cout << "=== Greatest Common Divisor (gcd) ===" << std::endl;

    // Basic gcd
    constexpr uint256 a {48U};
    constexpr uint256 b {18U};
    std::cout << "gcd(" << a << ", " << b << ") = " << boost::int256::gcd(a, b) << std::endl;

    // gcd with larger values
    constexpr uint256 large_a {123456789012345678ULL};
    constexpr uint256 large_b {987654321098765432ULL};
    std::cout << "gcd(" << large_a << ", " << large_b << ") = "
              << boost::int256::gcd(large_a, large_b) << std::endl;

    // gcd of values that need all four words
    constexpr uint256 huge_a {uint256{1U} << 200U};
    constexpr uint256 huge_b {uint256{1U} << 160U};
    std::cout << "gcd(2^200, 2^160) = " << boost::int256::gcd(huge_a, huge_b) << " (= 2^160)" << std::endl;

    // A 256-bit gcd of two products with a known common factor
    constexpr uint256 factor {uint256{1U} << 100U};
    std::cout << "gcd(3 * 2^100, 5 * 2^100) = " << boost::int256::gcd(3U * factor, 5U * factor)
              << " (= 2^100)" << std::endl;

    std::cout << "\n=== Least Common Multiple (lcm) ===" << std::endl;

    // Basic lcm
    constexpr uint256 x {12U};
    constexpr uint256 y {18U};
    std::cout << "lcm(" << x << ", " << y << ") = " << boost::int256::lcm(x, y) << std::endl;

    // lcm with coprime numbers
    constexpr uint256 p {7U};
    constexpr uint256 q {11U};
    std::cout << "lcm(" << p << ", " << q << ") = " << boost::int256::lcm(p, q)
              << " (coprime: lcm = p * q)" << std::endl;

    // The lcm of two 128-bit primes needs 256 bits, which is why the wide type helps
    constexpr uint256 prime_a {UINT64_C(18446744073709551557)};  // The largest 64-bit prime
    constexpr uint256 prime_b {UINT64_C(18446744073709551533)};  // The next one down
    std::cout << "lcm(largest two 64-bit primes) = " << boost::int256::lcm(prime_a, prime_b) << std::endl;

    // Relationship: gcd(a,b) * lcm(a,b) = a * b
    std::cout << "\nVerifying gcd * lcm = a * b:" << std::endl;
    const auto g {boost::int256::gcd(x, y)};
    const auto l {boost::int256::lcm(x, y)};
    std::cout << "gcd(" << x << ", " << y << ") * lcm(" << x << ", " << y << ") = " << (g * l) << std::endl;
    std::cout << x << " * " << y << " = " << (x * y) << std::endl;

    std::cout << "\n=== Midpoint ===" << std::endl;

    constexpr uint256 low {10U};
    constexpr uint256 high {20U};
    std::cout << "midpoint(" << low << ", " << high << ") = "
              << boost::int256::midpoint(low, high) << std::endl;

    // With an odd difference the result rounds toward the first argument
    constexpr uint256 odd_low {10U};
    constexpr uint256 odd_high {21U};
    std::cout << "midpoint(" << odd_low << ", " << odd_high << ") = "
              << boost::int256::midpoint(odd_low, odd_high) << " (rounds toward first arg)" << std::endl;
    std::cout << "midpoint(" << odd_high << ", " << odd_low << ") = "
              << boost::int256::midpoint(odd_high, odd_low) << " (rounds toward first arg)" << std::endl;

    std::cout << "\n--- Overflow-safe midpoint ---" << std::endl;

    // midpoint never overflows, which (a + b) / 2 does at this magnitude
    constexpr auto max_value {(std::numeric_limits<uint256>::max)()};
    constexpr auto max_minus_10 {max_value - 10U};
    std::cout << "midpoint(max, max - 10) = " << boost::int256::midpoint(max_value, max_minus_10) << std::endl;
    std::cout << "(max + (max - 10)) / 2  = " << ((max_value + max_minus_10) / 2U) << " (wrapped)" << std::endl;

    std::cout << "\n--- Binary search over the whole range ---" << std::endl;

    // A midpoint that cannot overflow is what lets a binary search run over the
    // full 256-bit range. This finds the integer square root of 2^200 by bisection.
    constexpr uint256 target {uint256{1U} << 200U};
    uint256 lo {0U};
    uint256 hi {max_value};
    while (hi - lo > 1U)
    {
        const auto mid {boost::int256::midpoint(lo, hi)};
        if (mid != 0U && mid <= target / mid)
        {
            lo = mid;
        }
        else
        {
            hi = mid;
        }
    }

    std::cout << "isqrt(2^200) by bisection = " << lo << " (= 2^100)" << std::endl;

    std::cout << "\n=== gcd, lcm and midpoint with int256 ===" << std::endl;

    using boost::int256::int256;

    // gcd and lcm work on the uint256 magnitudes of the operands, so the
    // result is always non-negative regardless of the operands' signs
    constexpr int256 neg_a {-48};
    constexpr int256 pos_b {18};
    std::cout << "gcd(" << neg_a << ", " << pos_b << ") = " << boost::int256::gcd(neg_a, pos_b)
              << " (always non-negative)" << std::endl;

    // gcd(MIN, 0) is the one case whose magnitude is not representable as a
    // positive int256; like unary -, it wraps rather than being undefined
    constexpr auto min_value {(std::numeric_limits<int256>::min)()};
    std::cout << "gcd(MIN, 0) = " << boost::int256::gcd(min_value, int256{0}) << " (wraps to MIN)" << std::endl;

    std::cout << "\n--- Signed midpoint ---" << std::endl;

    constexpr int256 neg {-100};
    constexpr int256 pos {100};
    std::cout << "midpoint(" << neg << ", " << pos << ") = "
              << boost::int256::midpoint(neg, pos) << std::endl;

    constexpr int256 neg2 {-100};
    constexpr int256 neg3 {-50};
    std::cout << "midpoint(" << neg2 << ", " << neg3 << ") = "
              << boost::int256::midpoint(neg2, neg3) << std::endl;

    return 0;
}
