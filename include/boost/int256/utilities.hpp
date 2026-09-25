// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_UTILITIES_HPP
#define BOOST_INT256_UTILITIES_HPP

#include <boost/int256/int256.hpp>
#include <boost/int256/bit.hpp>
#include <boost/int256/detail/traits.hpp>
#include <boost/int256/detail/config.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#endif

namespace boost {
namespace int256 {

namespace detail {

// Modular addition for 256-bit operands assuming 0 <= a, b < m
BOOST_INT256_HOST_DEVICE constexpr uint256 addmod(const uint256& a, const uint256& b, const uint256& m) noexcept
{
    const uint256 s {a + b};

    if (s < a || s >= m)
    {
        return s - m;
    }

    return s;
}

// Modular multiplication via shift-and-add for the full 256-bit modulus case
BOOST_INT256_TEST_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 mulmod_shift(uint256 a, uint256 b, const uint256& m) noexcept
{
    uint256 result {};

    while (b != 0U)
    {
        if ((b.words[0] & UINT64_C(1)) != UINT64_C(0))
        {
            result = addmod(result, a, m);
        }

        a = addmod(a, a, m);
        b >>= 1U;
    }

    return result;
}

// Modular multiplication when the modulus fits in 64 bits. The 128-bit product is
// exact in a 256-bit type, so a single reduction suffices.
BOOST_INT256_HOST_DEVICE constexpr std::uint64_t mulmod_word(const std::uint64_t a, const std::uint64_t b, const std::uint64_t m) noexcept
{
    return ((uint256{a} * uint256{b}) % uint256{m}).words[0];
}

} // namespace detail

// Computes (base ^ exp) mod m using fast modular exponentiation with
// optimizations specific to the boost::int256 library type
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 powm(uint256 base, uint256 exp, const uint256& m) noexcept
{
    if (BOOST_INT256_UNLIKELY(m == 0U))
    {
        return uint256{};
    }

    if (m == 1U)
    {
        return uint256{};
    }

    if (exp == 0U)
    {
        return uint256{1U};
    }

    base %= m;

    if (base == 0U)
    {
        return uint256{};
    }

    // Power-of-two modulus: reduction is just a bitmask.
    if (has_single_bit(m))
    {
        const uint256 mask {m - 1U};
        uint256 result {1U};

        while (exp != 0U)
        {
            if ((exp.words[0] & UINT64_C(1)) != UINT64_C(0))
            {
                result = (result * base) & mask;
            }

            base = (base * base) & mask;
            exp >>= 1U;
        }

        return result;
    }

    // Modulus fits in 64 bits: stay in 64-bit lanes.
    if ((m.words[1] | m.words[2] | m.words[3]) == UINT64_C(0))
    {
        const auto mm {m.words[0]};
        std::uint64_t result {1};
        auto b {base.words[0]};

        while (exp != 0U)
        {
            if ((exp.words[0] & UINT64_C(1)) != UINT64_C(0))
            {
                result = detail::mulmod_word(result, b, mm);
            }

            b = detail::mulmod_word(b, b, mm);
            exp >>= 1U;
        }

        return uint256{result};
    }

    // General 256-bit modulus: shift-and-add for each squaring keeps every
    // intermediate strictly below m without needing a 512-bit product.
    uint256 result {1U};

    while (exp != 0U)
    {
        if ((exp.words[0] & UINT64_C(1)) != UINT64_C(0))
        {
            result = detail::mulmod_shift(result, base, m);
        }

        base = detail::mulmod_shift(base, base, m);
        exp >>= 1U;
    }

    return result;
}

// Computes base^exp using exponentiation by squaring. The result is reduced
// modulo 2^256, mirroring the wrap-around behavior of operator*.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 ipow(uint256 base, std::uint64_t exp) noexcept
{
    uint256 result {1U};

    while (exp != 0U)
    {
        if ((exp & 1U) != 0U)
        {
            result *= base;
        }

        exp >>= 1U;

        if (exp != 0U)
        {
            base *= base;
        }
    }

    return result;
}

// Integer square root: returns floor(sqrt(n)).
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 isqrt(const uint256& n) noexcept
{
    if (n < 2U)
    {
        return n;
    }

    // 2^ceil(bit_width(n)/2) is the smallest power of two whose square exceeds n.
    uint256 x {uint256{1U} << ((bit_width(n) + 1) / 2)};

    while (true)
    {
        const uint256 y {(x + n / x) >> 1U};

        if (y >= x)
        {
            return x;
        }

        x = y;
    }
}

// Signed overload of powm. Returns the non-negative residue in [0, m); m <= 0 or
// exp < 0 gives 0. A negative base is reduced into [0, m) before the unsigned
// exponentiation runs, rather than letting a negative residue come back out of it.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 powm(const int256& base, const int256& exp, const int256& m) noexcept
{
    if (BOOST_INT256_UNLIKELY(m <= 0 || exp < 0))
    {
        return int256{};
    }

    // Explicit types: nvcc 12.8 drops the cast from `const auto x {static_cast<T>(e)}`
    const uint256 um {static_cast<uint256>(m)};

    uint256 ub {};

    if (base.signed_high() < 0)
    {
        const uint256 magnitude {static_cast<uint256>(abs(base))};
        const auto r {magnitude % um};
        ub = r == 0U ? uint256{} : um - r;
    }
    else
    {
        ub = static_cast<uint256>(base) % um;
    }

    return detail::to_signed_bits(powm(ub, static_cast<uint256>(exp), um));
}

// Signed overload of ipow. Wraps modulo 2^256 on overflow, matching operator*.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 ipow(int256 base, std::uint64_t exp) noexcept
{
    int256 result {1};

    while (exp != 0U)
    {
        if (static_cast<bool>(exp & 1U))
        {
            result *= base;
        }

        exp >>= 1U;

        if (exp != 0U)
        {
            base *= base;
        }
    }

    return result;
}

// Signed overload of isqrt. A negative input returns 0.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 isqrt(const int256& n) noexcept
{
    if (BOOST_INT256_UNLIKELY(n.signed_high() < 0))
    {
        return int256{};
    }

    return detail::to_signed_bits(isqrt(static_cast<uint256>(n)));
}

namespace detail {

// The C23 checked integer macros accept any integer type for their operands
// except bool, plain char, enumerated types, and bit-precise (_BitInt) types.
// The builtin 128-bit types are not std::is_integral in strict mode, so the
// library traits decide instead.
template <typename T>
struct valid_checked_type : std::integral_constant<bool, ((std::is_integral<T>::value || is_any_integer_v<T>) &&
                                                          !std::is_same<T, bool>::value &&
                                                          !std::is_same<T, char>::value) ||
                                                         is_int128_interop_v<T>> {};

template <>
struct valid_checked_type<uint256> : std::true_type {};

template <>
struct valid_checked_type<int256> : std::true_type {};

// Whether an operand type carries a sign. std::numeric_limits is not required to be
// specialized for the builtin 128-bit types, so the library trait answers instead.
// int256 is added directly rather than through is_signed_integer_v, which backs the
// SignedInteger concept used to mix a builtin into int256 itself and must not also
// match int256 (that would make int256 op int256 ambiguous with int256 op SignedInteger)
template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_signed_operand_v = is_signed_integer_v<T> || std::is_same<T, int256>::value ||
                                                        int128_interop<T>::is_signed;

// The extreme values of a target type expressed as 256-bit magnitudes, so the checked
// and comparison functions never need std::numeric_limits of a builtin 128-bit type
template <typename T>
struct decomposed_limits
{
    static BOOST_INT256_HOST_DEVICE constexpr T max_value() noexcept
    {
        return (std::numeric_limits<T>::max)();
    }

