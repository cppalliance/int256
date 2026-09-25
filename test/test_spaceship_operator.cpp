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

// int256 is only forward declared until detail/int256_imp.hpp is wired into
// int256.hpp; the section below is gated on that header's guard so this file
// keeps compiling before and after it lands.
#if defined(BOOST_INT256_DETAIL_INT256_IMP_HPP)
#  define BOOST_INT256_TEST_HAS_SIGNED 1
using boost::int256::int256;
#endif

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

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

void test_signed_same_type()
{
    constexpr int256 x {5};
    constexpr int256 y {6};
    constexpr int256 neg {-6};

    BOOST_TEST((x <=> y) == std::strong_ordering::less);
    BOOST_TEST((y <=> x) == std::strong_ordering::greater);
    BOOST_TEST((x <=> x) == std::strong_ordering::equivalent);

    // A negative value orders below every non-negative one, unlike uint256.
    BOOST_TEST((neg <=> x) == std::strong_ordering::less);
    BOOST_TEST((x <=> neg) == std::strong_ordering::greater);

    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    BOOST_TEST((max_v <=> min_v) == std::strong_ordering::greater);
    BOOST_TEST((min_v <=> max_v) == std::strong_ordering::less);
}

void test_signed_builtin_operands()
{
    constexpr int256 x {5};
    constexpr int256 neg {-5};

    BOOST_TEST((x <=> 4) == std::strong_ordering::greater);
    BOOST_TEST((x <=> 5) == std::strong_ordering::equivalent);
    BOOST_TEST((x <=> 6) == std::strong_ordering::less);

    // int256 vs a builtin is mathematically exact, unlike uint256's widening.
    BOOST_TEST((neg <=> -1) == std::strong_ordering::less);
    BOOST_TEST((neg <=> 0) == std::strong_ordering::less);
    BOOST_TEST((x <=> -1) == std::strong_ordering::greater);
}

void test_signed_float_operands()
{
    constexpr int256 x {5};
    constexpr int256 neg {-5};

    BOOST_TEST((x <=> 4.0) == std::partial_ordering::greater);
    BOOST_TEST((x <=> 5.0) == std::partial_ordering::equivalent);
    BOOST_TEST((neg <=> -4.0) == std::partial_ordering::less);
    BOOST_TEST((neg <=> 0.0) == std::partial_ordering::less);

    const double nan {std::numeric_limits<double>::quiet_NaN()};
    BOOST_TEST((x <=> nan) == std::partial_ordering::unordered);
}

// int256 vs uint256 goes through the usual arithmetic conversions, which
// answers strong_ordering, the same as same-type, since both operands become
// uint256 first.
void test_signed_unsigned_cross_type()
{
    constexpr int256 minus_one {-1};
    constexpr uint256 one_u {1U};

    BOOST_TEST((minus_one <=> one_u) == std::strong_ordering::greater);
    BOOST_TEST((one_u <=> minus_one) == std::strong_ordering::less);
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

int main()
{
    test_same_type();
    test_builtin_operands();
    test_float_operands();

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_signed_same_type();
    test_signed_builtin_operands();
    test_signed_float_operands();
    test_signed_unsigned_cross_type();
#endif

    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
