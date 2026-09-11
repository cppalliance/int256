// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_NUMERIC_HPP
#define BOOST_INT256_NUMERIC_HPP

#include <boost/int256/bit.hpp>
#include <boost/int256/cstdlib.hpp>
#include <boost/int256/utilities.hpp>
#include <boost/int256/detail/traits.hpp>
#include <boost/int256/detail/ctz.hpp>
#include <boost/int256/detail/config.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstdint>
#include <limits>
#include <type_traits>

#endif

namespace boost {
namespace int256 {

namespace detail {

// The target types saturating_cast accepts: the ten standard integer types, the
// library type itself, and the builtin 128-bit types where they exist
template <typename IntegerType>
struct reduced_integers
{
    static constexpr bool value {std::is_same<IntegerType, signed char>::value ||
                                 std::is_same<IntegerType, unsigned char>::value ||
                                 std::is_same<IntegerType, signed short>::value ||
                                 std::is_same<IntegerType, unsigned short>::value ||
                                 std::is_same<IntegerType, signed int>::value ||
                                 std::is_same<IntegerType, unsigned int>::value ||
                                 std::is_same<IntegerType, signed long>::value ||
                                 std::is_same<IntegerType, unsigned long>::value ||
                                 std::is_same<IntegerType, signed long long>::value ||
                                 std::is_same<IntegerType, unsigned long long>::value ||
                                 std::is_same<IntegerType, uint256>::value};
};

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

template <typename IntegerType>
BOOST_INT256_INLINE_CONSTEXPR bool is_reduced_integer_v {reduced_integers<IntegerType>::value ||
                                                         std::is_same<IntegerType, detail::builtin_i128>::value ||
                                                         std::is_same<IntegerType, detail::builtin_u128>::value};

#else

template <typename IntegerType>
BOOST_INT256_INLINE_CONSTEXPR bool is_reduced_integer_v {reduced_integers<IntegerType>::value};

#endif // 128-bit

} // namespace detail

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 saturating_add(const uint256& x, const uint256& y) noexcept
{
    const auto z {x + y};

    if (z < x)
    {
        return (std::numeric_limits<uint256>::max)();
    }

    return z;
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 saturating_sub(const uint256& x, const uint256& y) noexcept
{
    const auto z {x - y};

    if (z > x)
    {
        return (std::numeric_limits<uint256>::min)();
    }

    return z;
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 saturating_mul(const uint256& x, const uint256& y) noexcept
{
    uint256 res {};
    return ckd_mul(&res, x, y) ? (std::numeric_limits<uint256>::max)() : res;
}

// An unsigned quotient can never overflow, so this is a plain division. A zero divisor
// is undefined, exactly as it is for operator/ and for the builtin integer types.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 saturating_div(const uint256& x, const uint256& y) noexcept
{
    return x / y;
}

BOOST_INT256_EXPORT template <typename TargetType, std::enable_if_t<detail::is_reduced_integer_v<TargetType>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr TargetType saturating_cast(const uint256& value) noexcept
{
    BOOST_INT256_IF_CONSTEXPR (std::is_same<uint256, TargetType>::value)
    {
        return static_cast<TargetType>(value);
    }
    else
    {
        if (value > detail::decomposed_limits<TargetType>::max_magnitude())
        {
            return detail::decomposed_limits<TargetType>::max_value();
        }

        return static_cast<TargetType>(value);
    }
}

namespace detail {

// Binary GCD on a single word, used once both operands are narrow enough
BOOST_INT256_TEST_EXPORT BOOST_INT256_HOST_DEVICE constexpr std::uint64_t gcd64(std::uint64_t x, std::uint64_t y) noexcept
{
    if (x == 0)
    {
        return y;
    }
    if (y == 0)
    {
        return x;
    }

    const auto s {countr_zero(x | y)};
    x >>= countr_zero(x);

    do
    {
        y >>= countr_zero(y);
        if (x > y)
        {
            const auto temp {x};
            x = y;
            y = temp;
        }

        y -= x;
    } while (y != 0);

    return x << s;
}

} // namespace detail

// Binary (Stein) GCD. The loop drops into the 64-bit routine as soon as both
// operands fit in a single word.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 gcd(uint256 a, uint256 b) noexcept
{
    if (a == 0U)
    {
        return b;
    }
    if (b == 0U)
    {
        return a;
    }

    const auto a_zero {countr_zero(a)};
    const auto b_zero {countr_zero(b)};
    const auto shift {b_zero < a_zero ? b_zero : a_zero};
    a >>= shift;
    b >>= shift;

    // The invariant of the loop below is that a is odd
    a >>= countr_zero(a);

    while (true)
    {
        b >>= countr_zero(b);

        if (a > b)
        {
            const uint256 temp {a};
            a = b;
            b = temp;
        }

        b -= a;

        if (b == 0U)
        {
            return a << shift;
        }

        // Stop doing 256-bit math as soon as we can
        if (((a.words[1] | a.words[2] | a.words[3]) | (b.words[1] | b.words[2] | b.words[3])) == UINT64_C(0))
        {
            return uint256{detail::gcd64(a.words[0], b.words[0])} << shift;
        }
    }
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 lcm(const uint256& a, const uint256& b) noexcept
{
    if (a == 0U || b == 0U)
    {
        return uint256{};
    }

    // Calculate GCD first
    const auto g {gcd(a, b)};

    // Compute LCM avoiding overflow: (a/gcd) * b
    return (a / g) * b;
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 midpoint(const uint256& a, const uint256& b) noexcept
{
    // Bit manipulation formula works for unsigned integers
    auto mid {(a & b) + ((a ^ b) >> 1U)};

    // std::midpoint rounds towards the first parameter
    if (((a ^ b) & 1U) && a > b)
    {
        ++mid;
    }

    return mid;
}

// Quotient and remainder of a single division, following the div_result<T> proposed for the
// standard library by P3724 (Integer division)
BOOST_INT256_EXPORT template <typename T>
struct div_result
{
    T quotient;
    T remainder;
};

BOOST_INT256_EXPORT template <typename T>
BOOST_INT256_HOST_DEVICE constexpr bool operator==(const div_result<T>& lhs, const div_result<T>& rhs) noexcept
{
    return lhs.quotient == rhs.quotient && lhs.remainder == rhs.remainder;
}

BOOST_INT256_EXPORT template <typename T>
BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const div_result<T>& lhs, const div_result<T>& rhs) noexcept
{
    return !(lhs == rhs);
}

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR

BOOST_INT256_EXPORT template <typename T>
BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const div_result<T>& lhs, const div_result<T>& rhs) noexcept
{
    const auto quotient_order {lhs.quotient <=> rhs.quotient};
    return quotient_order != std::strong_ordering::equal ? quotient_order : lhs.remainder <=> rhs.remainder;
}

#endif // BOOST_INT256_HAS_SPACESHIP_OPERATOR

namespace detail {

// An unsigned quotient is never rounded down, so the only offsets are 0 and 1. The remainder
// of an incremented quotient is negative, and is returned reduced modulo 2^256.
BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> offset_quotient(const u256div_t& truncated, const uint256& y, const bool increment) noexcept
{
    return div_result<uint256>{increment ? truncated.quot + 1U : truncated.quot,
                               increment ? truncated.rem - y : truncated.rem};
}

// Round-to-nearest comparison shared by the ties functions: the truncated quotient grows in
// magnitude when the remainder is more than half the divisor. truncate_ties selects the strict
// form, which both breaks an exact tie towards zero and recovers the bit that y / 2 drops
// when y is odd.
BOOST_INT256_HOST_DEVICE constexpr bool nearest_increment(const uint256& abs_rem, const uint256& abs_half_y, const bool truncate_ties) noexcept
{
    return truncate_ties ? abs_rem > abs_half_y : abs_rem >= abs_half_y;
}

// An odd divisor cannot produce an exact tie, so every ties function truncates on it
BOOST_INT256_HOST_DEVICE constexpr bool is_odd(const uint256& x) noexcept
{
    return (x.words[0] & UINT64_C(1)) != UINT64_C(0);
}

} // namespace detail

// Rounds towards zero, which is what operator/ already does
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_to_zero(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    return div_result<uint256>{truncated.quot, truncated.rem};
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_to_zero(const uint256& x, const uint256& y) noexcept
{
    return x / y;
}

// Rounds away from zero, so the quotient grows unless the division is exact
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_away_zero(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, truncated.rem != 0U);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_away_zero(const uint256& x, const uint256& y) noexcept
{
    return div_rem_away_zero(x, y).quotient;
}

// Rounds towards positive infinity, which for an unsigned quotient is away from zero
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_to_pos_inf(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, truncated.rem != 0U);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_to_pos_inf(const uint256& x, const uint256& y) noexcept
{
    return div_rem_to_pos_inf(x, y).quotient;
}

// Rounds towards negative infinity, which for an unsigned quotient is truncation
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_to_neg_inf(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    return div_result<uint256>{truncated.quot, truncated.rem};
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_to_neg_inf(const uint256& x, const uint256& y) noexcept
{
    return x / y;
}

// Euclidean division, whose remainder is always in [0, y). An unsigned truncated
// remainder is already non-negative, so nothing needs fixing.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_euclid(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    return div_result<uint256>{truncated.quot, truncated.rem};
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_euclid(const uint256& x, const uint256& y) noexcept
{
    return x / y;
}

// Rounds to nearest, breaking an exact tie towards zero
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_ties_to_zero(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, detail::nearest_increment(truncated.rem, y >> 1U, true));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_ties_to_zero(const uint256& x, const uint256& y) noexcept
{
    return div_rem_ties_to_zero(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie away from zero
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_ties_away_zero(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, detail::nearest_increment(truncated.rem, y >> 1U, detail::is_odd(y)));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_ties_away_zero(const uint256& x, const uint256& y) noexcept
{
    return div_rem_ties_away_zero(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie towards positive infinity. An unsigned
// quotient is positive, so a tie always grows it.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_ties_to_pos_inf(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, detail::nearest_increment(truncated.rem, y >> 1U, detail::is_odd(y)));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_ties_to_pos_inf(const uint256& x, const uint256& y) noexcept
{
    return div_rem_ties_to_pos_inf(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie towards negative infinity, so an unsigned
// tie always truncates.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_ties_to_neg_inf(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    return detail::offset_quotient(truncated, y, detail::nearest_increment(truncated.rem, y >> 1U, true));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_ties_to_neg_inf(const uint256& x, const uint256& y) noexcept
{
    return div_rem_ties_to_neg_inf(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie to the odd quotient, so a tie only grows the
// quotient when truncation would have produced an even one
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_ties_to_odd(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    const auto increment {detail::nearest_increment(truncated.rem, y >> 1U, detail::is_odd(y) || detail::is_odd(truncated.quot))};
    return detail::offset_quotient(truncated, y, increment);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_ties_to_odd(const uint256& x, const uint256& y) noexcept
{
    return div_rem_ties_to_odd(x, y).quotient;
}

// Rounds to nearest, breaking an exact tie to the even quotient, so a tie only grows the
// quotient when truncation would have produced an odd one
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr div_result<uint256> div_rem_ties_to_even(const uint256& x, const uint256& y) noexcept
{
    const auto truncated {div(x, y)};
    const auto increment {detail::nearest_increment(truncated.rem, y >> 1U, detail::is_odd(y) || !detail::is_odd(truncated.quot))};
    return detail::offset_quotient(truncated, y, increment);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 div_ties_to_even(const uint256& x, const uint256& y) noexcept
{
    return div_rem_ties_to_even(x, y).quotient;
}

// The Euclidean remainder, which is always in [0, y)
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 rem_euclid(const uint256& x, const uint256& y) noexcept
{
    return x % y;
}

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_NUMERIC_HPP
