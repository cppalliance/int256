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

} // namespace

int main()
{
    test_presentation_types();
    test_the_maximum();
    test_width_fill_and_align();
    test_mixed_arguments();
    test_agrees_with_std_format();
    test_against_reference();

    return boost::report_errors();
}

#endif
