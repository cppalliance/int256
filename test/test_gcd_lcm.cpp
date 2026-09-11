// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// gcd and lcm. The implementation is a binary (Stein) gcd that drops into a
// 64-bit routine once both operands are narrow enough, so the cases that matter
// are the ones where the common factor is itself wider than a word: Boost.Int128
// returns 1 for gcd(2^64 + 1, 2^64 + 1), which is what a premature narrowing
// looks like. Wide common factors are therefore tested first here.

#ifndef BOOST_INT256_BUILD_MODULE

#include <boost/int256.hpp>

#else

import boost.int256;
// A module import carries no macros, so the BOOST_INT256_* feature and endianness
// macros have to come from the header even in the module build.
#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/literal_macros.hpp>

#endif

#include "oracle.hpp"

#include <cstddef>
#include <cstdint>

using boost::int256::uint256;
using boost::int256::gcd;
using boost::int256::lcm;

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
constexpr uint256 two_64 {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)};
constexpr uint256 two_128 {UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)};

// Failure reporter that does not need the reference, so the gcd checks stay active
// in the module build. hex_words is exported by oracle.hpp unconditionally.
bool report_gcd(const char* what, const uint256& a, const uint256& b,
                const uint256& got, const uint256& expected)
{
    std::cerr << "int256 gcd mismatch in " << what << '\n'
              << "  a        = " << hex_words(a) << '\n'
              << "  b        = " << hex_words(b) << '\n'
              << "  got      = " << hex_words(got) << '\n'
              << "  expected = " << hex_words(expected) << std::endl;

    return false;
}

// A slow Euclidean gcd on the library type, which shares no code with the binary
// algorithm under test.
uint256 reference_gcd(uint256 a, uint256 b)
{
    while (b != 0U)
    {
        const auto remainder {a % b};
        a = b;
        b = remainder;
    }

    return a;
}

//=====================================
// Wide common factors
//=====================================

// Every one of these has a common factor that does not fit in a single word, so a
// gcd that narrows too early answers 1.
void test_wide_common_factors()
{
    // The exact case Boost.Int128 gets wrong.
    {
        const uint256 v {two_64 + 1U};
        BOOST_TEST_EQ(gcd(v, v), v);
        BOOST_TEST_EQ(gcd(v, v * 2U), v);
        BOOST_TEST_EQ(gcd(v * 3U, v * 5U), v);
        BOOST_TEST_EQ(gcd(v * 7U, v * 7U), v * 7U);
    }

    // 3 * 2^64, whose common factor with 2^64 is 2^64.
    {
        const uint256 a {two_64 * 3U};
        BOOST_TEST_EQ(gcd(a, two_64), two_64);
        BOOST_TEST_EQ(gcd(two_64, a), two_64);
        BOOST_TEST_EQ(gcd(a, a), a);
        BOOST_TEST_EQ(gcd(a, two_64 * 6U), a);
    }

    // Powers of two: the gcd is the smaller one.
    for (unsigned i {0}; i < 256U; ++i)
    {
        for (unsigned j {0}; j < 256U; j += 37U)
        {
            const auto smaller {i < j ? i : j};
            BOOST_TEST_EQ(gcd(one << i, one << j), one << smaller);
        }
    }

    // A common factor with all four words set.
    {
        const uint256 factor {UINT64_C(0x0123456789ABCDEF), UINT64_C(0xFEDCBA9876543210),
                              UINT64_C(0x1122334455667788), UINT64_C(0x99AABBCCDDEEFF01)};
        BOOST_TEST_EQ(gcd(factor, factor), factor);

        // factor * 2 wraps, so use a value that shares the factor without wrapping.
        const uint256 narrow_factor {UINT64_C(0), UINT64_C(0x0123456789ABCDEF),
                                     UINT64_C(0xFEDCBA9876543210), UINT64_C(0x1122334455667789)};
        BOOST_TEST_EQ(gcd(narrow_factor, narrow_factor), narrow_factor);
        BOOST_TEST_EQ(gcd(narrow_factor, narrow_factor * 2U), narrow_factor);
        BOOST_TEST_EQ(gcd(narrow_factor * 3U, narrow_factor * 6U), narrow_factor * 3U);
    }

    // 2^128 and 2^128 + 1 are coprime, so the answer really is 1 here.
    BOOST_TEST_EQ(gcd(two_128, two_128 + 1U), one);
    BOOST_TEST_EQ(gcd(two_64, two_64 + 1U), one);
    BOOST_TEST_EQ(gcd(two_64 - 1U, two_64), one);

    // And the maximum against a wide factor of itself. 2^256 - 1 is divisible by
    // 2^64 - 1, verified against multiprecision.
    BOOST_TEST_EQ(gcd(uint256_max(), two_64 - 1U), two_64 - 1U);
    BOOST_TEST_EQ(gcd(uint256_max(), two_128 - 1U), two_128 - 1U);
}

