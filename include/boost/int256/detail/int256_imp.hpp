// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_INT256_IMP_HPP
#define BOOST_INT256_DETAIL_INT256_IMP_HPP

#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/fwd.hpp>
#include <boost/int256/detail/traits.hpp>
#include <boost/int256/detail/constants.hpp>
#include <boost/int256/detail/common_add.hpp>
#include <boost/int256/detail/common_mul.hpp>
#include <boost/int256/detail/common_div.hpp>
#include <boost/int256/detail/float_conversion.hpp>
#include <boost/int256/detail/uint256_imp.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstdint>
#include <limits>
#include <type_traits>

#endif

namespace boost {
namespace int256 {

// Two's complement over the same four words as uint256: words[0] is least significant on
// every platform, and the sign is bit 63 of words[3]. Every sign-dependent operation goes
// through signed_high() rather than reading words[3] directly
BOOST_INT256_EXPORT struct int256
{
    std::uint64_t words[4] {};

    // Defaulted basic construction. These stay plain constexpr because NVCC rejects
    // an execution space annotation on an implicitly defaulted member (#20012-D)
    constexpr int256() noexcept = default;
    constexpr int256(const int256&) noexcept = default;
    constexpr int256(int256&&) noexcept = default;
    constexpr int256& operator=(const int256&) noexcept = default;
    constexpr int256& operator=(int256&&) noexcept = default;

    // Bit copy, defined in detail/conversions.hpp once both types are complete
    BOOST_INT256_HOST_DEVICE constexpr int256(const uint256& v) noexcept;

    // Most significant word first, matching uint256 and the (high, low) idiom of Boost.Int128.
    // The top word is signed for parity with Boost.Int128's (int64_t hi, uint64_t lo)
    BOOST_INT256_HOST_DEVICE constexpr int256(const std::int64_t w3, const std::uint64_t w2,
                                              const std::uint64_t w1, const std::uint64_t w0) noexcept :
        words {w0, w1, w2, static_cast<std::uint64_t>(w3)} {}

    template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256(const SignedInteger v) noexcept :
        words {static_cast<std::uint64_t>(v),
               v < 0 ? UINT64_MAX : UINT64_C(0),
               v < 0 ? UINT64_MAX : UINT64_C(0),
               v < 0 ? UINT64_MAX : UINT64_C(0)} {}

    template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256(const UnsignedInteger v) noexcept :
        words {static_cast<std::uint64_t>(v), UINT64_C(0), UINT64_C(0), UINT64_C(0)} {}

    #if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

    // The right operand is a builtin_i128 rather than a plain 0 on purpose. Once both
    // uint256 and int256 have an implicit converting constructor from builtin_i128 in
    // scope, `builtin_i128 < 0` is ambiguous on MSVC between std::operator< and this
    // library's own comparison operators (which become viable candidates by converting
    // v to uint256 or int256); matching the literal's type exactly rules those out.
    // The shift is routed through the isolated-namespace helper (see config.hpp) for
    // the same reason, once int256 also has an exact match operator>> for (int256,
    // builtin128): a plain infix here would then also be ambiguous (finding F12)
    #ifdef BOOST_INT256_HAS_MSVC_INT128

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR int256(const detail::builtin_i128 v) noexcept :
        words {static_cast<std::uint64_t>(v),
               static_cast<std::uint64_t>(boost::int256_detail::builtin128_shr_u(static_cast<detail::builtin_u128>(v), 64)),
               v < static_cast<detail::builtin_i128>(0) ? UINT64_MAX : UINT64_C(0),
               v < static_cast<detail::builtin_i128>(0) ? UINT64_MAX : UINT64_C(0)} {}

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR int256(const detail::builtin_u128 v) noexcept :
        words {static_cast<std::uint64_t>(v),
               static_cast<std::uint64_t>(boost::int256_detail::builtin128_shr_u(v, 64)),
               UINT64_C(0),
               UINT64_C(0)} {}

    #else

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR int256(const detail::builtin_i128 v) noexcept :
        words {static_cast<std::uint64_t>(v),
               static_cast<std::uint64_t>(static_cast<detail::builtin_u128>(v) >> static_cast<detail::builtin_u128>(64)),
               v < static_cast<detail::builtin_i128>(0) ? UINT64_MAX : UINT64_C(0),
               v < static_cast<detail::builtin_i128>(0) ? UINT64_MAX : UINT64_C(0)} {}

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR int256(const detail::builtin_u128 v) noexcept :
        words {static_cast<std::uint64_t>(v),
               static_cast<std::uint64_t>(v >> static_cast<detail::builtin_u128>(64)),
               UINT64_C(0),
               UINT64_C(0)} {}

    #endif // BOOST_INT256_HAS_MSVC_INT128

    #endif // BOOST_INT256_HAS_INT128

    // NaN gives 0, >= 2^255 saturates at max, <= -2^255 saturates at min, and everything
    // else truncates toward zero. Defined below, after the uint256 magnitude is available
    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256(Float f) noexcept;

    // The high word read as a signed value. Every sign-dependent operation goes through
    // this rather than reading words[3] directly
    BOOST_INT256_HOST_DEVICE constexpr std::int64_t signed_high() const noexcept
    {
        return static_cast<std::int64_t>(words[3]);
    }

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

    // boost::int256_detail::builtin128_from_words (declared as a sibling of
    // boost::int256, see config.hpp) is used here instead of the plain infix `<<`/`|`,
    // on the MSVC path only. Unqualified lookup for those operators from inside
    // namespace boost::int256 also finds uint256's own operator<</operator| overloads
    // (declared earlier in the translation unit), and MSVC treats that as ambiguous
    // with std::_Unsigned128's own hidden-friend operators. Not needed outside MSVC,
    // where builtin_u128 is a fundamental type with no user-defined operators to
    // compete with
    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR operator detail::builtin_i128() const noexcept
    {
        #if defined(BOOST_INT256_HAS_MSVC_INT128)
        return static_cast<detail::builtin_i128>(boost::int256_detail::builtin128_from_words(words[1], words[0]));
        #else
        return static_cast<detail::builtin_i128>((static_cast<detail::builtin_u128>(words[1]) << static_cast<detail::builtin_u128>(64)) |
                                                  static_cast<detail::builtin_u128>(words[0]));
        #endif
    }

    BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR operator detail::builtin_u128() const noexcept
    {
        #if defined(BOOST_INT256_HAS_MSVC_INT128)
        return boost::int256_detail::builtin128_from_words(words[1], words[0]);
        #else
        return (static_cast<detail::builtin_u128>(words[1]) << static_cast<detail::builtin_u128>(64)) |
                static_cast<detail::builtin_u128>(words[0]);
        #endif
    }

    #endif // BOOST_INT256_HAS_INT128

    // Conversion to floating point, correctly rounded to nearest with ties to even
    BOOST_INT256_HOST_DEVICE constexpr operator float() const noexcept;
    BOOST_INT256_HOST_DEVICE constexpr operator double() const noexcept;

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
        return detail::signed_words_to_float<ExtFloat>(words);
    }
    #endif

