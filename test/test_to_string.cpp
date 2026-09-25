// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// to_string and to_wstring, which are decimal only and go through the same
// mini_to_chars the stream inserter uses. The reference renders the value
// independently, so the digits are checked rather than just the length.

#include <boost/int256.hpp>

#include "oracle.hpp"

#include <cstddef>
#include <string>

using boost::int256::uint256;
using boost::int256::int256;
using boost::int256::to_string;
using boost::int256::to_wstring;

namespace
{

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE
std::mt19937_64 rng {42};
#endif

std::wstring widen_string(const std::string& narrow)
{
    std::wstring wide_text;
    wide_text.reserve(narrow.size());
    for (const auto c : narrow)
    {
        wide_text.push_back(static_cast<wchar_t>(c));
    }

    return wide_text;
}

void test_known_answers()
{
    BOOST_TEST_EQ(to_string(uint256{}), std::string{"0"});
    BOOST_TEST_EQ(to_string(uint256{1U}), std::string{"1"});
    BOOST_TEST_EQ(to_string(uint256{42U}), std::string{"42"});
    BOOST_TEST_EQ(to_string(uint256{UINT64_MAX}), std::string{"18446744073709551615"});

    const uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    BOOST_TEST_EQ(to_string(one << 64U), std::string{"18446744073709551616"});
    BOOST_TEST_EQ(to_string(one << 128U), std::string{"340282366920938463463374607431768211456"});
    BOOST_TEST_EQ(to_string(one << 192U),
                  std::string{"6277101735386680763835789423207666416102355444464034512896"});
    BOOST_TEST_EQ(to_string(uint256_max()),
                  std::string{"115792089237316195423570985008687907853269984665640564039457584007913129639935"});
    BOOST_TEST_EQ(to_string(uint256_max()).size(), static_cast<std::size_t>(78));

    // No leading zeros, and no padding, at any width.
    for (unsigned k {0}; k < 256U; ++k)
    {
        const auto text {to_string(one << k)};
        BOOST_TEST(!text.empty());
        BOOST_TEST(text[0] != '0');
    }
}

void test_wide_matches_narrow()
{
    const uint256 values[] {
        uint256{}, uint256{1U}, uint256{42U}, uint256{UINT64_MAX}, uint256_max(),
        uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0))
    };

    for (const auto& v : values)
    {
        BOOST_TEST(to_wstring(v) == widen_string(to_string(v)));
    }

    BOOST_TEST_EQ(to_wstring(uint256_max()).size(), static_cast<std::size_t>(78));
}

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE
void test_against_reference()
{
    for (const auto& v : edge_values())
    {
        BOOST_INT256_TEST_STRING("to_string", v, to_string(v), mp_str(to_mp(v), 10, false));
        BOOST_TEST(to_wstring(v) == widen_string(mp_str(to_mp(v), 10, false)));
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        for (const auto s : all_shapes())
        {
            const auto v {random_uint256(rng, s)};
            BOOST_INT256_TEST_STRING("to_string", v, to_string(v), mp_str(to_mp(v), 10, false));
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}
#endif

// The overload is constrained to the library types, so a builtin does not pick it
// up by accident.
void test_constraint()
{
    static_assert(std::is_same<decltype(to_string(uint256{})), std::string>::value,
                  "to_string returns a std::string");
    static_assert(std::is_same<decltype(to_wstring(uint256{})), std::wstring>::value,
                  "to_wstring returns a std::wstring");
    static_assert(std::is_same<decltype(to_string(int256{})), std::string>::value,
                  "to_string(int256) returns a std::string");
    static_assert(std::is_same<decltype(to_wstring(int256{})), std::wstring>::value,
                  "to_wstring(int256) returns a std::wstring");
}

void test_signed_known_answers()
{
    BOOST_TEST_EQ(to_string(int256{}), std::string{"0"});
    BOOST_TEST_EQ(to_string(int256{1}), std::string{"1"});
    BOOST_TEST_EQ(to_string(int256{-1}), std::string{"-1"});
    BOOST_TEST_EQ(to_string(int256{-42}), std::string{"-42"});

    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    BOOST_TEST_EQ(to_string(min_v),
                  std::string{"-57896044618658097711785492504343953926634992332820282019728792003956564819968"});
    BOOST_TEST_EQ(to_string(max_v),
                  std::string{"57896044618658097711785492504343953926634992332820282019728792003956564819967"});

    // No leading zeros (other than a bare "0"), and the sign is the very
    // first character for a negative value.
    const int256 one {INT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    for (unsigned k {0}; k < 255U; ++k)
    {
        const auto positive_text {to_string(one << k)};
        BOOST_TEST(!positive_text.empty());
        BOOST_TEST(positive_text[0] != '0');

        const auto negative_text {to_string(-(one << k))};
        BOOST_TEST(!negative_text.empty());
        BOOST_TEST(negative_text[0] == '-');
        BOOST_TEST(negative_text[1] != '0');
    }
}

void test_signed_wide_matches_narrow()
{
    const int256 values[] {
        int256{}, int256{1}, int256{-1}, int256{42}, int256{-42},
        int256(INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)),
        int256(INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX)
    };

    for (const auto& v : values)
    {
        BOOST_TEST(to_wstring(v) == widen_string(to_string(v)));
    }
}

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE
void test_signed_against_reference()
{
    for (const auto& v : signed_edge_values())
    {
        // The sign has to be checked and rendered separately from the
        // magnitude, since mp_str (like to_mp) only knows uint256.
        const bool negative {v.signed_high() < 0};
        const uint256 magnitude {to_unsigned_bits(negative ? -v : v)};
        const auto digits {mp_str(to_mp(magnitude), 10, false)};
        const auto expected_text {negative ? std::string{"-"} + digits : digits};

        BOOST_TEST_EQ(to_string(v), expected_text);
        BOOST_TEST(to_wstring(v) == widen_string(expected_text));
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        for (const auto s : all_shapes())
        {
            for (const auto sg : {sign::pos, sign::neg, sign::mixed})
            {
                const int256 v {random_int256(rng, s, sg)};

                const bool negative {v.signed_high() < 0};
                const uint256 magnitude {to_unsigned_bits(negative ? -v : v)};
                const auto digits {mp_str(to_mp(magnitude), 10, false)};
                const auto expected_text {negative ? std::string{"-"} + digits : digits};

                BOOST_TEST_EQ(to_string(v), expected_text);
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}
#endif

} // namespace

int main()
{
    test_constraint();
    test_known_answers();
    test_wide_matches_narrow();
    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    test_against_reference();
    #endif

    test_signed_known_answers();
    test_signed_wide_matches_narrow();
    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    test_signed_against_reference();
    #endif

    return boost::report_errors();
}
