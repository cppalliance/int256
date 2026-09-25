//  Copyright 2026 Matt Borland
//  Distributed under the Boost Software License, Version 1.0.
//  https://www.boost.org/LICENSE_1_0.txt
//
// Wraparound at the range boundaries, and the mixed-operand paths that have to
// negate a scalar. Every operand goes through opaque() so the compiler cannot
// fold the operation away and evaluate it with a different code path than the
// one the library ships. test_constexpr_boundaries.cpp is the compile-time twin.

#include <boost/int256.hpp>
#include <boost/core/lightweight_test.hpp>

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

namespace
{

std::int64_t opaque(std::int64_t v)
{
    volatile std::int64_t s {v};
    return s;
}

std::uint64_t opaque(std::uint64_t v)
{
    volatile std::uint64_t s {v};
    return s;
}

int opaque(int v)
{
    volatile int s {v};
    return s;
}

unsigned opaque(unsigned v)
{
    volatile unsigned s {v};
    return s;
}

uint256 opaque(uint256 v)
{
    return uint256{opaque(v.words[3]), opaque(v.words[2]), opaque(v.words[1]), opaque(v.words[0])};
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

int256 opaque(int256 v)
{
    return int256{static_cast<std::int64_t>(opaque(v.words[3])), opaque(v.words[2]), opaque(v.words[1]), opaque(v.words[0])};
}

#endif

} // namespace

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

void test_signed_boundaries()
{
    const auto min_v {opaque(int256{INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)})};
    const auto max_v {opaque(int256{INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX})};
    const auto zero {opaque(int256{})};
    const auto one {opaque(int256{INT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)})};
    const auto minus_one {opaque(int256{-1})};

    // BOOST_TEST(a == b) rather than BOOST_TEST_EQ throughout this function:
    // int256 has no operator<< until D2 lands, and BOOST_TEST_EQ would try to
    // stream the operands on a failure (currently ambiguous).

    // MIN's magnitude (2^255) is not representable, so negation and abs both
    // wrap it back to itself, exactly like the built-in signed integers.
    BOOST_TEST(-min_v == min_v);
    BOOST_TEST(boost::int256::abs(min_v) == min_v);
    BOOST_TEST(boost::int256::abs(min_v + one) == max_v);
    BOOST_TEST(boost::int256::abs(max_v) == max_v);
    BOOST_TEST(boost::int256::abs(minus_one) == one);

    // MIN / -1 wraps to MIN instead of overflowing (the mathematical quotient
    // 2^255 is not representable); MIN % -1 is exactly zero.
    BOOST_TEST(min_v / minus_one == min_v);
    BOOST_TEST(min_v % minus_one == zero);

    // Wraparound at the signed range boundary.
    BOOST_TEST(max_v + one == min_v);
    BOOST_TEST(min_v - one == max_v);

    // Arithmetic right shift sign extends; logical left shift wraps in the
    // unsigned domain regardless of sign.
    BOOST_TEST((minus_one >> opaque(255)) == minus_one);
    BOOST_TEST((min_v >> opaque(1)) == opaque(int256{static_cast<std::int64_t>(UINT64_C(0xC000000000000000)), UINT64_C(0), UINT64_C(0), UINT64_C(0)}));
    BOOST_TEST((min_v << opaque(1)) == zero);

    // int256 / uint256 and %= use unsigned semantics on the right-hand side
    // (the int128 defect B regression is pinned in detail in
    // test_mixed_type_sign_conversion.cpp); here just the basic compound form
    // on a builtin scalar.
    {
        int256 t {max_v};
        t += one;
        BOOST_TEST(t == min_v);
    }
    {
        int256 t {min_v};
        t -= one;
        BOOST_TEST(t == max_v);
    }
    {
        int256 t {min_v};
        t /= minus_one;
        BOOST_TEST(t == min_v);
    }

    // The signed mixed paths must agree with promoting the scalar first,
    // including for the most negative int64_t, which cannot be negated inside
    // its own type.
    const auto m {opaque(static_cast<std::int64_t>((std::numeric_limits<std::int64_t>::min)()))};
    const int256 m256 {m};
    const auto a {opaque(int256{INT64_C(42), UINT64_C(0), UINT64_C(1234), UINT64_C(7)})};
    const auto b {opaque(int256{INT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(5)})};

    BOOST_TEST(a + m == a + m256);
    BOOST_TEST(m + a == m256 + a);
    BOOST_TEST(a - m == a - m256);
    BOOST_TEST(m - a == m256 - a);
    BOOST_TEST(a * m == a * m256);
    BOOST_TEST(m * a == m256 * a);
    BOOST_TEST(a / m == a / m256);
    BOOST_TEST(a % m == a % m256);
    BOOST_TEST(m / b == m256 / b);
    BOOST_TEST(m % b == m256 % b);
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

