// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Adapted from Boost.Int128 (Copyright 2022 Peter Dimov, Copyright 2023 Matt Borland)

#ifndef BOOST_INT256_DETAIL_MINI_TO_CHARS_HPP
#define BOOST_INT256_DETAIL_MINI_TO_CHARS_HPP

#include <boost/int256/int256.hpp>
#include <boost/int256/detail/common_div.hpp>
#include <boost/int256/detail/config.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstddef>
#include <cstdint>

#endif

namespace boost {
namespace int256 {
namespace detail {

// A 256-bit integer needs up to 256 binary digits (base 2); allow for a leading sign and a
// null terminator so mini_to_chars is safe for every supported base (2, 8, 10, 16).
// The macro gives this external linkage in a module build: a plain namespace-scope
// constexpr is TU-local, and naming a TU-local entity in an exported template body
// (to_string, the formatters) is an exposure the module build rejects.
BOOST_INT256_INLINE_CONSTEXPR std::size_t mini_to_chars_buffer_size {258};

#if !defined(BOOST_INT256_HAS_GPU_SUPPORT)

BOOST_INT256_INLINE_CONSTEXPR char lower_case_digit_table[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f'
};

static_assert(sizeof(lower_case_digit_table) == sizeof(char) * 16, "10 numbers, and 6 letters");

BOOST_INT256_INLINE_CONSTEXPR char upper_case_digit_table[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
};

static_assert(sizeof(upper_case_digit_table) == sizeof(char) * 16, "10 numbers, and 6 letters");

#endif // Namespace scope tables are unavailable on the device

// Writes the digits of v in the requested base into the end of buffer, NUL terminating at
// buffer end, and returns a pointer to the first character written.
BOOST_INT256_HOST_DEVICE constexpr char* mini_to_chars(char (&buffer)[mini_to_chars_buffer_size], uint256 v, const int base, const bool uppercase) noexcept
{
    #if defined(BOOST_INT256_HAS_GPU_SUPPORT)
    constexpr char lower_case_digit_table[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'
    };

    constexpr char upper_case_digit_table[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F'
    };
    #endif

    char* last {buffer + sizeof(buffer)};
    *--last = '\0';

    if (v == 0U)
    {
        *--last = '0';
        return last;
    }

    const auto digit_table {uppercase ? upper_case_digit_table : lower_case_digit_table};

    switch (base)
    {
        case 2:
            while (v != 0U)
            {
                *--last = (v.words[0] & UINT64_C(1)) != UINT64_C(0) ? '1' : '0';
                v >>= 1U;
            }
            break;

        case 8:
            while (v != 0U)
            {
                *--last = digit_table[static_cast<unsigned>(v.words[0] & UINT64_C(7))];
                v >>= 3U;
            }
            break;

        case 10:
        {
            // Peel nineteen decimal digits per wide division so the 256-bit division runs
            // at most four times. Every chunk but the last is zero padded.
            constexpr std::uint64_t decimal_chunk {UINT64_C(10000000000000000000)};
            constexpr int decimal_chunk_digits {19};

            while ((v.words[1] | v.words[2] | v.words[3]) != UINT64_C(0))
            {
                uint256 quotient {};
                std::uint64_t remainder {};
                one_word_div(v, decimal_chunk, quotient, remainder);
                v = quotient;

                for (int i {}; i < decimal_chunk_digits; ++i)
                {
                    *--last = digit_table[static_cast<unsigned>(remainder % UINT64_C(10))];
                    remainder /= UINT64_C(10);
                }
            }

            auto low_word {v.words[0]};
            while (low_word != UINT64_C(0))
            {
                *--last = digit_table[static_cast<unsigned>(low_word % UINT64_C(10))];
                low_word /= UINT64_C(10);
            }

            break;
        }

        case 16:
            while (v != 0U)
            {
                *--last = digit_table[static_cast<unsigned>(v.words[0] & UINT64_C(15))];
                v >>= 4U;
            }
            break;

        default:                        // LCOV_EXCL_LINE
            BOOST_INT256_UNREACHABLE;   // LCOV_EXCL_LINE
    }

    return last;
}

// Sign-magnitude: the magnitude is written by the uint256 overload above (abs() is
// already exact for min(), whose magnitude is 2^255), and a '-' is prepended for a
// negative value. The buffer is 258 (sign + 256 binary digits + NUL) so there is always
// room for the extra character
BOOST_INT256_HOST_DEVICE constexpr char* mini_to_chars(char (&buffer)[mini_to_chars_buffer_size], const int256& v, const int base, const bool uppercase) noexcept
{
    if (v.signed_high() < 0)
    {
        char* p {mini_to_chars(buffer, to_unsigned_bits(abs(v)), base, uppercase)};
        *--p = '-';
        return p;
    }

    return mini_to_chars(buffer, to_unsigned_bits(v), base, uppercase);
}

} // namespace detail
} // namespace int256
} // namespace boost

#endif // BOOST_INT256_DETAIL_MINI_TO_CHARS_HPP
