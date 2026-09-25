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

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
using boost::int256::int256;
using boost::int256::abs;
#endif

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
constexpr uint256 two_64 {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)};
constexpr uint256 two_128 {UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)};

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
constexpr int256 min_i {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
constexpr int256 max_i {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
#endif

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

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

//=====================================
// int256: gcd and lcm work on magnitudes
//=====================================

bool report_gcd_i(const char* what, const int256& a, const int256& b,
                   const int256& got, const int256& expected)
{
    std::cerr << "int256 gcd/lcm mismatch in " << what << '\n'
              << "  a        = " << hex_words(a) << '\n'
              << "  b        = " << hex_words(b) << '\n'
              << "  got      = " << hex_words(got) << '\n'
              << "  expected = " << hex_words(expected) << std::endl;

    return false;
}

// A slow Euclidean gcd on the magnitudes, sharing no code with either the
// binary algorithm under test or the signed-to-unsigned wrapper it sits
// behind (reference_gcd is itself independent of the unsigned binary gcd).
int256 reference_gcd_i(const int256& a, const int256& b)
{
    return to_signed_bits(reference_gcd(to_unsigned_bits(abs(a)), to_unsigned_bits(abs(b))));
}

// gcd(MIN, 0): abs(MIN) wraps back to MIN's own bit pattern (its magnitude,
// 2^255, does not fit in int256), so the "magnitude" gcd works with is that
// same bit pattern, and converting the unsigned result back reinterprets it
// as MIN again. This is the exact defect class Boost.Int128 has (dividing by
// an unreduced abs(MIN)); int256's gcd/lcm must not repeat it.
void test_min_defect()
{
    BOOST_TEST(gcd(min_i, int256{}) == min_i);
    BOOST_TEST(gcd(int256{}, min_i) == min_i);
    BOOST_TEST(gcd(min_i, min_i) == min_i);

    // MIN against a value whose magnitude does not divide 2^255 (any odd
    // value): the magnitudes are coprime, so gcd is 1, not a wrapped value.
    BOOST_TEST(gcd(min_i, int256{3}) == int256{1});
    BOOST_TEST(gcd(min_i, int256{-3}) == int256{1});

    // MIN against a power of two smaller than 2^255: the magnitude divides
    // 2^255 exactly, so gcd is that power of two.
    BOOST_TEST(gcd(min_i, int256{4}) == int256{4});

    // lcm(MIN, 1): the magnitude lcm is 2^255 (1 divides everything), and
    // converting that back to signed reinterprets it as MIN, exactly like
    // the doc comment on boost::int256::lcm(int256, int256) describes.
    BOOST_TEST(lcm(min_i, int256{1}) == min_i);
    BOOST_TEST(lcm(min_i, min_i) == min_i);

    // lcm(MIN, 0) is 0, matching every other lcm-with-zero case: gcd/lcm's
    // magnitude wrapper never even reaches the MIN special case here because
    // lcm(x, 0) short circuits to 0 in the unsigned implementation.
    BOOST_TEST(lcm(min_i, int256{}) == int256{});
    BOOST_TEST(lcm(int256{}, min_i) == int256{});
}

void test_zero_and_one_i()
{
    const int256 v {-42};

    BOOST_TEST(gcd(v, int256{}) == int256{42});
    BOOST_TEST(gcd(int256{}, v) == int256{42});
    BOOST_TEST(gcd(int256{}, int256{}) == int256{});

    BOOST_TEST(gcd(v, int256{1}) == int256{1});
    BOOST_TEST(gcd(int256{1}, v) == int256{1});

    BOOST_TEST(lcm(v, int256{}) == int256{});
    BOOST_TEST(lcm(int256{}, v) == int256{});
    BOOST_TEST(lcm(v, int256{1}) == int256{42});
}

void test_known_answers_i()
{
    // Every sign combination of the unsigned known answers: gcd/lcm only
    // ever look at magnitudes, so the sign of the operands must not affect
    // the magnitude of the result.
    BOOST_TEST(gcd(int256{12}, int256{18}) == int256{6});
    BOOST_TEST(gcd(int256{-12}, int256{18}) == int256{6});
    BOOST_TEST(gcd(int256{12}, int256{-18}) == int256{6});
    BOOST_TEST(gcd(int256{-12}, int256{-18}) == int256{6});

    BOOST_TEST(lcm(int256{4}, int256{6}) == int256{12});
    BOOST_TEST(lcm(int256{-4}, int256{6}) == int256{12});
    BOOST_TEST(lcm(int256{4}, int256{-6}) == int256{12});
    BOOST_TEST(lcm(int256{-4}, int256{-6}) == int256{12});
}

//=====================================
// The defining properties, checked against the unsigned magnitude and the
// independent Euclidean reference
//=====================================

void check_properties_i(const int256& a, const int256& b)
{
    const auto g {gcd(a, b)};

    // Symmetric.
    BOOST_TEST(g == gcd(b, a));

    // Agrees with an independent Euclidean gcd run on the magnitudes.
    BOOST_TEST(g == reference_gcd_i(a, b) ||
               report_gcd_i("signed gcd vs reference", a, b, g, reference_gcd_i(a, b)));

    // gcd(a, a) is |a| for every a, including MIN: abs(MIN) is MIN itself
    // (its true magnitude does not fit), and the round trip through the
    // unsigned magnitude and back is an identity on that same bit pattern.
    BOOST_TEST(gcd(a, a) == abs(a));

    const auto l {lcm(a, b)};

    if (a == 0 || b == 0)
    {
        BOOST_TEST(l == int256{});
        return;
    }

    // The magnitude wrapper: |lcm(a, b)| (as a bit pattern, which is what
    // wrapping means once the true magnitude does not fit) equals the
    // already-verified unsigned lcm of the magnitudes.
    const auto expected_bits {lcm(to_unsigned_bits(abs(a)), to_unsigned_bits(abs(b)))};
    BOOST_TEST(to_unsigned_bits(l) == expected_bits);
}

void test_corpus_i()
{
    for (const auto& a : signed_edge_values())
    {
        for (const auto& b : signed_edge_values())
        {
            check_properties_i(a, b);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_random_i()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        check_properties_i(random_int256(rng, shape::word4, sign::mixed),
                            random_int256(rng, shape::word4, sign::mixed));

        // Draws sharing a planted factor, one sign combination at a time, so
        // the interesting (non-coprime) answers show up reliably.
        {
            const int256 factor {static_cast<std::int64_t>((rng() >> 1) | 1U)};
            const auto a {factor * int256{static_cast<std::int64_t>(rng() >> 40)}};
            const auto b {factor * int256{static_cast<std::int64_t>(rng() >> 40)}};

            const auto g {gcd(a, b)};
            BOOST_TEST(g % abs(factor) == int256{});
            BOOST_TEST(g == reference_gcd_i(a, b) ||
                       report_gcd_i("gcd with a planted factor", a, b, g, reference_gcd_i(a, b)));
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_constexpr_i()
{
    static_assert(gcd(int256{12}, int256{18}) == int256{6}, "gcd(12, 18)");
    static_assert(gcd(int256{-12}, int256{18}) == int256{6}, "gcd(-12, 18)");
    static_assert(gcd(int256{}, int256{5}) == int256{5}, "gcd(0, 5)");
    static_assert(lcm(int256{4}, int256{6}) == int256{12}, "lcm(4, 6)");
    static_assert(lcm(int256{-4}, int256{6}) == int256{12}, "lcm(-4, 6)");
    static_assert(lcm(int256{}, int256{6}) == int256{}, "lcm(0, 6)");

    // The MIN defect check, at compile time too.
    static_assert(gcd(min_i, int256{}) == min_i, "gcd(MIN, 0) == MIN");
    static_assert(lcm(min_i, int256{1}) == min_i, "lcm(MIN, 1) == MIN");
    static_assert(gcd(max_i, int256{1}) == int256{1}, "gcd(MAX, 1)");
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_constexpr();
    test_zero_and_one();
    test_known_answers();
    test_wide_common_factors();
    test_corpus();
    test_random();

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_constexpr_i();
    test_min_defect();
    test_zero_and_one_i();
    test_known_answers_i();
    test_corpus_i();
    test_random_i();
    #endif

    return boost::report_errors();
}