void test_zero_and_one()
{
    const uint256 v {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(42)};

    // gcd(x, 0) is x, in both orders.
    BOOST_TEST_EQ(gcd(v, uint256{}), v);
    BOOST_TEST_EQ(gcd(uint256{}, v), v);
    BOOST_TEST_EQ(gcd(uint256{}, uint256{}), uint256{});
    BOOST_TEST_EQ(gcd(uint256_max(), uint256{}), uint256_max());

    // gcd(x, 1) is 1.
    BOOST_TEST_EQ(gcd(v, one), one);
    BOOST_TEST_EQ(gcd(one, v), one);
    BOOST_TEST_EQ(gcd(one, one), one);

    // lcm with zero is zero.
    BOOST_TEST_EQ(lcm(v, uint256{}), uint256{});
    BOOST_TEST_EQ(lcm(uint256{}, v), uint256{});
    BOOST_TEST_EQ(lcm(uint256{}, uint256{}), uint256{});

    // lcm with one is the other operand.
    BOOST_TEST_EQ(lcm(v, one), v);
    BOOST_TEST_EQ(lcm(one, v), v);
}

void test_known_answers()
{
    BOOST_TEST_EQ(gcd(uint256{12U}, uint256{18U}), uint256{6U});
    BOOST_TEST_EQ(gcd(uint256{18U}, uint256{12U}), uint256{6U});
    BOOST_TEST_EQ(gcd(uint256{17U}, uint256{5U}), one);
    BOOST_TEST_EQ(gcd(uint256{100U}, uint256{75U}), uint256{25U});
    BOOST_TEST_EQ(gcd(uint256{270U}, uint256{192U}), uint256{6U});

    BOOST_TEST_EQ(lcm(uint256{4U}, uint256{6U}), uint256{12U});
    BOOST_TEST_EQ(lcm(uint256{12U}, uint256{18U}), uint256{36U});
    BOOST_TEST_EQ(lcm(uint256{17U}, uint256{5U}), uint256{85U});
    BOOST_TEST_EQ(lcm(uint256{7U}, uint256{7U}), uint256{7U});
}

//=====================================
// The defining properties
//=====================================

