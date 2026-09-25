// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_IOSTREAM_HPP
#define BOOST_INT256_IOSTREAM_HPP

#include <boost/int256/int256.hpp>
#include <boost/int256/detail/mini_from_chars.hpp>
#include <boost/int256/detail/mini_to_chars.hpp>
#include <boost/int256/detail/utilities.hpp>
#include <boost/int256/detail/config.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <type_traits>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstring>
#include <limits>

#endif

namespace boost {
namespace int256 {

namespace detail {

template <typename T>
struct streamable_overload
{
    static constexpr bool value = std::is_same<T, uint256>::value || std::is_same<T, int256>::value;
};

template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_streamable_overload_v = streamable_overload<T>::value;

} // namespace detail

BOOST_INT256_EXPORT template <typename charT, typename traits, typename LibIntegerType>
auto operator>>(std::basic_istream<charT, traits>& is, LibIntegerType& v)
    -> std::enable_if_t<detail::is_streamable_overload_v<LibIntegerType>, std::basic_istream<charT, traits>&>
{
    charT t_buffer[detail::mini_to_chars_buffer_size] {};
    is >> std::ws >> std::setw(static_cast<int>(detail::mini_to_chars_buffer_size) - 1) >> t_buffer;

    const auto t_buffer_len {std::char_traits<charT>::length(t_buffer)};

    char buffer[detail::mini_to_chars_buffer_size] {};
    auto buffer_start {buffer};

    BOOST_INT256_IF_CONSTEXPR (!std::is_same<charT, char>::value)
    {
        auto first {buffer};
        auto t_first {t_buffer};
        const auto t_buffer_end {t_buffer + detail::strlen(t_buffer)};

        while (t_first != t_buffer_end)
        {
            *first++ = static_cast<char>(*t_first++);
        }
    }
    else
    {
        std::memcpy(buffer, t_buffer, sizeof(buffer));
    }

    const auto flags {is.flags()};
    int base {10};

    // How many characters were spliced out of the buffer as a base prefix (0 or 2 for an
    // explicit 0x/0X; octal never strips one, a leading zero there is an ordinary digit).
    // from_chars_integer_impl already strips a leading sign itself, for any base, and
    // widens its overflow allowance by one when it does, so nothing else here has to
    // special-case the sign: it stays wherever it was in the buffer
    std::size_t removed_prefix_chars {0};

    if (flags & std::ios_base::oct)
    {
        // No prefix is stripped: in base 8 a leading zero is already an ordinary digit,
        // so "017" reads as 15 and "08" reads as 0 leaving the '8' in the stream, which
        // is what num_get does for the builtin types.
        base = 8;
    }
    else if (flags & std::ios_base::hex)
    {
        base = 16;

        // The 0x/0X prefix can follow a leading sign ("-0x1a"), so the search for it
        // starts after one if the target type is signed. It is then spliced out in
        // place (the digits shift left over it) so the sign and the digits it strips
        // stay contiguous for from_chars_integer_impl, exactly as they would with no
        // prefix at all
        auto digit_scan {buffer_start};

        BOOST_INT256_IF_CONSTEXPR (std::numeric_limits<LibIntegerType>::is_signed)
        {
            if (*digit_scan == '-')
            {
                ++digit_scan;
            }
        }

        if (digit_scan[0] == '0' && (digit_scan[1] == 'x' || digit_scan[1] == 'X'))
        {
            char* dst {digit_scan};
            char* src {digit_scan + 2};

            while (*src != '\0')
            {
                *dst++ = *src++;
            }

            *dst = '\0';
            removed_prefix_chars = 2U;
        }
    }

    const auto r {detail::from_chars(buffer_start, buffer + detail::strlen(buffer), v, base)};

    // Put back unconsumed characters. Only a strictly negative r means digits (and any
    // sign) were extracted, and then -r of them were consumed on top of any spliced-out
    // base prefix. Anything else consumed nothing at all, so even the prefix goes back.
    std::size_t consumed {};
    if (r < 0)
    {
        consumed = removed_prefix_chars + static_cast<std::size_t>(-r);
    }

    BOOST_INT256_ASSERT(t_buffer_len >= consumed);
    const auto return_chars {t_buffer_len - consumed};

    for (std::size_t i {}; i < return_chars; ++i)
    {
        is.putback(t_buffer[t_buffer_len - i - 1]);
    }

    // from_chars returns the negated number of characters consumed on success, so
    // anything not negative means no digits were extracted: r == 0 is a first
    // character that is not a digit in the base, and r > 0 is an errno value
    // (EINVAL for an empty input or a sign, EDOM for a value that does not fit).
    // The stream has to report all of those as a failure. This must come after the
    // putback loop: putback fails its own sentry once failbit is set.
    if (r >= 0)
    {
        v = LibIntegerType{};
        is.setstate(std::ios_base::failbit);
    }

    return is;
}

BOOST_INT256_EXPORT template <typename charT, typename traits, typename LibIntegerType>
auto operator<<(std::basic_ostream<charT, traits>& os, const LibIntegerType& v)
    -> std::enable_if_t<detail::is_streamable_overload_v<LibIntegerType>, std::basic_ostream<charT, traits>&>
{
    char buffer[detail::mini_to_chars_buffer_size] {};

    const auto flags {os.flags()};
    int base {10};
    bool uppercase {false};
    if (flags & std::ios_base::oct)
    {
        base = 8;
    }
    else if (flags & std::ios_base::hex)
    {
        base = 16;
    }

    if (flags & std::ios_base::uppercase)
    {
        uppercase = true;
    }

    // mini_to_chars already writes a leading '-' for a negative int256 (never for
    // uint256), so the sign is peeled off the front here rather than reconstructed
    auto digits_first {detail::mini_to_chars(buffer, v, base, uppercase)};
    const bool negative {*digits_first == '-'};

    if (negative)
    {
        ++digits_first;
    }

    // "head" is the sign and the base prefix, kept as its own short run so that
    // std::internal can place the fill between it and the digits; the digits
    // themselves never move, unlike an earlier version of this function that spliced
    // the prefix in front of an already-written sign (Boost.Int128 finding C: that
    // gave "0x-ff" instead of "-0xff" when done in the wrong order)
    char head[3] {};
    std::size_t head_len {0};

    if (negative)
    {
        head[head_len++] = '-';
    }
    else
    {
        // showpos prints a '+' for a non-negative int256 in decimal only, exactly like
        // the builtin signed integer types; uint256 (is_signed false) never prints one
        BOOST_INT256_IF_CONSTEXPR (std::numeric_limits<LibIntegerType>::is_signed)
        {
            if ((flags & std::ios_base::showpos) && base == 10)
            {
                head[head_len++] = '+';
            }
        }
    }

    // A zero prints as a bare "0" with showbase, the same as the builtin types
    if ((flags & std::ios_base::showbase) && v != 0U)
    {
        if (base == 8)
        {
            head[head_len++] = '0';
        }
        else if (base == 16)
        {
            head[head_len++] = '0';
            head[head_len++] = uppercase ? 'X' : 'x';
        }
    }

    const auto digits_len {static_cast<std::size_t>(detail::strlen(digits_first))};
    const auto total_len {head_len + digits_len};

    // A formatted output function always consumes the requested width, whether or not
    // any padding is actually needed
    const auto requested_width {os.width()};
    os.width(0);
    const std::size_t pad {(requested_width > 0 && static_cast<std::size_t>(requested_width) > total_len) ?
                            static_cast<std::size_t>(requested_width) - total_len : std::size_t{0}};

    const charT fill_char {os.fill()};
    const auto adjust {flags & std::ios_base::adjustfield};

    charT t_head[3] {};
    charT t_digits[detail::mini_to_chars_buffer_size] {};

    BOOST_INT256_IF_CONSTEXPR (!std::is_same<charT, char>::value)
    {
        for (std::size_t i {}; i < head_len; ++i)
        {
            t_head[i] = static_cast<charT>(head[i]);
        }

        for (std::size_t i {}; i < digits_len; ++i)
        {
            t_digits[i] = static_cast<charT>(digits_first[i]);
        }
    }
    else
    {
        std::memcpy(t_head, head, head_len);
        std::memcpy(t_digits, digits_first, digits_len);
    }

    // Unformatted output (write, put): the width was already consumed above, so using
    // the formatted inserters here would pad a second time
    const auto write_fill = [&os, fill_char](const std::size_t n)
    {
        for (std::size_t i {}; i < n; ++i)
        {
            os.put(fill_char);
        }
    };

    if (adjust == std::ios_base::left)
    {
        os.write(t_head, static_cast<std::streamsize>(head_len));
        os.write(t_digits, static_cast<std::streamsize>(digits_len));
        write_fill(pad);
    }
    else if (adjust == std::ios_base::internal)
    {
        os.write(t_head, static_cast<std::streamsize>(head_len));
        write_fill(pad);
        os.write(t_digits, static_cast<std::streamsize>(digits_len));
    }
    else
    {
        // right, or no adjustfield flag set: the builtin signed and unsigned integer
        // types both pad before the whole sign-and-prefix-and-digits sequence here
        write_fill(pad);
        os.write(t_head, static_cast<std::streamsize>(head_len));
        os.write(t_digits, static_cast<std::streamsize>(digits_len));
    }

    return os;
}

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_IOSTREAM_HPP