    static BOOST_INT256_HOST_DEVICE constexpr uint256 max_magnitude() noexcept
    {
        return static_cast<uint256>((std::numeric_limits<T>::max)());
    }

    // Magnitude of the most negative value, and zero for an unsigned type
    static BOOST_INT256_HOST_DEVICE constexpr uint256 min_magnitude() noexcept
    {
        return is_signed_operand_v<T> ? max_magnitude() + uint256{1U} : uint256{};
    }
};

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

template <>
struct decomposed_limits<builtin_u128>
{
    static BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR builtin_u128 max_value() noexcept
    {
        return ~static_cast<builtin_u128>(0);
    }

    static BOOST_INT256_HOST_DEVICE constexpr uint256 max_magnitude() noexcept
    {
        return uint256{UINT64_C(0), UINT64_C(0), UINT64_MAX, UINT64_MAX};
    }

    static BOOST_INT256_HOST_DEVICE constexpr uint256 min_magnitude() noexcept
    {
        return uint256{};
    }
};

template <>
struct decomposed_limits<builtin_i128>
{
    static BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR builtin_i128 max_value() noexcept
    {
        // On MSVC this is routed through the isolated-namespace helper (see
        // config.hpp): a plain infix >> here, even with an exactly typed right
        // operand, is ambiguous between std::_Unsigned128's hidden friend and
        // uint256's own operator>>(uint256, builtin_u128) exact match (finding F12)
        #ifdef BOOST_INT256_HAS_MSVC_INT128
        return static_cast<builtin_i128>(boost::int256_detail::builtin128_shr_u(~static_cast<builtin_u128>(0), 1));
        #else
        return static_cast<builtin_i128>(~static_cast<builtin_u128>(0) >> static_cast<builtin_u128>(1));
        #endif
    }

    static BOOST_INT256_HOST_DEVICE constexpr uint256 max_magnitude() noexcept
    {
        return uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x7FFFFFFFFFFFFFFF), UINT64_MAX};
    }

    static BOOST_INT256_HOST_DEVICE constexpr uint256 min_magnitude() noexcept
    {
        return uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_C(0)};
    }
};

#endif // Builtin 128-bit limits

// Widen an integer operand to its 256-bit two's complement bit pattern. The converting
// constructor sign fills the upper words for a signed operand and zero fills for an
// unsigned one, so a single conversion covers both.
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr uint256 ckd_widen(const T value) noexcept
{
    return static_cast<uint256>(value);
}

// Sign and magnitude of an operand together with its 256-bit two's complement
// image. magnitude is the absolute value; negative records the sign.
struct ckd_operand
{
    uint256 raw;
    uint256 magnitude;
    bool negative;
};

template <typename T>
BOOST_INT256_HOST_DEVICE constexpr ckd_operand ckd_decompose(const T value) noexcept
{
    const uint256 raw {ckd_widen(value)};
    const bool negative {is_signed_operand_v<T> && ((raw >> 255U) != 0U)};
    return ckd_operand{raw, negative ? uint256{} - raw : raw, negative};
}

// Exact signed sum of two operands given as (magnitude, sign). carry marks a
// 257th bit, which no 256-bit or narrower target can represent.
struct ckd_sum_result
{
    uint256 magnitude;
    bool negative;
    bool carry;
};

BOOST_INT256_HOST_DEVICE constexpr ckd_sum_result ckd_signed_sum(const uint256& a_magnitude, const bool a_negative,
                                                                const uint256& b_magnitude, const bool b_negative) noexcept
{
    if (a_negative == b_negative)
    {
        // Equal signs: magnitudes add and may overflow into a 257th bit.
        const uint256 magnitude {a_magnitude + b_magnitude};
        return ckd_sum_result{magnitude, a_negative, magnitude < a_magnitude};
    }

    // Opposite signs: the smaller magnitude is subtracted and never carries.
    if (a_magnitude >= b_magnitude)
    {
        return ckd_sum_result{a_magnitude - b_magnitude, a_negative, false};
    }

    return ckd_sum_result{b_magnitude - a_magnitude, b_negative, false};
}

// Whether a result of the given sign and magnitude fits in T1. exceeds_width
// forces overflow when the true magnitude does not even fit in 256 bits.
template <typename T1>
BOOST_INT256_HOST_DEVICE constexpr bool ckd_overflows(const uint256& magnitude, const bool negative, const bool exceeds_width) noexcept
{
    if (exceeds_width)
    {
        return true;
    }

    if (negative)
    {
        return magnitude > decomposed_limits<T1>::min_magnitude();
    }

    return magnitude > decomposed_limits<T1>::max_magnitude();
}

} // namespace detail

