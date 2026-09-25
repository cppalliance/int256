// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The C23 <stdckdint.h> checked operations. The contract is that the result
// always receives the exact value reduced modulo the width of the target type,
// and the return value says whether that reduction lost anything. Both halves
// are checked here: a caller that only looks at the bool would not notice a
// wrong stored value, and vice versa.

#include <boost/int256.hpp>

#include "oracle.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

using boost::int256::uint256;
using boost::int256::int256;
using boost::int256::ckd_add;
using boost::int256::ckd_sub;
using boost::int256::ckd_mul;

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 u256_one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

//=====================================
// Into a uint256 target
//=====================================

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

void check_into_uint256(const uint256& a, const uint256& b)
{
    const wide wa {to_wide(a)};
    const wide wb {to_wide(b)};

    // Addition.
    {
        uint256 result {};
        const bool overflowed {ckd_add(&result, a, b)};
        const wide exact {wa + wb};

        BOOST_TEST_EQ(overflowed, !fits_256(exact));
        BOOST_TEST(result == from_wide_truncated(exact) ||
                   report_mismatch("ckd_add result", a, b, result, to_mp(from_wide_truncated(exact))));
    }

    // Subtraction, which underflows rather than overflows.
    {
        uint256 result {};
        const bool overflowed {ckd_sub(&result, a, b)};

        BOOST_TEST_EQ(overflowed, a < b);
        BOOST_TEST_EQ(result, a - b);
    }

    // Multiplication.
    {
        uint256 result {};
        const bool overflowed {ckd_mul(&result, a, b)};
        const wide exact {wa * wb};

        BOOST_TEST_EQ(overflowed, !fits_256(exact));
        BOOST_TEST(result == from_wide_truncated(exact) ||
                   report_mismatch("ckd_mul result", a, b, result, to_mp(from_wide_truncated(exact))));
    }
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE

void test_uint256_target_known_answers()
{
    const auto max {uint256_max()};

    {
        uint256 result {};
        BOOST_TEST(!ckd_add(&result, uint256{2U}, uint256{3U}));
        BOOST_TEST_EQ(result, uint256{5U});
    }
    {
        uint256 result {};
        BOOST_TEST(ckd_add(&result, max, u256_one));
        BOOST_TEST_EQ(result, uint256{});           // the wrapped value is still stored
    }
    {
        uint256 result {};
        BOOST_TEST(ckd_add(&result, max, max));
        BOOST_TEST_EQ(result, max - 1U);
    }
    {
        uint256 result {};
        BOOST_TEST(!ckd_add(&result, max - 1U, u256_one));
        BOOST_TEST_EQ(result, max);
    }
    {
        uint256 result {};
        BOOST_TEST(!ckd_sub(&result, uint256{5U}, uint256{3U}));
        BOOST_TEST_EQ(result, uint256{2U});
    }
    {
        uint256 result {};
        BOOST_TEST(ckd_sub(&result, uint256{}, u256_one));
        BOOST_TEST_EQ(result, max);
    }
    {
        uint256 result {};
        BOOST_TEST(!ckd_sub(&result, max, max));
        BOOST_TEST_EQ(result, uint256{});
    }
    {
        uint256 result {};
        BOOST_TEST(!ckd_mul(&result, uint256{5U}, uint256{3U}));
        BOOST_TEST_EQ(result, uint256{15U});
    }
    {
        uint256 result {};
        BOOST_TEST(ckd_mul(&result, max, uint256{2U}));
        BOOST_TEST_EQ(result, max - 1U);
    }
    {
        // 2^128 squared is exactly 2^256, the first product that does not fit.
        uint256 result {};
        BOOST_TEST(ckd_mul(&result, u256_one << 128U, u256_one << 128U));
        BOOST_TEST_EQ(result, uint256{});

        // And (2^128 - 1) squared does fit.
        uint256 fits {};
        BOOST_TEST(!ckd_mul(&fits, (u256_one << 128U) - 1U, (u256_one << 128U) - 1U));
        BOOST_TEST_EQ(fits, ((u256_one << 128U) - 1U) * ((u256_one << 128U) - 1U));
    }
    {
        // Multiplying by zero or one never overflows.
        uint256 result {};
        BOOST_TEST(!ckd_mul(&result, max, uint256{}));
        BOOST_TEST_EQ(result, uint256{});
        BOOST_TEST(!ckd_mul(&result, max, u256_one));
        BOOST_TEST_EQ(result, max);
    }
}

//=====================================
// Into a narrower target
//=====================================

// The exact result is reduced modulo the target width, and the flag reports
// whether it fitted. A uint256 operand that does not fit the target at all is
// still an overflow with a truncated result.
template <typename T>
void check_narrow_target()
{
    const auto target_max {(std::numeric_limits<T>::max)()};

    {
        T result {};
        BOOST_TEST(!ckd_add(&result, static_cast<T>(1), static_cast<T>(2)));
        BOOST_TEST_EQ(result, static_cast<T>(3));
    }
    {
        // One past the target maximum. The stored value is the exact result
        // reduced modulo the width of the target, which for an unsigned type is
        // zero and for a signed one is the minimum.
        T result {};
        BOOST_TEST(ckd_add(&result, target_max, static_cast<T>(1)));
        BOOST_TEST_EQ(result, (std::numeric_limits<T>::min)());
    }
    {
        // A uint256 operand well above the target range.
        T result {};
        BOOST_TEST(ckd_add(&result, uint256_max(), static_cast<T>(0)));
    }
    {
        T result {};
        BOOST_TEST(ckd_add(&result, uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)), static_cast<T>(0)));
    }
    {
        // Subtraction below zero for an unsigned target, or below the minimum for
        // a signed one.
        T result {};
        const bool overflowed {ckd_sub(&result, static_cast<T>(0), static_cast<T>(1))};
        BOOST_TEST_EQ(overflowed, !(static_cast<T>(-1) < static_cast<T>(0)));
    }
    {
        // Multiplication that leaves the target range.
        T result {};
        BOOST_TEST(ckd_mul(&result, target_max, static_cast<T>(2)));
    }
    {
        T result {};
        BOOST_TEST(!ckd_mul(&result, static_cast<T>(3), static_cast<T>(5)));
        BOOST_TEST_EQ(result, static_cast<T>(15));
    }
}

