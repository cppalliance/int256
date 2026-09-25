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

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
using boost::int256::int256;
#endif

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 u256_one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
constexpr int256 min_i {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
constexpr int256 max_i {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
#endif

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
    BOOST_TEST(to_b - to_a <= u256_one);

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
    BOOST_TEST_EQ(midpoint(max, u256_one), (max / 2U) + 1U);
    BOOST_TEST_EQ(midpoint(max - 1U, max), max - 1U);
    BOOST_TEST_EQ(midpoint(max, max - 1U), max);

    // Two values whose sum is above 2^256.
    const uint256 high_a {UINT64_C(0xC000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    const uint256 high_b {UINT64_C(0xE000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    BOOST_TEST_EQ(midpoint(high_a, high_b), (uint256{UINT64_C(0xD000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)}));

    // Across every word boundary.
    for (unsigned k {1}; k < 256U; ++k)
    {
        const uint256 power {u256_one << k};
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

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

//=====================================
// int256: rounds towards the first argument, and the intermediate difference
// (computed in the unsigned domain) never overflows even at MIN/MAX
//=====================================

void check_midpoint_i(const int256& a, const int256& b)
{
    const auto m {midpoint(a, b)};

    const auto low {a < b ? a : b};
    const auto high {a < b ? b : a};
    BOOST_TEST(m >= low);
    BOOST_TEST(m <= high);

    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    // Every quantity below is computed in the wide signed domain, so nothing
    // here can overflow the way a naive (a + b) / 2 would in int256 itself.
    const smp sa {to_smp(a)};
    const smp sb {to_smp(b)};
    const smp sm {to_smp(m)};

    const smp to_a {sm > sa ? sm - sa : sa - sm};
    const smp to_b {sm > sb ? sm - sb : sb - sm};

    BOOST_TEST(to_a <= to_b);
    BOOST_TEST(to_b - to_a <= 1);

    // Directly against the definition: a + (b - a) / 2, where division
    // truncates towards zero, exactly like int division on a builtin type.
    const smp expected {sa + (sb - sa) / 2};

    BOOST_TEST(sm == expected ||
               report_mismatch_i("midpoint", a, b, m, expected));
    #endif
}

void test_known_answers_i()
{
    BOOST_TEST(midpoint(int256{0}, int256{10}) == int256{5});
    BOOST_TEST(midpoint(int256{10}, int256{0}) == int256{5});
    BOOST_TEST(midpoint(int256{0}, int256{-10}) == int256{-5});
    BOOST_TEST(midpoint(int256{-10}, int256{0}) == int256{-5});
    BOOST_TEST(midpoint(int256{0}, int256{9}) == int256{4});
    BOOST_TEST(midpoint(int256{9}, int256{0}) == int256{5});
    BOOST_TEST(midpoint(int256{0}, int256{-9}) == int256{-4});
    BOOST_TEST(midpoint(int256{-9}, int256{0}) == int256{-5});
    BOOST_TEST(midpoint(int256{-4}, int256{4}) == int256{0});
    BOOST_TEST(midpoint(int256{-5}, int256{5}) == int256{0});
    BOOST_TEST(midpoint(int256{5}, int256{-5}) == int256{0});
    BOOST_TEST(midpoint(int256{-42}, int256{-42}) == int256{-42});
}

// The two extremes a naive average gets wrong: MIN + MAX overflows a plain
// sum, and MIN - MIN or MAX - MAX as a naive (a + b) / 2 fares no better once
// the intermediate sum itself is out of range.
void test_extremes_i()
{
    BOOST_TEST(midpoint(min_i, min_i) == min_i);
    BOOST_TEST(midpoint(max_i, max_i) == max_i);

    // The true midpoint of MIN and MAX is -0.5; rounding towards MIN (the
    // more negative of the two, and the first argument) rounds down to -1.
    BOOST_TEST(midpoint(min_i, max_i) == int256{-1});
    // Swapped: rounding towards MAX (the first argument here) rounds up to 0.
    BOOST_TEST(midpoint(max_i, min_i) == int256{0});

    // MIN's magnitude (2^255) is even, so there is no rounding ambiguity and
    // both argument orders agree.
    BOOST_TEST(midpoint(min_i, int256{0}) == min_i / 2);
    BOOST_TEST(midpoint(int256{0}, min_i) == min_i / 2);

    // MAX's magnitude (2^255 - 1) is odd, so the rounding direction matters.
    BOOST_TEST(midpoint(max_i, int256{0}) == (max_i / 2) + 1);
    BOOST_TEST(midpoint(int256{0}, max_i) == max_i / 2);

    // Across every word boundary, both signs.
    for (unsigned k {1}; k < 255U; ++k)
    {
        const int256 power {static_cast<int256>(u256_one << k)};
        check_midpoint_i(int256{}, power);
        check_midpoint_i(power, power);
        check_midpoint_i(-power, power);
        check_midpoint_i(power, -power);
        check_midpoint_i(power - 1, power);
        check_midpoint_i(power, power - 1);
    }
}

void test_corpus_and_random_i()
{
    for (const auto& a : signed_edge_values())
    {
        for (const auto& b : signed_edge_values())
        {
            check_midpoint_i(a, b);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto a {random_int256(rng, shape::word4, sign::mixed)};
        const auto b {random_int256(rng, shape::word4, sign::mixed)};

        check_midpoint_i(a, b);
        check_midpoint_i(b, a);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_constexpr_i()
{
    static_assert(midpoint(int256{0}, int256{10}) == int256{5}, "midpoint(0, 10)");
    static_assert(midpoint(int256{0}, int256{-10}) == int256{-5}, "midpoint(0, -10)");
    static_assert(midpoint(int256{0}, int256{9}) == int256{4}, "rounds towards the first argument");
    static_assert(midpoint(int256{9}, int256{0}) == int256{5}, "rounds towards the first argument");

    static_assert(midpoint(min_i, min_i) == min_i, "no overflow at MIN");
    static_assert(midpoint(max_i, max_i) == max_i, "no overflow at MAX");
    static_assert(midpoint(min_i, max_i) == int256{-1}, "MIN to MAX, rounding towards MIN");
    static_assert(midpoint(max_i, min_i) == int256{0}, "MAX to MIN, rounding towards MAX");
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_constexpr();
    test_known_answers();
    test_no_overflow();
    test_corpus_and_random();

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_constexpr_i();
    test_known_answers_i();
    test_extremes_i();
    test_corpus_and_random_i();
    #endif

    return boost::report_errors();
}
