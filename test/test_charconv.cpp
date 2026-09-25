// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The opt-in Boost.Charconv integration. These are std::to_chars and
// std::from_chars semantics rather than the errno style of the internal
// detail::from_chars: the result carries an error code, and to_chars reports
// value_too_large rather than truncating.

#include "oracle.hpp"

// The integration is opt-in and needs Boost.Charconv to be present.
#if __has_include(<boost/charconv.hpp>)
#  include <boost/int256/charconv.hpp>
#  define BOOST_INT256_TEST_HAS_CHARCONV
#endif

#if defined(BOOST_INT256_ORACLE_UNAVAILABLE) || !defined(BOOST_INT256_TEST_HAS_CHARCONV)

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <boost/core/detail/string_view.hpp>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

using boost::int256::uint256;
using boost::int256::int256;

namespace
{

std::mt19937_64 rng {42};

// The maximum needs 256 characters in base 2, plus room to detect an overrun.
constexpr std::size_t buffer_size {300};

std::string to_string_in_base(const uint256& v, int base, bool* ok)
{
    char buffer[buffer_size] {};
    const auto r {boost::charconv::to_chars(buffer, buffer + sizeof(buffer), v, base)};

    if (ok != nullptr)
    {
        *ok = static_cast<bool>(r);
    }

    if (!r)
    {
        return std::string{};
    }

    return std::string{buffer, r.ptr};
}

//=====================================
// to_chars
//=====================================

void check_to_chars(const uint256& v, int base)
{
    bool ok {false};
    const auto text {to_string_in_base(v, base, &ok)};

    BOOST_TEST(ok);
    BOOST_INT256_TEST_STRING("charconv to_chars", v, text, mp_str(to_mp(v), base, false));
}

void test_to_chars_every_base()
{
    for (int base {2}; base <= 36; ++base)
    {
        for (const auto& v : edge_values())
        {
            check_to_chars(v, base);
        }

        // Zero is "0" in every base.
        bool ok {false};
        BOOST_TEST_EQ(to_string_in_base(uint256{}, base, &ok), std::string{"0"});
        BOOST_TEST(ok);

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  base = " << base << std::endl;
            return;
        }
    }
}

void test_to_chars_lengths()
{
    const uint256 max {uint256_max()};

    // The lengths of the maximum, recomputed against multiprecision.
    const int bases[] {2, 3, 7, 8, 10, 16, 32, 36};
    const std::size_t lengths[] {256U, 162U, 92U, 86U, 78U, 64U, 52U, 50U};

    for (std::size_t i {0}; i < 8U; ++i)
    {
        bool ok {false};
        const auto text {to_string_in_base(max, bases[i], &ok)};
        BOOST_TEST(ok);
        BOOST_TEST_EQ(text.size(), lengths[i]);
    }

    // The base 36 rendering of the maximum, spelled out.
    bool ok {false};
    BOOST_TEST_EQ(to_string_in_base(max, 36, &ok),
                  std::string{"6dp5qcb22im238nr3wvp0ic7q99w035jmy2iw7i6n43d37jtof"});
}

// A buffer one character short must report value_too_large and leave the buffer
// alone rather than writing a truncated value.
void test_to_chars_value_too_large()
{
    const uint256 max {uint256_max()};

    {
        // 64 hex digits fit exactly; 63 do not.
        char exact[64] {};
        const auto fits {boost::charconv::to_chars(exact, exact + sizeof(exact), max, 16)};
        BOOST_TEST(fits);
        BOOST_TEST_EQ(fits.ptr, exact + 64);

        char tight[63] {};
        const auto overflows {boost::charconv::to_chars(tight, tight + sizeof(tight), max, 16)};
        BOOST_TEST(!overflows);
        BOOST_TEST(overflows.ec == std::errc::value_too_large);
    }
    {
        // The same in base 10.
        char exact[78] {};
        BOOST_TEST(boost::charconv::to_chars(exact, exact + sizeof(exact), max, 10));

        char tight[77] {};
        const auto overflows {boost::charconv::to_chars(tight, tight + sizeof(tight), max, 10)};
        BOOST_TEST(!overflows);
        BOOST_TEST(overflows.ec == std::errc::value_too_large);
    }
    {
        // A zero length buffer never fits, not even for zero. The generic
        // Boost.Charconv base path guards with (first <= last) and then writes '0'
        // for a zero value, so without an explicit check here this overran the
        // buffer by one byte.
        char nothing[1] {};
        const auto overflows {boost::charconv::to_chars(nothing, nothing, uint256{}, 10)};
        BOOST_TEST(!overflows);

        // In every base, and for a nonzero value too.
        for (int base {2}; base <= 36; ++base)
        {
            BOOST_TEST(!boost::charconv::to_chars(nothing, nothing, uint256{}, base));
            BOOST_TEST(!boost::charconv::to_chars(nothing, nothing, uint256_max(), base));
        }
    }
}

