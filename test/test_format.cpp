// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// std::format support. The formatter has to accept the whole integer format spec,
// so each presentation type, the alternate form, and the width, fill and align
// options are checked against the string the reference renders. The file is a
// trivial pass where <format> is unavailable.

#include "oracle.hpp"

#include <boost/int256/format.hpp>

#if defined(BOOST_INT256_ORACLE_UNAVAILABLE) || !defined(BOOST_INT256_HAS_FORMAT)

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <format>
#include <string>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

void test_presentation_types()
{
    const uint256 v {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(255)};

    BOOST_TEST_EQ(std::format("{}", v), std::string{"255"});
    BOOST_TEST_EQ(std::format("{:d}", v), std::string{"255"});
    BOOST_TEST_EQ(std::format("{:x}", v), std::string{"ff"});
    BOOST_TEST_EQ(std::format("{:X}", v), std::string{"FF"});
    BOOST_TEST_EQ(std::format("{:#x}", v), std::string{"0xff"});
    BOOST_TEST_EQ(std::format("{:#X}", v), std::string{"0XFF"});
    BOOST_TEST_EQ(std::format("{:b}", v), std::string{"11111111"});
    BOOST_TEST_EQ(std::format("{:#b}", v), std::string{"0b11111111"});
    BOOST_TEST_EQ(std::format("{:B}", v), std::string{"11111111"});
    BOOST_TEST_EQ(std::format("{:#B}", v), std::string{"0B11111111"});
    BOOST_TEST_EQ(std::format("{:o}", v), std::string{"377"});
    BOOST_TEST_EQ(std::format("{:#o}", v), std::string{"0377"});

    // Zero in every presentation type.
    BOOST_TEST_EQ(std::format("{}", uint256{}), std::string{"0"});

    // The alternate form of zero. The octal prefix is a leading zero that a
    // lone "0" already is, so it stays one character, while hex and binary do
    // take their prefix. This is what the built-in integers do.
    BOOST_TEST_EQ(std::format("{:#o}", uint256{}), std::string{"0"});
    BOOST_TEST_EQ(std::format("{:#x}", uint256{}), std::string{"0x0"});
    BOOST_TEST_EQ(std::format("{:#X}", uint256{}), std::string{"0X0"});
    BOOST_TEST_EQ(std::format("{:#b}", uint256{}), std::string{"0b0"});
    BOOST_TEST_EQ(std::format("{:#B}", uint256{}), std::string{"0B0"});

    BOOST_TEST_EQ(std::format("{:x}", uint256{}), std::string{"0"});
    BOOST_TEST_EQ(std::format("{:#x}", uint256{}), std::string{"0x0"});
    BOOST_TEST_EQ(std::format("{:b}", uint256{}), std::string{"0"});
    BOOST_TEST_EQ(std::format("{:o}", uint256{}), std::string{"0"});
    BOOST_TEST_EQ(std::format("{:#o}", uint256{}), std::string{"0"});
}

void test_the_maximum()
{
    const auto max {uint256_max()};

    BOOST_TEST_EQ(std::format("{}", max), mp_str(to_mp(max), 10, false));
    BOOST_TEST_EQ(std::format("{}", max).size(), static_cast<std::size_t>(78));
    BOOST_TEST_EQ(std::format("{:x}", max), mp_str(to_mp(max), 16, false));
    BOOST_TEST_EQ(std::format("{:X}", max), mp_str(to_mp(max), 16, true));
    BOOST_TEST_EQ(std::format("{:b}", max), mp_str(to_mp(max), 2, false));
    BOOST_TEST_EQ(std::format("{:b}", max).size(), static_cast<std::size_t>(256));
    BOOST_TEST_EQ(std::format("{:o}", max), mp_str(to_mp(max), 8, false));
    BOOST_TEST_EQ(std::format("{:#x}", max), std::string{"0x"} + mp_str(to_mp(max), 16, false));
}

