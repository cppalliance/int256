// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// midpoint. The whole point of the bit manipulation formula is that it never
// overflows, so midpoint(max, max) and midpoint(max, 0) are the cases that
// matter: a naive (a + b) / 2 wraps on the first and is the reason this function
// exists. std::midpoint rounds towards the first argument, which for an unsigned
// type means rounding down when a < b and up when a > b.

#ifndef BOOST_INT256_BUILD_MODULE

#include <boost/int256.hpp>

#else

import boost.int256;
// A module import carries no macros, so the BOOST_INT256_* feature and endianness
// macros have to come from the header even in the module build.
#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/literal_macros.hpp>

#endif

#include "oracle.hpp"

#include <cstddef>
#include <cstdint>

using boost::int256::uint256;
using boost::int256::midpoint;

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

void check_midpoint(const uint256& a, const uint256& b)
{
    const auto m {midpoint(a, b)};

    // The midpoint lies between the two operands, inclusive.
    const auto low {a < b ? a : b};
    const auto high {a < b ? b : a};
    BOOST_TEST(m >= low);
    BOOST_TEST(m <= high);

    // The rounding is towards the first argument, so the result is never further
    // from a than from b, and the two distances differ by at most one.
    const auto to_a {m > a ? m - a : a - m};
    const auto to_b {m > b ? m - b : b - m};

    BOOST_TEST(to_a <= to_b);
    BOOST_TEST(to_b - to_a <= one);

    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    // Against the reference, where the sum cannot wrap because the reference
    // halves it before it is stored back.
    const mp sum_low {to_mp(low)};
    const mp sum_high {to_mp(high)};
    const mp difference {sum_high - sum_low};
    const mp expected_down {sum_low + mp{difference / 2}};
    const mp expected_up {sum_low + mp{difference / 2} + ((difference % 2) != 0 ? mp{1} : mp{0})};

    BOOST_TEST(to_mp(m) == (a <= b ? expected_down : expected_up) ||
               report_mismatch("midpoint", a, b, m, a <= b ? expected_down : expected_up));
    #endif
}

void test_known_answers()
{
    BOOST_TEST_EQ(midpoint(uint256{0U}, uint256{10U}), uint256{5U});
    BOOST_TEST_EQ(midpoint(uint256{10U}, uint256{0U}), uint256{5U});
    BOOST_TEST_EQ(midpoint(uint256{0U}, uint256{9U}), uint256{4U});
    BOOST_TEST_EQ(midpoint(uint256{9U}, uint256{0U}), uint256{5U});
    BOOST_TEST_EQ(midpoint(uint256{4U}, uint256{4U}), uint256{4U});
    BOOST_TEST_EQ(midpoint(uint256{}, uint256{}), uint256{});
    BOOST_TEST_EQ(midpoint(uint256{1U}, uint256{2U}), uint256{1U});
    BOOST_TEST_EQ(midpoint(uint256{2U}, uint256{1U}), uint256{2U});
}

// The two cases a naive average gets wrong.
void test_no_overflow()
{
    const auto max {uint256_max()};

    BOOST_TEST_EQ(midpoint(max, max), max);
    // The maximum is odd, so the exact midpoint of 0 and it falls between two
    // integers and the rounding direction decides which one comes out.
    BOOST_TEST_EQ(midpoint(max, uint256{}), (max / 2U) + 1U);
    BOOST_TEST_EQ(midpoint(uint256{}, max), max / 2U);
    BOOST_TEST_EQ(midpoint(max, one), (max / 2U) + 1U);
    BOOST_TEST_EQ(midpoint(max - 1U, max), max - 1U);
    BOOST_TEST_EQ(midpoint(max, max - 1U), max);

    // Two values whose sum is above 2^256.
    const uint256 high_a {UINT64_C(0xC000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    const uint256 high_b {UINT64_C(0xE000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    BOOST_TEST_EQ(midpoint(high_a, high_b), (uint256{UINT64_C(0xD000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)}));

    // Across every word boundary.
    for (unsigned k {1}; k < 256U; ++k)
    {
        const uint256 power {one << k};
        BOOST_TEST_EQ(midpoint(uint256{}, power), power / 2U);
        BOOST_TEST_EQ(midpoint(power, power), power);
        check_midpoint(power - 1U, power);
        check_midpoint(power, power - 1U);
    }
}

void test_corpus_and_random()
{
    for (const auto& a : edge_values())
    {
        for (const auto& b : edge_values())
        {
            check_midpoint(a, b);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto a {random_uint256(rng, shape::word4)};
        const auto b {random_uint256(rng, shape::word4)};

        check_midpoint(a, b);
        check_midpoint(b, a);

        // Adjacent values, where the rounding direction is the whole answer.
        check_midpoint(a, a + 1U);
        check_midpoint(a + 1U, a);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_constexpr()
{
    static_assert(midpoint(uint256{0U}, uint256{10U}) == uint256{5U}, "midpoint(0, 10)");
    static_assert(midpoint(uint256{0U}, uint256{9U}) == uint256{4U}, "rounds towards the first argument");
    static_assert(midpoint(uint256{9U}, uint256{0U}) == uint256{5U}, "rounds towards the first argument");

    constexpr uint256 max {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    static_assert(midpoint(max, max) == max, "no overflow at the top of the range");
    static_assert(midpoint(uint256{}, max) == max / 2U, "rounding towards the smaller end");
    static_assert(midpoint(max, uint256{}) == (max / 2U) + 1U, "rounding towards the larger end");
}

} // namespace

int main()
{
    test_constexpr();
    test_known_answers();
    test_no_overflow();
    test_corpus_and_random();

    return boost::report_errors();
}