void check_properties(const uint256& a, const uint256& b)
{
    const auto g {gcd(a, b)};

    // Symmetric.
    BOOST_TEST_EQ(g, gcd(b, a));

    // Agrees with a slow Euclidean gcd.
    BOOST_TEST(g == reference_gcd(a, b) ||
               report_gcd("gcd", a, b, g, reference_gcd(a, b)));

    if (g != 0U)
    {
        // Divides both operands.
        BOOST_TEST_EQ(a % g, uint256{});
        BOOST_TEST_EQ(b % g, uint256{});

        // And dividing both by it leaves them coprime.
        BOOST_TEST_EQ(gcd(a / g, b / g), (a == 0U && b == 0U) ? uint256{} : one);
    }
    else
    {
        BOOST_TEST_EQ(a, uint256{});
        BOOST_TEST_EQ(b, uint256{});
    }

    // gcd(a, a) is a.
    BOOST_TEST_EQ(gcd(a, a), a);

    // lcm is (a / gcd) * b, which is only meaningful while it fits: the library
    // documents no saturation, so a wrapped product says nothing and is skipped.
    const auto l {lcm(a, b)};

    if (a == 0U || b == 0U)
    {
        BOOST_TEST_EQ(l, uint256{});
        return;
    }

    const uint256 reduced {a / g};
    const bool fits {reduced <= uint256_max() / b};

    if (fits)
    {
        BOOST_TEST_EQ(l, reduced * b);

        // The least common multiple is a multiple of both operands.
        BOOST_TEST_EQ(l % a, uint256{});
        BOOST_TEST_EQ(l % b, uint256{});

        // And it is the least one: no proper divisor of it that is still a multiple
        // of a is also a multiple of b. Checking l / p for the small primes is
        // enough to catch an lcm that is a multiple too large.
        for (const std::uint64_t p : {UINT64_C(2), UINT64_C(3), UINT64_C(5)})
        {
            const uint256 prime {p};
            if (l % prime == 0U)
            {
                const uint256 smaller {l / prime};
                BOOST_TEST(!(smaller % a == 0U && smaller % b == 0U && smaller != 0U));
            }
        }
    }
}

void test_corpus()
{
    for (const auto& a : edge_values())
    {
        for (const auto& b : edge_values())
        {
            check_properties(a, b);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_random()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        // Independent draws, which are usually coprime.
        check_properties(random_uint256(rng, shape::word4), random_uint256(rng, shape::word4));

        // And draws sharing a planted factor, which is where the interesting
        // answers are. The factor is kept narrow enough that the products do not
        // wrap.
        {
            const uint256 factor {UINT64_C(0), UINT64_C(0), UINT64_C(0), rng() | UINT64_C(1)};
            const uint256 a {factor * uint256{rng() >> 1}};
            const uint256 b {factor * uint256{rng() >> 1}};

            const auto g {gcd(a, b)};
            BOOST_TEST_EQ(g % factor, uint256{});
            BOOST_TEST(g == reference_gcd(a, b) ||
                       report_gcd("gcd with a planted factor", a, b, g, reference_gcd(a, b)));
        }

        // A factor wider than one word, which is the case that catches an early
        // narrowing.
        {
            const uint256 factor {UINT64_C(0), UINT64_C(0), rng() | UINT64_C(1), rng()};
            const auto a {factor * uint256{(rng() % 1000U) + 1U}};
            const auto b {factor * uint256{(rng() % 1000U) + 1U}};

            const auto g {gcd(a, b)};
            BOOST_TEST_EQ(g % factor, uint256{});
            BOOST_TEST(g == reference_gcd(a, b) ||
                       report_gcd("gcd with a wide planted factor", a, b, g, reference_gcd(a, b)));
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_constexpr()
{
    static_assert(gcd(uint256{12U}, uint256{18U}) == uint256{6U}, "gcd(12, 18)");
    static_assert(gcd(uint256{}, uint256{5U}) == uint256{5U}, "gcd(0, 5)");
    static_assert(gcd(uint256{5U}, uint256{}) == uint256{5U}, "gcd(5, 0)");
    static_assert(lcm(uint256{4U}, uint256{6U}) == uint256{12U}, "lcm(4, 6)");
    static_assert(lcm(uint256{}, uint256{6U}) == uint256{}, "lcm(0, 6)");

    // A wide common factor at compile time.
    constexpr uint256 wide {two_64 + 1U};
    static_assert(gcd(wide, wide) == wide, "gcd of a wide value with itself");
    static_assert(gcd(two_64, two_64 * 3U) == two_64, "gcd(2^64, 3 * 2^64)");
    static_assert(gcd(two_128, two_128 + 1U) == uint256{1U}, "2^128 and 2^128 + 1 are coprime");
}

} // namespace

int main()
{
    test_constexpr();
    test_zero_and_one();
    test_known_answers();
    test_wide_common_factors();
    test_corpus();
    test_random();

    return boost::report_errors();
}