void test_width_fill_and_align()
{
    const auto max {uint256_max()};
    const auto decimal {mp_str(to_mp(max), 10, false)};

    // A width wider than the 78 digit maximum, so the padding is visible.
    {
        const auto text {std::format("{:>80}", max)};
        BOOST_TEST_EQ(text.size(), static_cast<std::size_t>(80));
        BOOST_TEST_EQ(text, std::string{"  "} + decimal);
    }
    {
        const auto text {std::format("{:*^80}", max)};
        BOOST_TEST_EQ(text.size(), static_cast<std::size_t>(80));
        BOOST_TEST_EQ(text, std::string{"*"} + decimal + std::string{"*"});
    }
    {
        const auto text {std::format("{:0>80}", max)};
        BOOST_TEST_EQ(text, std::string{"00"} + decimal);
    }
    {
        // Zero padding through the sign aware form, in hex.
        const auto text {std::format("{:080x}", max)};
        BOOST_TEST_EQ(text.size(), static_cast<std::size_t>(80));
        BOOST_TEST_EQ(text, std::string(16, '0') + mp_str(to_mp(max), 16, false));
    }
    {
        // Left align.
        const auto text {std::format("{:<80}", max)};
        BOOST_TEST_EQ(text, decimal + std::string{"  "});
    }
    {
        // A width narrower than the value does not truncate.
        BOOST_TEST_EQ(std::format("{:1}", max), decimal);
    }

    // And the same on a small value, where the padding dominates.
    BOOST_TEST_EQ(std::format("{:5}", uint256{42U}), std::string{"   42"});
    BOOST_TEST_EQ(std::format("{:<5}", uint256{42U}), std::string{"42   "});
    BOOST_TEST_EQ(std::format("{:^5}", uint256{42U}), std::string{" 42  "});
    BOOST_TEST_EQ(std::format("{:05}", uint256{42U}), std::string{"00042"});
}

void test_sign_options()
{
    // The type is unsigned, so a plus sign is always shown when asked for and a
    // space is always a space.
    BOOST_TEST_EQ(std::format("{:+}", uint256{42U}), std::string{"+42"});
    BOOST_TEST_EQ(std::format("{:+}", uint256{}), std::string{"+0"});
    BOOST_TEST_EQ(std::format("{: }", uint256{42U}), std::string{" 42"});
    BOOST_TEST_EQ(std::format("{:-}", uint256{42U}), std::string{"42"});
    BOOST_TEST_EQ(std::format("{:+}", uint256_max()), std::string{"+"} + mp_str(to_mp(uint256_max()), 10, false));
}

void test_against_reference()
{
    for (const auto& v : edge_values())
    {
        BOOST_INT256_TEST_STRING("format dec", v, std::format("{}", v), mp_str(to_mp(v), 10, false));
        BOOST_INT256_TEST_STRING("format hex", v, std::format("{:x}", v), mp_str(to_mp(v), 16, false));
        BOOST_INT256_TEST_STRING("format HEX", v, std::format("{:X}", v), mp_str(to_mp(v), 16, true));
        BOOST_INT256_TEST_STRING("format oct", v, std::format("{:o}", v), mp_str(to_mp(v), 8, false));
        BOOST_INT256_TEST_STRING("format bin", v, std::format("{:b}", v), mp_str(to_mp(v), 2, false));
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto v {random_uint256(rng, shape::word4)};

        BOOST_INT256_TEST_STRING("format dec", v, std::format("{}", v), mp_str(to_mp(v), 10, false));
        BOOST_INT256_TEST_STRING("format hex", v, std::format("{:x}", v), mp_str(to_mp(v), 16, false));
        BOOST_INT256_TEST_STRING("format bin", v, std::format("{:b}", v), mp_str(to_mp(v), 2, false));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// Several arguments in one call, and a uint256 alongside other types.
void test_mixed_arguments()
{
    BOOST_TEST_EQ(std::format("{} and {}", uint256{1U}, uint256{2U}), std::string{"1 and 2"});
    BOOST_TEST_EQ(std::format("{}:{:x}", uint256{255U}, uint256{255U}), std::string{"255:ff"});
    BOOST_TEST_EQ(std::format("{} {} {}", 1, uint256{2U}, "three"), std::string{"1 2 three"});
    BOOST_TEST_EQ(std::format("{1} {0}", uint256{1U}, uint256{2U}), std::string{"2 1"});
}

} // namespace

int main()
{
    test_presentation_types();
    test_the_maximum();
    test_width_fill_and_align();
    test_sign_options();
    test_mixed_arguments();
    test_against_reference();

    return boost::report_errors();
}

#endif