int main()
{
    const auto max {opaque((std::numeric_limits<uint256>::max)())};
    const auto zero {opaque(uint256{})};
    const auto one {opaque(uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)})};

    // Two's complement rollover: no undefined behavior, is_modulo is true.
    BOOST_TEST_EQ(max + one, zero);
    BOOST_TEST_EQ(zero - one, max);
    BOOST_TEST_EQ(max + opaque(1U), zero);
    BOOST_TEST_EQ(zero - opaque(1U), max);
    BOOST_TEST_EQ(max * opaque(2U), max - one);
    BOOST_TEST_EQ(-zero, zero);
    BOOST_TEST_EQ(-max, one);
    BOOST_TEST_EQ(-one, max);

    // Constructing from a negative scalar sign fills, so uint256{-1} is the
    // maximum. This is well formed on purpose, so it is pinned here rather than
    // being a compile-fail case.
    BOOST_TEST_EQ(uint256{opaque(-1)}, max);
    BOOST_TEST_EQ(uint256{opaque(std::int64_t{-1})}, max);
    BOOST_TEST_EQ(uint256{opaque((std::numeric_limits<std::int64_t>::min)())},
                  (max - (uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0x8000000000000000)}) + one));

    // The mixed signed paths subtract the magnitude of a negative scalar. The
    // most negative value cannot be negated inside its own type, so the scalar
    // result must equal promoting the scalar to uint256 first.
    const auto m {opaque((std::numeric_limits<std::int64_t>::min)())};
    const uint256 m256 {m};
    const auto a {opaque(uint256{UINT64_C(42), UINT64_C(0), UINT64_C(1234), UINT64_C(7)})};
    const auto b {opaque(uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(5)})};

    BOOST_TEST_EQ(a + m, a + m256);
    BOOST_TEST_EQ(m + a, m256 + a);
    BOOST_TEST_EQ(a - m, a - m256);
    BOOST_TEST_EQ(m - a, m256 - a);
    BOOST_TEST_EQ(a * m, a * m256);
    BOOST_TEST_EQ(m * a, m256 * a);
    BOOST_TEST_EQ(a / m, a / m256);
    BOOST_TEST_EQ(a % m, a % m256);
    BOOST_TEST_EQ(m / b, m256 / b);
    BOOST_TEST_EQ(m % b, m256 % b);

    // A divisor whose low word is zero must not be mistaken for zero.
    const auto two_64 {opaque(uint256{UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)})};
    const auto two_128 {opaque(uint256{UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)})};
    const auto two_192 {opaque(uint256{UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)})};

    BOOST_TEST_EQ(opaque(std::uint64_t{5}) / two_64, zero);
    BOOST_TEST_EQ(opaque(std::uint64_t{5}) % two_64, uint256{5U});
    BOOST_TEST_EQ(two_128 / two_64, two_64);
    BOOST_TEST_EQ(two_192 / two_128, two_64);
    BOOST_TEST_EQ(max / two_192, uint256{UINT64_MAX});

    // Compound forms on the boundary.
    {
        uint256 t {max};
        t += one;
        BOOST_TEST_EQ(t, zero);
    }
    {
        uint256 t {zero};
        t -= one;
        BOOST_TEST_EQ(t, max);
    }
    {
        uint256 t {max};
        t *= opaque(2U);
        BOOST_TEST_EQ(t, max - one);
    }

    // Shifting the top bit out, and shifting by the largest defined count.
    const auto top {opaque(uint256{UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)})};
    BOOST_TEST_EQ(top << opaque(1), zero);
    BOOST_TEST_EQ(one << opaque(255), top);
    BOOST_TEST_EQ(top >> opaque(255), one);
    BOOST_TEST_EQ(max >> opaque(255), one);
    BOOST_TEST_EQ(max << opaque(255), top);

    // Increment and decrement across every word boundary.
    {
        uint256 t {max};
        ++t;
        BOOST_TEST_EQ(t, zero);
        --t;
        BOOST_TEST_EQ(t, max);
    }
    {
        uint256 t {opaque(uint256{UINT64_C(0), UINT64_MAX, UINT64_MAX, UINT64_MAX})};
        ++t;
        BOOST_TEST_EQ(t, two_192);
        --t;
        BOOST_TEST_EQ(t, (uint256{UINT64_C(0), UINT64_MAX, UINT64_MAX, UINT64_MAX}));
    }

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_signed_boundaries();
#endif

    return boost::report_errors();
}
