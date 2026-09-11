// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_BUILD_MODULE

#include <boost/int256.hpp>

#else

import boost.int256;
// A module import carries no macros, so the BOOST_INT256_* feature and endianness
// macros have to come from the header even in the module build.
#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/literal_macros.hpp>

#endif

#include <boost/core/lightweight_test.hpp>

#include <climits>
#include <cstdint>
#include <limits>

using boost::int256::uint256;

void test_basics()
{
    static_assert(std::numeric_limits<uint256>::is_specialized, "Should be true");
    static_assert(!std::numeric_limits<uint256>::is_signed, "Should be false");
    static_assert(std::numeric_limits<uint256>::is_integer, "Should be true");
    static_assert(std::numeric_limits<uint256>::is_exact, "Should be true");
    static_assert(!std::numeric_limits<uint256>::has_infinity, "Should be false");
    static_assert(!std::numeric_limits<uint256>::has_quiet_NaN, "Should be false");
    static_assert(!std::numeric_limits<uint256>::has_signaling_NaN, "Should be false");

    // C++23 deprecated the following two members
    #if defined(__GNUC__) && __cplusplus > 202002L
    #  pragma GCC diagnostic push
    #  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(_MSC_VER)
    #  pragma warning(push)
    #  pragma warning(disable:4996)
    #endif

    static_assert(std::numeric_limits<uint256>::has_denorm == std::denorm_absent, "No denorm");
    static_assert(!std::numeric_limits<uint256>::has_denorm_loss, "No denorm");

    #if defined(__GNUC__) && __cplusplus > 202002L
    #  pragma GCC diagnostic pop
    #elif defined(_MSC_VER)
    #  pragma warning(pop)
    #endif

    static_assert(std::numeric_limits<uint256>::round_style == std::round_toward_zero, "Integer rounding");
    static_assert(!std::numeric_limits<uint256>::is_iec559, "Should be false");
    static_assert(std::numeric_limits<uint256>::is_bounded, "Should be true");
    static_assert(std::numeric_limits<uint256>::is_modulo, "Should be true");
    static_assert(std::numeric_limits<uint256>::digits == CHAR_BIT * sizeof(uint256), "256 bits");
    static_assert(std::numeric_limits<uint256>::digits == 256, "256 bits");

    // 256 * log10(2) = 77.06, so 10^77 fits and 10^78 does not
    static_assert(std::numeric_limits<uint256>::digits10 == 77, "77 base 10 digits");
    static_assert(std::numeric_limits<uint256>::max_digits10 == 0, "0 since not fp");
    static_assert(std::numeric_limits<uint256>::radix == 2, "Should be 2");
    static_assert(std::numeric_limits<uint256>::min_exponent == 0, "Should be 0");
    static_assert(std::numeric_limits<uint256>::min_exponent10 == 0, "Should be 0");
    static_assert(std::numeric_limits<uint256>::max_exponent == 0, "Should be 0");
    static_assert(std::numeric_limits<uint256>::max_exponent10 == 0, "Should be 0");
    static_assert(std::numeric_limits<uint256>::traps == std::numeric_limits<std::uint64_t>::traps,
                  "Uses what std::uint64_t does");
    static_assert(!std::numeric_limits<uint256>::tinyness_before, "should be false");
}

void test_values()
{
    constexpr uint256 zero {};
    constexpr uint256 all_ones {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    static_assert((std::numeric_limits<uint256>::min)() == zero, "min is zero");
    static_assert((std::numeric_limits<uint256>::lowest)() == zero, "lowest is zero");
    static_assert((std::numeric_limits<uint256>::max)() == all_ones, "max is all ones");
    static_assert(std::numeric_limits<uint256>::epsilon() == zero, "epsilon is zero");
    static_assert(std::numeric_limits<uint256>::round_error() == zero, "round_error is zero");
    static_assert(std::numeric_limits<uint256>::infinity() == zero, "infinity is zero");
    static_assert(std::numeric_limits<uint256>::quiet_NaN() == zero, "quiet_NaN is zero");
    static_assert(std::numeric_limits<uint256>::signaling_NaN() == zero, "signaling_NaN is zero");
    static_assert(std::numeric_limits<uint256>::denorm_min() == zero, "denorm_min is zero");

    // Each word of the maximum is set, and one more wraps to zero.
    const auto maximum {(std::numeric_limits<uint256>::max)()};
    BOOST_TEST_EQ(maximum.words[0], UINT64_MAX);
    BOOST_TEST_EQ(maximum.words[1], UINT64_MAX);
    BOOST_TEST_EQ(maximum.words[2], UINT64_MAX);
    BOOST_TEST_EQ(maximum.words[3], UINT64_MAX);
    BOOST_TEST_EQ(maximum + uint256{1U}, zero);
}

// digits10 is the count of base 10 digits that always round trip, so 10^77 must
// fit and 10^78 must not.
void test_digits10_is_exact()
{
    uint256 power {1U};
    for (int i {0}; i < 77; ++i)
    {
        const uint256 next {power * 10U};
        BOOST_TEST(next / 10U == power);
        power = next;
    }

    // 10^77 is representable.
    BOOST_TEST(power != 0U);
    BOOST_TEST(power < (std::numeric_limits<uint256>::max)());

    // 10^78 is not: the multiply wraps and the division no longer round trips.
    const uint256 overflowed {power * 10U};
    BOOST_TEST(overflowed / 10U != power);
}

int main()
{
    test_basics();
    test_values();
    test_digits10_is_exact();

    return boost::report_errors();
}
