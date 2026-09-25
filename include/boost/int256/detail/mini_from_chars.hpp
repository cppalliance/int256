// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Adapted from Boost.Int128 (Copyright 2022 Peter Dimov, Copyright 2023 Matt Borland)

#ifndef BOOST_INT256_DETAIL_MINI_FROM_CHARS_HPP
#define BOOST_INT256_DETAIL_MINI_FROM_CHARS_HPP

#include <boost/int256/int256.hpp>
#include <boost/int256/detail/config.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cerrno>
#include <limits>
#include <cstddef>

#if !(defined(BOOST_INT256_HAS_GPU_SUPPORT) || defined(BOOST_INT256_DISABLE_EXCEPTIONS))
#include <stdexcept>
#endif

#endif

namespace boost {
namespace int256 {
namespace detail {

namespace impl {

#if !defined(BOOST_INT256_HAS_GPU_SUPPORT)

BOOST_INT256_INLINE_CONSTEXPR unsigned char uchar_values[] =
     {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 255, 255, 255, 255, 255, 255,
      255,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35, 255, 255, 255, 255, 255,
      255,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

static_assert(sizeof(uchar_values) == 256, "uchar_values should represent all 256 values of unsigned char");

#endif // Namespace scope tables are unavailable on the device

// Convert characters for 0-9, A-Z, a-z to 0-35. The digit separator ' is 254. Anything else is 255
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr auto digit_from_char(char val) noexcept -> unsigned char
{
    #if defined(BOOST_INT256_HAS_GPU_SUPPORT)

    constexpr unsigned char uchar_values[] =
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
     255, 255, 255, 255, 255, 255, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255,
       0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 255, 255, 255, 255, 255, 255,
     255,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
      25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35, 255, 255, 255, 255, 255,
     255,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
      25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35, 255, 255, 255, 255, 255,
     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
     255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

    static_assert(sizeof(uchar_values) == 256, "uchar_values should represent all 256 values of unsigned char");

    #endif // Namespace scope tables are unavailable on the device

    return uchar_values[static_cast<unsigned char>(val)];
}

// Parses [first, last) in the given base. Returns an errno value on failure, and the
// negated number of characters consumed on success.
template <typename Integer, typename Unsigned_Integer, bool is_literal_parse = false>
BOOST_INT256_HOST_DEVICE constexpr int from_chars_integer_impl(const char* first, const char* last, Integer& value, int base) noexcept
{
    // Guard the base before it is used as a divisor below: base 0 would divide by zero
    // and base 1 would never terminate
    if (base < 2 || base > 36)
    {
        return EINVAL;
    }

    if (last - first <= 0)
    {
        return EINVAL;
    }

    Unsigned_Integer result {};
    Unsigned_Integer overflow_value {};
    Unsigned_Integer max_digit {};

    const auto unsigned_base {static_cast<Unsigned_Integer>(base)};

    // Strip sign if the type is signed
    // Negative sign will be appended at the end of parsing
    bool is_negative {false};
    static_cast<void>(is_negative);
    auto next {first};

    BOOST_INT256_IF_CONSTEXPR (std::numeric_limits<Integer>::is_signed)
    {
        if (*next == '-')
        {
            is_negative = true;
            ++next;
        }

        overflow_value = static_cast<Unsigned_Integer>((std::numeric_limits<Integer>::max)());
        max_digit = static_cast<Unsigned_Integer>((std::numeric_limits<Integer>::max)());

        if (is_negative)
        {
            ++overflow_value;
            ++max_digit;
        }
    }
    else
    {
        if (*next == '-' || *next == '+')
        {
            return EINVAL;
        }

        overflow_value = (std::numeric_limits<Unsigned_Integer>::max)();
        max_digit = (std::numeric_limits<Unsigned_Integer>::max)();
    }

    overflow_value /= unsigned_base;
    max_digit %= unsigned_base;

    // If the only character was a sign abort now
    if (next == last)
    {
        return EINVAL;
    }

    // Where the digits start, so that a parse which consumed none can be recognized
    const auto digits_first {next};

    bool overflowed {false};

    const std::ptrdiff_t nc {last - next};

    // For bases 2..10 the first digits10 characters always fit in the unsigned
    // For bases above 10, the safe window is shorter, so we must check with each iteration
    const std::ptrdiff_t nd {
        base <= 10
            ? static_cast<std::ptrdiff_t>(std::numeric_limits<Integer>::digits10)
            : std::ptrdiff_t{0}
    };

    const std::ptrdiff_t fast_limit {nd < nc ? nd : nc};
    std::ptrdiff_t i {0};

    for (; i < fast_limit; ++i)
    {
        const auto raw_digit {digit_from_char(*next)};

        // When parsing a user-defined literal skip the digit separator ' (marked as 254)
        BOOST_INT256_IF_CONSTEXPR (is_literal_parse)
        {
            if (raw_digit == 254)
            {
                ++next;
                continue;
            }
        }

        const auto current_digit {static_cast<Unsigned_Integer>(raw_digit)};

        if (current_digit >= unsigned_base)
        {
            break;
        }

        result = static_cast<Unsigned_Integer>(result * unsigned_base + current_digit);
        ++next;
    }

    for (; i < nc; ++i)
    {
        const auto raw_digit {digit_from_char(*next)};

        // When parsing a user-defined literal skip the digit separator ' (marked as 254)
        BOOST_INT256_IF_CONSTEXPR (is_literal_parse)
        {
            if (raw_digit == 254)
            {
                ++next;
                continue;
            }
        }

        const auto current_digit {static_cast<Unsigned_Integer>(raw_digit)};

        if (current_digit >= unsigned_base)
        {
            break;
        }

        if (result < overflow_value || (result == overflow_value && current_digit <= max_digit))
        {
            result = static_cast<Unsigned_Integer>(result * unsigned_base + current_digit);
        }
        else
        {
            overflowed = true;
            break;
        }

        ++next;
    }

    // Return the parsed value, adding the sign back if applicable
    // If we have overflowed, then we do not return the result
    if (overflowed)
    {
        return EDOM;
    }

    // std::from_chars leaves the caller's value untouched unless it extracted digits,
    // and this entry point follows the same contract. Consuming nothing is reported as
    // a count of zero rather than an errno value, so the caller can tell it apart from
    // an empty range.
    if (next != digits_first)
    {
        value = static_cast<Integer>(result);

        BOOST_INT256_IF_CONSTEXPR (std::numeric_limits<Integer>::is_signed)
        {
            if (is_negative)
            {
                value = static_cast<Integer>(-(static_cast<Unsigned_Integer>(value)));
            }
        }
    }

    // This value will be negative to differentiate from errno values
    // since they are in the range of acceptable distances

    // This cast is useless on 32-bit platforms
    #if defined(__GNUC__) && !defined(__clang__)
    #  pragma GCC diagnostic push
    #  pragma GCC diagnostic ignored "-Wuseless-cast"
    #endif

    return static_cast<int>(first - next);

    #if defined(__GNUC__) && !defined(__clang__)
    #  pragma GCC diagnostic pop
    #endif
}

} // namespace impl

BOOST_INT256_TEST_EXPORT BOOST_INT256_HOST_DEVICE constexpr int from_chars(const char* first, const char* last, uint256& value, int base = 10) noexcept
{
    return impl::from_chars_integer_impl<uint256, uint256>(first, last, value, base);
}

BOOST_INT256_TEST_EXPORT BOOST_INT256_HOST_DEVICE constexpr int from_chars(const char* first, const char* last, int256& value, int base = 10) noexcept
{
    return impl::from_chars_integer_impl<int256, uint256>(first, last, value, base);
}

// Parsing entry points for the user-defined literals. Unlike from_chars these skip the
// C++ digit separator ' so that literals such as 1'234'567_u256 are accepted.
BOOST_INT256_TEST_EXPORT BOOST_INT256_HOST_DEVICE constexpr int from_chars_literal(const char* first, const char* last, uint256& value, int base = 10) noexcept
{
    return impl::from_chars_integer_impl<uint256, uint256, true>(first, last, value, base);
}

BOOST_INT256_TEST_EXPORT BOOST_INT256_HOST_DEVICE constexpr int from_chars_literal(const char* first, const char* last, int256& value, int base = 10) noexcept
{
    return impl::from_chars_integer_impl<int256, uint256, true>(first, last, value, base);
}

// Rejects an out of range literal
[[noreturn]] BOOST_INT256_HOST_DEVICE inline void parse_literal_out_of_range()
{
    #if defined(BOOST_INT256_HAS_GPU_SUPPORT) || defined(BOOST_INT256_DISABLE_EXCEPTIONS)
    BOOST_INT256_UNREACHABLE;
    #else
    BOOST_INT256_THROW_EXCEPTION(std::out_of_range("Literal is out of range of the target type"));
    #endif
}

// Rejects an invalid literal
[[noreturn]] BOOST_INT256_HOST_DEVICE inline void parse_invalid_literal()
{
    #if defined(BOOST_INT256_HAS_GPU_SUPPORT) || defined(BOOST_INT256_DISABLE_EXCEPTIONS)
    BOOST_INT256_UNREACHABLE;
    #else
    BOOST_INT256_THROW_EXCEPTION(std::invalid_argument("Literal is not a valid integer"));
    #endif
}

// GCC before 6 rejects a constexpr function that contains a throw-expression or a
// call to a non-constexpr function anywhere in its body so we need to use unreachable in that case
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 6
#  define BOOST_INT256_REJECT_LITERAL(reporter) BOOST_INT256_UNREACHABLE
#else
#  define BOOST_INT256_REJECT_LITERAL(reporter) reporter()
#endif

// Parse a user-defined literal, hard-failing on any malformed or out-of-range input.
// A C++ base prefix (0x/0X hex, 0b/0B binary, or a leading 0 for octal) is stripped and
// the digits parsed in that base, otherwise handled as base 10
// A malformed or out-of-range literal is a compile-time error in a constant expression and
// terminates the program at run time (the reporters throw out of this noexcept function).
template <typename Integer>
BOOST_INT256_HOST_DEVICE constexpr Integer parse_literal(const char* first, const char* last) noexcept
{
    Integer value {};

    // A leading sign stays with the digits; a base prefix, if present, follows it.
    auto next {first};
    const bool negative {next != last && *next == '-'};
    if (negative)
    {
        ++next;
    }

    int base {10};
    bool prefixed {false};

    if (last - next >= 2 && *next == '0')
    {
        const char marker {next[1]};
        if (marker == 'x' || marker == 'X')
        {
            base = 16;
            next += 2;
            prefixed = true;
        }
        else if (marker == 'b' || marker == 'B')
        {
            base = 2;
            next += 2;
            prefixed = true;
        }
        else
        {
            base = 8;
            next += 1;
            prefixed = true;
        }
    }

    // With no prefix, from_chars_literal handles the sign and the full decimal range.
    // Overflow is reported as EDOM; anything else short of full consumption is malformed.
    if (!prefixed)
    {
        const auto status {from_chars_literal(first, last, value)};
        if (status == EDOM)
        {
            BOOST_INT256_REJECT_LITERAL(parse_literal_out_of_range);
        }
        else if (status != first - last)
        {
            BOOST_INT256_REJECT_LITERAL(parse_invalid_literal);
        }

        return value;
    }

    // Prefixed: parse the magnitude as uint256 in the detected base, reinterpret it as
    // Integer, then reapply the sign
    uint256 magnitude {};
    const auto status {from_chars_literal(next, last, magnitude, base)};
    if (status == EDOM)
    {
        BOOST_INT256_REJECT_LITERAL(parse_literal_out_of_range);
    }
    else if (status != next - last)
    {
        BOOST_INT256_REJECT_LITERAL(parse_invalid_literal);
    }

    // from_chars_literal above only range checked the magnitude against uint256's own
    // full width. A cooked literal never carries a '-' (-X_i256 is unary minus applied
    // to X_i256, so X alone reaches this function), and only the string/macro forms
    // pass a '-' through as part of the character sequence, so the bound tightens here:
    // at most max() with no sign, or 2^255 (one more, min()'s magnitude) with one
    BOOST_INT256_IF_CONSTEXPR (std::numeric_limits<Integer>::is_signed)
    {
        const uint256 limit {negative ? static_cast<uint256>((std::numeric_limits<Integer>::max)()) + UINT64_C(1)
                                       : static_cast<uint256>((std::numeric_limits<Integer>::max)())};

        if (magnitude > limit)
        {
            BOOST_INT256_REJECT_LITERAL(parse_literal_out_of_range);
        }
    }

    value = static_cast<Integer>(magnitude);

    if (negative)
    {
        BOOST_INT256_IF_CONSTEXPR (std::numeric_limits<Integer>::is_signed)
        {
            value = static_cast<Integer>(-value);
        }
        else
        {
            // parse_literal is a named detail entry point, so a negative sign for an
            // unsigned target is rejected rather than assumed impossible
            BOOST_INT256_REJECT_LITERAL(parse_invalid_literal);
        }
    }

    return value;
}

} // namespace detail
} // namespace int256
} // namespace boost

#undef BOOST_INT256_REJECT_LITERAL

#endif // BOOST_INT256_DETAIL_MINI_FROM_CHARS_HPP
