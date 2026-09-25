// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Small mixed-sign comparisons, the shape a user actually writes. The point is
// that a signed literal on the right compiles without a conversion warning and
// answers the way the builtins do.

#include <boost/int256/int256.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstdint>

using boost::int256::uint256;

// int256 is only forward declared until detail/int256_imp.hpp is wired into
// int256.hpp; the section below is gated on that header's guard so this file
// keeps compiling before and after it lands.
#if defined(BOOST_INT256_DETAIL_INT256_IMP_HPP)
#  define BOOST_INT256_TEST_HAS_SIGNED 1
using boost::int256::int256;
#endif

void test_signed_literals()
{
    constexpr uint256 x {5U};

    BOOST_TEST(x > 4);
    BOOST_TEST(x >= 4);
    BOOST_TEST(x >= 5);
    BOOST_TEST(x == 5);
    BOOST_TEST(x != 0);
    BOOST_TEST(x <= 5);
    BOOST_TEST(x < 10);

    BOOST_TEST(4 < x);
    BOOST_TEST(4 <= x);
    BOOST_TEST(5 == x);
    BOOST_TEST(0 != x);
    BOOST_TEST(5 >= x);
    BOOST_TEST(10 > x);
}

void test_unsigned_literals()
{
    constexpr uint256 x {5U};

    BOOST_TEST(x > 4U);
    BOOST_TEST(x >= 4U);
    BOOST_TEST(x == 5U);
    BOOST_TEST(x != 3U);
    BOOST_TEST(x <= 5U);
    BOOST_TEST(x < 10U);

    BOOST_TEST(4U < x);
    BOOST_TEST(10U > x);
}

// A value that does not fit the low word must not compare equal to a scalar that
// only matches those low bits.
void test_upper_words_participate()
{
    constexpr uint256 x {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(5)};

    BOOST_TEST(x != 5U);
    BOOST_TEST(x != 5);
    BOOST_TEST(x > 5U);
    BOOST_TEST(x > 5);
    BOOST_TEST(!(x < 5U));
    BOOST_TEST(!(x <= 5U));

    constexpr uint256 y {UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(5)};
    BOOST_TEST(y != 5U);
    BOOST_TEST(y > 5U);
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

// int256 with a signed or unsigned literal answers exactly like the builtins,
// since every builtin fits inside int256's range and comparisons are
// mathematically exact (no widening surprise the way uint256 has).
void test_int256_signed_literals()
{
    constexpr int256 x {5};
    constexpr int256 neg {-5};

    BOOST_TEST(x > 4);
    BOOST_TEST(x >= 5);
    BOOST_TEST(x == 5);
    BOOST_TEST(x != -5);
    BOOST_TEST(x <= 5);
    BOOST_TEST(x < 10);

    BOOST_TEST(neg < 0);
    BOOST_TEST(neg < x);
    BOOST_TEST(neg == -5);
    BOOST_TEST(-5 == neg);
    BOOST_TEST(neg > -10);
    BOOST_TEST(-10 < neg);
}

void test_int256_unsigned_literals()
{
    constexpr int256 x {5};
    constexpr int256 neg {-5};

    BOOST_TEST(x > 4U);
    BOOST_TEST(x == 5U);
    BOOST_TEST(x != 3U);
    BOOST_TEST(x <= 5U);
    BOOST_TEST(x < 10U);

    // A negative int256 is exactly (mathematically) less than any unsigned
    // scalar, unlike uint256{-5}, which becomes a huge positive value.
    BOOST_TEST(neg < 0U);
    BOOST_TEST(0U > neg);
    BOOST_TEST(neg != 5U);
}

// int256 vs uint256 goes through the usual arithmetic conversions (both
// become uint256), which is a different, deliberately surprising, rule from
// int256 vs a builtin scalar.
void test_int256_uint256_cross_type()
{
    const int256 minus_one {-1};
    const uint256 one_u {1U};

    BOOST_TEST(minus_one > one_u);
    BOOST_TEST(one_u < minus_one);
    BOOST_TEST(minus_one == (uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}));
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

int main()
{
    test_signed_literals();
    test_unsigned_literals();
    test_upper_words_participate();

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_int256_signed_literals();
    test_int256_unsigned_literals();
    test_int256_uint256_cross_type();
#endif

    return boost::report_errors();
}
