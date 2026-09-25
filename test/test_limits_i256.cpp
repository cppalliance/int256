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

using boost::int256::int256;

void test_basics()
{
    static_assert(std::numeric_limits<int256>::is_specialized, "Should be true");
    static_assert(std::numeric_limits<int256>::is_signed, "Should be true");
    static_assert(std::numeric_limits<int256>::is_integer, "Should be true");
    static_assert(std::numeric_limits<int256>::is_exact, "Should be true");
    static_assert(!std::numeric_limits<int256>::has_infinity, "Should be false");
    static_assert(!std::numeric_limits<int256>::has_quiet_NaN, "Should be false");
    static_assert(!std::numeric_limits<int256>::has_signaling_NaN, "Should be false");

    // C++23 deprecated the following two members
    #if defined(__GNUC__) && __cplusplus > 202002L
    #  pragma GCC diagnostic push
    #  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(_MSC_VER)
    #  pragma warning(push)
    #  pragma warning(disable:4996)
    #endif

    static_assert(std::numeric_limits<int256>::has_denorm == std::denorm_absent, "No denorm");
    static_assert(!std::numeric_limits<int256>::has_denorm_loss, "No denorm");

    #if defined(__GNUC__) && __cplusplus > 202002L
    #  pragma GCC diagnostic pop
    #elif defined(_MSC_VER)
    #  pragma warning(pop)
    #endif

    static_assert(std::numeric_limits<int256>::round_style == std::round_toward_zero, "Integer rounding");
    static_assert(!std::numeric_limits<int256>::is_iec559, "Should be false");
    static_assert(std::numeric_limits<int256>::is_bounded, "Should be true");
    static_assert(std::numeric_limits<int256>::is_modulo, "Should be true");

    // One bit is the sign, so 255 magnitude bits remain.
    static_assert(std::numeric_limits<int256>::digits == 255, "255 bits");

    // 255 * log10(2) = 76.76, so 10^76 fits and 10^77 does not.
    static_assert(std::numeric_limits<int256>::digits10 == 76, "76 base 10 digits");
    static_assert(std::numeric_limits<int256>::max_digits10 == 0, "0 since not fp");
    static_assert(std::numeric_limits<int256>::radix == 2, "Should be 2");
    static_assert(std::numeric_limits<int256>::min_exponent == 0, "Should be 0");
    static_assert(std::numeric_limits<int256>::min_exponent10 == 0, "Should be 0");
    static_assert(std::numeric_limits<int256>::max_exponent == 0, "Should be 0");
    static_assert(std::numeric_limits<int256>::max_exponent10 == 0, "Should be 0");
    static_assert(std::numeric_limits<int256>::traps == std::numeric_limits<std::uint64_t>::traps,
                  "Uses what std::uint64_t does");
    static_assert(!std::numeric_limits<int256>::tinyness_before, "should be false");
}

void test_values()
{
    constexpr int256 zero {};
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    static_assert((std::numeric_limits<int256>::min)() == min_v, "min is INT64_MIN, 0, 0, 0");
    static_assert((std::numeric_limits<int256>::lowest)() == min_v, "lowest equals min for an integer type");
    static_assert((std::numeric_limits<int256>::max)() == max_v, "max is INT64_MAX, ~0, ~0, ~0");
    static_assert(std::numeric_limits<int256>::epsilon() == zero, "epsilon is zero");
    static_assert(std::numeric_limits<int256>::round_error() == zero, "round_error is zero");
    static_assert(std::numeric_limits<int256>::infinity() == zero, "infinity is zero");
    static_assert(std::numeric_limits<int256>::quiet_NaN() == zero, "quiet_NaN is zero");
    static_assert(std::numeric_limits<int256>::signaling_NaN() == zero, "signaling_NaN is zero");
    static_assert(std::numeric_limits<int256>::denorm_min() == zero, "denorm_min is zero");

    const auto minimum {(std::numeric_limits<int256>::min)()};
    BOOST_TEST_EQ(minimum.words[3], UINT64_C(0x8000000000000000));
    BOOST_TEST_EQ(minimum.words[2], UINT64_C(0));
    BOOST_TEST_EQ(minimum.words[1], UINT64_C(0));
    BOOST_TEST_EQ(minimum.words[0], UINT64_C(0));
    BOOST_TEST_EQ(minimum.signed_high(), INT64_MIN);

    const auto maximum {(std::numeric_limits<int256>::max)()};
    BOOST_TEST_EQ(maximum.words[3], static_cast<std::uint64_t>(INT64_MAX));
    BOOST_TEST_EQ(maximum.words[2], UINT64_MAX);
    BOOST_TEST_EQ(maximum.words[1], UINT64_MAX);
    BOOST_TEST_EQ(maximum.words[0], UINT64_MAX);
    BOOST_TEST_EQ(maximum.signed_high(), INT64_MAX);

    // MIN and MAX wrap into each other, exactly like the builtins.
    // BOOST_TEST(a == b) rather than BOOST_TEST_EQ: int256 has no operator<<
    // until D2 lands.
    BOOST_TEST(maximum + int256{1} == minimum);
    BOOST_TEST(minimum - int256{1} == maximum);

    // MIN / -1 is defined to wrap back to MIN, not overflow (issue class the
    // built-in signed integers cannot answer without UB).
    BOOST_TEST(minimum / int256{-1} == minimum);
    BOOST_TEST(minimum % int256{-1} == int256{0});
}

// digits10 is the count of base 10 digits that always round trip, so 10^76 must
// fit and 10^77 must not (MAX has 77 decimal digits, so 10^77 exceeds it).
void test_digits10_is_exact()
{
    int256 power {1};
    for (int i {0}; i < 76; ++i)
    {
        const int256 next {power * 10};
        BOOST_TEST(next / 10 == power);
        power = next;
    }

    // 10^76 is representable and positive.
    BOOST_TEST(power > 0);
    BOOST_TEST(power < (std::numeric_limits<int256>::max)());

    // 10^77 does not round trip: the multiply wraps past MAX and the division
    // no longer recovers the previous power of ten.
    const int256 overflowed {power * 10};
    BOOST_TEST(overflowed / 10 != power);
}

int main()
{
    test_basics();
    test_values();
    test_digits10_is_exact();

    return boost::report_errors();
}
