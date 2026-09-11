// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Every operator evaluated inside a consteval function, so that a path which
// silently falls back to a runtime-only intrinsic stops compiling instead of
// quietly changing which code the constant evaluator runs. The division here is
// the Knuth path, because the reciprocal accelerator is gated to run time.

#include <boost/int256.hpp>

// Only warns on the oldest MSVC the library supports.
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4307)
#endif

#if defined(__cpp_consteval) && __cpp_consteval >= 201811L
#  define BOOST_INT256_TEST_CONSTEVAL consteval
#else
#  define BOOST_INT256_TEST_CONSTEVAL constexpr
#endif

using boost::int256::uint256;
using namespace boost::int256::literals;

template <typename T>
BOOST_INT256_TEST_CONSTEVAL bool test_small()
{
    constexpr T x {2U};
    constexpr T y {3U};

    static_assert(x < y, "");
    static_assert(x <= y, "");
    static_assert(y > x, "");
    static_assert(y >= x, "");
    static_assert(y == 3U, "");
    static_assert(x != 3U, "");
    static_assert(x != y, "");

    static_assert(x + y == 5U, "");
    static_assert(y - x == 1U, "");
    static_assert(y * x == 6U, "");
    static_assert(y / x == 1U, "");
    static_assert(y % x == 1U, "");

    static_assert(x + 2U == 4U, "");
    static_assert(x - 2U == 0U, "");
    static_assert(x * 2U == 4U, "");
    static_assert(x / 2U == 1U, "");
    static_assert(x % 2U == 0U, "");

    static_assert((x & y) == 2U, "");
    static_assert((x | y) == 3U, "");
    static_assert((x ^ y) == 1U, "");
    static_assert((x << 1U) == 4U, "");
    static_assert((y >> 1U) == 1U, "");

    static_assert(+x == 2U, "");
    static_assert(-(-x) == 2U, "");

    return true;
}

template <typename T>
BOOST_INT256_TEST_CONSTEVAL bool test_wide()
{
    // Operands wide enough that every word takes part.
    constexpr T x {UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)};
    constexpr T y {UINT64_C(0), UINT64_C(0), UINT64_MAX, UINT64_MAX - 2U};

    static_assert(x > y, "");
    static_assert(x >= y, "");
    static_assert(y < x, "");
    static_assert(y <= x, "");
    static_assert(x == x, "");
    static_assert(x != y, "");

    // Add with a carry out of words 0 and 1.
    constexpr T carry_lhs {UINT64_C(0), UINT64_C(0), UINT64_MAX, UINT64_MAX};
    constexpr T carry_sum {UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)};
    static_assert(carry_lhs + T{1U} == carry_sum, "");
    static_assert(carry_sum - T{1U} == carry_lhs, "");

    // A product that needs all four result words.
    constexpr T two_128 {UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)};
    constexpr T two_128_minus_one {UINT64_C(0), UINT64_C(0), UINT64_MAX, UINT64_MAX};
    constexpr T square {UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFE), UINT64_C(0), UINT64_C(1)};
    static_assert(two_128_minus_one * two_128_minus_one == square, "");

    // Division with a two, three and four word divisor.
    constexpr T max {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    constexpr T ones {UINT64_C(1), UINT64_C(1), UINT64_C(1), UINT64_C(1)};
    static_assert(max / (two_128 - T{1U}) == T(UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(1)), "");
    static_assert(max / ones == T{UINT64_MAX}, "");
    static_assert(max / max == T{1U}, "");
    static_assert(max % max == T{}, "");
    static_assert(square / two_128_minus_one == two_128_minus_one, "");
    static_assert(square % two_128_minus_one == T{}, "");

    // Shifts that cross every word boundary.
    constexpr T one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    static_assert(((one << 64U) >> 64U) == one, "");
    static_assert(((one << 128U) >> 128U) == one, "");
    static_assert(((one << 192U) >> 192U) == one, "");
    static_assert(((one << 255U) >> 255U) == one, "");
    static_assert(((x << 65U) >> 65U) == T(UINT64_C(0), UINT64_C(2), UINT64_C(3), UINT64_C(4)), "");

    // Bitwise over all four words.
    static_assert((max & x) == x, "");
    static_assert((T{} | x) == x, "");
    static_assert((x ^ x) == T{}, "");
    static_assert(~(~x) == x, "");

    return true;
}

BOOST_INT256_TEST_CONSTEVAL bool test_literals()
{
    // The literal parser runs in constant evaluation.
    static_assert(340282366920938463463374607431768211456_u256 ==
                  uint256(UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)), "2^128");
    static_assert(115792089237316195423570985008687907853269984665640564039457584007913129639935_u256 ==
                  uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX), "2^256 - 1");
    static_assert(18446744073709551616_U256 == uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)), "2^64");
    static_assert(0x10000000000000000_u256 == uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)), "2^64 hex");

    static_assert(BOOST_INT256_UINT256_C(115792089237316195423570985008687907853269984665640564039457584007913129639935) ==
                  uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX), "UINT256_C at the maximum");

    return true;
}

int main()
{
    static_assert(test_small<uint256>(), "");
    static_assert(test_wide<uint256>(), "");
    static_assert(test_literals(), "");

    return 0;
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif
