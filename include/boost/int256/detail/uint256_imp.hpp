// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_UINT256_IMP_HPP
#define BOOST_INT256_DETAIL_UINT256_IMP_HPP

#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/fwd.hpp>
#include <boost/int256/detail/traits.hpp>
#include <boost/int256/detail/constants.hpp>
#include <boost/int256/detail/clz.hpp>
#include <boost/int256/detail/common_add.hpp>
#include <boost/int256/detail/common_mul.hpp>
#include <boost/int256/detail/common_div.hpp>
#include <boost/int256/detail/float_conversion.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

#endif

namespace boost {
namespace int256 {

// The word order is fixed on every platform: words[0] is the least significant.
// The type is deliberately not over-aligned, so that it stays usable in a std::vector
// under C++14 and does not draw MSVC C4324
BOOST_INT256_EXPORT struct uint256
{
    std::uint64_t words[4] {};

    // Defaulted basic construction. These stay plain constexpr because NVCC rejects
    // an execution space annotation on an implicitly defaulted member (#20012-D)
    constexpr uint256() noexcept = default;
    constexpr uint256(const uint256&) noexcept = default;
    constexpr uint256(uint256&&) noexcept = default;
    constexpr uint256& operator=(const uint256&) noexcept = default;
    constexpr uint256& operator=(uint256&&) noexcept = default;

    // Bit copy, defined in detail/conversions.hpp once both types are complete
    BOOST_INT256_HOST_DEVICE constexpr uint256(const int256& v) noexcept;

    // Most significant word first, matching the (high, low) idiom of Boost.Int128
    BOOST_INT256_HOST_DEVICE constexpr uint256(const std::uint64_t w3, const std::uint64_t w2,
                                               const std::uint64_t w1, const std::uint64_t w0) noexcept :
        words {w0, w1, w2, w3} {}

    template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256(const SignedInteger v) noexcept :
        words {static_cast<std::uint64_t>(v),
               v < 0 ? UINT64_MAX : UINT64_C(0),
               v < 0 ? UINT64_MAX : UINT64_C(0),
               v < 0 ? UINT64_MAX : UINT64_C(0)} {}

    template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256(const UnsignedInteger v) noexcept :
        words {static_cast<std::uint64_t>(v), UINT64_C(0), UINT64_C(0), UINT64_C(0)} {}

    #if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

    // Every shift of a builtin 128-bit value takes an exactly typed right operand. On MSVC
    // builtin_u128 is the class std::_Unsigned128, so with the converting constructor below in
    // scope `builtin_u128 >> int` is ambiguous between std::operator>> and this type's operators.
    // The shift and the v < 0 sign test are both routed through the isolated-namespace helpers
    // (see config.hpp) on MSVC, once uint256 also has an exact match operator<</operator>> for
    // (uint256, builtin128): a plain infix here would then also be ambiguous (finding F12)
    #ifdef BOOST_INT256_HAS_MSVC_INT128

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR uint256(const detail::builtin_i128 v) noexcept :
        words {static_cast<std::uint64_t>(v),
               static_cast<std::uint64_t>(boost::int256_detail::builtin128_shr_u(static_cast<detail::builtin_u128>(v), 64)),
               v < static_cast<detail::builtin_i128>(0) ? UINT64_MAX : UINT64_C(0),
               v < static_cast<detail::builtin_i128>(0) ? UINT64_MAX : UINT64_C(0)} {}

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR uint256(const detail::builtin_u128 v) noexcept :
        words {static_cast<std::uint64_t>(v),
               static_cast<std::uint64_t>(boost::int256_detail::builtin128_shr_u(v, 64)),
               UINT64_C(0),
               UINT64_C(0)} {}

    #else

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR uint256(const detail::builtin_i128 v) noexcept :
        words {static_cast<std::uint64_t>(v),
               static_cast<std::uint64_t>(static_cast<detail::builtin_u128>(v) >> static_cast<detail::builtin_u128>(64)),
               v < 0 ? UINT64_MAX : UINT64_C(0),
               v < 0 ? UINT64_MAX : UINT64_C(0)} {}

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR uint256(const detail::builtin_u128 v) noexcept :
        words {static_cast<std::uint64_t>(v),
               static_cast<std::uint64_t>(v >> static_cast<detail::builtin_u128>(64)),
               UINT64_C(0),
               UINT64_C(0)} {}

    #endif // BOOST_INT256_HAS_MSVC_INT128

    #endif // BOOST_INT256_HAS_INT128

    // Boost.Int128's uint128 and int128, behaving exactly as the builtin 128-bit
    // constructors above. Only viable once <boost/int128.hpp> has been included before
    // this library (see detail/int128_interop.hpp)
    template <BOOST_INT256_DEFAULTED_INT128_INTEROP_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256(const Int128Type v) noexcept :
        words {v.low, v.high, detail::int128_interop_fill(v), detail::int128_interop_fill(v)} {}

    // NaN and negative values give 0, anything at or above 2^256 saturates, and everything
    // else is truncated toward zero
    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256(Float f) noexcept;

    // Integer conversion operators
    BOOST_INT256_HOST_DEVICE explicit constexpr operator bool() const noexcept
    {
        return (words[0] | words[1] | words[2] | words[3]) != UINT64_C(0);
    }

    template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr operator SignedInteger() const noexcept { return static_cast<SignedInteger>(words[0]); }

    #ifdef _MSC_VER
    #  pragma warning(push)
    #  pragma warning(disable:4127)
    #endif

    template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr operator UnsignedInteger() const noexcept
    {
        BOOST_INT256_IF_CONSTEXPR (std::is_same<UnsignedInteger, bool>::value)
        {
            return static_cast<UnsignedInteger>((words[0] | words[1] | words[2] | words[3]) != UINT64_C(0));
        }
        else
        {
            return static_cast<UnsignedInteger>(words[0]);
        }
    }

    #ifdef _MSC_VER
    #  pragma warning(pop)
    #endif

    #if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR operator detail::builtin_i128() const noexcept
    {
        return static_cast<detail::builtin_i128>((static_cast<detail::builtin_u128>(words[1]) << static_cast<detail::builtin_u128>(64)) |
                                                  static_cast<detail::builtin_u128>(words[0]));
    }

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR operator detail::builtin_u128() const noexcept
    {
        return (static_cast<detail::builtin_u128>(words[1]) << static_cast<detail::builtin_u128>(64)) |
                static_cast<detail::builtin_u128>(words[0]);
    }

    #endif // BOOST_INT256_HAS_INT128

    // Keeps the low 128 bits, as the builtin 128-bit conversions above do
    template <BOOST_INT256_DEFAULTED_INT128_INTEROP_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr operator Int128Type() const noexcept
    {
        return detail::int128_interop<Int128Type>::from_words(words[1], words[0]);
    }

    // Conversion to floating point, correctly rounded to nearest with ties to even
    BOOST_INT256_HOST_DEVICE constexpr operator float() const noexcept;
    BOOST_INT256_HOST_DEVICE constexpr operator double() const noexcept;

    // long doubles do not exist on the CUDA or SYCL (spir64) device
    #if !defined(BOOST_INT256_HAS_GPU_SUPPORT)
    constexpr operator long double() const noexcept;
    #endif

    // C++23 <stdfloat> extended types (std::float16_t, float32_t, float64_t, float128_t,
    // bfloat16_t). is_extended_floating_point_v excludes whichever of these turn out to
    // be a plain alias for float/double/long double on the current platform, so this
    // never redeclares one of the three operators above
    #if !defined(BOOST_INT256_HAS_GPU_SUPPORT) && defined(BOOST_INT256_HAS_STDFLOAT)
    template <typename ExtFloat, std::enable_if_t<detail::is_extended_floating_point_v<ExtFloat>, bool> = true>
    BOOST_INT256_HOST_DEVICE constexpr operator ExtFloat() const noexcept
    {
        return detail::unsigned_words_to_float<ExtFloat>(words);
    }
    #endif

    // Compound OR
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator|=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator|=(const uint256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline uint256& operator|=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound AND
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator&=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator&=(const uint256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline uint256& operator&=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound XOR
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator^=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator^=(const uint256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline uint256& operator^=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Left Shift
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator<<=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator<<=(const uint256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline uint256& operator<<=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Right Shift
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator>>=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator>>=(const uint256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline uint256& operator>>=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator++() noexcept;
    BOOST_INT256_HOST_DEVICE constexpr uint256 operator++(int) noexcept;
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator--() noexcept;
    BOOST_INT256_HOST_DEVICE constexpr uint256 operator--(int) noexcept;

    // Compound Addition
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator+=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator+=(const uint256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline uint256& operator+=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Subtraction
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator-=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator-=(const uint256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline uint256& operator-=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Multiplication
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator*=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator*=(const uint256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline uint256& operator*=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Division
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator/=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator/=(const uint256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline uint256& operator/=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Modulo
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator%=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr uint256& operator%=(const uint256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline uint256& operator%=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound assignment with floating point types.
    // Matches the builtin: this value is converted to Float, the operation is applied in
    // floating point, and the result is converted back, truncating toward zero
    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator+=(Float rhs) noexcept;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator-=(Float rhs) noexcept;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator*=(Float rhs) noexcept;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr uint256& operator/=(Float rhs) noexcept;

    // The builtin does not allow a floating point operand for these, so neither do we.
    // Without these the implicit floating point constructor would silently truncate rhs
    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE uint256& operator%=(Float rhs) = delete;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE uint256& operator&=(Float rhs) = delete;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE uint256& operator|=(Float rhs) = delete;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE uint256& operator^=(Float rhs) = delete;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE uint256& operator<<=(Float rhs) = delete;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE uint256& operator>>=(Float rhs) = delete;
};

namespace detail {

// Non-zero when any word above the least significant one is set. Every mixed comparison
// against a value that fits in one word starts here
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr std::uint64_t upper_words(const uint256& x) noexcept
{
    return x.words[1] | x.words[2] | x.words[3];
}

// A shift by a negative amount or by an amount of 256 or more is undefined behavior,
// exactly as for the built-in shift operators, so the count arrives here already
// reduced to an unsigned value below 256
BOOST_INT256_HOST_DEVICE constexpr uint256 shift_left(const uint256& value, const unsigned count) noexcept
{
    BOOST_INT256_ASSERT_MSG(count < 256U, "Shift count is out of range");

    const auto word_shift {count >> 6U};
    const auto bit_shift {count & 63U};

    // A bit shift of zero would make the complementary shift a shift by 64
    if (bit_shift == 0U)
    {
        switch (word_shift)
        {
            case 0U:
                return value;
            case 1U:
                return uint256{value.words[2], value.words[1], value.words[0], UINT64_C(0)};
            case 2U:
                return uint256{value.words[1], value.words[0], UINT64_C(0), UINT64_C(0)};
            default:
                return uint256{value.words[0], UINT64_C(0), UINT64_C(0), UINT64_C(0)};
        }
    }

    const auto complement {64U - bit_shift};

    switch (word_shift)
    {
        case 0U:
            return uint256{(value.words[3] << bit_shift) | (value.words[2] >> complement),
                           (value.words[2] << bit_shift) | (value.words[1] >> complement),
                           (value.words[1] << bit_shift) | (value.words[0] >> complement),
                            value.words[0] << bit_shift};
        case 1U:
            return uint256{(value.words[2] << bit_shift) | (value.words[1] >> complement),
                           (value.words[1] << bit_shift) | (value.words[0] >> complement),
                            value.words[0] << bit_shift,
                            UINT64_C(0)};
        case 2U:
            return uint256{(value.words[1] << bit_shift) | (value.words[0] >> complement),
                            value.words[0] << bit_shift,
                            UINT64_C(0),
                            UINT64_C(0)};
        default:
            return uint256{value.words[0] << bit_shift, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    }
}

BOOST_INT256_HOST_DEVICE constexpr uint256 shift_right(const uint256& value, const unsigned count) noexcept
{
    BOOST_INT256_ASSERT_MSG(count < 256U, "Shift count is out of range");

    const auto word_shift {count >> 6U};
    const auto bit_shift {count & 63U};

    if (bit_shift == 0U)
    {
        switch (word_shift)
        {
            case 0U:
                return value;
            case 1U:
                return uint256{UINT64_C(0), value.words[3], value.words[2], value.words[1]};
            case 2U:
                return uint256{UINT64_C(0), UINT64_C(0), value.words[3], value.words[2]};
            default:
                return uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), value.words[3]};
        }
    }

    const auto complement {64U - bit_shift};

    switch (word_shift)
    {
        case 0U:
            return uint256{ value.words[3] >> bit_shift,
                           (value.words[2] >> bit_shift) | (value.words[3] << complement),
                           (value.words[1] >> bit_shift) | (value.words[2] << complement),
                           (value.words[0] >> bit_shift) | (value.words[1] << complement)};
        case 1U:
            return uint256{ UINT64_C(0),
                            value.words[3] >> bit_shift,
                           (value.words[2] >> bit_shift) | (value.words[3] << complement),
                           (value.words[1] >> bit_shift) | (value.words[2] << complement)};
        case 2U:
            return uint256{ UINT64_C(0),
                            UINT64_C(0),
                            value.words[3] >> bit_shift,
                           (value.words[2] >> bit_shift) | (value.words[3] << complement)};
        default:
            return uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), value.words[3] >> bit_shift};
    }
}

} // namespace detail

//=====================================
// Absolute Value function
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 abs(const uint256& value) noexcept
{
    return value;
}

//=====================================
// Float Conversion Operators
//=====================================

BOOST_INT256_HOST_DEVICE constexpr uint256::operator float() const noexcept
{
    return detail::unsigned_words_to_float<float>(words);
}

BOOST_INT256_HOST_DEVICE constexpr uint256::operator double() const noexcept
{
    return detail::unsigned_words_to_float<double>(words);
}

#if !defined(BOOST_INT256_HAS_GPU_SUPPORT)

constexpr uint256::operator long double() const noexcept
{
    return detail::unsigned_words_to_float<long double>(words);
}

#endif // BOOST_INT256_HAS_GPU_SUPPORT

//=====================================
// Float Construction
//=====================================

// The inverse of operator(Float). Scaling down by 2^64 is exact, and the loop only divides
// while the value is at least 2^64, so no intermediate ever underflows and neither 2^128
// nor 2^192 is ever materialized as a Float
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4127) // Conditional expression is constant pre-C++17
#endif

template <BOOST_INT256_FLOATING_POINT_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256::uint256(Float f) noexcept
{
    // A type this narrow (std::float16_t, max_exponent 16) cannot hold the ladder's
    // 2^32/2^64 scale constants below: converting an out of range value into a
    // floating-point type is undefined behavior and not a constant expression. Every
    // value of such a type is exactly representable in float, so widening first and
    // reusing the float constructor is exact and loses nothing. bfloat16_t shares
    // float's exponent range, so it is not narrow by this test and needs no widening
    BOOST_INT256_IF_CONSTEXPR (std::numeric_limits<Float>::max_exponent < 128)
    {
        *this = uint256(static_cast<float>(f));
        return;
    }

    constexpr Float two_32 {static_cast<Float>(UINT64_C(1) << 32U)};
    constexpr Float two_64 {two_32 * two_32};

    // !(f >= 0) catches both NaN and negative values without using <cmath>
    if (!(f >= Float{0}))
    {
        return;
    }

    int top {};
    Float scaled {f};

    while (scaled >= two_64)
    {
        if (top == 3)
        {
            // The value is at or above 2^256, so it saturates
            words[0] = UINT64_MAX;
            words[1] = UINT64_MAX;
            words[2] = UINT64_MAX;
            words[3] = UINT64_MAX;
            return;
        }

        scaled /= two_64;
        ++top;
    }

    // Every subtraction of the truncated word is exact, so the words are the exact
    // digits of the value in base 2^64
    for (int i {top}; i >= 0; --i)
    {
        const auto word {detail::float_to_uint64(scaled)};
        words[i] = word;
        scaled = (scaled - static_cast<Float>(word)) * two_64;
    }
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

//=====================================
// Unary Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator+(const uint256& value) noexcept
{
    return value;
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator-(const uint256& value) noexcept
{
    return detail::sub_words(uint256{}, value);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator~(const uint256& value) noexcept
{
    return uint256{~value.words[3], ~value.words[2], ~value.words[1], ~value.words[0]};
}

//=====================================
// Builtin 128-bit operand forwarders
//=====================================

// Every mixed operation with a builtin 128-bit operand converts it to a uint256 and
// re-dispatches. For the signed type this is the sign-filled value, which gives the same
// answer modulo 2^256 that the builtin would give
#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

#define BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(op, return_type)                                                                                        \
    BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR return_type operator op(const uint256& lhs, const detail::builtin_i128 rhs) noexcept \
    {                                                                                                                                                         \
        return lhs op static_cast<uint256>(rhs);                                                                                                              \
    }                                                                                                                                                         \
                                                                                                                                                              \
    BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR return_type operator op(const detail::builtin_i128 lhs, const uint256& rhs) noexcept \
    {                                                                                                                                                         \
        return static_cast<uint256>(lhs) op rhs;                                                                                                              \
    }                                                                                                                                                         \
                                                                                                                                                              \
    BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR return_type operator op(const uint256& lhs, const detail::builtin_u128 rhs) noexcept \
    {                                                                                                                                                         \
        return lhs op static_cast<uint256>(rhs);                                                                                                              \
    }                                                                                                                                                         \
                                                                                                                                                              \
    BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR return_type operator op(const detail::builtin_u128 lhs, const uint256& rhs) noexcept \
    {                                                                                                                                                         \
        return static_cast<uint256>(lhs) op rhs;                                                                                                              \
    }

#endif // BOOST_INT256_HAS_INT128 || BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Equality Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator==(const uint256& lhs, const uint256& rhs) noexcept
{
    #if defined(BOOST_INT256_HAS_AVX2)

    // One 256-bit compare. The loads go through memcpy because uint256 is not over-aligned
    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        __m256i a {};
        __m256i b {};
        std::memcpy(&a, lhs.words, sizeof(a));
        std::memcpy(&b, rhs.words, sizeof(b));

        return _mm256_movemask_epi8(_mm256_cmpeq_epi64(a, b)) == -1;
    }

    #elif defined(BOOST_INT256_HAS_SSE2)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        __m128i a_low {};
        __m128i a_high {};
        __m128i b_low {};
        __m128i b_high {};
        std::memcpy(&a_low, lhs.words, sizeof(a_low));
        std::memcpy(&a_high, lhs.words + 2, sizeof(a_high));
        std::memcpy(&b_low, rhs.words, sizeof(b_low));
        std::memcpy(&b_high, rhs.words + 2, sizeof(b_high));

        const auto equal {_mm_and_si128(_mm_cmpeq_epi32(a_low, b_low), _mm_cmpeq_epi32(a_high, b_high))};

        return _mm_movemask_epi8(equal) == 0xFFFF;
    }

    #endif

    return (lhs.words[0] == rhs.words[0]) & (lhs.words[1] == rhs.words[1]) &
           (lhs.words[2] == rhs.words[2]) & (lhs.words[3] == rhs.words[3]);
}

// Explicit bool overloads keep MSVC from warning about an unsafe use of bool (C4804)
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator==(const uint256& lhs, const bool rhs) noexcept
{
    return detail::upper_words(lhs) == UINT64_C(0) && lhs.words[0] == static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator==(const bool lhs, const uint256& rhs) noexcept
{
    return detail::upper_words(rhs) == UINT64_C(0) && rhs.words[0] == static_cast<std::uint64_t>(lhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator==(const uint256& lhs, const SignedInteger rhs) noexcept
{
    return lhs == uint256{rhs};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator==(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return uint256{lhs} == rhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator==(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::upper_words(lhs) == UINT64_C(0) && lhs.words[0] == static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator==(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return detail::upper_words(rhs) == UINT64_C(0) && rhs.words[0] == static_cast<std::uint64_t>(lhs);
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(==, bool)
#endif

//=====================================
// Inequality Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const uint256& lhs, const uint256& rhs) noexcept
{
    #if defined(BOOST_INT256_HAS_AVX2)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        __m256i a {};
        __m256i b {};
        std::memcpy(&a, lhs.words, sizeof(a));
        std::memcpy(&b, rhs.words, sizeof(b));

        return _mm256_movemask_epi8(_mm256_cmpeq_epi64(a, b)) != -1;
    }

    #elif defined(BOOST_INT256_HAS_SSE2)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        __m128i a_low {};
        __m128i a_high {};
        __m128i b_low {};
        __m128i b_high {};
        std::memcpy(&a_low, lhs.words, sizeof(a_low));
        std::memcpy(&a_high, lhs.words + 2, sizeof(a_high));
        std::memcpy(&b_low, rhs.words, sizeof(b_low));
        std::memcpy(&b_high, rhs.words + 2, sizeof(b_high));

        const auto equal {_mm_and_si128(_mm_cmpeq_epi32(a_low, b_low), _mm_cmpeq_epi32(a_high, b_high))};

        return _mm_movemask_epi8(equal) != 0xFFFF;
    }

    #endif

    return (lhs.words[0] != rhs.words[0]) | (lhs.words[1] != rhs.words[1]) |
           (lhs.words[2] != rhs.words[2]) | (lhs.words[3] != rhs.words[3]);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const uint256& lhs, const bool rhs) noexcept
{
    return detail::upper_words(lhs) != UINT64_C(0) || lhs.words[0] != static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const bool lhs, const uint256& rhs) noexcept
{
    return detail::upper_words(rhs) != UINT64_C(0) || rhs.words[0] != static_cast<std::uint64_t>(lhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const uint256& lhs, const SignedInteger rhs) noexcept
{
    return lhs != uint256{rhs};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return uint256{lhs} != rhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::upper_words(lhs) != UINT64_C(0) || lhs.words[0] != static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return detail::upper_words(rhs) != UINT64_C(0) || rhs.words[0] != static_cast<std::uint64_t>(lhs);
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(!=, bool)
#endif

//=====================================
// Less than Operators
//=====================================

// The borrow chain is branch free and compiles to a compare followed by three
// subtract with borrow steps
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator<(const uint256& lhs, const uint256& rhs) noexcept
{
    bool borrow {lhs.words[0] < rhs.words[0]};
    borrow = lhs.words[1] < rhs.words[1] || (lhs.words[1] == rhs.words[1] && borrow);
    borrow = lhs.words[2] < rhs.words[2] || (lhs.words[2] == rhs.words[2] && borrow);
    borrow = lhs.words[3] < rhs.words[3] || (lhs.words[3] == rhs.words[3] && borrow);

    return borrow;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<(const uint256& lhs, const SignedInteger rhs) noexcept
{
    return lhs < uint256{rhs};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return uint256{lhs} < rhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::upper_words(lhs) == UINT64_C(0) && lhs.words[0] < static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return detail::upper_words(rhs) != UINT64_C(0) || static_cast<std::uint64_t>(lhs) < rhs.words[0];
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(<, bool)
#endif

//=====================================
// Less Equal Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const uint256& lhs, const uint256& rhs) noexcept
{
    return !(rhs < lhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const uint256& lhs, const SignedInteger rhs) noexcept
{
    return !(uint256{rhs} < lhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return !(rhs < uint256{lhs});
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::upper_words(lhs) == UINT64_C(0) && lhs.words[0] <= static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return detail::upper_words(rhs) != UINT64_C(0) || static_cast<std::uint64_t>(lhs) <= rhs.words[0];
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(<=, bool)
#endif

//=====================================
// Greater Than Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator>(const uint256& lhs, const uint256& rhs) noexcept
{
    return rhs < lhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>(const uint256& lhs, const SignedInteger rhs) noexcept
{
    return uint256{rhs} < lhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return rhs < uint256{lhs};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::upper_words(lhs) != UINT64_C(0) || lhs.words[0] > static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return detail::upper_words(rhs) == UINT64_C(0) && static_cast<std::uint64_t>(lhs) > rhs.words[0];
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(>, bool)
#endif

//=====================================
// Greater Equal Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const uint256& lhs, const uint256& rhs) noexcept
{
    return !(lhs < rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const uint256& lhs, const SignedInteger rhs) noexcept
{
    return !(lhs < uint256{rhs});
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return !(uint256{lhs} < rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::upper_words(lhs) != UINT64_C(0) || lhs.words[0] >= static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return detail::upper_words(rhs) == UINT64_C(0) && static_cast<std::uint64_t>(lhs) >= rhs.words[0];
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(>=, bool)
#endif

//=====================================
// Spaceship Operator
//=====================================

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const uint256& lhs, const uint256& rhs) noexcept
{
    if (lhs < rhs)
    {
        return std::strong_ordering::less;
    }
    else if (lhs == rhs)
    {
        return std::strong_ordering::equivalent;
    }
    else
    {
        return std::strong_ordering::greater;
    }
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    if (lhs < rhs)
    {
        return std::strong_ordering::less;
    }
    else if (lhs == rhs)
    {
        return std::strong_ordering::equivalent;
    }
    else
    {
        return std::strong_ordering::greater;
    }
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    if (lhs < rhs)
    {
        return std::strong_ordering::less;
    }
    else if (lhs == rhs)
    {
        return std::strong_ordering::equivalent;
    }
    else
    {
        return std::strong_ordering::greater;
    }
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const uint256& lhs, const SignedInteger rhs) noexcept
{
    if (lhs < rhs)
    {
        return std::strong_ordering::less;
    }
    else if (lhs == rhs)
    {
        return std::strong_ordering::equivalent;
    }
    else
    {
        return std::strong_ordering::greater;
    }
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const SignedInteger lhs, const uint256& rhs) noexcept
{
    if (lhs < rhs)
    {
        return std::strong_ordering::less;
    }
    else if (lhs == rhs)
    {
        return std::strong_ordering::equivalent;
    }
    else
    {
        return std::strong_ordering::greater;
    }
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(<=>, std::strong_ordering)
#endif

#endif // BOOST_INT256_HAS_SPACESHIP_OPERATOR

//=====================================
// OR Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator|(const uint256& lhs, const uint256& rhs) noexcept
{
    return uint256{lhs.words[3] | rhs.words[3], lhs.words[2] | rhs.words[2],
                   lhs.words[1] | rhs.words[1], lhs.words[0] | rhs.words[0]};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator|(const uint256& lhs, const SignedInteger rhs) noexcept
{
    const auto fill {rhs < 0 ? UINT64_MAX : UINT64_C(0)};
    return uint256{lhs.words[3] | fill, lhs.words[2] | fill, lhs.words[1] | fill,
                   lhs.words[0] | static_cast<std::uint64_t>(rhs)};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator|(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return rhs | lhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator|(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return uint256{lhs.words[3], lhs.words[2], lhs.words[1], lhs.words[0] | static_cast<std::uint64_t>(rhs)};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator|(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return uint256{rhs.words[3], rhs.words[2], rhs.words[1], rhs.words[0] | static_cast<std::uint64_t>(lhs)};
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(|, uint256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator|=(const Integer rhs) noexcept
{
    *this = *this | rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator|=(const uint256& rhs) noexcept
{
    *this = *this | rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline uint256& uint256::operator|=(const Integer rhs) noexcept
{
    *this = *this | rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// AND Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator&(const uint256& lhs, const uint256& rhs) noexcept
{
    return uint256{lhs.words[3] & rhs.words[3], lhs.words[2] & rhs.words[2],
                   lhs.words[1] & rhs.words[1], lhs.words[0] & rhs.words[0]};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator&(const uint256& lhs, const SignedInteger rhs) noexcept
{
    const auto fill {rhs < 0 ? UINT64_MAX : UINT64_C(0)};
    return uint256{lhs.words[3] & fill, lhs.words[2] & fill, lhs.words[1] & fill,
                   lhs.words[0] & static_cast<std::uint64_t>(rhs)};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator&(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return rhs & lhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator&(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), lhs.words[0] & static_cast<std::uint64_t>(rhs)};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator&(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), rhs.words[0] & static_cast<std::uint64_t>(lhs)};
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(&, uint256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator&=(const Integer rhs) noexcept
{
    *this = *this & rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator&=(const uint256& rhs) noexcept
{
    *this = *this & rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline uint256& uint256::operator&=(const Integer rhs) noexcept
{
    *this = *this & rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// XOR Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator^(const uint256& lhs, const uint256& rhs) noexcept
{
    return uint256{lhs.words[3] ^ rhs.words[3], lhs.words[2] ^ rhs.words[2],
                   lhs.words[1] ^ rhs.words[1], lhs.words[0] ^ rhs.words[0]};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator^(const uint256& lhs, const SignedInteger rhs) noexcept
{
    const auto fill {rhs < 0 ? UINT64_MAX : UINT64_C(0)};
    return uint256{lhs.words[3] ^ fill, lhs.words[2] ^ fill, lhs.words[1] ^ fill,
                   lhs.words[0] ^ static_cast<std::uint64_t>(rhs)};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator^(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return rhs ^ lhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator^(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return uint256{lhs.words[3], lhs.words[2], lhs.words[1], lhs.words[0] ^ static_cast<std::uint64_t>(rhs)};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator^(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return uint256{rhs.words[3], rhs.words[2], rhs.words[1], rhs.words[0] ^ static_cast<std::uint64_t>(lhs)};
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(^, uint256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator^=(const Integer rhs) noexcept
{
    *this = *this ^ rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator^=(const uint256& rhs) noexcept
{
    *this = *this ^ rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline uint256& uint256::operator^=(const Integer rhs) noexcept
{
    *this = *this ^ rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Left Shift Operator
//=====================================

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator<<(const uint256& lhs, const Integer rhs) noexcept
{
    return detail::shift_left(lhs, static_cast<unsigned>(rhs));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator<<(const uint256& lhs, const uint256& rhs) noexcept
{
    // Out-of-range counts (256 or more, or with any upper word set) are undefined,
    // matching the built-in operators; forward the count to the scalar overload
    return detail::shift_left(lhs, static_cast<unsigned>(rhs.words[0]));
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR detail::builtin_u128 operator<<(const detail::builtin_u128 lhs, const uint256& rhs) noexcept
{
    // On MSVC this is routed through the isolated-namespace helper (see config.hpp)
    // rather than a plain infix <<, to avoid an ambiguity with this library's own
    // operator<< (Boost.Int256 finding F12)
    #ifdef BOOST_INT256_HAS_MSVC_INT128
    return boost::int256_detail::builtin128_shl_u(lhs, rhs.words[0]);
    #else
    return lhs << static_cast<detail::builtin_u128>(rhs.words[0]);
    #endif
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR detail::builtin_i128 operator<<(const detail::builtin_i128 lhs, const uint256& rhs) noexcept
{
    #ifdef BOOST_INT256_HAS_MSVC_INT128
    return static_cast<detail::builtin_i128>(boost::int256_detail::builtin128_shl_u(static_cast<detail::builtin_u128>(lhs), rhs.words[0]));
    #else
    return lhs << static_cast<detail::builtin_u128>(rhs.words[0]);
    #endif
}

// The count flavor (library type on the left, MSVC's builtin 128-bit type on the
// right) has no exact match otherwise: std::_Unsigned128/_Signed128's own hidden
// friend operator<< and uint256's operator<<(uint256, uint256) both need one
// user-defined conversion, on different arguments, so MSVC ties them (finding F12).
// is_any_integer_v does not cover MSVC's distinct _Unsigned128/_Signed128 class
// types, unlike native __int128, so the generic template below never applies to them
#ifdef BOOST_INT256_HAS_MSVC_INT128

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR uint256 operator<<(const uint256& lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs << static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR uint256 operator<<(const uint256& lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs << static_cast<std::uint64_t>(rhs);
}

#endif // BOOST_INT256_HAS_MSVC_INT128

#endif

// A shift takes its value and its result type from the left operand after integral promotion,
// and only the count from the right, exactly as the builtin does

BOOST_INT256_EXPORT template <typename Integer, std::enable_if_t<detail::is_any_integer_v<Integer> && (sizeof(Integer) * 8 <= 64), bool> = true>
BOOST_INT256_HOST_DEVICE constexpr detail::promoted_t<Integer> operator<<(const Integer lhs, const uint256& rhs) noexcept
{
    // Shifted in the unsigned domain: a negative left operand then gets the C++20
    // result in every language mode instead of undefined behavior before C++20
    using promoted = detail::promoted_t<Integer>;
    return static_cast<promoted>(static_cast<std::make_unsigned_t<promoted>>(static_cast<promoted>(lhs)) << rhs.words[0]);
}

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator<<=(const Integer rhs) noexcept
{
    *this = *this << rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator<<=(const uint256& rhs) noexcept
{
    *this = *this << rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline uint256& uint256::operator<<=(const Integer rhs) noexcept
{
    *this = *this << rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Right Shift Operator
//=====================================

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator>>(const uint256& lhs, const Integer rhs) noexcept
{
    return detail::shift_right(lhs, static_cast<unsigned>(rhs));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator>>(const uint256& lhs, const uint256& rhs) noexcept
{
    return detail::shift_right(lhs, static_cast<unsigned>(rhs.words[0]));
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR detail::builtin_u128 operator>>(const detail::builtin_u128 lhs, const uint256& rhs) noexcept
{
    // On MSVC this is routed through the isolated-namespace helper (see config.hpp)
    // rather than a plain infix >>, to avoid an ambiguity with this library's own
    // operator>> (Boost.Int256 finding F12)
    #ifdef BOOST_INT256_HAS_MSVC_INT128
    return boost::int256_detail::builtin128_shr_u(lhs, rhs.words[0]);
    #else
    return lhs >> static_cast<detail::builtin_u128>(rhs.words[0]);
    #endif
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR detail::builtin_i128 operator>>(const detail::builtin_i128 lhs, const uint256& rhs) noexcept
{
    // lhs stays signed (an arithmetic shift), only the count is unsigned
    #ifdef BOOST_INT256_HAS_MSVC_INT128
    return boost::int256_detail::builtin128_shr_i(lhs, rhs.words[0]);
    #else
    return lhs >> static_cast<detail::builtin_u128>(rhs.words[0]);
    #endif
}

// See the note above the analogous operator<< overloads: MSVC's distinct
// _Unsigned128/_Signed128 need this dedicated exact match too (finding F12)
#ifdef BOOST_INT256_HAS_MSVC_INT128

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR uint256 operator>>(const uint256& lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs >> static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR uint256 operator>>(const uint256& lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs >> static_cast<std::uint64_t>(rhs);
}

#endif // BOOST_INT256_HAS_MSVC_INT128

#endif

BOOST_INT256_EXPORT template <typename Integer, std::enable_if_t<detail::is_any_integer_v<Integer> && (sizeof(Integer) * 8 <= 64), bool> = true>
BOOST_INT256_HOST_DEVICE constexpr detail::promoted_t<Integer> operator>>(const Integer lhs, const uint256& rhs) noexcept
{
    return static_cast<detail::promoted_t<Integer>>(lhs) >> rhs.words[0];
}

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator>>=(const Integer rhs) noexcept
{
    *this = *this >> rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator>>=(const uint256& rhs) noexcept
{
    *this = *this >> rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline uint256& uint256::operator>>=(const Integer rhs) noexcept
{
    *this = *this >> rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Increment and Decrement Operators
//=====================================

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator++() noexcept
{
    if (++words[0] == UINT64_C(0))
    {
        if (++words[1] == UINT64_C(0))
        {
            if (++words[2] == UINT64_C(0))
            {
                ++words[3];
            }
        }
    }

    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256 uint256::operator++(int) noexcept
{
    const auto temp {*this};
    ++(*this);
    return temp;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator--() noexcept
{
    if (--words[0] == UINT64_MAX)
    {
        if (--words[1] == UINT64_MAX)
        {
            if (--words[2] == UINT64_MAX)
            {
                --words[3];
            }
        }
    }

    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256 uint256::operator--(int) noexcept
{
    const auto temp {*this};
    --(*this);
    return temp;
}

//=====================================
// Addition Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator+(const uint256& lhs, const uint256& rhs) noexcept
{
    return detail::add_words(lhs, rhs);
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to an unsigned type, result still unsigned
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator+(const uint256& lhs, const SignedInteger rhs) noexcept
{
    return rhs < 0 ? detail::sub_words(lhs, -static_cast<std::uint64_t>(rhs)) :
                     detail::add_words(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator+(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return lhs < 0 ? detail::sub_words(rhs, -static_cast<std::uint64_t>(lhs)) :
                     detail::add_words(rhs, static_cast<std::uint64_t>(lhs));
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator+(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::add_words(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator+(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return detail::add_words(rhs, static_cast<std::uint64_t>(lhs));
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(+, uint256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator+=(const Integer rhs) noexcept
{
    *this = *this + rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator+=(const uint256& rhs) noexcept
{
    *this = *this + rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline uint256& uint256::operator+=(const Integer rhs) noexcept
{
    *this = *this + rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Subtraction Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator-(const uint256& lhs, const uint256& rhs) noexcept
{
    return detail::sub_words(lhs, rhs);
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to an unsigned type, result still unsigned
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator-(const uint256& lhs, const SignedInteger rhs) noexcept
{
    return rhs < 0 ? detail::add_words(lhs, -static_cast<std::uint64_t>(rhs)) :
                     detail::sub_words(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator-(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return lhs < 0 ? detail::sub_words(-rhs, -static_cast<std::uint64_t>(lhs)) :
                     detail::add_words(-rhs, static_cast<std::uint64_t>(lhs));
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator-(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::sub_words(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator-(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return detail::add_words(-rhs, static_cast<std::uint64_t>(lhs));
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(-, uint256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator-=(const Integer rhs) noexcept
{
    *this = *this - rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator-=(const uint256& rhs) noexcept
{
    *this = *this - rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline uint256& uint256::operator-=(const Integer rhs) noexcept
{
    *this = *this - rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Multiplication Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator*(const uint256& lhs, const uint256& rhs) noexcept
{
    return detail::mul_words(lhs, rhs);
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to an unsigned type, result still unsigned
#endif

// The magnitude is multiplied and the result negated, since a one word product of the
// sign-filled value would lose the upper words of the sign extension
BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator*(const uint256& lhs, const SignedInteger rhs) noexcept
{
    const auto abs_rhs {rhs < 0 ? -static_cast<std::uint64_t>(rhs) : static_cast<std::uint64_t>(rhs)};
    const auto result {detail::mul_words(lhs, abs_rhs)};

    return rhs < 0 ? -result : result;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator*(const SignedInteger lhs, const uint256& rhs) noexcept
{
    const auto abs_lhs {lhs < 0 ? -static_cast<std::uint64_t>(lhs) : static_cast<std::uint64_t>(lhs)};
    const auto result {detail::mul_words(rhs, abs_lhs)};

    return lhs < 0 ? -result : result;
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator*(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::mul_words(lhs, static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator*(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    return detail::mul_words(rhs, static_cast<std::uint64_t>(lhs));
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(*, uint256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator*=(const Integer rhs) noexcept
{
    *this = *this * rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator*=(const uint256& rhs) noexcept
{
    *this = *this * rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline uint256& uint256::operator*=(const Integer rhs) noexcept
{
    *this = *this * rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Division Operator
//=====================================

// The mixed flavors dispatch to each other, so they need forward declarations
BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(const uint256& lhs, SignedInteger rhs) noexcept;

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(SignedInteger lhs, const uint256& rhs) noexcept;

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(const uint256& lhs, UnsignedInteger rhs) noexcept;

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(UnsignedInteger lhs, const uint256& rhs) noexcept;

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(const uint256& lhs, const uint256& rhs) noexcept;

BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(const uint256& lhs, const uint256& rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == UINT64_C(0)))
    {
        // Division by zero is undefined behavior for the builtin integer types (a hardware
        // trap). We match that: marking it unreachable keeps codegen branch free
        BOOST_INT256_UNREACHABLE;
    }

    uint256 quotient {};
    uint256 remainder {};

    detail::divide<false>(lhs, rhs, quotient, remainder);

    return quotient;
}

template <BOOST_INT256_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0U))
    {
        BOOST_INT256_UNREACHABLE;
    }

    uint256 quotient {};

    detail::one_word_div(lhs, static_cast<std::uint64_t>(rhs), quotient);

    return quotient;
}

template <BOOST_INT256_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == UINT64_C(0)))
    {
        BOOST_INT256_UNREACHABLE;
    }

    if (rhs > lhs)
    {
        return uint256{};
    }

    return uint256{static_cast<std::uint64_t>(lhs) / rhs.words[0]};
}

template <BOOST_INT256_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(const uint256& lhs, const SignedInteger rhs) noexcept
{
    return rhs < 0 ? lhs / uint256{rhs} : lhs / static_cast<std::uint64_t>(rhs);
}

template <BOOST_INT256_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return lhs < 0 ? uint256{lhs} / rhs : static_cast<std::uint64_t>(lhs) / rhs;
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(/, uint256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator/=(const Integer rhs) noexcept
{
    *this = *this / rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator/=(const uint256& rhs) noexcept
{
    *this = *this / rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline uint256& uint256::operator/=(const Integer rhs) noexcept
{
    *this = *this / rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Modulo Operator
//=====================================

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(const uint256& lhs, SignedInteger rhs) noexcept;

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(SignedInteger lhs, const uint256& rhs) noexcept;

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(const uint256& lhs, UnsignedInteger rhs) noexcept;

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(UnsignedInteger lhs, const uint256& rhs) noexcept;

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(const uint256& lhs, const uint256& rhs) noexcept;

BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(const uint256& lhs, const uint256& rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == UINT64_C(0)))
    {
        BOOST_INT256_UNREACHABLE;
    }

    uint256 quotient {};
    uint256 remainder {};

    detail::divide<true>(lhs, rhs, quotient, remainder);

    return remainder;
}

template <BOOST_INT256_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(const uint256& lhs, const UnsignedInteger rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0U))
    {
        BOOST_INT256_UNREACHABLE;
    }

    uint256 quotient {};
    std::uint64_t remainder {};

    detail::one_word_div(lhs, static_cast<std::uint64_t>(rhs), quotient, remainder);

    return uint256{remainder};
}

template <BOOST_INT256_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(const UnsignedInteger lhs, const uint256& rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == UINT64_C(0)))
    {
        BOOST_INT256_UNREACHABLE;
    }

    if (rhs > lhs)
    {
        return uint256{static_cast<std::uint64_t>(lhs)};
    }

    return uint256{static_cast<std::uint64_t>(lhs) % rhs.words[0]};
}

template <BOOST_INT256_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(const uint256& lhs, const SignedInteger rhs) noexcept
{
    return rhs < 0 ? lhs % uint256{rhs} : lhs % static_cast<std::uint64_t>(rhs);
}

template <BOOST_INT256_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(const SignedInteger lhs, const uint256& rhs) noexcept
{
    return lhs < 0 ? uint256{lhs} % rhs : static_cast<std::uint64_t>(lhs) % rhs;
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP(%, uint256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator%=(const Integer rhs) noexcept
{
    *this = *this % rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator%=(const uint256& rhs) noexcept
{
    *this = *this % rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline uint256& uint256::operator%=(const Integer rhs) noexcept
{
    *this = *this % rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
#undef BOOST_INT256_DETAIL_U256_BUILTIN128_BINARY_OP
#endif

//=====================================
// Built-in Integer Compound Assignment
//=====================================

// Compound assignment with a built-in integer on the left.
// The builtin applies the operation to the common type of the two operands and converts
// the result back to the type of the left operand, so each of these is the binary operator
// above followed by that conversion, which matches what a builtin integer does.
// detail/traits.hpp defines which types Integer may be

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4804) // Unsafe use of type bool in operation
#endif

#define BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(op, compound_op)                                                  \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>                                              \
    BOOST_INT256_HOST_DEVICE constexpr Integer& operator compound_op(Integer& lhs, const uint256& rhs) noexcept         \
    {                                                                                                                  \
        lhs = static_cast<Integer>(lhs op rhs);                                                                        \
        return lhs;                                                                                                    \
    }

BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(|, |=)
BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(&, &=)
BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(^, ^=)
BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(+, +=)
BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(-, -=)
BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(*, *=)
BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(/, /=)
BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(%, %=)

// The shifts take the value from the left operand alone, so only the count comes from rhs
BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(<<, <<=)
BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP(>>, >>=)

#undef BOOST_INT256_DETAIL_U256_INTEGER_COMPOUND_OP

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

//=====================================
// Floating Point Operators
//=====================================

// The usual arithmetic conversions convert the integer operand to the floating point type
// before the operation is applied, so each of these computes exactly what a builtin integer
// computes for the same expression.
// detail/traits.hpp defines which types Float may be

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#define BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(op, return_type)                                                       \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>                                        \
    BOOST_INT256_HOST_DEVICE constexpr return_type operator op(const uint256& lhs, const Float rhs) noexcept            \
    {                                                                                                                   \
        return static_cast<Float>(lhs) op rhs;                                                                          \
    }                                                                                                                   \
                                                                                                                        \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>                                        \
    BOOST_INT256_HOST_DEVICE constexpr return_type operator op(const Float lhs, const uint256& rhs) noexcept            \
    {                                                                                                                   \
        return lhs op static_cast<Float>(rhs);                                                                           \
    }

BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(+, Float)
BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(-, Float)
BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(*, Float)
BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(/, Float)

BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(==, bool)
BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(!=, bool)
BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(<, bool)
BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(<=, bool)
BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(>, bool)
BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(>=, bool)

// Mixing an integer and a floating point type yields a partial ordering because of NaN
#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR

BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP(<=>, std::partial_ordering)

#endif // BOOST_INT256_HAS_SPACESHIP_OPERATOR

#undef BOOST_INT256_DETAIL_U256_FLOAT_BINARY_OP

// Compound assignment converts the result back to uint256, truncating toward zero.
// A result that is NaN or outside the range of the type saturates as the floating point
// constructor does, rather than being undefined as it is for the builtin

#define BOOST_INT256_DETAIL_U256_FLOAT_COMPOUND_OP(op, compound_op)                                                     \
    template <BOOST_INT256_FLOATING_POINT_CONCEPT>                                                                      \
    BOOST_INT256_HOST_DEVICE constexpr uint256& uint256::operator compound_op(const Float rhs) noexcept                 \
    {                                                                                                                   \
        *this = static_cast<uint256>(static_cast<Float>(*this) op rhs);                                                 \
        return *this;                                                                                                   \
    }                                                                                                                   \
                                                                                                                        \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>                                        \
    BOOST_INT256_HOST_DEVICE constexpr Float& operator compound_op(Float& lhs, const uint256& rhs) noexcept             \
    {                                                                                                                   \
        lhs compound_op static_cast<Float>(rhs);                                                                        \
        return lhs;                                                                                                     \
    }

BOOST_INT256_DETAIL_U256_FLOAT_COMPOUND_OP(+, +=)
BOOST_INT256_DETAIL_U256_FLOAT_COMPOUND_OP(-, -=)
BOOST_INT256_DETAIL_U256_FLOAT_COMPOUND_OP(*, *=)
BOOST_INT256_DETAIL_U256_FLOAT_COMPOUND_OP(/, /=)

#undef BOOST_INT256_DETAIL_U256_FLOAT_COMPOUND_OP

#ifdef __clang__
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

// The builtin allows no floating point operand for the modulo, bitwise and shift operators.
// Deleting them keeps that a compile error here, rather than letting the implicit floating
// point constructor silently truncate the operand

#define BOOST_INT256_DETAIL_U256_FLOAT_DELETED_OP(op)                                                                   \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>                                        \
    BOOST_INT256_HOST_DEVICE uint256 operator op(const uint256& lhs, Float rhs) = delete;                               \
                                                                                                                        \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>                                        \
    BOOST_INT256_HOST_DEVICE uint256 operator op(Float lhs, const uint256& rhs) = delete;

BOOST_INT256_DETAIL_U256_FLOAT_DELETED_OP(%)
BOOST_INT256_DETAIL_U256_FLOAT_DELETED_OP(&)
BOOST_INT256_DETAIL_U256_FLOAT_DELETED_OP(|)
BOOST_INT256_DETAIL_U256_FLOAT_DELETED_OP(^)
BOOST_INT256_DETAIL_U256_FLOAT_DELETED_OP(<<)
BOOST_INT256_DETAIL_U256_FLOAT_DELETED_OP(>>)

#undef BOOST_INT256_DETAIL_U256_FLOAT_DELETED_OP

namespace detail {

template <bool>
class numeric_limits_impl_u256
{
public:

    // Member constants
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;

    // C++23 deprecated the following two members
    #if defined(__GNUC__) && __cplusplus > 202002L
    #  pragma GCC diagnostic push
    #  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(_MSC_VER)
    #  pragma warning(push)
    #  pragma warning(disable:4996)
    #endif

    static constexpr std::float_denorm_style has_denorm = std::denorm_absent;
    static constexpr bool has_denorm_loss = false;

    #if defined(__GNUC__) && __cplusplus > 202002L
    #  pragma GCC diagnostic pop
    #elif defined(_MSC_VER)
    #  pragma warning(pop)
    #endif

    static constexpr std::float_round_style round_style = std::round_toward_zero;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = true;
    static constexpr int digits = 256;
    static constexpr int digits10 = 77;
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = std::numeric_limits<std::uint64_t>::traps;
    static constexpr bool tinyness_before = false;

    // Member functions
    BOOST_INT256_HOST_DEVICE static constexpr auto (min)        () -> boost::int256::uint256 { return {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto lowest       () -> boost::int256::uint256 { return {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto (max)        () -> boost::int256::uint256 { return {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto epsilon      () -> boost::int256::uint256 { return {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto round_error  () -> boost::int256::uint256 { return {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto infinity     () -> boost::int256::uint256 { return {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto quiet_NaN    () -> boost::int256::uint256 { return {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto signaling_NaN() -> boost::int256::uint256 { return {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto denorm_min   () -> boost::int256::uint256 { return {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
};

#if !defined(__cpp_inline_variables) || __cpp_inline_variables < 201606L

template <bool b> constexpr bool numeric_limits_impl_u256<b>::is_specialized;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::is_signed;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::is_integer;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::is_exact;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::has_infinity;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::has_quiet_NaN;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::has_signaling_NaN;

// These members were deprecated in C++23; suppress the deprecation warning rather
// than dropping the definitions
#if defined(__GNUC__) && __cplusplus > 202002L
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif

template <bool b> constexpr std::float_denorm_style numeric_limits_impl_u256<b>::has_denorm;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::has_denorm_loss;

#if defined(__GNUC__) && __cplusplus > 202002L
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif

template <bool b> constexpr std::float_round_style numeric_limits_impl_u256<b>::round_style;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::is_iec559;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::is_bounded;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::is_modulo;
template <bool b> constexpr int numeric_limits_impl_u256<b>::digits;
template <bool b> constexpr int numeric_limits_impl_u256<b>::digits10;
template <bool b> constexpr int numeric_limits_impl_u256<b>::max_digits10;
template <bool b> constexpr int numeric_limits_impl_u256<b>::radix;
template <bool b> constexpr int numeric_limits_impl_u256<b>::min_exponent;
template <bool b> constexpr int numeric_limits_impl_u256<b>::min_exponent10;
template <bool b> constexpr int numeric_limits_impl_u256<b>::max_exponent;
template <bool b> constexpr int numeric_limits_impl_u256<b>::max_exponent10;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::traps;
template <bool b> constexpr bool numeric_limits_impl_u256<b>::tinyness_before;

#endif // !defined(__cpp_inline_variables) || __cpp_inline_variables < 201606L

} // namespace detail

} // namespace int256
} // namespace boost

namespace std {

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wmismatched-tags"
#endif

template <>
class numeric_limits<boost::int256::uint256> :
    public boost::int256::detail::numeric_limits_impl_u256<true> {};

#ifdef __clang__
#  pragma clang diagnostic pop
#endif

} // namespace std

#endif // BOOST_INT256_DETAIL_UINT256_IMP_HPP