void test_narrow_targets()
{
    check_narrow_target<unsigned char>();
    check_narrow_target<unsigned short>();
    check_narrow_target<unsigned int>();
    check_narrow_target<unsigned long>();
    check_narrow_target<unsigned long long>();
    check_narrow_target<signed char>();
    check_narrow_target<short>();
    check_narrow_target<int>();
    check_narrow_target<long>();
    check_narrow_target<long long>();
}

// Mixed operand types, which is the shape the C23 macros are used in.
void test_mixed_operands()
{
    {
        uint256 result {};
        BOOST_TEST(!ckd_add(&result, uint256{5U}, 3));
        BOOST_TEST_EQ(result, uint256{8U});
    }
    {
        uint256 result {};
        BOOST_TEST(!ckd_add(&result, 5, uint256{3U}));
        BOOST_TEST_EQ(result, uint256{8U});
    }
    {
        uint256 result {};
        BOOST_TEST(!ckd_mul(&result, uint256{5U}, 3U));
        BOOST_TEST_EQ(result, uint256{15U});
    }
    {
        // A negative operand is a mathematical value, not a widened one, so
        // subtracting it adds.
        uint256 result {};
        BOOST_TEST(!ckd_sub(&result, uint256{5U}, -3));
        BOOST_TEST_EQ(result, uint256{8U});
    }
    {
        // And adding a negative to a small unsigned underflows.
        uint256 result {};
        BOOST_TEST(ckd_add(&result, uint256{1U}, -3));
    }
    {
        // A narrow target with a uint256 operand that fits.
        unsigned result {};
        BOOST_TEST(!ckd_add(&result, uint256{100U}, 5U));
        BOOST_TEST_EQ(result, 105U);
    }
}

//=====================================
// Into an int256 target
//=====================================

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

void check_into_int256(const int256& a, const int256& b)
{
    const smp ma {to_smp(a)};
    const smp mb {to_smp(b)};
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    const smp smp_min {to_smp(min_v)};
    const smp smp_max {to_smp(max_v)};

    {
        int256 result {};
        const bool overflowed {ckd_add(&result, a, b)};
        const smp exact {ma + mb};
        const bool fits {exact >= smp_min && exact <= smp_max};

        BOOST_TEST_EQ(overflowed, !fits);
        BOOST_TEST(result == from_smp_wrapped(exact));
    }
    {
        int256 result {};
        const bool overflowed {ckd_sub(&result, a, b)};
        const smp exact {ma - mb};
        const bool fits {exact >= smp_min && exact <= smp_max};

        BOOST_TEST_EQ(overflowed, !fits);
        BOOST_TEST(result == from_smp_wrapped(exact));
    }
    {
        int256 result {};
        const bool overflowed {ckd_mul(&result, a, b)};
        const smp exact {ma * mb};
        const bool fits {exact >= smp_min && exact <= smp_max};

        BOOST_TEST_EQ(overflowed, !fits);
        BOOST_TEST(result == from_smp_wrapped(exact));
    }
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE

void test_int256_target_known_answers()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    const int256 one_i {1};

    {
        int256 result {};
        BOOST_TEST(!ckd_add(&result, int256{2}, int256{3}));
        BOOST_TEST(result == int256{5});
    }
    {
        // Overflow at the top: the wrapped value (MIN) is still stored.
        int256 result {};
        BOOST_TEST(ckd_add(&result, max_v, one_i));
        BOOST_TEST(result == min_v);
    }
    {
        // Overflow at the bottom.
        int256 result {};
        BOOST_TEST(ckd_sub(&result, min_v, one_i));
        BOOST_TEST(result == max_v);
    }
    {
        int256 result {};
        BOOST_TEST(!ckd_sub(&result, int256{5}, int256{3}));
        BOOST_TEST(result == int256{2});
    }
    {
        int256 result {};
        BOOST_TEST(ckd_mul(&result, max_v, int256{2}));
    }
    {
        int256 result {};
        BOOST_TEST(!ckd_mul(&result, int256{5}, int256{3}));
        BOOST_TEST(result == int256{15});
    }
    {
        // MIN negated overflows (its magnitude, 2^255, is one past MAX).
        int256 result {};
        BOOST_TEST(ckd_mul(&result, min_v, int256{-1}));
        BOOST_TEST(result == min_v);
    }
    {
        // MAX negated fits exactly.
        int256 result {};
        BOOST_TEST(!ckd_mul(&result, max_v, int256{-1}));
        BOOST_TEST(result == -max_v);
    }
}

