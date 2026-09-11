// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The stream inserter and extractor.
//
// The extractor here deliberately differs from Boost.Int128, which has three
// defects this implementation fixes and which are therefore pinned as tests:
//   * a lone "0" read in hex must consume the digit and yield 0, leaving nothing
//     behind, rather than treating the zero as a base prefix and failing;
//   * "0f" read in hex must yield 15, not 0 followed by an unread 'f', because a
//     bare leading zero is not a prefix in hex;
//   * "0x1fz" must yield 31 and leave only "z" unread, so the prefix length is
//     counted as consumed.

#include "oracle.hpp"

#ifdef BOOST_INT256_ORACLE_UNAVAILABLE

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

std::string insert(const uint256& v, std::ios_base::fmtflags flags)
{
    std::ostringstream os;
    os.flags(flags);
    os << v;

    return os.str();
}

//=====================================
// Output
//=====================================

// The digits must match what the reference renders for the same base and case.
void check_output(const uint256& v)
{
    BOOST_INT256_TEST_STRING("operator<< dec", v, insert(v, std::ios_base::dec), mp_str(to_mp(v), 10, false));
    BOOST_INT256_TEST_STRING("operator<< hex", v, insert(v, std::ios_base::hex), mp_str(to_mp(v), 16, false));
    BOOST_INT256_TEST_STRING("operator<< oct", v, insert(v, std::ios_base::oct), mp_str(to_mp(v), 8, false));

    BOOST_INT256_TEST_STRING("operator<< hex uppercase", v,
                             insert(v, std::ios_base::hex | std::ios_base::uppercase),
                             mp_str(to_mp(v), 16, true));
}