// Checked addition following the C23 <stdckdint.h> ckd_add contract.
//
// Computes a + b as if both operands were represented in a signed integer
// type of infinite range and then converts that exact result to the type
// pointed to by result. *result always receives the exact result wrapped
// around to the width of *result. Returns false when *result represents the
// exact mathematical sum, and true when the sum did not fit and wrap-around
// occurred.
BOOST_INT256_EXPORT template <typename T1, typename T2, typename T3>
BOOST_INT256_HOST_DEVICE constexpr bool ckd_add(T1* result, const T2 a, const T3 b) noexcept
{
    static_assert(detail::valid_checked_type<T1>::value &&
                  detail::valid_checked_type<T2>::value &&
                  detail::valid_checked_type<T3>::value,
                  "ckd_add operands must be integer types other than bool and plain char.");

    const auto op_a {detail::ckd_decompose(a)};
    const auto op_b {detail::ckd_decompose(b)};

    // The modular sum of the widened images is the exact sum mod 2^256, which
    // is all the wrapped result needs for any target no wider than 256 bits.
    *result = detail::convert_to<T1>(op_a.raw + op_b.raw);

    const auto sum {detail::ckd_signed_sum(op_a.magnitude, op_a.negative, op_b.magnitude, op_b.negative)};
    return detail::ckd_overflows<T1>(sum.magnitude, sum.negative, sum.carry);
}

// Checked subtraction following the C23 <stdckdint.h> ckd_sub contract.
//
// Behaves as ckd_add for a - b: *result receives the exact difference wrapped
// to its width, and the return value reports whether that difference did not
// fit.
BOOST_INT256_EXPORT template <typename T1, typename T2, typename T3>
BOOST_INT256_HOST_DEVICE constexpr bool ckd_sub(T1* result, const T2 a, const T3 b) noexcept
{
    static_assert(detail::valid_checked_type<T1>::value &&
                  detail::valid_checked_type<T2>::value &&
                  detail::valid_checked_type<T3>::value,
                  "ckd_sub operands must be integer types other than bool and plain char.");

    const auto op_a {detail::ckd_decompose(a)};
    const auto op_b {detail::ckd_decompose(b)};

    *result = detail::convert_to<T1>(op_a.raw - op_b.raw);

    // a - b is a + (-b): negating b flips its sign while keeping its magnitude.
    const auto difference {detail::ckd_signed_sum(op_a.magnitude, op_a.negative, op_b.magnitude, !op_b.negative)};
    return detail::ckd_overflows<T1>(difference.magnitude, difference.negative, difference.carry);
}

// Checked multiplication following the C23 <stdckdint.h> ckd_mul contract.
//
// Computes a * b as if both operands had infinite range, stores the result
// wrapped to the width of *result, and returns true when the exact product did
// not fit.
BOOST_INT256_EXPORT template <typename T1, typename T2, typename T3>
BOOST_INT256_HOST_DEVICE constexpr bool ckd_mul(T1* result, const T2 a, const T3 b) noexcept
{
    static_assert(detail::valid_checked_type<T1>::value &&
                  detail::valid_checked_type<T2>::value &&
                  detail::valid_checked_type<T3>::value,
                  "ckd_mul operands must be integer types other than bool and plain char.");

    const auto op_a {detail::ckd_decompose(a)};
    const auto op_b {detail::ckd_decompose(b)};

    *result = detail::convert_to<T1>(op_a.raw * op_b.raw);

    // The product magnitude needs more than 256 bits exactly when it exceeds
    // the 256-bit maximum. Dividing the maximum by one magnitude tests that
    // without forming a 512-bit product.
    const bool exceeds_width {op_a.magnitude != 0U &&
                              op_b.magnitude > ((std::numeric_limits<uint256>::max)() / op_a.magnitude)};

    const uint256 product_magnitude {op_a.magnitude * op_b.magnitude};
    const bool product_negative {op_a.negative != op_b.negative};

    return detail::ckd_overflows<T1>(product_magnitude, product_negative, exceeds_width);
}