// int256 as an operand into a uint256 target, and vice versa, matching the
// mixed shape the C23 macros are used in.
void test_int256_mixed_operands()
{
    {
        uint256 result {};
        BOOST_TEST(!ckd_add(&result, uint256{5U}, int256{3}));
        BOOST_TEST_EQ(result, uint256{8U});
    }
    {
        // A negative int256 operand subtracts from the uint256 target.
        uint256 result {};
        BOOST_TEST(!ckd_add(&result, uint256{5U}, int256{-3}));
        BOOST_TEST_EQ(result, uint256{2U});
    }
    {
        uint256 result {};
        BOOST_TEST(ckd_add(&result, uint256{1U}, int256{-3}));
    }
    {
        int256 result {};
        BOOST_TEST(!ckd_add(&result, int256{5}, uint256{3U}));
        BOOST_TEST(result == int256{8});
    }
    {
        // A uint256 operand too large for int256's positive range overflows.
        int256 result {};
        BOOST_TEST(ckd_add(&result, int256{0}, uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}));
    }
}

void test_signed_random()
{
    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    for (const auto& a : signed_edge_values())
    {
        for (const auto& b : signed_edge_values())
        {
            check_into_int256(a, b);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        check_into_int256(random_int256(rng, shape::word4, sign::mixed),
                          random_int256(rng, shape::word4, sign::mixed));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
    #endif
}

struct signed_outcome
{
    bool overflowed;
    int256 value;
};

constexpr signed_outcome run_ckd_add_i(const int256& a, const int256& b) noexcept
{
    int256 result {};
    const bool overflowed {ckd_add(&result, a, b)};

    return signed_outcome{overflowed, result};
}

void test_signed_constexpr()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    constexpr auto sum {run_ckd_add_i(max_v, int256{1})};
    static_assert(sum.overflowed, "MAX + 1 overflows");
    static_assert(sum.value == min_v, "and the wrapped value (MIN) is stored");
}

void test_random()
{
    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    for (const auto& a : edge_values())
    {
        for (const auto& b : edge_values())
        {
            check_into_uint256(a, b);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        check_into_uint256(random_uint256(rng, shape::word4), random_uint256(rng, shape::word4));

        // Near the boundary, where the flag is a close call.
        check_into_uint256(uint256_max() - uint256{rng() % 100U}, uint256{rng() % 100U});
        check_into_uint256(random_uint256(rng, shape::word2), random_uint256(rng, shape::word2));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
    #endif
}

// Both halves of the contract, in a form usable in a constant expression. A
// constexpr lambda would need C++17, and the language floor here is C++14.
struct outcome
{
    bool overflowed;
    uint256 value;
};

constexpr outcome run_ckd_add(const uint256& a, const uint256& b) noexcept
{
    uint256 result {};
    const bool overflowed {ckd_add(&result, a, b)};

    return outcome{overflowed, result};
}

void test_constexpr()
{
    constexpr uint256 max {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    constexpr auto sum {run_ckd_add(max, uint256{1U})};
    static_assert(sum.overflowed, "the sum does not fit");
    static_assert(sum.value == uint256{}, "and the wrapped value is stored");

    constexpr auto small {run_ckd_add(uint256{2U}, uint256{3U})};
    static_assert(!small.overflowed, "the sum fits");
    static_assert(small.value == uint256{5U}, "and it is exact");
}

} // namespace

int main()
{
    test_constexpr();
    test_uint256_target_known_answers();
    test_narrow_targets();
    test_mixed_operands();
    test_random();

    test_signed_constexpr();
    test_int256_target_known_answers();
    test_int256_mixed_operands();
    test_signed_random();

    return boost::report_errors();
}
