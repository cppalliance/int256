// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR

#include <boost/core/lightweight_test.hpp>

#include <compare>
#include <cstdint>
#include <limits>

using boost::int256::uint256;

void test_same_type()
{
    constexpr uint256 x {5U};
    constexpr uint256 y {6U};

    BOOST_TEST((x <=> y) == std::strong_ordering::less);
    BOOST_TEST((y <=> x) == std::strong_ordering::greater);
    BOOST_TEST((x <=> x) == std::strong_ordering::equivalent);
    BOOST_TEST((x <=> x) == std::strong_ordering::equal);

    // The ordering has to look at every word, not just the low one.
    constexpr uint256 big {UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(5)};
    BOOST_TEST((big <=> x) == std::strong_ordering::greater);
    BOOST_TEST((x <=> big) == std::strong_ordering::less);

    constexpr uint256 maximum {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    BOOST_TEST((maximum <=> uint256{}) == std::strong_ordering::greater);
    BOOST_TEST((uint256{} <=> maximum) == std::strong_ordering::less);
}

void test_builtin_operands()
{
    constexpr uint256 x {5U};

    BOOST_TEST((x <=> 4U) == std::strong_ordering::greater);
    BOOST_TEST((x <=> 5U) == std::strong_ordering::equivalent);
    BOOST_TEST((x <=> 5U) == std::strong_ordering::equal);
    BOOST_TEST((x <=> 6U) == std::strong_ordering::less);

    BOOST_TEST((x <=> 4) == std::strong_ordering::greater);
    BOOST_TEST((x <=> 5) == std::strong_ordering::equivalent);
    BOOST_TEST((x <=> 6) == std::strong_ordering::less);

    // A negative operand widens to a huge unsigned value, so it orders above.
    BOOST_TEST((x <=> -1) == std::strong_ordering::less);
    constexpr uint256 maximum {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    BOOST_TEST((maximum <=> -1) == std::strong_ordering::equivalent);
}

void test_float_operands()
{
    constexpr uint256 x {5U};

    BOOST_TEST((x <=> 4.0) == std::partial_ordering::greater);
    BOOST_TEST((x <=> 5.0) == std::partial_ordering::equivalent);
    BOOST_TEST((x <=> 6.0) == std::partial_ordering::less);

    // A NaN operand is unordered.
    const double nan {std::numeric_limits<double>::quiet_NaN()};
    BOOST_TEST((x <=> nan) == std::partial_ordering::unordered);

    BOOST_TEST((x <=> std::numeric_limits<double>::infinity()) == std::partial_ordering::less);
    BOOST_TEST((x <=> -std::numeric_limits<double>::infinity()) == std::partial_ordering::greater);
}

int main()
{
    test_same_type();
    test_builtin_operands();
    test_float_operands();

    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