//=====================================
// from_chars
//=====================================

void test_from_chars_round_trips()
{
    for (int base {2}; base <= 36; ++base)
    {
        for (const auto& v : edge_values())
        {
            bool ok {false};
            const auto text {to_string_in_base(v, base, &ok)};
            BOOST_TEST(ok);

            uint256 parsed {};
            const auto r {boost::charconv::from_chars(text.data(), text.data() + text.size(), parsed, base)};

            BOOST_TEST(r);
            BOOST_TEST(r.ec == std::errc{});
            BOOST_TEST_EQ(r.ptr, text.data() + text.size());
            BOOST_TEST_EQ(parsed, v);
        }

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  base = " << base << std::endl;
            return;
        }
    }
}

void test_from_chars_maximum_and_overflow()
{
    const uint256 max {uint256_max()};

    for (int base {2}; base <= 36; ++base)
    {
        bool ok {false};
        const auto text {to_string_in_base(max, base, &ok)};
        BOOST_TEST(ok);

        // The maximum parses and consumes the whole string.
        {
            uint256 parsed {};
            const auto r {boost::charconv::from_chars(text.data(), text.data() + text.size(), parsed, base)};
            BOOST_TEST(r);
            BOOST_TEST_EQ(r.ptr, text.data() + text.size());
            BOOST_TEST_EQ(parsed, max);
        }

        // Appending a zero multiplies by the base, which no longer fits.
        {
            auto too_big {text};
            too_big.push_back('0');

            uint256 parsed {};
            const auto r {boost::charconv::from_chars(too_big.data(), too_big.data() + too_big.size(), parsed, base)};
            BOOST_TEST(!r);
            BOOST_TEST(r.ec == std::errc::result_out_of_range);
        }
    }

    // The decimal boundary spelled out.
    {
        const std::string text {"115792089237316195423570985008687907853269984665640564039457584007913129639935"};
        uint256 parsed {};
        const auto r {boost::charconv::from_chars(text.data(), text.data() + text.size(), parsed)};
        BOOST_TEST(r);
        BOOST_TEST_EQ(parsed, max);
    }
    {
        const std::string text {"115792089237316195423570985008687907853269984665640564039457584007913129639936"};
        uint256 parsed {};
        const auto r {boost::charconv::from_chars(text.data(), text.data() + text.size(), parsed)};
        BOOST_TEST(!r);
        BOOST_TEST(r.ec == std::errc::result_out_of_range);
    }
}

void test_from_chars_invalid()
{
    // A sign, in either direction, is not part of an unsigned value.
    for (const char* text : {"+1", "-1", "+", "-", ""})
    {
        uint256 parsed {};
        const std::string s {text};
        const auto r {boost::charconv::from_chars(s.data(), s.data() + s.size(), parsed)};
        BOOST_TEST(!r);
        BOOST_TEST(r.ec == std::errc::invalid_argument);
    }

    // A leading character that is not a digit in the base.
    {
        const std::string s {"zzz"};
        uint256 parsed {};
        const auto r {boost::charconv::from_chars(s.data(), s.data() + s.size(), parsed)};
        BOOST_TEST(!r);
        BOOST_TEST(r.ec == std::errc::invalid_argument);
    }

    // A 0x prefix is not consumed in base 16: the parse stops at the x, having
    // read the leading zero.
    {
        const std::string s {"0x10"};
        uint256 parsed {};
        const auto r {boost::charconv::from_chars(s.data(), s.data() + s.size(), parsed, 16)};
        BOOST_TEST(r);
        BOOST_TEST_EQ(parsed, uint256{});
        BOOST_TEST_EQ(r.ptr, s.data() + 1);
    }
}

