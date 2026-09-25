// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The {fmt} formatter, which mirrors the std::format one. The file is a trivial
// pass where {fmt} is not installed, so it is safe to build everywhere.

// {fmt} is a compiled library by default, and the test Jamfile adds no link
// flags for it, so the header only build is what keeps this test self contained.
// The examples and Boost.Int128's own test do the same.
#define FMT_HEADER_ONLY

#include "oracle.hpp"

#include <boost/int256/fmt_format.hpp>

#if defined(BOOST_INT256_ORACLE_UNAVAILABLE) || !defined(BOOST_INT256_HAS_FMT_FORMAT)

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

// {fmt} is not clean under the strict warning set either.
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wold-style-cast"
#  pragma clang diagnostic ignored "-Wsign-conversion"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wundef"
#  pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wundef"
#  pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <fmt/format.h>

#include <string>

using boost::int256::uint256;

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
using boost::int256::int256;
#endif

namespace
{

std::mt19937_64 rng {42};

void test_presentation_types()
{
    const uint256 v {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(255)};

    BOOST_TEST_EQ(fmt::format("{}", v), std::string{"255"});
    BOOST_TEST_EQ(fmt::format("{:d}", v), std::string{"255"});
    BOOST_TEST_EQ(fmt::format("{:x}", v), std::string{"ff"});
    BOOST_TEST_EQ(fmt::format("{:X}", v), std::string{"FF"});
    BOOST_TEST_EQ(fmt::format("{:#x}", v), std::string{"0xff"});
    BOOST_TEST_EQ(fmt::format("{:#X}", v), std::string{"0XFF"});
    BOOST_TEST_EQ(fmt::format("{:b}", v), std::string{"11111111"});
    BOOST_TEST_EQ(fmt::format("{:#b}", v), std::string{"0b11111111"});
    BOOST_TEST_EQ(fmt::format("{:#B}", v), std::string{"0B11111111"});
    BOOST_TEST_EQ(fmt::format("{:o}", v), std::string{"377"});
    BOOST_TEST_EQ(fmt::format("{:#o}", v), std::string{"0377"});

    BOOST_TEST_EQ(fmt::format("{}", uint256{}), std::string{"0"});

    // The alternate form of zero. The octal prefix is a leading zero that a
    // lone "0" already is, so it stays one character, while hex and binary do
    // take their prefix. This is what the built-in integers do.
    BOOST_TEST_EQ(fmt::format("{:#o}", uint256{}), std::string{"0"});
    BOOST_TEST_EQ(fmt::format("{:#x}", uint256{}), std::string{"0x0"});
    BOOST_TEST_EQ(fmt::format("{:#X}", uint256{}), std::string{"0X0"});
    BOOST_TEST_EQ(fmt::format("{:#b}", uint256{}), std::string{"0b0"});
    BOOST_TEST_EQ(fmt::format("{:#B}", uint256{}), std::string{"0B0"});

    BOOST_TEST_EQ(fmt::format("{:x}", uint256{}), std::string{"0"});
    BOOST_TEST_EQ(fmt::format("{:#x}", uint256{}), std::string{"0x0"});
}

void test_the_maximum()
{
    const auto max {uint256_max()};

    BOOST_TEST_EQ(fmt::format("{}", max), mp_str(to_mp(max), 10, false));
    BOOST_TEST_EQ(fmt::format("{}", max).size(), static_cast<std::size_t>(78));
    BOOST_TEST_EQ(fmt::format("{:x}", max), mp_str(to_mp(max), 16, false));
    BOOST_TEST_EQ(fmt::format("{:X}", max), mp_str(to_mp(max), 16, true));
    BOOST_TEST_EQ(fmt::format("{:b}", max), mp_str(to_mp(max), 2, false));
    BOOST_TEST_EQ(fmt::format("{:b}", max).size(), static_cast<std::size_t>(256));
    BOOST_TEST_EQ(fmt::format("{:o}", max), mp_str(to_mp(max), 8, false));
}

void test_width_fill_and_align()
{
    const auto max {uint256_max()};
    const auto decimal {mp_str(to_mp(max), 10, false)};

    BOOST_TEST_EQ(fmt::format("{:>80}", max), std::string{"  "} + decimal);
    BOOST_TEST_EQ(fmt::format("{:*^80}", max), std::string{"*"} + decimal + std::string{"*"});
    BOOST_TEST_EQ(fmt::format("{:0>80}", max), std::string{"00"} + decimal);
    BOOST_TEST_EQ(fmt::format("{:080x}", max), std::string(16, '0') + mp_str(to_mp(max), 16, false));
    BOOST_TEST_EQ(fmt::format("{:<80}", max), decimal + std::string{"  "});

    BOOST_TEST_EQ(fmt::format("{:5}", uint256{42U}), std::string{"   42"});
    BOOST_TEST_EQ(fmt::format("{:<5}", uint256{42U}), std::string{"42   "});
    BOOST_TEST_EQ(fmt::format("{:^5}", uint256{42U}), std::string{" 42  "});
    BOOST_TEST_EQ(fmt::format("{:05}", uint256{42U}), std::string{"00042"});
    BOOST_TEST_EQ(fmt::format("{:+}", uint256{42U}), std::string{"+42"});
}

void test_against_reference()
{
    for (const auto& v : edge_values())
    {
        BOOST_INT256_TEST_STRING("fmt dec", v, fmt::format("{}", v), mp_str(to_mp(v), 10, false));
        BOOST_INT256_TEST_STRING("fmt hex", v, fmt::format("{:x}", v), mp_str(to_mp(v), 16, false));
        BOOST_INT256_TEST_STRING("fmt oct", v, fmt::format("{:o}", v), mp_str(to_mp(v), 8, false));
        BOOST_INT256_TEST_STRING("fmt bin", v, fmt::format("{:b}", v), mp_str(to_mp(v), 2, false));
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto v {random_uint256(rng, shape::word4)};

        BOOST_INT256_TEST_STRING("fmt dec", v, fmt::format("{}", v), mp_str(to_mp(v), 10, false));
        BOOST_INT256_TEST_STRING("fmt hex", v, fmt::format("{:x}", v), mp_str(to_mp(v), 16, false));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// The {fmt} and std::format renderings must agree wherever both are available.
void test_agrees_with_std_format()
{
    #ifdef BOOST_INT256_HAS_FORMAT
    for (const auto& v : edge_values())
    {
        BOOST_TEST_EQ(fmt::format("{}", v), std::format("{}", v));
        BOOST_TEST_EQ(fmt::format("{:x}", v), std::format("{:x}", v));
        BOOST_TEST_EQ(fmt::format("{:#X}", v), std::format("{:#X}", v));
        BOOST_TEST_EQ(fmt::format("{:b}", v), std::format("{:b}", v));
        BOOST_TEST_EQ(fmt::format("{:>80}", v), std::format("{:>80}", v));
        BOOST_TEST_EQ(fmt::format("{:080x}", v), std::format("{:080x}", v));
    }
    #endif
}

void test_mixed_arguments()
{
    BOOST_TEST_EQ(fmt::format("{} and {}", uint256{1U}, uint256{2U}), std::string{"1 and 2"});
    BOOST_TEST_EQ(fmt::format("{}:{:x}", uint256{255U}, uint256{255U}), std::string{"255:ff"});
    BOOST_TEST_EQ(fmt::format("{} {} {}", 1, uint256{2U}, "three"), std::string{"1 2 three"});
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

//=====================================
// int256: the same sign-before-prefix contract as std::format
//=====================================

constexpr int256 min_i {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
constexpr int256 max_i {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

std::string signed_mp_str(const int256& v, int base, bool uppercase)
{
    const bool negative {v.signed_high() < 0};
    const auto magnitude {to_unsigned_bits(boost::int256::abs(v))};
    const auto digits {mp_str(to_mp(magnitude), base, uppercase)};

    return negative ? std::string{"-"} + digits : digits;
}

void test_presentation_types_i()
{
    const int256 v {-255};

    BOOST_TEST_EQ(fmt::format("{}", v), std::string{"-255"});
    BOOST_TEST_EQ(fmt::format("{:d}", v), std::string{"-255"});
    BOOST_TEST_EQ(fmt::format("{:x}", v), std::string{"-ff"});
    BOOST_TEST_EQ(fmt::format("{:X}", v), std::string{"-FF"});
    BOOST_TEST_EQ(fmt::format("{:#x}", v), std::string{"-0xff"});
    BOOST_TEST_EQ(fmt::format("{:#X}", v), std::string{"-0XFF"});
    BOOST_TEST_EQ(fmt::format("{:b}", v), std::string{"-11111111"});
    BOOST_TEST_EQ(fmt::format("{:#b}", v), std::string{"-0b11111111"});
    BOOST_TEST_EQ(fmt::format("{:o}", v), std::string{"-377"});
    BOOST_TEST_EQ(fmt::format("{:#o}", v), std::string{"-0377"});

    BOOST_TEST_EQ(fmt::format("{}", int256{255}), std::string{"255"});
    BOOST_TEST_EQ(fmt::format("{}", int256{}), std::string{"0"});
    BOOST_TEST_EQ(fmt::format("{:#x}", int256{}), std::string{"0x0"});
}

void test_the_extremes_i()
{
    BOOST_TEST_EQ(fmt::format("{}", min_i), signed_mp_str(min_i, 10, false));
    BOOST_TEST_EQ(fmt::format("{}", max_i), signed_mp_str(max_i, 10, false));
    BOOST_TEST_EQ(fmt::format("{}", min_i).size(), fmt::format("{}", max_i).size() + 1U);
    BOOST_TEST_EQ(fmt::format("{:x}", min_i), signed_mp_str(min_i, 16, false));
    BOOST_TEST_EQ(fmt::format("{:b}", min_i), signed_mp_str(min_i, 2, false));
    BOOST_TEST_EQ(fmt::format("{:o}", min_i), signed_mp_str(min_i, 8, false));
}

void test_width_fill_and_align_i()
{
    BOOST_TEST_EQ(fmt::format("{:06}", int256{-42}), std::string{"-00042"});
    BOOST_TEST_EQ(fmt::format("{:+06}", int256{42}), std::string{"+00042"});
    BOOST_TEST_EQ(fmt::format("{:#08x}", int256{-255}), std::string{"-0x000ff"});
    BOOST_TEST_EQ(fmt::format("{:+}", int256{-5}), std::string{"-5"});
    BOOST_TEST_EQ(fmt::format("{:>8}", int256{-42}), std::string{"     -42"});
    BOOST_TEST_EQ(fmt::format("{:<8}", int256{-42}), std::string{"-42     "});
    BOOST_TEST_EQ(fmt::format("{:*^8}", int256{-42}), std::string{"**-42***"});
}

void test_against_reference_i()
{
    for (const auto& v : signed_edge_values())
    {
        BOOST_TEST_EQ(fmt::format("{}", v), signed_mp_str(v, 10, false));
        BOOST_TEST_EQ(fmt::format("{:x}", v), signed_mp_str(v, 16, false));
        BOOST_TEST_EQ(fmt::format("{:o}", v), signed_mp_str(v, 8, false));
        BOOST_TEST_EQ(fmt::format("{:b}", v), signed_mp_str(v, 2, false));
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto v {random_int256(rng, shape::word4, sign::mixed)};

        BOOST_TEST_EQ(fmt::format("{}", v), signed_mp_str(v, 10, false));
        BOOST_TEST_EQ(fmt::format("{:x}", v), signed_mp_str(v, 16, false));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// The {fmt} and std::format renderings must agree for int256 too, wherever
// both are available.
void test_agrees_with_std_format_i()
{
    #ifdef BOOST_INT256_HAS_FORMAT
    for (const auto& v : signed_edge_values())
    {
        BOOST_TEST_EQ(fmt::format("{}", v), std::format("{}", v));
        BOOST_TEST_EQ(fmt::format("{:x}", v), std::format("{:x}", v));
        BOOST_TEST_EQ(fmt::format("{:#X}", v), std::format("{:#X}", v));
        BOOST_TEST_EQ(fmt::format("{:b}", v), std::format("{:b}", v));
        BOOST_TEST_EQ(fmt::format("{:>80}", v), std::format("{:>80}", v));
        BOOST_TEST_EQ(fmt::format("{:080x}", v), std::format("{:080x}", v));
    }
    #endif
}

void test_mixed_arguments_i()
{
    BOOST_TEST_EQ(fmt::format("{} and {}", int256{-1}, int256{2}), std::string{"-1 and 2"});
    BOOST_TEST_EQ(fmt::format("{}:{:x}", int256{-255}, int256{255}), std::string{"-255:ff"});
    BOOST_TEST_EQ(fmt::format("{} {} {}", 1, int256{-2}, "three"), std::string{"1 -2 three"});
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_presentation_types();
    test_the_maximum();
    test_width_fill_and_align();
    test_mixed_arguments();
    test_agrees_with_std_format();
    test_against_reference();

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_presentation_types_i();
    test_the_extremes_i();
    test_width_fill_and_align_i();
    test_mixed_arguments_i();
    test_agrees_with_std_format_i();
    test_against_reference_i();
    #endif

    return boost::report_errors();
}

#endif
