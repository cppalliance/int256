// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// detail::from_chars over every base from 2 to 36. The return value is errno
// style: a positive value is an error code and nothing was consumed, while a
// non-positive value is the negated number of characters consumed. EDOM means the
// digits were valid but the value did not fit, which is the case that has to be
// distinguished from a malformed input.

#ifndef BOOST_INT256_BUILD_MODULE

#include <boost/int256.hpp>

#else

import boost.int256;
// A module import carries no macros, so the BOOST_INT256_* feature and endianness
// macros have to come from the header even in the module build.
#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/literal_macros.hpp>

#endif

#include "oracle.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <string>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

int parse(const std::string& text, uint256& value, int base)
{
    return boost::int256::detail::from_chars(text.c_str(), text.c_str() + text.size(), value, base);
}

// Renders a value in the requested base without using the library, so the round
// trip below is against an independent encoder.
std::string render(uint256 v, int base, bool uppercase)
{
    if (v == 0U)
    {
        return std::string{"0"};
    }

    const char* const digits {uppercase ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        : "0123456789abcdefghijklmnopqrstuvwxyz"};

    const uint256 radix {static_cast<std::uint64_t>(base)};

    std::string reversed;
    while (v != 0U)
    {
        const auto digit {static_cast<unsigned>(static_cast<std::uint64_t>(v % radix))};
        reversed.push_back(digits[digit]);
        v = v / radix;
    }

    std::string result;
    for (std::size_t i {reversed.size()}; i > 0U; --i)
    {
        result.push_back(reversed[i - 1U]);
    }

    return result;
}

//=====================================
// Round trips in every base
//=====================================

void check_round_trip(const uint256& v, int base)
{
    for (int pass {0}; pass < 2; ++pass)
    {
        const bool uppercase {pass == 1};
        const auto text {render(v, base, uppercase)};

        uint256 parsed {};
        const auto status {parse(text, parsed, base)};

        if (parsed != v || status != -static_cast<int>(text.size()))
        {
            std::cerr << "from_chars round trip failed in base " << base
                      << (uppercase ? " uppercase" : " lowercase") << '\n'
                      << "  text     = \"" << text << "\"\n"
                      << "  status   = " << status << " (expected " << -static_cast<int>(text.size()) << ")\n"
                      << "  got      = " << hex_words(parsed) << '\n'
                      << "  expected = " << hex_words(v) << std::endl;
        }

        BOOST_TEST_EQ(parsed, v);
        BOOST_TEST_EQ(status, -static_cast<int>(text.size()));
    }
}