void test_from_chars_partial()
{
    {
        const std::string s {"123abc"};
        uint256 parsed {};
        const auto r {boost::charconv::from_chars(s.data(), s.data() + s.size(), parsed)};
        BOOST_TEST(r);
        BOOST_TEST_EQ(parsed, uint256{123U});
        BOOST_TEST_EQ(r.ptr, s.data() + 3);
    }
    {
        const std::string s {"1012 rest"};
        uint256 parsed {};
        const auto r {boost::charconv::from_chars(s.data(), s.data() + s.size(), parsed, 2)};
        BOOST_TEST(r);
        BOOST_TEST_EQ(parsed, uint256{5U});
        BOOST_TEST_EQ(r.ptr, s.data() + 3);
    }
}

// The string_view overload is the convenient entry point and must agree with the
// pointer pair form.
void test_string_view_overload()
{
    {
        const boost::core::string_view sv {"12345"};
        uint256 parsed {};
        const auto r {boost::charconv::from_chars(sv, parsed)};
        BOOST_TEST(r);
        BOOST_TEST_EQ(parsed, uint256{12345U});
        BOOST_TEST_EQ(r.ptr, sv.data() + sv.size());
    }
    {
        const boost::core::string_view sv {"ff"};
        uint256 parsed {};
        const auto r {boost::charconv::from_chars(sv, parsed, 16)};
        BOOST_TEST(r);
        BOOST_TEST_EQ(parsed, uint256{255U});
    }
    {
        const boost::core::string_view sv {
            "115792089237316195423570985008687907853269984665640564039457584007913129639936"};
        uint256 parsed {};
        const auto r {boost::charconv::from_chars(sv, parsed)};
        BOOST_TEST(!r);
        BOOST_TEST(r.ec == std::errc::result_out_of_range);
    }
    {
        const boost::core::string_view sv {"-1"};
        uint256 parsed {};
        const auto r {boost::charconv::from_chars(sv, parsed)};
        BOOST_TEST(!r);
        BOOST_TEST(r.ec == std::errc::invalid_argument);
    }

    // The three forms agree on a random sample.
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto v {random_uint256(rng, shape::word4)};
        bool ok {false};
        const auto text {to_string_in_base(v, 10, &ok)};
        BOOST_TEST(ok);

        uint256 from_pointers {};
        const auto a {boost::charconv::from_chars(text.data(), text.data() + text.size(), from_pointers)};

        uint256 from_view {};
        const auto b {boost::charconv::from_chars(boost::core::string_view{text}, from_view)};

        BOOST_TEST(a);
        BOOST_TEST(b);
        BOOST_TEST_EQ(from_pointers, v);
        BOOST_TEST_EQ(from_view, v);

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

        bool ok {false};
        const auto text {to_string_in_base(v, base, &ok)};
        BOOST_TEST(ok);
        BOOST_INT256_TEST_STRING("charconv to_chars", v, text, mp_str(to_mp(v), base, false));

        uint256 parsed {};
        const auto r {boost::charconv::from_chars(text.data(), text.data() + text.size(), parsed, base)};
        BOOST_TEST(r);
        BOOST_TEST_EQ(parsed, v);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// The trait specializations the generic charconv implementation dispatches on.
void test_traits()
{
    static_assert(!boost::charconv::detail::is_signed<uint256>::value, "uint256 is unsigned");
    static_assert(std::is_same<boost::charconv::detail::make_unsigned<uint256>::type, uint256>::value,
                  "make_unsigned is the identity");
    // Now that int256 exists, make_signed<uint256> lands on it instead of
    // uint256 itself (the int128 defect this mirrors: charconv.hpp:38-43
    // temporarily mapped it to uint256 before int256 landed).
    static_assert(std::is_same<boost::charconv::detail::make_signed<uint256>::type, int256>::value,
                  "make_signed<uint256> is int256");

    static_assert(boost::charconv::detail::is_signed<int256>::value, "int256 is signed");
    static_assert(std::is_same<boost::charconv::detail::make_unsigned<int256>::type, uint256>::value,
                  "make_unsigned<int256> is uint256");
    static_assert(std::is_same<boost::charconv::detail::make_signed<int256>::type, int256>::value,
                  "make_signed<int256> is the identity");
}

std::string to_string_in_base_i(const int256& v, int base, bool* ok)
{
    char buffer[buffer_size] {};
    const auto r {boost::charconv::to_chars(buffer, buffer + sizeof(buffer), v, base)};

    if (ok != nullptr)
    {
        *ok = static_cast<bool>(r);
    }

    if (!r)
    {
        return std::string{};
    }

    return std::string{buffer, r.ptr};
}

// Every base, every value in the signed corpus, both directions, mirroring
// test_from_chars_round_trips above.
void test_signed_round_trips()
{
    for (int base {2}; base <= 36; ++base)
    {
        for (const auto& v : signed_edge_values())
        {
            bool ok {false};
            const auto text {to_string_in_base_i(v, base, &ok)};
            BOOST_TEST(ok);

            int256 parsed {};
            const auto r {boost::charconv::from_chars(text.data(), text.data() + text.size(), parsed, base)};

            BOOST_TEST(r);
            BOOST_TEST(r.ec == std::errc{});
            BOOST_TEST_EQ(r.ptr, text.data() + text.size());
            BOOST_TEST(parsed == v);
        }

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  base = " << base << std::endl;
            return;
        }
    }
}

