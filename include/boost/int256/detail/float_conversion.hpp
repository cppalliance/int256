// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_FLOAT_CONVERSION_HPP
#define BOOST_INT256_DETAIL_FLOAT_CONVERSION_HPP

#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/constants.hpp>
#include <boost/int256/detail/clz.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstdint>
#include <cstddef>
#include <limits>
#include <type_traits>

#endif

namespace boost {
namespace int256 {
namespace detail {

// 2^exp as a T, exactly, for 0 <= exp <= 255. Splitting at every 2^64 keeps each factor
// inside the range of an exact conversion from a 64-bit integer, so no rounding happens here.
// Only the taken branch is evaluated, so the shift counts are always in range
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T exact_power_of_two(const int exp) noexcept
{
    return exp < 64  ? static_cast<T>(UINT64_C(1) << exp) :
           exp < 128 ? static_cast<T>(UINT64_C(1) << (exp - 64)) * offset_value_v<T> :
           exp < 192 ? static_cast<T>(UINT64_C(1) << (exp - 128)) * offset_value_v<T> * offset_value_v<T>
                     : static_cast<T>(UINT64_C(1) << (exp - 192)) * offset_value_v<T> * offset_value_v<T> * offset_value_v<T>;
}

// A significand of 64 bits or more holds each word exactly, and holds the scaled high word
// exactly as well, so the addition is the only rounding. For the single format widths, x87
// 80-bit (64) and IEEE binary128 (113), that makes the two term form exact. IBM double-double
// (106) also takes this path: correct rounding is not well defined for a format that is a sum
// of two doubles, and the two term form lands closer to the exact value than the compiler's
// own conversion does
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T two_word_to_float_impl(const std::uint64_t high, const std::uint64_t low,
                                                           std::true_type) noexcept
{
    return static_cast<T>(high) * offset_value_v<T> + static_cast<T>(low);
}

// float and double cannot hold the high word or the sum exactly, so high * 2^64 + low rounds
// as many as three times and lands up to one ulp away from the correctly rounded result.
// Round the 128-bit value to exactly digits bits here instead, once, then apply an exact
// power of two. See the note on ties in the body
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T two_word_to_float_impl(const std::uint64_t high, const std::uint64_t low,
                                                           std::false_type) noexcept
{
    // Anything below 2^64 is one conversion the compiler already rounds correctly
    if (high == UINT64_C(0))
    {
        return static_cast<T>(low);
    }

    constexpr int digits {std::numeric_limits<T>::digits};
    constexpr int residue_bits {64 - digits};
    constexpr std::uint64_t residue_mask {(UINT64_C(1) << residue_bits) - UINT64_C(1)};
    constexpr std::uint64_t halfway {UINT64_C(1) << (residue_bits - 1)};

    // Normalize so bit 127 of the pair is set, which puts the significand at the top of the
    // high word. high is non-zero here, so the distance is always less than 64
    const auto shift {static_cast<unsigned>(countl_zero(high))};
    const auto norm_high {shift == 0U ? high : ((high << shift) | (low >> (64U - shift)))};
    const auto norm_low {shift == 0U ? low : (low << shift)};

    const auto significand {norm_high >> residue_bits};
    const auto residue {norm_high & residue_mask};

    // Round to nearest, ties to even. The discarded part is residue * 2^64 + norm_low, so it
    // is above the halfway point when residue is, and it is exactly the halfway point only
    // when residue equals halfway and every lower bit is clear
    const bool round_up {residue > halfway ||
                         (residue == halfway && (norm_low != UINT64_C(0) || (significand & UINT64_C(1)) != UINT64_C(0)))};

    const auto rounded {significand + (round_up ? UINT64_C(1) : UINT64_C(0))};

    // rounded holds digits bits, or digits + 1 when it carried, in which case it is a power of
    // two. Either way the conversion and the scaling are exact, so the product is the value
    // rounded exactly once
    return static_cast<T>(rounded) * exact_power_of_two<T>(static_cast<int>(64U - shift) + residue_bits);
}

// Converts the 128-bit value (high, low) to T, correctly rounded to nearest with ties to even
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T two_word_to_float(const std::uint64_t high, const std::uint64_t low) noexcept
{
    return two_word_to_float_impl<T>(high, low,
                                     std::integral_constant<bool, (std::numeric_limits<T>::digits >= 64)>{});
}

// Values of at least 2^128 whose significand is narrower than one word (float and double).
// The top 64 bits are converted in one correctly rounded step and then scaled by an exact
// power of two. Everything below them only decides ties, so it is folded into the low bit,
// which is discarded by that conversion; the injection turns a tie into a round up and can
// never turn a round down into a tie because the value below a tie is odd
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T wide_to_float_narrow_significand(const std::uint64_t (&words)[4], const std::size_t top) noexcept
{
    static_assert(std::numeric_limits<T>::digits <= 62, "At least two bits have to be discarded");

    const auto shift {static_cast<unsigned>(countl_zero(words[top]))};
    const auto bit_length {static_cast<int>(64U * top) + 64 - static_cast<int>(shift)};

    // Every value this wide is above the largest finite T when T is a float
    if (bit_length > std::numeric_limits<T>::max_exponent)
    {
        return std::numeric_limits<T>::infinity();
    }

    auto significand {shift == 0U ? words[top] : ((words[top] << shift) | (words[top - 1U] >> (64U - shift)))};

    std::uint64_t rest {shift == 0U ? words[top - 1U] : (words[top - 1U] << shift)};
    for (std::size_t i {}; i + 1U < top; ++i)
    {
        rest |= words[i];
    }

    if (rest != UINT64_C(0))
    {
        significand |= UINT64_C(1);
    }

    return static_cast<T>(significand) * exact_power_of_two<T>(bit_length - 64);
}

// Values of at least 2^128 whose significand is at least one word wide (x87 long double,
// IEEE binary128, IBM double-double). The top 128 bits are converted with the exact two term
// form, whose single addition is the only rounding, and the sticky bit is folded in as above
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T wide_to_float_wide_significand(const std::uint64_t (&words)[4], const std::size_t top) noexcept
{
    static_assert(std::numeric_limits<T>::digits >= 64 && std::numeric_limits<T>::digits <= 126,
                  "The significand has to be one to two words wide");

    const auto shift {static_cast<unsigned>(countl_zero(words[top]))};

    const auto significand_high {shift == 0U ? words[top] : ((words[top] << shift) | (words[top - 1U] >> (64U - shift)))};
    auto significand_low {shift == 0U ? words[top - 1U] : ((words[top - 1U] << shift) | (words[top - 2U] >> (64U - shift)))};

    std::uint64_t rest {shift == 0U ? words[top - 2U] : (words[top - 2U] << shift)};
    for (std::size_t i {}; i + 2U < top; ++i)
    {
        rest |= words[i];
    }

    if (rest != UINT64_C(0))
    {
        significand_low |= UINT64_C(1);
    }

    // Weight of bit 0 of the retained 128 bits
    const auto exponent {static_cast<int>(64U * (top - 1U)) - static_cast<int>(shift)};

    return (static_cast<T>(significand_high) * offset_value_v<T> + static_cast<T>(significand_low)) *
           exact_power_of_two<T>(exponent);
}

template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T wide_to_float_impl(const std::uint64_t (&words)[4], const std::size_t top,
                                                        std::true_type) noexcept
{
    return wide_to_float_wide_significand<T>(words, top);
}

template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T wide_to_float_impl(const std::uint64_t (&words)[4], const std::size_t top,
                                                        std::false_type) noexcept
{
    return wide_to_float_narrow_significand<T>(words, top);
}

// Converts a 256-bit value to T, correctly rounded to nearest with ties to even
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T unsigned_words_to_float(const std::uint64_t (&words)[4]) noexcept
{
    if ((words[3] | words[2]) == UINT64_C(0))
    {
        return two_word_to_float<T>(words[1], words[0]);
    }

    return wide_to_float_impl<T>(words, words[3] != UINT64_C(0) ? 3U : 2U,
                                 std::integral_constant<bool, (std::numeric_limits<T>::digits >= 64)>{});
}

// The other direction: a value in [0, 2^64) truncated toward zero into a 64-bit word.
// A cast straight to an unsigned type takes the compiler's unsigned conversion path, which
// subtracts 2^63 and puts the top bit back afterwards. Clang 7 emits that subtraction inside the
// window where it has already set the x87 control word to single precision, so an 80-bit long
// double loses every significand bit past the 24th. Splitting at 2^63 here keeps both conversions
// inside the signed range, where the conversion is one instruction on every compiler
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr std::uint64_t float_to_uint64(const T value) noexcept
{
    constexpr T two_63 {static_cast<T>(UINT64_C(1) << 63)};

    // value - two_63 is exact: it is a multiple of the ulp of value, and it is below 2^63
    return value < two_63
           ? static_cast<std::uint64_t>(static_cast<std::int64_t>(value))
           : static_cast<std::uint64_t>(static_cast<std::int64_t>(value - two_63)) | (UINT64_C(1) << 63);
}

} // namespace detail
} // namespace int256
} // namespace boost

#endif // BOOST_INT256_DETAIL_FLOAT_CONVERSION_HPP
