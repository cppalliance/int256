// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The saturating operations and saturating_cast. For an unsigned type the two
// clamps are zero and the maximum, so what has to be pinned is that the ordinary
// operators wrap where these clamp: every case below states both answers.

#include <boost/int256.hpp>

#include "oracle.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

using boost::int256::uint256;
using boost::int256::saturating_add;
using boost::int256::saturating_sub;
using boost::int256::saturating_mul;
using boost::int256::saturating_div;
using boost::int256::saturating_cast;

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

void check_saturating(const uint256& a, const uint256& b)
{
    const wide wa {to_wide(a)};
    const wide wb {to_wide(b)};
    const wide limit {(wide{1} << 256) - 1};

    // Addition clamps at the maximum.
    {
        const wide exact {wa + wb};
        const uint256 expected {exact > limit ? uint256_max() : from_wide_truncated(exact)};
        BOOST_TEST(saturating_add(a, b) == expected ||
                   report_mismatch("saturating_add", a, b, saturating_add(a, b), to_mp(expected)));
    }

    // Subtraction clamps at zero.
    {
        const uint256 expected {a < b ? uint256{} : a - b};
        BOOST_TEST(saturating_sub(a, b) == expected ||
                   report_mismatch("saturating_sub", a, b, saturating_sub(a, b), to_mp(expected)));
    }

    // Multiplication clamps at the maximum.
    {
        const wide exact {wa * wb};
        const uint256 expected {exact > limit ? uint256_max() : from_wide_truncated(exact)};
        BOOST_TEST(saturating_mul(a, b) == expected ||
                   report_mismatch("saturating_mul", a, b, saturating_mul(a, b), to_mp(expected)));
    }

    // Division cannot overflow for an unsigned type, so it is the plain quotient.
    if (b != 0U)
    {
        BOOST_TEST_EQ(saturating_div(a, b), a / b);
    }
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE

// Where the clamps and the wrapping operators disagree, which is the point.
void test_clamps()
{
    const auto max {uint256_max()};

    // Addition at the top.
    BOOST_TEST_EQ(saturating_add(max, one), max);
    BOOST_TEST_EQ(max + one, uint256{});                 // the operator wraps
    BOOST_TEST_EQ(saturating_add(max, max), max);
    BOOST_TEST_EQ(saturating_add(max, uint256{}), max);
    BOOST_TEST_EQ(saturating_add(max - 1U, one), max);
    BOOST_TEST_EQ(saturating_add(max - 2U, one), max - 1U);
    BOOST_TEST_EQ(saturating_add(uint256{}, uint256{}), uint256{});

    // Subtraction at the bottom.
    BOOST_TEST_EQ(saturating_sub(uint256{}, one), uint256{});
    BOOST_TEST_EQ(uint256{} - one, max);                 // the operator wraps
    BOOST_TEST_EQ(saturating_sub(one, uint256{2U}), uint256{});
    BOOST_TEST_EQ(saturating_sub(uint256{5U}, uint256{5U}), uint256{});
    BOOST_TEST_EQ(saturating_sub(uint256{5U}, uint256{3U}), uint256{2U});
    BOOST_TEST_EQ(saturating_sub(max, max), uint256{});
    BOOST_TEST_EQ(saturating_sub(max, uint256{}), max);

    // Multiplication at the top.
    BOOST_TEST_EQ(saturating_mul(max, uint256{2U}), max);
    BOOST_TEST_EQ(max * uint256{2U}, max - 1U);          // the operator wraps
    BOOST_TEST_EQ(saturating_mul(max, max), max);
    BOOST_TEST_EQ(saturating_mul(max, one), max);
    BOOST_TEST_EQ(saturating_mul(max, uint256{}), uint256{});
    BOOST_TEST_EQ(saturating_mul(uint256{}, max), uint256{});

    // 2^128 squared is exactly 2^256, one past the range.
    BOOST_TEST_EQ(saturating_mul(one << 128U, one << 128U), max);
    BOOST_TEST_EQ(saturating_mul(one << 127U, one << 128U), one << 255U);
    BOOST_TEST_EQ(saturating_mul(one << 255U, uint256{2U}), max);

    // Just below the overflow: (2^128 - 1)^2 fits.
    BOOST_TEST_EQ(saturating_mul((one << 128U) - 1U, (one << 128U) - 1U),
                  ((one << 128U) - 1U) * ((one << 128U) - 1U));

    // Division never saturates.
    BOOST_TEST_EQ(saturating_div(max, one), max);
    BOOST_TEST_EQ(saturating_div(max, max), one);
    BOOST_TEST_EQ(saturating_div(uint256{}, max), uint256{});
}

//=====================================
// saturating_cast
//=====================================

template <typename T>
void check_cast()
{
    // A value inside the target range comes through unchanged.
    BOOST_TEST_EQ(saturating_cast<T>(uint256{}), static_cast<T>(0));
    BOOST_TEST_EQ(saturating_cast<T>(one), static_cast<T>(1));

    const auto target_max {(std::numeric_limits<T>::max)()};
    BOOST_TEST_EQ(saturating_cast<T>(uint256{static_cast<std::uint64_t>(target_max)}), target_max);

    // One past the target maximum clamps rather than truncating.
    BOOST_TEST_EQ(saturating_cast<T>(uint256{static_cast<std::uint64_t>(target_max)} + 1U), target_max);

    // And so does anything much larger, including values with upper words set.
    BOOST_TEST_EQ(saturating_cast<T>(uint256_max()), target_max);
    BOOST_TEST_EQ(saturating_cast<T>(uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0))), target_max);
    BOOST_TEST_EQ(saturating_cast<T>(uint256(UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0))), target_max);

    // Random values in and out of range.
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto v {random_uint256(rng, shape::word4)};
        const auto got {saturating_cast<T>(v)};

        if (v > uint256{static_cast<std::uint64_t>(target_max)})
        {
            BOOST_TEST_EQ(got, target_max);
        }
        else
        {
            BOOST_TEST_EQ(static_cast<std::uint64_t>(got), static_cast<std::uint64_t>(v.words[0]));
        }
    }
}

