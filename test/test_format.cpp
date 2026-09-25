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

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
using boost::int256::int256;
#endif

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

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

//=====================================
// int256: the sign is printed before any base prefix ("-0xff", never
// "0x-ff", the int128 defect the contract calls out), and it is always
// present for a negative value regardless of the sign option
//=====================================

constexpr int256 min_i {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
constexpr int256 max_i {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

// The magnitude, rendered through the already-verified unsigned path, with
// the sign prepended separately: abs(MIN) is exact (it wraps back to MIN's
// own bit pattern, whose magnitude to_unsigned_bits reports correctly), so
// this needs no special case for it.
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

    BOOST_TEST_EQ(std::format("{}", v), std::string{"-255"});
    BOOST_TEST_EQ(std::format("{:d}", v), std::string{"-255"});
    BOOST_TEST_EQ(std::format("{:x}", v), std::string{"-ff"});
    BOOST_TEST_EQ(std::format("{:X}", v), std::string{"-FF"});
    BOOST_TEST_EQ(std::format("{:#x}", v), std::string{"-0xff"});
    BOOST_TEST_EQ(std::format("{:#X}", v), std::string{"-0XFF"});
    BOOST_TEST_EQ(std::format("{:b}", v), std::string{"-11111111"});
    BOOST_TEST_EQ(std::format("{:#b}", v), std::string{"-0b11111111"});
    BOOST_TEST_EQ(std::format("{:o}", v), std::string{"-377"});
    BOOST_TEST_EQ(std::format("{:#o}", v), std::string{"-0377"});

    // A positive value looks exactly like the unsigned case.
    BOOST_TEST_EQ(std::format("{}", int256{255}), std::string{"255"});
    BOOST_TEST_EQ(std::format("{:#x}", int256{255}), std::string{"0xff"});

    // Zero has no sign in any presentation type.
    BOOST_TEST_EQ(std::format("{}", int256{}), std::string{"0"});
    BOOST_TEST_EQ(std::format("{:#x}", int256{}), std::string{"0x0"});
    BOOST_TEST_EQ(std::format("{:#o}", int256{}), std::string{"0"});
}

void test_the_extremes_i()
{
    BOOST_TEST_EQ(std::format("{}", min_i), signed_mp_str(min_i, 10, false));
    BOOST_TEST_EQ(std::format("{}", max_i), signed_mp_str(max_i, 10, false));

    // MIN's magnitude (2^255) and MAX's (2^255 - 1) have the same digit
    // count, so MIN's decimal string is exactly one character longer than
    // MAX's: the sign.
    BOOST_TEST_EQ(std::format("{}", min_i).size(), std::format("{}", max_i).size() + 1U);

    BOOST_TEST_EQ(std::format("{:x}", min_i), signed_mp_str(min_i, 16, false));
    BOOST_TEST_EQ(std::format("{:b}", min_i), signed_mp_str(min_i, 2, false));
    BOOST_TEST_EQ(std::format("{:o}", min_i), signed_mp_str(min_i, 8, false));

    // MIN in hex is a sign, "0x", and 64 hex digits (2^255 is 64 hex digits,
    // one bit into the 64th), matching the top bit pattern.
    BOOST_TEST_EQ(std::format("{:#x}", min_i), std::string{"-0x8"} + std::string(63, '0'));
}

void test_width_fill_and_align_i()
{
    // Zero padding through the sign-aware form: the sign is part of the
    // content and the zeros go after it, exactly like a builtin int.
    BOOST_TEST_EQ(std::format("{:06}", int256{-42}), std::string{"-00042"});
    BOOST_TEST_EQ(std::format("{:+06}", int256{42}), std::string{"+00042"});
    BOOST_TEST_EQ(std::format("{:#08x}", int256{-255}), std::string{"-0x000ff"});

    // An explicit + or space never overrides an actual negative sign.
    BOOST_TEST_EQ(std::format("{:+}", int256{-5}), std::string{"-5"});
    BOOST_TEST_EQ(std::format("{: }", int256{-5}), std::string{"-5"});

    // Alignment treats the sign as part of the content, not something to pad
    // around.
    BOOST_TEST_EQ(std::format("{:>8}", int256{-42}), std::string{"     -42"});
    BOOST_TEST_EQ(std::format("{:<8}", int256{-42}), std::string{"-42     "});
    BOOST_TEST_EQ(std::format("{:*^8}", int256{-42}), std::string{"**-42***"});
}

void test_sign_options_i()
{
    BOOST_TEST_EQ(std::format("{:+}", int256{42}), std::string{"+42"});
    BOOST_TEST_EQ(std::format("{:+}", int256{-42}), std::string{"-42"});
    BOOST_TEST_EQ(std::format("{:+}", int256{}), std::string{"+0"});
    BOOST_TEST_EQ(std::format("{: }", int256{42}), std::string{" 42"});
    BOOST_TEST_EQ(std::format("{: }", int256{-42}), std::string{"-42"});
    BOOST_TEST_EQ(std::format("{:-}", int256{42}), std::string{"42"});
    BOOST_TEST_EQ(std::format("{:-}", int256{-42}), std::string{"-42"});
}

void test_against_reference_i()
{
    for (const auto& v : signed_edge_values())
    {
        BOOST_TEST_EQ(std::format("{}", v), signed_mp_str(v, 10, false));
        BOOST_TEST_EQ(std::format("{:x}", v), signed_mp_str(v, 16, false));
        BOOST_TEST_EQ(std::format("{:X}", v), signed_mp_str(v, 16, true));
        BOOST_TEST_EQ(std::format("{:o}", v), signed_mp_str(v, 8, false));
        BOOST_TEST_EQ(std::format("{:b}", v), signed_mp_str(v, 2, false));
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto v {random_int256(rng, shape::word4, sign::mixed)};

        BOOST_TEST_EQ(std::format("{}", v), signed_mp_str(v, 10, false));
        BOOST_TEST_EQ(std::format("{:x}", v), signed_mp_str(v, 16, false));
        BOOST_TEST_EQ(std::format("{:b}", v), signed_mp_str(v, 2, false));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_mixed_arguments_i()
{
    BOOST_TEST_EQ(std::format("{} and {}", int256{-1}, int256{2}), std::string{"-1 and 2"});
    BOOST_TEST_EQ(std::format("{}:{:x}", int256{-255}, int256{255}), std::string{"-255:ff"});
    BOOST_TEST_EQ(std::format("{} {} {}", 1, int256{-2}, "three"), std::string{"1 -2 three"});
    BOOST_TEST_EQ(std::format("{1} {0}", int256{-1}, uint256{2U}), std::string{"2 -1"});
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_presentation_types();
    test_the_maximum();
    test_width_fill_and_align();
    test_sign_options();
    test_mixed_arguments();
    test_against_reference();

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_presentation_types_i();
    test_the_extremes_i();
    test_width_fill_and_align_i();
    test_sign_options_i();
    test_mixed_arguments_i();
    test_against_reference_i();
    #endif

    return boost::report_errors();
}

#endif