    // Compound OR
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator|=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr int256& operator|=(const int256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline int256& operator|=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound AND
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator&=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr int256& operator&=(const int256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline int256& operator&=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound XOR
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator^=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr int256& operator^=(const int256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline int256& operator^=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Left Shift
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator<<=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr int256& operator<<=(const int256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline int256& operator<<=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Right Shift
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator>>=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr int256& operator>>=(const int256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline int256& operator>>=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    BOOST_INT256_HOST_DEVICE constexpr int256& operator++() noexcept;
    BOOST_INT256_HOST_DEVICE constexpr int256 operator++(int) noexcept;
    BOOST_INT256_HOST_DEVICE constexpr int256& operator--() noexcept;
    BOOST_INT256_HOST_DEVICE constexpr int256 operator--(int) noexcept;

    // Compound Addition
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator+=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr int256& operator+=(const int256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline int256& operator+=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Subtraction
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator-=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr int256& operator-=(const int256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline int256& operator-=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Multiplication
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator*=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr int256& operator*=(const int256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline int256& operator*=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Division
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator/=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr int256& operator/=(const int256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline int256& operator/=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound Modulo
    template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator%=(Integer rhs) noexcept;

    BOOST_INT256_HOST_DEVICE constexpr int256& operator%=(const int256& rhs) noexcept;

    #ifdef BOOST_INT256_HAS_MSVC_INT128

    template <BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT>
    BOOST_INT256_HOST_DEVICE inline int256& operator%=(Integer rhs) noexcept;

    #endif // BOOST_INT256_HAS_MSVC_INT128

    // Compound assignment with floating point types. Matches the builtin: this value is
    // converted to Float, the operation is applied in floating point, and the result is
    // converted back, truncating toward zero
    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator+=(Float rhs) noexcept;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator-=(Float rhs) noexcept;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator*=(Float rhs) noexcept;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE constexpr int256& operator/=(Float rhs) noexcept;

    // The builtin does not allow a floating point operand for these, so neither do we
    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE int256& operator%=(Float rhs) = delete;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE int256& operator&=(Float rhs) = delete;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE int256& operator|=(Float rhs) = delete;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE int256& operator^=(Float rhs) = delete;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE int256& operator<<=(Float rhs) = delete;

    template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>
    BOOST_INT256_HOST_DEVICE int256& operator>>=(Float rhs) = delete;
};

namespace detail {

// Builds an int256 from raw words with a direct member write, avoiding the narrowing that
// list-initializing the public (int64_t, uint64_t, uint64_t, uint64_t) constructor with a
// plain uint64_t top word would trigger
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr int256 from_words(const std::uint64_t w3, const std::uint64_t w2,
                                                                               const std::uint64_t w1, const std::uint64_t w0) noexcept
{
    int256 result {};
    result.words[0] = w0;
    result.words[1] = w1;
    result.words[2] = w2;
    result.words[3] = w3;
    return result;
}

// Bit copies between the two 256-bit layouts. Every sign-agnostic int256 operator is
// computed by converting to uint256, running the existing uint256 machinery, and
// converting back, so these two are on the hot path of nearly every operator below
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr uint256 to_unsigned_bits(const int256& v) noexcept
{
    return uint256{v.words[3], v.words[2], v.words[1], v.words[0]};
}

BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr int256 to_signed_bits(const uint256& v) noexcept
{
    return from_words(v.words[3], v.words[2], v.words[1], v.words[0]);
}

// Mirrors add_words<uint256> exactly (same carry chain, same builtin fast paths) but
// builds the result through from_words instead of a direct T{r3, r2, r1, r0} list
// initialization, which would narrow the uint64_t carry-out into int256's int64_t top
// word constructor parameter. operator+ calls this directly instead of routing through
// add_words<uint256> plus a to_unsigned_bits/to_signed_bits round trip: the perf-parity
// asm pass measured that round trip costing two extra register moves on gcc
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr int256 add_words_signed(const int256& lhs, const int256& rhs) noexcept
{
    #if defined(BOOST_INT256_ADD_CARRY) && !defined(BOOST_INT256_HAS_BUILTIN_ADD_OVERFLOW) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        unsigned long long r[4] {};
        unsigned char carry {};

        carry = BOOST_INT256_ADD_CARRY(carry, lhs.words[0], rhs.words[0], &r[0]);
        carry = BOOST_INT256_ADD_CARRY(carry, lhs.words[1], rhs.words[1], &r[1]);
        carry = BOOST_INT256_ADD_CARRY(carry, lhs.words[2], rhs.words[2], &r[2]);
        BOOST_INT256_ADD_CARRY(carry, lhs.words[3], rhs.words[3], &r[3]);

        return from_words(static_cast<std::uint64_t>(r[3]), static_cast<std::uint64_t>(r[2]),
                           static_cast<std::uint64_t>(r[1]), static_cast<std::uint64_t>(r[0]));
    }

    #elif defined(BOOST_INT256_HAS_BUILTIN_ADDCLL) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        unsigned long long carry {};
        const auto r0 {__builtin_addcll(lhs.words[0], rhs.words[0], carry, &carry)};
        const auto r1 {__builtin_addcll(lhs.words[1], rhs.words[1], carry, &carry)};
        const auto r2 {__builtin_addcll(lhs.words[2], rhs.words[2], carry, &carry)};
        const auto r3 {__builtin_addcll(lhs.words[3], rhs.words[3], carry, &carry)};

        return from_words(static_cast<std::uint64_t>(r3), static_cast<std::uint64_t>(r2),
                           static_cast<std::uint64_t>(r1), static_cast<std::uint64_t>(r0));
    }

    #endif

    std::uint64_t r0 {};
    std::uint64_t r1 {};
    std::uint64_t r2 {};
    std::uint64_t r3 {};

    auto carry {add_carry(lhs.words[0], rhs.words[0], UINT64_C(0), r0)};
    carry = add_carry(lhs.words[1], rhs.words[1], carry, r1);
    carry = add_carry(lhs.words[2], rhs.words[2], carry, r2);
    add_carry(lhs.words[3], rhs.words[3], carry, r3);

    return from_words(r3, r2, r1, r0);
}

// Logical left shift, sign-agnostic like uint256's own shift_left, whose exact structure
// this mirrors: a version that instead called detail::shift_left(uint256, unsigned) through
// to_unsigned_bits/to_signed_bits measured a full 32-byte stack copy on the perf-parity asm
// pass, because shift_left is not small enough to always inline and a reference parameter
// forces the temporary to be materialized somewhere addressable. Building the result
// through from_words avoids ever needing that temporary
BOOST_INT256_HOST_DEVICE constexpr int256 shift_left(const int256& value, const unsigned count) noexcept
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
                return from_words(value.words[2], value.words[1], value.words[0], UINT64_C(0));
            case 2U:
                return from_words(value.words[1], value.words[0], UINT64_C(0), UINT64_C(0));
            default:
                return from_words(value.words[0], UINT64_C(0), UINT64_C(0), UINT64_C(0));
        }
    }

    const auto complement {64U - bit_shift};

    switch (word_shift)
    {
        case 0U:
            return from_words((value.words[3] << bit_shift) | (value.words[2] >> complement),
                              (value.words[2] << bit_shift) | (value.words[1] >> complement),
                              (value.words[1] << bit_shift) | (value.words[0] >> complement),
                               value.words[0] << bit_shift);
        case 1U:
            return from_words((value.words[2] << bit_shift) | (value.words[1] >> complement),
                              (value.words[1] << bit_shift) | (value.words[0] >> complement),
                               value.words[0] << bit_shift,
                               UINT64_C(0));
        case 2U:
            return from_words((value.words[1] << bit_shift) | (value.words[0] >> complement),
                               value.words[0] << bit_shift,
                               UINT64_C(0),
                               UINT64_C(0));
        default:
            return from_words(value.words[0] << bit_shift, UINT64_C(0), UINT64_C(0), UINT64_C(0));
    }
}

// Arithmetic right shift: the vacated bits are filled with the sign bit, unlike uint256's
// logical shift_right. count is already reduced to [0, 256) by the caller, exactly as for
// uint256's shift helpers
BOOST_INT256_HOST_DEVICE constexpr int256 shift_right_signed(const int256& value, const unsigned count) noexcept
{
    BOOST_INT256_ASSERT_MSG(count < 256U, "Shift count is out of range");

    const auto fill {static_cast<std::uint64_t>(value.signed_high() >> 63)};
    const auto word_shift {count >> 6U};
    const auto bit_shift {count & 63U};

    if (bit_shift == 0U)
    {
        switch (word_shift)
        {
            case 0U:
                return value;
            case 1U:
                return from_words(fill, value.words[3], value.words[2], value.words[1]);
            case 2U:
                return from_words(fill, fill, value.words[3], value.words[2]);
            default:
                return from_words(fill, fill, fill, value.words[3]);
        }
    }

    const auto complement {64U - bit_shift};

    switch (word_shift)
    {
        case 0U:
            return from_words(static_cast<std::uint64_t>(value.signed_high() >> bit_shift),
                              (value.words[2] >> bit_shift) | (value.words[3] << complement),
                              (value.words[1] >> bit_shift) | (value.words[2] << complement),
                              (value.words[0] >> bit_shift) | (value.words[1] << complement));
        case 1U:
            return from_words(fill,
                              static_cast<std::uint64_t>(value.signed_high() >> bit_shift),
                              (value.words[2] >> bit_shift) | (value.words[3] << complement),
                              (value.words[1] >> bit_shift) | (value.words[2] << complement));
        case 2U:
            return from_words(fill, fill,
                              static_cast<std::uint64_t>(value.signed_high() >> bit_shift),
                              (value.words[2] >> bit_shift) | (value.words[3] << complement));
        default:
            return from_words(fill, fill, fill, static_cast<std::uint64_t>(value.signed_high() >> bit_shift));
    }
}

} // namespace detail

//=====================================
// Float Conversion Operators
//=====================================

BOOST_INT256_HOST_DEVICE constexpr int256::operator float() const noexcept
{
    return detail::signed_words_to_float<float>(words);
}

BOOST_INT256_HOST_DEVICE constexpr int256::operator double() const noexcept
{
    return detail::signed_words_to_float<double>(words);
}

#if !defined(BOOST_INT256_HAS_GPU_SUPPORT)

constexpr int256::operator long double() const noexcept
{
    return detail::signed_words_to_float<long double>(words);
}

#endif // BOOST_INT256_HAS_GPU_SUPPORT

//=====================================
// Float Construction
//=====================================

// The magnitude is truncated toward zero using uint256's own ladder (which never
// materializes a floating point value at or above 2^256), then the sign is reapplied by
// negating the bit image. The 2^255 boundary is checked with an exact uint256 constant so
// no Float ever has to represent 2^255 itself, which would overflow to infinity for float
template <BOOST_INT256_FLOATING_POINT_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256::int256(Float f) noexcept
{
    // NaN fails both comparisons, and is left as the default-initialized zero
    if (!(f >= Float{0}) && !(f <= Float{0}))
    {
        return;
    }

    const bool negative {f < Float{0}};
    const uint256 magnitude {negative ? -f : f};
    constexpr uint256 two_255 {UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)};

    if (negative)
    {
        *this = magnitude >= two_255 ? detail::from_words(UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0))
                                      : detail::to_signed_bits(-magnitude);
    }
    else
    {
        *this = magnitude >= two_255 ? detail::from_words(UINT64_C(0x7FFFFFFFFFFFFFFF), UINT64_MAX, UINT64_MAX, UINT64_MAX)
                                      : detail::to_signed_bits(magnitude);
    }
}

//=====================================
// Unary Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator+(const int256& value) noexcept
{
    return value;
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator-(const int256& value) noexcept
{
    return detail::to_signed_bits(-detail::to_unsigned_bits(value));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator~(const int256& value) noexcept
{
    return detail::to_signed_bits(~detail::to_unsigned_bits(value));
}

//=====================================
// Builtin 128-bit operand forwarders
//=====================================

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

#define BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(op, return_type)                                                                                     \
    BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR return_type operator op(const int256& lhs, const detail::builtin_i128 rhs) noexcept \
    {                                                                                                                                                      \
        return lhs op static_cast<int256>(rhs);                                                                                                            \
    }                                                                                                                                                      \
                                                                                                                                                             \
    BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR return_type operator op(const detail::builtin_i128 lhs, const int256& rhs) noexcept \
    {                                                                                                                                                      \
        return static_cast<int256>(lhs) op rhs;                                                                                                            \
    }                                                                                                                                                      \
                                                                                                                                                             \
    BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR return_type operator op(const int256& lhs, const detail::builtin_u128 rhs) noexcept \
    {                                                                                                                                                      \
        return lhs op static_cast<int256>(rhs);                                                                                                            \
    }                                                                                                                                                      \
                                                                                                                                                             \
    BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR return_type operator op(const detail::builtin_u128 lhs, const int256& rhs) noexcept \
    {                                                                                                                                                      \
        return static_cast<int256>(lhs) op rhs;                                                                                                            \
    }

#endif // BOOST_INT256_HAS_INT128 || BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Equality Operators
//=====================================

// Bitwise: the two's complement bit pattern determines equality regardless of the
// interpretation. Written directly on the words, mirroring uint256's own operator==
// (including its SIMD fast paths) rather than going through to_unsigned_bits(): the
// perf-parity asm pass measured that indirection reconstructing each operand through
// scalar loads and a shuffle on x86-64 instead of the clean vector load below
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator==(const int256& lhs, const int256& rhs) noexcept
{
    #if defined(BOOST_INT256_HAS_AVX2)

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

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator==(const int256& lhs, const bool rhs) noexcept
{
    return (lhs.words[1] | lhs.words[2] | lhs.words[3]) == UINT64_C(0) && lhs.words[0] == static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator==(const bool lhs, const int256& rhs) noexcept
{
    return (rhs.words[1] | rhs.words[2] | rhs.words[3]) == UINT64_C(0) && rhs.words[0] == static_cast<std::uint64_t>(lhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator==(const int256& lhs, const SignedInteger rhs) noexcept
{
    return lhs == int256{rhs};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator==(const SignedInteger lhs, const int256& rhs) noexcept
{
    return int256{lhs} == rhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator==(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    return (lhs.words[1] | lhs.words[2] | lhs.words[3]) == UINT64_C(0) && lhs.words[0] == static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator==(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    return (rhs.words[1] | rhs.words[2] | rhs.words[3]) == UINT64_C(0) && rhs.words[0] == static_cast<std::uint64_t>(lhs);
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(==, bool)
#endif

//=====================================
// Inequality Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const int256& lhs, const int256& rhs) noexcept
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

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const int256& lhs, const bool rhs) noexcept
{
    return (lhs.words[1] | lhs.words[2] | lhs.words[3]) != UINT64_C(0) || lhs.words[0] != static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const bool lhs, const int256& rhs) noexcept
{
    return (rhs.words[1] | rhs.words[2] | rhs.words[3]) != UINT64_C(0) || rhs.words[0] != static_cast<std::uint64_t>(lhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const int256& lhs, const SignedInteger rhs) noexcept
{
    return lhs != int256{rhs};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const SignedInteger lhs, const int256& rhs) noexcept
{
    return int256{lhs} != rhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    return (lhs.words[1] | lhs.words[2] | lhs.words[3]) != UINT64_C(0) || lhs.words[0] != static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    return (rhs.words[1] | rhs.words[2] | rhs.words[3]) != UINT64_C(0) || rhs.words[0] != static_cast<std::uint64_t>(lhs);
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(!=, bool)
#endif

//=====================================
// Less than Operators
//=====================================

// Flipping bit 63 of both operands turns a signed comparison into the unsigned borrow
// chain that uint256 already has, so this is the same instruction sequence plus two xors
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator<(const int256& lhs, const int256& rhs) noexcept
{
    constexpr std::uint64_t sign_bit {UINT64_C(0x8000000000000000)};
    const uint256 lu {lhs.words[3] ^ sign_bit, lhs.words[2], lhs.words[1], lhs.words[0]};
    const uint256 ru {rhs.words[3] ^ sign_bit, rhs.words[2], rhs.words[1], rhs.words[0]};

    return lu < ru;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<(const int256& lhs, const SignedInteger rhs) noexcept
{
    return lhs < int256{rhs};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<(const SignedInteger lhs, const int256& rhs) noexcept
{
    return int256{lhs} < rhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.signed_high() < 0 || ((lhs.words[1] | lhs.words[2] | lhs.words[3]) == UINT64_C(0) && lhs.words[0] < static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    // rhs.signed_high() only reports the sign, not the magnitude: a positive value with
    // words[1] or words[2] set but words[3] == 0 (anything in [2^64, 2^192)) still has
    // signed_high() == 0, so ">= 0" is the sign test and "upper != 0" alone already proves
    // rhs is bigger than any single-word lhs
    return rhs.signed_high() >= 0 && ((rhs.words[1] | rhs.words[2] | rhs.words[3]) != UINT64_C(0) || static_cast<std::uint64_t>(lhs) < rhs.words[0]);
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(<, bool)
#endif

//=====================================
// Less Equal Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const int256& lhs, const int256& rhs) noexcept
{
    return !(rhs < lhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const int256& lhs, const SignedInteger rhs) noexcept
{
    return !(int256{rhs} < lhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const SignedInteger lhs, const int256& rhs) noexcept
{
    return !(rhs < int256{lhs});
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    return lhs.signed_high() < 0 || ((lhs.words[1] | lhs.words[2] | lhs.words[3]) == UINT64_C(0) && lhs.words[0] <= static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    // See the note in operator<(UnsignedInteger, const int256&) above: the sign test is
    // ">= 0", not "> 0", and "upper != 0" alone already proves rhs is the larger value
    return rhs.signed_high() >= 0 && ((rhs.words[1] | rhs.words[2] | rhs.words[3]) != UINT64_C(0) || static_cast<std::uint64_t>(lhs) <= rhs.words[0]);
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(<=, bool)
#endif

//=====================================
// Greater Than Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator>(const int256& lhs, const int256& rhs) noexcept
{
    return rhs < lhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>(const int256& lhs, const SignedInteger rhs) noexcept
{
    return int256{rhs} < lhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>(const SignedInteger lhs, const int256& rhs) noexcept
{
    return rhs < int256{lhs};
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    // lhs.signed_high() only reports the sign, not the magnitude: a positive value with
    // words[1] or words[2] set but words[3] == 0 (anything in [2^64, 2^192)) still has
    // signed_high() == 0, so ">= 0" is the sign test and "upper != 0" alone already proves
    // lhs is bigger than any single-word rhs
    return lhs.signed_high() >= 0 && ((lhs.words[1] | lhs.words[2] | lhs.words[3]) != UINT64_C(0) || lhs.words[0] > static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    return rhs.signed_high() < 0 || ((rhs.words[1] | rhs.words[2] | rhs.words[3]) == UINT64_C(0) && static_cast<std::uint64_t>(lhs) > rhs.words[0]);
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(>, bool)
#endif

//=====================================
// Greater Equal Operators
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const int256& lhs, const int256& rhs) noexcept
{
    return !(lhs < rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const int256& lhs, const SignedInteger rhs) noexcept
{
    return !(lhs < int256{rhs});
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const SignedInteger lhs, const int256& rhs) noexcept
{
    return !(int256{lhs} < rhs);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    // See the note in operator>(const int256&, UnsignedInteger) above: the sign test is
    // ">= 0", not "> 0", and "upper != 0" alone already proves lhs is the larger value
    return lhs.signed_high() >= 0 && ((lhs.words[1] | lhs.words[2] | lhs.words[3]) != UINT64_C(0) || lhs.words[0] >= static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    return rhs.signed_high() < 0 || ((rhs.words[1] | rhs.words[2] | rhs.words[3]) == UINT64_C(0) && static_cast<std::uint64_t>(lhs) >= rhs.words[0]);
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(>=, bool)
#endif

//=====================================
// Spaceship Operator
//=====================================

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const int256& lhs, const int256& rhs) noexcept
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
BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const int256& lhs, const UnsignedInteger rhs) noexcept
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
BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const UnsignedInteger lhs, const int256& rhs) noexcept
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
BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const int256& lhs, const SignedInteger rhs) noexcept
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
BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const SignedInteger lhs, const int256& rhs) noexcept
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
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(<=>, std::strong_ordering)
#endif

#endif // BOOST_INT256_HAS_SPACESHIP_OPERATOR

//=====================================
// OR Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator|(const int256& lhs, const int256& rhs) noexcept
{
    return detail::to_signed_bits(detail::to_unsigned_bits(lhs) | detail::to_unsigned_bits(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator|(const int256& lhs, const SignedInteger rhs) noexcept
{
    const auto fill {rhs < 0 ? UINT64_MAX : UINT64_C(0)};
    return detail::from_words(lhs.words[3] | fill, lhs.words[2] | fill, lhs.words[1] | fill, lhs.words[0] | static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator|(const SignedInteger lhs, const int256& rhs) noexcept
{
    return rhs | lhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator|(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::from_words(lhs.words[3], lhs.words[2], lhs.words[1], lhs.words[0] | static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator|(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    return rhs | lhs;
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(|, int256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator|=(const Integer rhs) noexcept
{
    *this = *this | rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator|=(const int256& rhs) noexcept
{
    *this = *this | rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline int256& int256::operator|=(const Integer rhs) noexcept
{
    *this = *this | rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// AND Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator&(const int256& lhs, const int256& rhs) noexcept
{
    return detail::to_signed_bits(detail::to_unsigned_bits(lhs) & detail::to_unsigned_bits(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator&(const int256& lhs, const SignedInteger rhs) noexcept
{
    const auto fill {rhs < 0 ? UINT64_MAX : UINT64_C(0)};
    return detail::from_words(lhs.words[3] & fill, lhs.words[2] & fill, lhs.words[1] & fill, lhs.words[0] & static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator&(const SignedInteger lhs, const int256& rhs) noexcept
{
    return rhs & lhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator&(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::from_words(UINT64_C(0), UINT64_C(0), UINT64_C(0), lhs.words[0] & static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator&(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    return rhs & lhs;
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(&, int256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator&=(const Integer rhs) noexcept
{
    *this = *this & rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator&=(const int256& rhs) noexcept
{
    *this = *this & rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline int256& int256::operator&=(const Integer rhs) noexcept
{
    *this = *this & rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// XOR Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator^(const int256& lhs, const int256& rhs) noexcept
{
    return detail::to_signed_bits(detail::to_unsigned_bits(lhs) ^ detail::to_unsigned_bits(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator^(const int256& lhs, const SignedInteger rhs) noexcept
{
    const auto fill {rhs < 0 ? UINT64_MAX : UINT64_C(0)};
    return detail::from_words(lhs.words[3] ^ fill, lhs.words[2] ^ fill, lhs.words[1] ^ fill, lhs.words[0] ^ static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator^(const SignedInteger lhs, const int256& rhs) noexcept
{
    return rhs ^ lhs;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator^(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::from_words(lhs.words[3], lhs.words[2], lhs.words[1], lhs.words[0] ^ static_cast<std::uint64_t>(rhs));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator^(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    return rhs ^ lhs;
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(^, int256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator^=(const Integer rhs) noexcept
{
    *this = *this ^ rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator^=(const int256& rhs) noexcept
{
    *this = *this ^ rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline int256& int256::operator^=(const Integer rhs) noexcept
{
    *this = *this ^ rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Left Shift Operator
//=====================================

// Logical, and sign-agnostic like uint256's own shift
BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator<<(const int256& lhs, const Integer rhs) noexcept
{
    return detail::shift_left(lhs, static_cast<unsigned>(rhs));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator<<(const int256& lhs, const int256& rhs) noexcept
{
    return detail::shift_left(lhs, static_cast<unsigned>(rhs.words[0]));
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR detail::builtin_u128 operator<<(const detail::builtin_u128 lhs, const int256& rhs) noexcept
{
    // On MSVC this is routed through the isolated-namespace helper (see config.hpp)
    // rather than a plain infix <<, to avoid an ambiguity with this library's own
    // operator<< (finding F12)
    #ifdef BOOST_INT256_HAS_MSVC_INT128
    return boost::int256_detail::builtin128_shl_u(lhs, rhs.words[0]);
    #else
    return lhs << static_cast<detail::builtin_u128>(rhs.words[0]);
    #endif
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR detail::builtin_i128 operator<<(const detail::builtin_i128 lhs, const int256& rhs) noexcept
{
    #ifdef BOOST_INT256_HAS_MSVC_INT128
    return static_cast<detail::builtin_i128>(boost::int256_detail::builtin128_shl_u(static_cast<detail::builtin_u128>(lhs), rhs.words[0]));
    #else
    return lhs << static_cast<detail::builtin_u128>(rhs.words[0]);
    #endif
}

// The count flavor (library type on the left, MSVC's builtin 128-bit type on the
// right) needs the same dedicated exact match as uint256's own operator<< (finding
// F12): std::_Unsigned128/_Signed128's hidden friend and int256's operator<<(int256,
// int256) both need one user-defined conversion, on different arguments, and MSVC
// ties them otherwise
#ifdef BOOST_INT256_HAS_MSVC_INT128

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR int256 operator<<(const int256& lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs << static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR int256 operator<<(const int256& lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs << static_cast<std::uint64_t>(rhs);
}

#endif // BOOST_INT256_HAS_MSVC_INT128

#endif

BOOST_INT256_EXPORT template <typename Integer, std::enable_if_t<detail::is_any_integer_v<Integer> && (sizeof(Integer) * 8 <= 64), bool> = true>
BOOST_INT256_HOST_DEVICE constexpr detail::promoted_t<Integer> operator<<(const Integer lhs, const int256& rhs) noexcept
{
    return static_cast<detail::promoted_t<Integer>>(lhs) << rhs.words[0];
}

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator<<=(const Integer rhs) noexcept
{
    *this = *this << rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator<<=(const int256& rhs) noexcept
{
    *this = *this << rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline int256& int256::operator<<=(const Integer rhs) noexcept
{
    *this = *this << rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Right Shift Operator
//=====================================

// Arithmetic: the vacated bits are filled with the sign bit
BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator>>(const int256& lhs, const Integer rhs) noexcept
{
    return detail::shift_right_signed(lhs, static_cast<unsigned>(rhs));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator>>(const int256& lhs, const int256& rhs) noexcept
{
    return detail::shift_right_signed(lhs, static_cast<unsigned>(rhs.words[0]));
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR detail::builtin_u128 operator>>(const detail::builtin_u128 lhs, const int256& rhs) noexcept
{
    // On MSVC this is routed through the isolated-namespace helper (see config.hpp)
    // rather than a plain infix >>, to avoid an ambiguity with this library's own
    // operator>> (finding F12)
    #ifdef BOOST_INT256_HAS_MSVC_INT128
    return boost::int256_detail::builtin128_shr_u(lhs, rhs.words[0]);
    #else
    return lhs >> static_cast<detail::builtin_u128>(rhs.words[0]);
    #endif
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR detail::builtin_i128 operator>>(const detail::builtin_i128 lhs, const int256& rhs) noexcept
{
    // lhs stays signed (an arithmetic shift), only the count is unsigned
    #ifdef BOOST_INT256_HAS_MSVC_INT128
    return boost::int256_detail::builtin128_shr_i(lhs, rhs.words[0]);
    #else
    return lhs >> static_cast<detail::builtin_u128>(rhs.words[0]);
    #endif
}

// See the note above the analogous operator<< overloads (finding F12)
#ifdef BOOST_INT256_HAS_MSVC_INT128

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR int256 operator>>(const int256& lhs, const detail::builtin_u128 rhs) noexcept
{
    return lhs >> static_cast<std::uint64_t>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE BOOST_INT256_BUILTIN_CONSTEXPR int256 operator>>(const int256& lhs, const detail::builtin_i128 rhs) noexcept
{
    return lhs >> static_cast<std::uint64_t>(rhs);
}

#endif // BOOST_INT256_HAS_MSVC_INT128

#endif

BOOST_INT256_EXPORT template <typename Integer, std::enable_if_t<detail::is_any_integer_v<Integer> && (sizeof(Integer) * 8 <= 64), bool> = true>
BOOST_INT256_HOST_DEVICE constexpr detail::promoted_t<Integer> operator>>(const Integer lhs, const int256& rhs) noexcept
{
    return static_cast<detail::promoted_t<Integer>>(lhs) >> rhs.words[0];
}

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator>>=(const Integer rhs) noexcept
{
    *this = *this >> rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator>>=(const int256& rhs) noexcept
{
    *this = *this >> rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline int256& int256::operator>>=(const Integer rhs) noexcept
{
    *this = *this >> rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Increment and Decrement Operators
//=====================================

// Ripple with early exit, direct on this object's own words: going through
// to_unsigned_bits/uint256's own ++/to_signed_bits measured worse on the perf-parity asm
// pass (the indirection lost the branchy early-exit shape uint256's version compiles to,
// landing on a larger branch-free select chain instead)
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator++() noexcept
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

BOOST_INT256_HOST_DEVICE constexpr int256 int256::operator++(int) noexcept
{
    const auto temp {*this};
    ++(*this);
    return temp;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator--() noexcept
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

BOOST_INT256_HOST_DEVICE constexpr int256 int256::operator--(int) noexcept
{
    const auto temp {*this};
    --(*this);
    return temp;
}

//=====================================
// Addition Operator
//=====================================

// The low 256 bits of a two's complement sum do not depend on sign, so this runs on the
// same carry chain as uint256's own addition (see detail::add_words_signed above)
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator+(const int256& lhs, const int256& rhs) noexcept
{
    return detail::add_words_signed(lhs, rhs);
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to an unsigned type, result still unsigned
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator+(const int256& lhs, const SignedInteger rhs) noexcept
{
    const auto lu {detail::to_unsigned_bits(lhs)};
    return detail::to_signed_bits(rhs < 0 ? detail::sub_words(lu, -static_cast<std::uint64_t>(rhs)) :
                                            detail::add_words(lu, static_cast<std::uint64_t>(rhs)));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator+(const SignedInteger lhs, const int256& rhs) noexcept
{
    return rhs + lhs;
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator+(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::to_signed_bits(detail::add_words(detail::to_unsigned_bits(lhs), static_cast<std::uint64_t>(rhs)));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator+(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    return rhs + lhs;
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(+, int256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator+=(const Integer rhs) noexcept
{
    *this = *this + rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator+=(const int256& rhs) noexcept
{
    *this = *this + rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline int256& int256::operator+=(const Integer rhs) noexcept
{
    *this = *this + rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Subtraction Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator-(const int256& lhs, const int256& rhs) noexcept
{
    return detail::to_signed_bits(detail::sub_words(detail::to_unsigned_bits(lhs), detail::to_unsigned_bits(rhs)));
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to an unsigned type, result still unsigned
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator-(const int256& lhs, const SignedInteger rhs) noexcept
{
    const auto lu {detail::to_unsigned_bits(lhs)};
    return detail::to_signed_bits(rhs < 0 ? detail::add_words(lu, -static_cast<std::uint64_t>(rhs)) :
                                            detail::sub_words(lu, static_cast<std::uint64_t>(rhs)));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator-(const SignedInteger lhs, const int256& rhs) noexcept
{
    return int256{lhs} - rhs;
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator-(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::to_signed_bits(detail::sub_words(detail::to_unsigned_bits(lhs), static_cast<std::uint64_t>(rhs)));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator-(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    return int256{lhs} - rhs;
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(-, int256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator-=(const Integer rhs) noexcept
{
    *this = *this - rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator-=(const int256& rhs) noexcept
{
    *this = *this - rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline int256& int256::operator-=(const Integer rhs) noexcept
{
    *this = *this - rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Absolute Value function
//=====================================

// Branch-free two's complement absolute value: (x ^ mask) - mask, where mask is all ones
// for a negative value and zero otherwise. abs(min()) is min(), matching the builtin
// signed integer types. Defined after operator^ and operator- (not a template, so an
// earlier definition would see only builtin candidates and be ambiguous)
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 abs(const int256& value) noexcept
{
    const auto s {static_cast<std::uint64_t>(value.signed_high() >> 63)};
    const auto mask {detail::from_words(s, s, s, s)};

    return (value ^ mask) - mask;
}

//=====================================
// Multiplication Operator
//=====================================

// The low 256 bits of a two's complement product do not depend on sign either, so the
// full int256 x int256 form runs on the bit image directly
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator*(const int256& lhs, const int256& rhs) noexcept
{
    return detail::to_signed_bits(detail::mul_words(detail::to_unsigned_bits(lhs), detail::to_unsigned_bits(rhs)));
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to unsigned type, result still unsigned
#endif

// A one-word scalar multiply of the sign-filled value would lose the upper words of the
// sign extension, so the magnitude is multiplied instead and the result negated
BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator*(const int256& lhs, const SignedInteger rhs) noexcept
{
    const auto abs_rhs {rhs < 0 ? -static_cast<std::uint64_t>(rhs) : static_cast<std::uint64_t>(rhs)};
    const auto result {detail::mul_words(detail::to_unsigned_bits(lhs), abs_rhs)};

    return rhs < 0 ? detail::to_signed_bits(-result) : detail::to_signed_bits(result);
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator*(const SignedInteger lhs, const int256& rhs) noexcept
{
    return rhs * lhs;
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator*(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    return detail::to_signed_bits(detail::mul_words(detail::to_unsigned_bits(lhs), static_cast<std::uint64_t>(rhs)));
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator*(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    return rhs * lhs;
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(*, int256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator*=(const Integer rhs) noexcept
{
    *this = *this * rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator*=(const int256& rhs) noexcept
{
    *this = *this * rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline int256& int256::operator*=(const Integer rhs) noexcept
{
    *this = *this * rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Division Operator
//=====================================

// Division and modulo are computed entirely through uint256 magnitudes (to_unsigned_bits
// of abs(), never int256's own signed comparisons on an "abs" result), so a divisor of
// min() carries a magnitude of exactly 2^255, which upper_words correctly reports as
// wider than one word. This is the fix for Boost.Int128 defect A, which instead compared
// an abs() result with Boost.Int128's own signed operator> and divided by a bit pattern
// that still looked like a huge negative number

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wassume"
#endif

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator/(const int256& lhs, const int256& rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0))
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto abs_lhs {detail::to_unsigned_bits(abs(lhs))};
    const auto abs_rhs {detail::to_unsigned_bits(abs(rhs))};
    const auto negative_res {(lhs.signed_high() < 0) != (rhs.signed_high() < 0)};

    if (abs_lhs < abs_rhs)
    {
        return int256{};
    }

    uint256 quotient {};

    if ((abs_rhs.words[1] | abs_rhs.words[2] | abs_rhs.words[3]) == UINT64_C(0))
    {
        detail::one_word_div(abs_lhs, abs_rhs.words[0], quotient);
    }
    else
    {
        uint256 remainder {};
        detail::divide<false>(abs_lhs, abs_rhs, quotient, remainder);
    }

    const auto result {detail::to_signed_bits(quotient)};
    return negative_res ? -result : result;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator/(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0U))
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto abs_lhs {detail::to_unsigned_bits(abs(lhs))};
    uint256 quotient {};
    detail::one_word_div(abs_lhs, static_cast<std::uint64_t>(rhs), quotient);

    const auto result {detail::to_signed_bits(quotient)};
    return lhs.signed_high() < 0 ? -result : result;
}

// |rhs| for a min() divisor is exactly 2^255, which needs more than one word, so this
// branch (rather than a division) is what keeps that case from reaching a zero divisor
BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator/(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0))
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto abs_rhs {detail::to_unsigned_bits(abs(rhs))};

    if ((abs_rhs.words[1] | abs_rhs.words[2] | abs_rhs.words[3]) != UINT64_C(0))
    {
        return int256{};
    }

    const auto quotient {static_cast<std::uint64_t>(lhs) / abs_rhs.words[0]};
    const auto result {detail::from_words(UINT64_C(0), UINT64_C(0), UINT64_C(0), quotient)};

    return rhs.signed_high() < 0 ? -result : result;
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to unsigned type, result still unsigned
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator/(const int256& lhs, const SignedInteger rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0))
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto negative_res {(lhs.signed_high() < 0) != (rhs < 0)};
    const auto abs_rhs {rhs < 0 ? -static_cast<std::uint64_t>(rhs) : static_cast<std::uint64_t>(rhs)};
    const auto abs_lhs {detail::to_unsigned_bits(abs(lhs))};

    uint256 quotient {};
    detail::one_word_div(abs_lhs, abs_rhs, quotient);

    const auto result {detail::to_signed_bits(quotient)};
    return negative_res ? -result : result;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator/(const SignedInteger lhs, const int256& rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0))
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto abs_rhs {detail::to_unsigned_bits(abs(rhs))};

    if ((abs_rhs.words[1] | abs_rhs.words[2] | abs_rhs.words[3]) != UINT64_C(0))
    {
        return int256{};
    }

    const auto negative_res {(lhs < 0) != (rhs.signed_high() < 0)};
    const auto abs_lhs {lhs < 0 ? -static_cast<std::uint64_t>(lhs) : static_cast<std::uint64_t>(lhs)};
    const auto quotient {abs_lhs / abs_rhs.words[0]};
    const auto result {detail::from_words(UINT64_C(0), UINT64_C(0), UINT64_C(0), quotient)};

    return negative_res ? -result : result;
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(/, int256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator/=(const Integer rhs) noexcept
{
    *this = *this / rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator/=(const int256& rhs) noexcept
{
    *this = *this / rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline int256& int256::operator/=(const Integer rhs) noexcept
{
    *this = *this / rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

//=====================================
// Modulo Operator
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator%(const int256& lhs, const int256& rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0))
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto abs_lhs {detail::to_unsigned_bits(abs(lhs))};
    const auto abs_rhs {detail::to_unsigned_bits(abs(rhs))};
    const auto is_neg {lhs.signed_high() < 0};

    if (abs_lhs < abs_rhs)
    {
        return lhs;
    }

    uint256 remainder {};

    if ((abs_rhs.words[1] | abs_rhs.words[2] | abs_rhs.words[3]) == UINT64_C(0))
    {
        uint256 quotient {};
        std::uint64_t word_remainder {};
        detail::one_word_div(abs_lhs, abs_rhs.words[0], quotient, word_remainder);
        remainder = uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), word_remainder};
    }
    else
    {
        uint256 quotient {};
        detail::divide<true>(abs_lhs, abs_rhs, quotient, remainder);
    }

    const auto result {detail::to_signed_bits(remainder)};
    return is_neg ? -result : result;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator%(const int256& lhs, const UnsignedInteger rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0U))
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto abs_lhs {detail::to_unsigned_bits(abs(lhs))};
    uint256 quotient {};
    std::uint64_t remainder {};
    detail::one_word_div(abs_lhs, static_cast<std::uint64_t>(rhs), quotient, remainder);

    const auto result {detail::from_words(UINT64_C(0), UINT64_C(0), UINT64_C(0), remainder)};
    return lhs.signed_high() < 0 ? -result : result;
}

// |rhs| for a min() divisor is exactly 2^255, which needs more than one word, so this
// branch (rather than a modulo) is what keeps that case from reaching a zero divisor.
// This is the exact shape of Boost.Int128 defect A
BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator%(const UnsignedInteger lhs, const int256& rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0))
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto abs_rhs {detail::to_unsigned_bits(abs(rhs))};

    if ((abs_rhs.words[1] | abs_rhs.words[2] | abs_rhs.words[3]) != UINT64_C(0))
    {
        return int256{lhs};
    }

    const auto remainder {static_cast<std::uint64_t>(lhs) % abs_rhs.words[0]};
    return int256{remainder};
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4146) // Unary minus applied to unsigned type, result still unsigned
#endif

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator%(const int256& lhs, const SignedInteger rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0))
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto abs_rhs {rhs < 0 ? -static_cast<std::uint64_t>(rhs) : static_cast<std::uint64_t>(rhs)};
    const auto abs_lhs {detail::to_unsigned_bits(abs(lhs))};

    uint256 quotient {};
    std::uint64_t remainder {};
    detail::one_word_div(abs_lhs, abs_rhs, quotient, remainder);

    const auto result {detail::from_words(UINT64_C(0), UINT64_C(0), UINT64_C(0), remainder)};
    return lhs.signed_high() < 0 ? -result : result;
}

BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256 operator%(const SignedInteger lhs, const int256& rhs) noexcept
{
    if (BOOST_INT256_UNLIKELY(rhs == 0))
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto abs_rhs {detail::to_unsigned_bits(abs(rhs))};

    if ((abs_rhs.words[1] | abs_rhs.words[2] | abs_rhs.words[3]) != UINT64_C(0))
    {
        return int256{lhs};
    }

    const auto abs_lhs {lhs < 0 ? -static_cast<std::uint64_t>(lhs) : static_cast<std::uint64_t>(lhs)};
    const auto remainder {abs_lhs % abs_rhs.words[0]};
    const auto result {detail::from_words(UINT64_C(0), UINT64_C(0), UINT64_C(0), remainder)};

    return lhs < 0 ? -result : result;
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP(%, int256)
#endif

template <BOOST_INT256_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator%=(const Integer rhs) noexcept
{
    *this = *this % rhs;
    return *this;
}

BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator%=(const int256& rhs) noexcept
{
    *this = *this % rhs;
    return *this;
}

#ifdef BOOST_INT256_HAS_MSVC_INT128

template <BOOST_INT256_128BIT_INTEGER_CONCEPT>
BOOST_INT256_HOST_DEVICE inline int256& int256::operator%=(const Integer rhs) noexcept
{
    *this = *this % rhs;
    return *this;
}

#endif // BOOST_INT256_HAS_MSVC_INT128

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)
#undef BOOST_INT256_DETAIL_I256_BUILTIN128_BINARY_OP
#endif

//=====================================
// Built-in Integer Compound Assignment
//=====================================

// Compound assignment with a built-in integer on the left. Each of these is the binary
// operator above followed by a conversion back to Integer, matching what the builtin does

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4804) // Unsafe use of type bool in operation
#endif

#define BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(op, compound_op)                                                    \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_INTEGER_CONCEPT>                                                \
    BOOST_INT256_HOST_DEVICE constexpr Integer& operator compound_op(Integer& lhs, const int256& rhs) noexcept          \
    {                                                                                                                    \
        lhs = static_cast<Integer>(lhs op rhs);                                                                          \
        return lhs;                                                                                                      \
    }

BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(|, |=)
BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(&, &=)
BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(^, ^=)
BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(+, +=)
BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(-, -=)
BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(*, *=)
BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(/, /=)
BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(%, %=)
BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(<<, <<=)
BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP(>>, >>=)

#undef BOOST_INT256_DETAIL_I256_INTEGER_COMPOUND_OP

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

//=====================================
// Floating Point Operators
//=====================================

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#define BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(op, return_type)                                                        \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>                                         \
    BOOST_INT256_HOST_DEVICE constexpr return_type operator op(const int256& lhs, const Float rhs) noexcept             \
    {                                                                                                                    \
        return static_cast<Float>(lhs) op rhs;                                                                           \
    }                                                                                                                    \
                                                                                                                          \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>                                         \
    BOOST_INT256_HOST_DEVICE constexpr return_type operator op(const Float lhs, const int256& rhs) noexcept             \
    {                                                                                                                    \
        return lhs op static_cast<Float>(rhs);                                                                           \
    }

BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(+, Float)
BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(-, Float)
BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(*, Float)
BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(/, Float)

BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(==, bool)
BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(!=, bool)
BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(<, bool)
BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(<=, bool)
BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(>, bool)
BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(>=, bool)

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR

BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP(<=>, std::partial_ordering)

#endif // BOOST_INT256_HAS_SPACESHIP_OPERATOR

#undef BOOST_INT256_DETAIL_I256_FLOAT_BINARY_OP

#define BOOST_INT256_DETAIL_I256_FLOAT_COMPOUND_OP(op, compound_op)                                                      \
    template <BOOST_INT256_FLOATING_POINT_CONCEPT>                                                                       \
    BOOST_INT256_HOST_DEVICE constexpr int256& int256::operator compound_op(const Float rhs) noexcept                   \
    {                                                                                                                    \
        *this = static_cast<int256>(static_cast<Float>(*this) op rhs);                                                  \
        return *this;                                                                                                   \
    }                                                                                                                    \
                                                                                                                          \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>                                         \
    BOOST_INT256_HOST_DEVICE constexpr Float& operator compound_op(Float& lhs, const int256& rhs) noexcept              \
    {                                                                                                                    \
        lhs compound_op static_cast<Float>(rhs);                                                                         \
        return lhs;                                                                                                      \
    }

BOOST_INT256_DETAIL_I256_FLOAT_COMPOUND_OP(+, +=)
BOOST_INT256_DETAIL_I256_FLOAT_COMPOUND_OP(-, -=)
BOOST_INT256_DETAIL_I256_FLOAT_COMPOUND_OP(*, *=)
BOOST_INT256_DETAIL_I256_FLOAT_COMPOUND_OP(/, /=)

#undef BOOST_INT256_DETAIL_I256_FLOAT_COMPOUND_OP

#ifdef __clang__
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

#define BOOST_INT256_DETAIL_I256_FLOAT_DELETED_OP(op)                                                                    \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>                                         \
    BOOST_INT256_HOST_DEVICE int256 operator op(const int256& lhs, Float rhs) = delete;                                 \
                                                                                                                          \
    BOOST_INT256_EXPORT template <BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT>                                         \
    BOOST_INT256_HOST_DEVICE int256 operator op(Float lhs, const int256& rhs) = delete;

BOOST_INT256_DETAIL_I256_FLOAT_DELETED_OP(%)
BOOST_INT256_DETAIL_I256_FLOAT_DELETED_OP(&)
BOOST_INT256_DETAIL_I256_FLOAT_DELETED_OP(|)
BOOST_INT256_DETAIL_I256_FLOAT_DELETED_OP(^)
BOOST_INT256_DETAIL_I256_FLOAT_DELETED_OP(<<)
BOOST_INT256_DETAIL_I256_FLOAT_DELETED_OP(>>)

#undef BOOST_INT256_DETAIL_I256_FLOAT_DELETED_OP

namespace detail {

template <bool>
class numeric_limits_impl_i256
{
public:

    // Member constants
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
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
    static constexpr int digits = 255;
    static constexpr int digits10 = 76;
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = std::numeric_limits<std::uint64_t>::traps;
    static constexpr bool tinyness_before = false;

    // Member functions
    BOOST_INT256_HOST_DEVICE static constexpr auto (min)        () -> boost::int256::int256 { return {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto lowest       () -> boost::int256::int256 { return {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto (max)        () -> boost::int256::int256 { return {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto epsilon      () -> boost::int256::int256 { return {0, UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto round_error  () -> boost::int256::int256 { return {0, UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto infinity     () -> boost::int256::int256 { return {0, UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto quiet_NaN    () -> boost::int256::int256 { return {0, UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto signaling_NaN() -> boost::int256::int256 { return {0, UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
    BOOST_INT256_HOST_DEVICE static constexpr auto denorm_min   () -> boost::int256::int256 { return {0, UINT64_C(0), UINT64_C(0), UINT64_C(0)}; }
};

#if !defined(__cpp_inline_variables) || __cpp_inline_variables < 201606L

template <bool b> constexpr bool numeric_limits_impl_i256<b>::is_specialized;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::is_signed;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::is_integer;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::is_exact;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::has_infinity;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::has_quiet_NaN;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::has_signaling_NaN;

#if defined(__GNUC__) && __cplusplus > 202002L
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif

template <bool b> constexpr std::float_denorm_style numeric_limits_impl_i256<b>::has_denorm;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::has_denorm_loss;

#if defined(__GNUC__) && __cplusplus > 202002L
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif

template <bool b> constexpr std::float_round_style numeric_limits_impl_i256<b>::round_style;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::is_iec559;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::is_bounded;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::is_modulo;
template <bool b> constexpr int numeric_limits_impl_i256<b>::digits;
template <bool b> constexpr int numeric_limits_impl_i256<b>::digits10;
template <bool b> constexpr int numeric_limits_impl_i256<b>::max_digits10;
template <bool b> constexpr int numeric_limits_impl_i256<b>::radix;
template <bool b> constexpr int numeric_limits_impl_i256<b>::min_exponent;
template <bool b> constexpr int numeric_limits_impl_i256<b>::min_exponent10;
template <bool b> constexpr int numeric_limits_impl_i256<b>::max_exponent;
template <bool b> constexpr int numeric_limits_impl_i256<b>::max_exponent10;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::traps;
template <bool b> constexpr bool numeric_limits_impl_i256<b>::tinyness_before;

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
class numeric_limits<boost::int256::int256> :
    public boost::int256::detail::numeric_limits_impl_i256<true> {};

#ifdef __clang__
#  pragma clang diagnostic pop
#endif

} // namespace std

#endif // BOOST_INT256_DETAIL_INT256_IMP_HPP