void test_output_bases()
{
    for (const auto& v : edge_values())
    {
        check_output(v);
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        check_output(random_uint256(rng, shape::word4));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_output_known_answers()
{
    const uint256 max {uint256_max()};

    BOOST_TEST_EQ(insert(uint256{}, std::ios_base::dec), std::string{"0"});
    BOOST_TEST_EQ(insert(uint256{}, std::ios_base::hex), std::string{"0"});
    BOOST_TEST_EQ(insert(uint256{}, std::ios_base::oct), std::string{"0"});

    BOOST_TEST_EQ(insert(max, std::ios_base::dec),
                  std::string{"115792089237316195423570985008687907853269984665640564039457584007913129639935"});
    BOOST_TEST_EQ(insert(max, std::ios_base::dec).size(), static_cast<std::size_t>(78));
    BOOST_TEST_EQ(insert(max, std::ios_base::hex),
                  std::string{"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"});
    BOOST_TEST_EQ(insert(max, std::ios_base::hex).size(), static_cast<std::size_t>(64));
    BOOST_TEST_EQ(insert(max, std::ios_base::hex | std::ios_base::uppercase),
                  std::string{"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"});
    BOOST_TEST_EQ(insert(max, std::ios_base::oct).size(), static_cast<std::size_t>(86));

    // 2^64, 2^128 and 2^192 in decimal.
    const uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    BOOST_TEST_EQ(insert(one << 64U, std::ios_base::dec), std::string{"18446744073709551616"});
    BOOST_TEST_EQ(insert(one << 128U, std::ios_base::dec),
                  std::string{"340282366920938463463374607431768211456"});
    BOOST_TEST_EQ(insert(one << 192U, std::ios_base::dec),
                  std::string{"6277101735386680763835789423207666416102355444464034512896"});
}

void test_showbase()
{
    const uint256 v {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(255)};

    BOOST_TEST_EQ(insert(v, std::ios_base::hex | std::ios_base::showbase), std::string{"0xff"});
    BOOST_TEST_EQ(insert(v, std::ios_base::hex | std::ios_base::showbase | std::ios_base::uppercase),
                  std::string{"0XFF"});
    BOOST_TEST_EQ(insert(v, std::ios_base::oct | std::ios_base::showbase), std::string{"0377"});

    // Decimal has no prefix.
    BOOST_TEST_EQ(insert(v, std::ios_base::dec | std::ios_base::showbase), std::string{"255"});

    // A zero prints as a bare "0" under showbase, with no prefix, which is what the
    // built-in integers do: the # flag adds 0x only to a nonzero value, and for octal
    // it forces a leading zero that a lone "0" already has. Rather than restate that
    // rule, the expectation is taken from a builtin of the same value, so this stays
    // correct if the standard library ever disagrees with the reading above.
    for (const auto base : {std::ios_base::hex, std::ios_base::oct, std::ios_base::dec})
    {
        const auto flags {base | std::ios_base::showbase};

        std::ostringstream builtin_stream;
        builtin_stream.flags(flags);
        builtin_stream << 0U;

        BOOST_TEST_EQ(insert(uint256{}, flags), builtin_stream.str());
    }

    // And a nonzero value does get the prefix, again matching the builtin.
    for (const auto base : {std::ios_base::hex, std::ios_base::oct, std::ios_base::dec})
    {
        const auto flags {base | std::ios_base::showbase};

        std::ostringstream builtin_stream;
        builtin_stream.flags(flags);
        builtin_stream << 255U;

        BOOST_TEST_EQ(insert(uint256{255U}, flags), builtin_stream.str());
    }
}

// The width, fill and adjust flags are applied by the stream itself, so they must
// survive the inserter writing through a char pointer.
void test_width_and_fill()
{
    const uint256 v {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(42)};

    {
        std::ostringstream os;
        os << std::setw(10) << v;
        BOOST_TEST_EQ(os.str(), std::string{"        42"});
    }
    {
        std::ostringstream os;
        os << std::setw(10) << std::setfill('0') << v;
        BOOST_TEST_EQ(os.str(), std::string{"0000000042"});
    }
    {
        std::ostringstream os;
        os << std::setw(10) << std::left << v;
        BOOST_TEST_EQ(os.str(), std::string{"42        "});
    }
    {
        std::ostringstream os;
        os << std::setw(10) << std::right << std::setfill('*') << v;
        BOOST_TEST_EQ(os.str(), std::string{"********42"});
    }
    {
        // A width narrower than the value does not truncate.
        std::ostringstream os;
        os << std::setw(1) << v;
        BOOST_TEST_EQ(os.str(), std::string{"42"});
    }
    {
        // The full 78 digit maximum inside a wider field.
        std::ostringstream os;
        os << std::setw(80) << uint256_max();
        BOOST_TEST_EQ(os.str().size(), static_cast<std::size_t>(80));
        BOOST_TEST_EQ(os.str().substr(0, 2), std::string{"  "});
    }
}

// A wide stream takes the same path through a widening loop.
void test_wide_stream()
{
    std::wostringstream os;
    os << uint256{42U};
    BOOST_TEST(os.str() == std::wstring{L"42"});

    std::wostringstream hex;
    hex << std::hex << uint256{255U};
    BOOST_TEST(hex.str() == std::wstring{L"ff"});

    std::wostringstream wide;
    wide << uint256_max();
    BOOST_TEST_EQ(wide.str().size(), static_cast<std::size_t>(78));
}

//=====================================
// Input
//=====================================

struct extract_result
{
    uint256 value;
    bool failed;
    std::string remaining;
};

extract_result extract(const std::string& text, std::ios_base::fmtflags flags)
{
    std::istringstream is {text};
    is.flags(flags | (is.flags() & ~std::ios_base::basefield));
    is.unsetf(std::ios_base::basefield);
    is.setf(flags & std::ios_base::basefield, std::ios_base::basefield);

    uint256 value {};
    is >> value;

    extract_result result {};
    result.value = value;
    result.failed = is.fail();

    // The state has to be read before this loop: reading to the end always ends by
    // setting failbit, so is.fail() afterwards says nothing about the extraction.
    is.clear(is.rdstate() & ~std::ios_base::failbit);

    std::string rest;
    char c {};
    while (is.get(c))
    {
        rest.push_back(c);
    }
    result.remaining = rest;

    return result;
}

void test_input_bases()
{
    {
        const auto r {extract("12345", std::ios_base::dec)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{12345U});
        BOOST_TEST_EQ(r.remaining, std::string{});
    }
    {
        const auto r {extract("ff", std::ios_base::hex)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{255U});
    }
    {
        const auto r {extract("777", std::ios_base::oct)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{511U});
    }
    {
        // The 0x prefix is accepted in hex.
        const auto r {extract("0x1f", std::ios_base::hex)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{31U});
        BOOST_TEST_EQ(r.remaining, std::string{});
    }
    {
        const auto r {extract("0X1F", std::ios_base::hex)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{31U});
    }
    {
        // Leading whitespace is skipped.
        const auto r {extract("   42", std::ios_base::dec)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{42U});
    }
}

// The three corrected behaviors. Each of these is wrong in Boost.Int128.
void test_corrected_hex_prefix_handling()
{
    {
        // A lone zero in hex is the digit zero, fully consumed.
        const auto r {extract("0", std::ios_base::hex)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{});
        BOOST_TEST_EQ(r.remaining, std::string{});
    }
    {
        // A bare leading zero is not a prefix in hex, so 0f is 15.
        const auto r {extract("0f", std::ios_base::hex)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{15U});
        BOOST_TEST_EQ(r.remaining, std::string{});
    }
    {
        // The prefix counts as consumed, so only the trailing z is left.
        const auto r {extract("0x1fz", std::ios_base::hex)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{31U});
        BOOST_TEST_EQ(r.remaining, std::string{"z"});
    }
    {
        // The same in the other cases.
        const auto r {extract("0X1Fz", std::ios_base::hex)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{31U});
        BOOST_TEST_EQ(r.remaining, std::string{"z"});
    }
    {
        // A leading zero followed by more zeros is still zero.
        const auto r {extract("000", std::ios_base::hex)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{});
    }
    {
        // And in octal a lone zero is zero, while a leading zero before digits is
        // the octal prefix.
        const auto r {extract("0", std::ios_base::oct)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{});
        BOOST_TEST_EQ(r.remaining, std::string{});
    }
    {
        const auto r {extract("017", std::ios_base::oct)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{15U});
    }
}

void test_input_leaves_trailing_characters()
{
    {
        const auto r {extract("42abc", std::ios_base::dec)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{42U});
        BOOST_TEST_EQ(r.remaining, std::string{"abc"});
    }
    {
        // A hex digit is not a decimal digit.
        const auto r {extract("42f", std::ios_base::dec)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{42U});
        BOOST_TEST_EQ(r.remaining, std::string{"f"});
    }
    {
        // 8 and 9 are not octal digits.
        const auto r {extract("178", std::ios_base::oct)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256{15U});
        BOOST_TEST_EQ(r.remaining, std::string{"8"});
    }
}

// An extraction that consumes no digits must set failbit and zero the value, which
// is what operator>> does for the built-in arithmetic types.
void test_input_failure()
{
    // Nothing that could be a digit at all. The characters must all still be there
    // to read afterwards, whatever the stream state says.
    {
        const auto r {extract("abc", std::ios_base::dec)};
        BOOST_TEST(r.failed);
        BOOST_TEST_EQ(r.remaining, std::string{"abc"});
    }
    {
        // The empty input fails through the character array extraction itself, so
        // this one holds today.
        const auto r {extract("", std::ios_base::dec)};
        BOOST_TEST(r.failed);
    }
    {
        const auto r {extract("zz", std::ios_base::hex)};
        BOOST_TEST(r.failed);
        BOOST_TEST_EQ(r.remaining, std::string{"zz"});
    }
    {
        // A sign is not part of an unsigned value.
        const auto r {extract("-1", std::ios_base::dec)};
        BOOST_TEST(r.failed);
        BOOST_TEST_EQ(r.remaining, std::string{"-1"});
    }
}

void test_input_overflow()
{
    // The maximum reads back exactly.
    {
        const auto r {extract("115792089237316195423570985008687907853269984665640564039457584007913129639935",
                              std::ios_base::dec)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256_max());
    }

    // One past the maximum does not. The digits are all left unread.
    {
        const auto text {std::string{"115792089237316195423570985008687907853269984665640564039457584007913129639936"}};
        const auto r {extract(text, std::ios_base::dec)};
        BOOST_TEST(r.failed);
        BOOST_TEST_EQ(r.remaining, text);
    }

    // And the same in hex.
    {
        const auto r {extract("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
                              std::ios_base::hex)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, uint256_max());
    }
    {
        const auto text {std::string{"10000000000000000000000000000000000000000000000000000000000000000"}};
        const auto r {extract(text, std::ios_base::hex)};
        BOOST_TEST(r.failed);
        BOOST_TEST_EQ(r.remaining, text);
    }
}

//=====================================
// Round trips
//=====================================

// Writing then reading a value must return it, in every base.
void test_round_trips()
{
    const std::ios_base::fmtflags bases[] {std::ios_base::dec, std::ios_base::hex, std::ios_base::oct};

    for (const auto& v : edge_values())
    {
        for (const auto base : bases)
        {
            const auto text {insert(v, base)};
            const auto r {extract(text, base)};

            BOOST_TEST(!r.failed);
            BOOST_TEST(r.value == v || report_string_mismatch("round trip", v, text, mp_str(to_mp(v), 10, false)));
        }

        // And through showbase, which the extractor has to skip.
        const auto with_prefix {insert(v, std::ios_base::hex | std::ios_base::showbase)};
        const auto r {extract(with_prefix, std::ios_base::hex)};
        BOOST_TEST(!r.failed);
        BOOST_TEST_EQ(r.value, v);
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto v {random_uint256(rng, shape::word4)};

        for (const auto base : bases)
        {
            const auto text {insert(v, base)};
            const auto r {extract(text, base)};

            BOOST_TEST(!r.failed);
            BOOST_TEST_EQ(r.value, v);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// Several values in a row from one stream, which only works if the extractor
// leaves the stream positioned correctly.
// The loop that a stream extractor exists to support. Before failbit was set, this
// spun forever: the extraction reported success, consumed nothing, and put every
// character back, so the condition never became false.
void test_failed_extraction_terminates()
{
    std::istringstream is {"abc def"};

    uint256 v {UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)};
    int n {0};
    while (is >> v && n < 1000)
    {
        ++n;
    }

    BOOST_TEST_EQ(n, 0);
    BOOST_TEST(is.fail());

    // And the value was zeroed rather than left at whatever it held.
    BOOST_TEST_EQ(v, uint256{});
}

// Every way an extraction can consume nothing sets failbit and zeroes the value.
void test_failbit_and_zeroing()
{
    struct rejected_case
    {
        const char* text;
        std::ios_base::fmtflags flags;
        const char* what;
    };

    const rejected_case cases[] {
        {"-5", std::ios_base::dec, "a negative sign"},
        {"+5", std::ios_base::dec, "a positive sign"},
        {"abc", std::ios_base::dec, "no digits at all"},
        {"", std::ios_base::dec, "an empty stream"},
        {"115792089237316195423570985008687907853269984665640564039457584007913129639936",
         std::ios_base::dec, "one past the maximum in decimal"},
        {"10000000000000000000000000000000000000000000000000000000000000000",
         std::ios_base::hex, "one past the maximum in hex"},
        {"zz", std::ios_base::hex, "no hex digits"},
        {"9", std::ios_base::oct, "not an octal digit"}
    };

    for (const auto& c : cases)
    {
        std::istringstream is {c.text};
        is.unsetf(std::ios_base::basefield);
        is.setf(c.flags, std::ios_base::basefield);

        uint256 v {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
        is >> v;

        if (!is.fail() || v != uint256{})
        {
            std::cerr << "extraction of \"" << c.text << "\" (" << c.what
                      << ") should have failed and zeroed: fail=" << is.fail()
                      << " value=" << hex_words(v) << std::endl;
        }

        BOOST_TEST(is.fail());
        BOOST_TEST_EQ(v, uint256{});
    }
}

void test_successive_reads()
{
    std::istringstream is {"1 22 333 4444"};

    uint256 a {};
    uint256 b {};
    uint256 c {};
    uint256 d {};
    is >> a >> b >> c >> d;

    BOOST_TEST(!is.fail());
    BOOST_TEST_EQ(a, uint256{1U});
    BOOST_TEST_EQ(b, uint256{22U});
    BOOST_TEST_EQ(c, uint256{333U});
    BOOST_TEST_EQ(d, uint256{4444U});
}

} // namespace

int main()
{
    test_output_known_answers();
    test_showbase();
    test_width_and_fill();
    test_wide_stream();
    test_input_bases();
    test_corrected_hex_prefix_handling();
    test_input_leaves_trailing_characters();
    test_input_failure();
    test_input_overflow();
    test_failed_extraction_terminates();
    test_failbit_and_zeroing();
    test_successive_reads();
    test_output_bases();
    test_round_trips();

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE
