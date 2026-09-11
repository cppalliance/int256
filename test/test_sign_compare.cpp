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

int main()
{
    test_signed_literals();
    test_unsigned_literals();
    test_upper_words_participate();

    return boost::report_errors();
}