void test_saturating_cast()
{
    check_cast<signed char>();
    check_cast<unsigned char>();
    check_cast<short>();
    check_cast<unsigned short>();
    check_cast<int>();
    check_cast<unsigned int>();
    check_cast<long>();
    check_cast<unsigned long>();
    check_cast<long long>();
    check_cast<unsigned long long>();

    // Casting to uint256 is the identity.
    BOOST_TEST_EQ(saturating_cast<uint256>(uint256_max()), uint256_max());
    BOOST_TEST_EQ(saturating_cast<uint256>(uint256{42U}), uint256{42U});

    #if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
    // The builtin 128-bit types are valid targets too.
    using builtin_u128 = boost::int256::detail::builtin_u128;
    const auto as_u128 {saturating_cast<builtin_u128>(uint256_max())};
    BOOST_TEST(as_u128 == static_cast<builtin_u128>(~static_cast<builtin_u128>(0)));

    const auto small {saturating_cast<builtin_u128>(uint256{42U})};
    BOOST_TEST(small == static_cast<builtin_u128>(42));
    #endif
}

void test_corpus_and_random()
{
    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    for (const auto& a : edge_values())
    {
        for (const auto& b : edge_values())
        {
            check_saturating(a, b);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        check_saturating(random_uint256(rng, shape::word4), random_uint256(rng, shape::word4));

        // Operands near the clamps, where the decision is close.
        check_saturating(uint256_max() - uint256{rng() % 100U}, uint256{rng() % 100U});
        check_saturating(uint256{rng() % 100U}, uint256_max() - uint256{rng() % 100U});

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
    #endif
}

void test_constexpr()
{
    constexpr uint256 max {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    static_assert(saturating_add(max, uint256{1U}) == max, "addition clamps");
    static_assert(saturating_sub(uint256{}, uint256{1U}) == uint256{}, "subtraction clamps");
    static_assert(saturating_mul(max, uint256{2U}) == max, "multiplication clamps");
    static_assert(saturating_add(uint256{2U}, uint256{3U}) == uint256{5U}, "no clamp needed");
    static_assert(saturating_sub(uint256{5U}, uint256{3U}) == uint256{2U}, "no clamp needed");
    static_assert(saturating_mul(uint256{5U}, uint256{3U}) == uint256{15U}, "no clamp needed");
    static_assert(saturating_div(max, uint256{1U}) == max, "division never clamps");
    static_assert(saturating_cast<int>(max) == (std::numeric_limits<int>::max)(), "the cast clamps");
    static_assert(saturating_cast<int>(uint256{7U}) == 7, "the cast passes a small value through");
}

} // namespace

int main()
{
    test_constexpr();
    test_clamps();
    test_saturating_cast();
    test_corpus_and_random();

    return boost::report_errors();
}