// MAX and MIN, every base: the sign never gets in the way of the magnitude
// digits, and appending a digit overflows in either direction.
void test_signed_from_chars_maximum_and_minimum()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    for (int base {2}; base <= 36; ++base)
    {
        for (const auto& v : {min_v, max_v})
        {
            bool ok {false};
            const auto text {to_string_in_base_i(v, base, &ok)};
            BOOST_TEST(ok);

            int256 parsed {};
            const auto r {boost::charconv::from_chars(text.data(), text.data() + text.size(), parsed, base)};
            BOOST_TEST(r);
            BOOST_TEST_EQ(r.ptr, text.data() + text.size());
            BOOST_TEST(parsed == v);

            // Appending a zero multiplies the magnitude by the base, which no
            // longer fits either boundary.
            auto too_big {text};
            too_big.push_back('0');

            int256 overflowed {};
            const auto r2 {boost::charconv::from_chars(too_big.data(), too_big.data() + too_big.size(), overflowed, base)};
            BOOST_TEST(!r2);
            BOOST_TEST(r2.ec == std::errc::result_out_of_range);
        }
    }

    // A leading '+' is invalid, matching the literal and detail::from_chars rule.
    {
        const std::string text {"+1"};
        int256 parsed {};
        const auto r {boost::charconv::from_chars(text.data(), text.data() + text.size(), parsed)};
        BOOST_TEST(!r);
        BOOST_TEST(r.ec == std::errc::invalid_argument);
    }
}

void test_signed_random_round_trips()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const int256 v {random_int256(rng, shape::word4, sign::mixed)};

        char buffer[buffer_size] {};
        const auto to_r {boost::charconv::to_chars(buffer, buffer + sizeof(buffer), v)};
        BOOST_TEST(to_r);

        int256 parsed {};
        const auto from_r {boost::charconv::from_chars(buffer, to_r.ptr, parsed)};
        BOOST_TEST(from_r);
        BOOST_TEST(parsed == v);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

} // namespace

int main()
{
    test_traits();
    test_to_chars_lengths();
    test_to_chars_value_too_large();
    test_from_chars_invalid();
    test_from_chars_partial();
    test_string_view_overload();
    test_from_chars_maximum_and_overflow();
    test_to_chars_every_base();
    test_from_chars_round_trips();
    test_random_round_trips();

    test_signed_round_trips();
    test_signed_from_chars_maximum_and_minimum();
    test_signed_random_round_trips();

    return boost::report_errors();
}

#endif