namespace detail {

// See: https://eel.is/c++draft/utility.intcmp
// [Note 1: These function templates cannot be used to compare byte, char, char8_t,
// char16_t, char32_t, wchar_t, and bool. end note]
template <typename T>
struct valid_comparison_type
{
    static constexpr bool value = std::is_integral<T>::value &&
                                  !std::is_same<T, char>::value &&
                                  !std::is_same<T, char16_t>::value &&
                                  !std::is_same<T, char32_t>::value &&
                                  !std::is_same<T, wchar_t>::value &&
                                  !std::is_same<T, bool>::value
                                  #if defined(__cpp_char8_t)
                                  && !std::is_same<T, char8_t>::value
                                  #endif
                                  #if defined(__cpp_lib_byte) && __cpp_lib_byte >= 201603L
                                  && !std::is_same<T, std::byte>::value
                                  #endif
                                  ;
};

template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_valid_comparison_type_v = valid_comparison_type<T>::value;

// Allow the builtins to be used when available
template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_wide_type_v = std::is_same<T, uint256>::value ||
                                                    std::is_same<T, int256>::value
    #if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
                                                    || std::is_same<T, builtin_i128>::value
                                                    || std::is_same<T, builtin_u128>::value
    #endif
                                                    || is_int128_interop_v<T>;

template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_valid_comparison_operand_v = is_valid_comparison_type_v<T> ||
                                                                   is_wide_type_v<T>;

// Mathematical equality of two integers regardless of their signedness, via the
// same (sign, magnitude) decomposition.
template <typename T, typename U>
BOOST_INT256_HOST_DEVICE constexpr bool cmp_equal_impl(const T lhs, const U rhs) noexcept
{
    const auto a {ckd_decompose(lhs)};
    const auto b {ckd_decompose(rhs)};

    return (a.negative == b.negative) && (a.magnitude == b.magnitude);
}

// Mathematical less-than of two integers regardless of their signedness, via the
// same (sign, magnitude) decomposition.
template <typename T, typename U>
BOOST_INT256_HOST_DEVICE constexpr bool cmp_less_impl(const T lhs, const U rhs) noexcept
{
    const auto a {ckd_decompose(lhs)};
    const auto b {ckd_decompose(rhs)};

    if (a.negative != b.negative)
    {
        return a.negative;
    }

    return a.negative ? (a.magnitude > b.magnitude) : (a.magnitude < b.magnitude);
}

template <typename T, typename U>
BOOST_INT256_INLINE_CONSTEXPR bool enable_comparison_v = is_valid_comparison_operand_v<T> &&
                                                         is_valid_comparison_operand_v<U> &&
                                                         (is_wide_type_v<T> || is_wide_type_v<U>);

} // namespace detail

// C++26 integer comparison functions (https://eel.is/c++draft/utility.intcmp)
// extended to the library and builtin wide types and available from C++14.

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool cmp_equal(const T lhs, const U rhs) noexcept
{
    return detail::cmp_equal_impl(lhs, rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool cmp_not_equal(const T lhs, const U rhs) noexcept
{
    return !detail::cmp_equal_impl(lhs, rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool cmp_less(const T lhs, const U rhs) noexcept
{
    return detail::cmp_less_impl(lhs, rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool cmp_greater(const T lhs, const U rhs) noexcept
{
    return detail::cmp_less_impl(rhs, lhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool cmp_less_equal(const T lhs, const U rhs) noexcept
{
    return !detail::cmp_less_impl(rhs, lhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::enable_comparison_v<T, U>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool cmp_greater_equal(const T lhs, const U rhs) noexcept
{
    return !detail::cmp_less_impl(lhs, rhs);
}

// Whether t is representable in the target type R.
BOOST_INT256_EXPORT template <typename R, typename T, std::enable_if_t<detail::enable_comparison_v<R, T>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool in_range(const T t) noexcept
{
    const auto op {detail::ckd_decompose(t)};

    if (op.negative)
    {
        return detail::is_signed_operand_v<R> && op.magnitude <= detail::decomposed_limits<R>::min_magnitude();
    }

    return op.magnitude <= detail::decomposed_limits<R>::max_magnitude();
}

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_UTILITIES_HPP
