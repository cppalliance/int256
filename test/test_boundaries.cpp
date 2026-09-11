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

} // namespace

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

    return boost::report_errors();
}