void test_round_trips_every_base()
{
    for (int base {2}; base <= 36; ++base)
    {
        for (const auto& v : edge_values())
        {
            check_round_trip(v, base);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_random_round_trips()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto v {random_uint256(rng, shape::word4)};
        const auto base {static_cast<int>(2U + (i % 35U))};

        check_round_trip(v, base);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

//=====================================
// The maximum, and one past it, in every base
//=====================================

// The overflow check runs per digit, so the boundary has to be exercised in each
// base rather than only in base 10.
void test_overflow_in_every_base()
{
    const uint256 max {uint256_max()};

    for (int base {2}; base <= 36; ++base)
    {
        // The maximum parses exactly and consumes everything.
        {
            const auto text {render(max, base, false)};
            uint256 parsed {};
            const auto status {parse(text, parsed, base)};

            BOOST_TEST_EQ(parsed, max);
            BOOST_TEST_EQ(status, -static_cast<int>(text.size()));
        }

        // One past the maximum is EDOM: the digits were valid, the value was not.
        {
            // max + 1 in this base is a one followed by zeros only when the base is
            // a power of two, so it is built by hand instead: append a zero to the
            // maximum's own rendering, which is base * max and so far above it.
            auto text {render(max, base, false)};
            text.push_back('0');

            uint256 parsed {};
            const auto status {parse(text, parsed, base)};

            BOOST_TEST_EQ(status, EDOM);
        }
    }

    // The decimal and hexadecimal boundaries spelled out.
    {
        uint256 parsed {};
        BOOST_TEST_EQ(parse("115792089237316195423570985008687907853269984665640564039457584007913129639935",
                            parsed, 10), -78);
        BOOST_TEST_EQ(parsed, max);
    }
    {
        uint256 parsed {};
        BOOST_TEST_EQ(parse("115792089237316195423570985008687907853269984665640564039457584007913129639936",
                            parsed, 10), EDOM);
    }
    {
        uint256 parsed {};
        BOOST_TEST_EQ(parse("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", parsed, 16), -64);
        BOOST_TEST_EQ(parsed, max);
    }
    {
        uint256 parsed {};
        BOOST_TEST_EQ(parse("10000000000000000000000000000000000000000000000000000000000000000", parsed, 16), EDOM);
    }
    {
        // The maximum in base 36 is 50 characters long.
        uint256 parsed {};
        const std::string text {"6dp5qcb22im238nr3wvp0ic7q99w035jmy2iw7i6n43d37jtof"};
        BOOST_TEST_EQ(text.size(), static_cast<std::size_t>(50));
        BOOST_TEST_EQ(parse(text, parsed, 36), -50);
        BOOST_TEST_EQ(parsed, max);
    }
    {
        // And in base 2 it is 256 ones.
        const std::string text(256, '1');
        uint256 parsed {};
        BOOST_TEST_EQ(parse(text, parsed, 2), -256);
        BOOST_TEST_EQ(parsed, max);
    }
}

//=====================================
// Malformed inputs
//=====================================

// The full return contract, which is finer than "errno on failure":
//   r <  0  success, and -r characters were consumed
//   r == 0  no characters were consumed because the first one is not a digit in
//           the base; the value is set to zero
//   r >  0  an errno value: EINVAL for an empty input or a leading sign, EDOM for
//           digits that are valid but do not fit
// A caller therefore tests r < 0 for success, not r != EINVAL.
void test_malformed()
{
    uint256 parsed {};

    // Empty, and a sign, are the only EINVAL cases.
    BOOST_TEST_EQ(boost::int256::detail::from_chars(nullptr, nullptr, parsed, 10), EINVAL);
    BOOST_TEST_EQ(parse("", parsed, 10), EINVAL);
    BOOST_TEST_EQ(parse("+1", parsed, 10), EINVAL);
    BOOST_TEST_EQ(parse("-1", parsed, 10), EINVAL);
    BOOST_TEST_EQ(parse("+", parsed, 10), EINVAL);
    BOOST_TEST_EQ(parse("-", parsed, 10), EINVAL);

    // A leading character that is not a digit in the base consumes nothing, which
    // is reported as zero rather than as an errno value.
    BOOST_TEST_EQ(parse("z", parsed, 10), 0);
    BOOST_TEST_EQ(parsed, uint256{});
    BOOST_TEST_EQ(parse("g", parsed, 16), 0);
    BOOST_TEST_EQ(parse("2", parsed, 2), 0);
    BOOST_TEST_EQ(parse("8", parsed, 8), 0);
    BOOST_TEST_EQ(parse(" 1", parsed, 10), 0);

    // Whichever of the two it is, nothing was consumed, so a caller looking only
    // at the sign of the result reaches the right conclusion.
    BOOST_TEST(parse("", parsed, 10) >= 0);
    BOOST_TEST(parse("z", parsed, 10) >= 0);
    BOOST_TEST(parse("1", parsed, 10) < 0);
}

// On a failure that consumes nothing, the output is left exactly as the caller had
// it. A routine that zeroed it on the way in would destroy a value the caller was
// still using, and a partially written one would be worse.
void test_failure_leaves_the_value_untouched()
{
    const uint256 sentinel {UINT64_C(0x0123456789ABCDEF), UINT64_C(0xFEDCBA9876543210),
                            UINT64_C(0x1122334455667788), UINT64_C(0x99AABBCCDDEEFF00)};

    struct failing_case
    {
        const char* text;
        int base;
        const char* what;
    };

    const failing_case cases[] {
        {"", 10, "empty"},
        {"+1", 10, "a leading plus"},
        {"-1", 10, "a leading minus"},
        {"zzz", 10, "no digits in the base"},
        {"g", 16, "not a hex digit"},
        {"2", 2, "not a binary digit"},
        {"115792089237316195423570985008687907853269984665640564039457584007913129639936", 10,
         "one past the maximum"}
    };

    for (const auto& c : cases)
    {
        uint256 value {sentinel};
        const auto status {parse(c.text, value, c.base)};

        BOOST_TEST(status >= 0);

        if (value != sentinel)
        {
            std::cerr << "from_chars(\"" << c.text << "\", base " << c.base << ") ("
                      << c.what << ") modified the output on failure: "
                      << hex_words(value) << std::endl;
        }

        BOOST_TEST_EQ(value, sentinel);
    }
}

// A base outside 2 to 36 is not a parse failure, it is a misuse, and it is
// reported as EINVAL without touching the output.
void test_invalid_bases()
{
    const uint256 sentinel {UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)};

    for (const int base : {0, 1, 37, 100, -1})
    {
        uint256 value {sentinel};
        const auto status {parse("101", value, base)};

        if (status != EINVAL)
        {
            std::cerr << "from_chars with base " << base << " returned " << status
                      << ", expected EINVAL" << std::endl;
        }

        BOOST_TEST_EQ(status, EINVAL);
        BOOST_TEST_EQ(value, sentinel);
    }
}

// Parsing stops at the first character that is not a digit in the base, and the
// consumed count says where.
void test_partial_consumption()
{
    {
        uint256 parsed {};
        BOOST_TEST_EQ(parse("123abc", parsed, 10), -3);
        BOOST_TEST_EQ(parsed, uint256{123U});
    }
    {
        uint256 parsed {};
        BOOST_TEST_EQ(parse("101201", parsed, 2), -3);
        BOOST_TEST_EQ(parsed, uint256{5U});
    }
    {
        uint256 parsed {};
        BOOST_TEST_EQ(parse("7778", parsed, 8), -3);
        BOOST_TEST_EQ(parsed, uint256{511U});
    }
    {
        uint256 parsed {};
        BOOST_TEST_EQ(parse("ffg", parsed, 16), -2);
        BOOST_TEST_EQ(parsed, uint256{255U});
    }
    {
        // A base 16 parse stops at the x of an 0x prefix, which is why the stream
        // extractor has to strip the prefix itself.
        uint256 parsed {};
        BOOST_TEST_EQ(parse("0x10", parsed, 16), -1);
        BOOST_TEST_EQ(parsed, uint256{});
    }
}

// The digit alphabet: every base accepts exactly its own digits, in both cases
// above base 10.
void test_digit_alphabet()
{
    for (int base {2}; base <= 36; ++base)
    {
        for (int digit {0}; digit < 36; ++digit)
        {
            const char lower {digit < 10 ? static_cast<char>('0' + digit)
                                         : static_cast<char>('a' + digit - 10)};
            const char upper {digit < 10 ? static_cast<char>('0' + digit)
                                         : static_cast<char>('A' + digit - 10)};

            const std::string lower_text(1, lower);
            const std::string upper_text(1, upper);

            uint256 parsed {};
            if (digit < base)
            {
                BOOST_TEST_EQ(parse(lower_text, parsed, base), -1);
                BOOST_TEST_EQ(parsed, uint256{static_cast<std::uint64_t>(digit)});

                uint256 upper_parsed {};
                BOOST_TEST_EQ(parse(upper_text, upper_parsed, base), -1);
                BOOST_TEST_EQ(upper_parsed, uint256{static_cast<std::uint64_t>(digit)});
            }
            else
            {
                // Not a digit in this base, so nothing is consumed.
                BOOST_TEST_EQ(parse(lower_text, parsed, base), 0);
                BOOST_TEST_EQ(parse(upper_text, parsed, base), 0);
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  base = " << base << std::endl;
            return;
        }
    }
}

// Leading zeros are digits, not a prefix, so they are consumed and do not change
// the value.
void test_leading_zeros()
{
    for (int base {2}; base <= 36; ++base)
    {
        uint256 parsed {};
        BOOST_TEST_EQ(parse("0", parsed, base), -1);
        BOOST_TEST_EQ(parsed, uint256{});

        uint256 padded {};
        BOOST_TEST_EQ(parse("0001", padded, base), -4);
        BOOST_TEST_EQ(padded, uint256{1U});

        uint256 many {};
        const std::string text {std::string(64, '0') + "1"};
        BOOST_TEST_EQ(many = uint256{}, uint256{});
        BOOST_TEST_EQ(parse(text, many, base), -65);
        BOOST_TEST_EQ(many, uint256{1U});
    }
}

// The parse result, in a form usable in a constant expression. A constexpr lambda
// would need C++17, and the language floor here is C++14.
struct parsed_pair
{
    int status;
    uint256 value;
};

constexpr parsed_pair run_parse(const char* first, const char* last, int base) noexcept
{
    uint256 value {};
    const auto status {boost::int256::detail::from_chars(first, last, value, base)};

    return parsed_pair{status, value};
}

// The parse runs in constant evaluation, which is what the user-defined literals
// depend on.
void test_constexpr()
{
    constexpr char decimal[] {"340282366920938463463374607431768211456"};
    constexpr char hexadecimal[] {"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"};

    constexpr auto decimal_result {run_parse(decimal, decimal + sizeof(decimal) - 1U, 10)};
    static_assert(decimal_result.status == -39, "2^128 has 39 decimal digits");
    static_assert(decimal_result.value == uint256(UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)),
                  "2^128 parses to words[2] == 1");

    constexpr auto hex_result {run_parse(hexadecimal, hexadecimal + sizeof(hexadecimal) - 1U, 16)};
    static_assert(hex_result.status == -64, "the maximum has 64 hex digits");
    static_assert(hex_result.value == uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX),
                  "the maximum parses to all ones");
}

} // namespace

int main()
{
    test_constexpr();
    test_malformed();
    test_failure_leaves_the_value_untouched();
    test_invalid_bases();
    test_partial_consumption();
    test_leading_zeros();
    test_digit_alphabet();
    test_overflow_in_every_base();
    test_round_trips_every_base();
    test_random_round_trips();

    return boost::report_errors();
}
