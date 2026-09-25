// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// powm, modular exponentiation. There are three specializations inside: a fully
// narrow path when everything fits one word, a shift and add mulmod when the
// modulus is wide, and the square and multiply loop on top. The modulus width is
// therefore what the cases below vary, since that is what selects the path.

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
using boost::int256::powm;

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
using boost::int256::int256;
#endif

namespace
{

std::mt19937_64 rng {42};

// A slow square and multiply reference. The intermediate products need 512 bits,
// so the reference type is uint512_t: doing this in a fixed 256-bit type would
// wrap on the squaring and quietly give the wrong answer for a wide modulus.
#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

wide reference_powm(wide base, wide exp, const wide& m)
{
    if (m == 1)
    {
        return wide{0};
    }

    base %= m;

    wide result {1};
    while (exp != 0)
    {
        if ((exp & wide{1}) != 0)
        {
            result = wide{result * base} % m;
        }
        base = wide{base * base} % m;
        exp >>= 1;
    }

    return result;
}

void check_powm(const uint256& base, const uint256& exp, const uint256& m)
{
    const auto got {powm(base, exp, m)};
    const auto expected {reference_powm(to_wide(base), to_wide(exp), to_wide(m))};

    if (to_wide(got) != expected)
    {
        std::cerr << "powm mismatch\n"
                  << "  base     = " << hex_words(base) << '\n'
                  << "  exponent = " << hex_words(exp) << '\n'
                  << "  modulus  = " << hex_words(m) << '\n'
                  << "  got      = " << hex_words(got) << '\n'
                  << "  expected = " << expected << std::endl;
    }

    BOOST_TEST(to_wide(got) == expected);

    // The result is always reduced.
    BOOST_TEST(got < m);
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE

void test_known_answers()
{
    // Small cases done by hand.
    BOOST_TEST_EQ(powm(uint256{2U}, uint256{10U}, uint256{1000U}), uint256{24U});   // 1024 mod 1000
    BOOST_TEST_EQ(powm(uint256{3U}, uint256{4U}, uint256{5U}), uint256{1U});        // 81 mod 5
    BOOST_TEST_EQ(powm(uint256{2U}, uint256{}, uint256{7U}), uint256{1U});          // anything to the zero
    BOOST_TEST_EQ(powm(uint256{}, uint256{5U}, uint256{7U}), uint256{});            // zero to a positive power
    BOOST_TEST_EQ(powm(uint256{5U}, uint256{1U}, uint256{7U}), uint256{5U});
    BOOST_TEST_EQ(powm(uint256{7U}, uint256{1U}, uint256{7U}), uint256{});

    // A modulus of one reduces everything to zero.
    BOOST_TEST_EQ(powm(uint256{12345U}, uint256{6789U}, uint256{1U}), uint256{});
    BOOST_TEST_EQ(powm(uint256{}, uint256{}, uint256{1U}), uint256{});

    // Fermat: a^(p-1) is 1 modulo a prime p that does not divide a.
    const std::uint64_t primes[] {UINT64_C(7), UINT64_C(101), UINT64_C(65537),
                                  UINT64_C(2147483647), UINT64_C(1000000007)};
    for (const auto p : primes)
    {
        const uint256 modulus {p};
        for (std::uint64_t a {2}; a < 10U; ++a)
        {
            if (a % p != 0U)
            {
                BOOST_TEST_EQ(powm(uint256{a}, modulus - 1U, modulus), uint256{1U});
            }
        }
    }

    // A wide modulus, where the shift and add mulmod is the path taken. 2^127 - 1
    // is a Mersenne prime, so Fermat applies there too.
    {
        const uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
        const uint256 mersenne {(one << 127U) - 1U};
        BOOST_TEST_EQ(powm(uint256{2U}, mersenne - 1U, mersenne), one);
        BOOST_TEST_EQ(powm(uint256{3U}, mersenne - 1U, mersenne), one);
        BOOST_TEST_EQ(powm(uint256{7U}, mersenne - 1U, mersenne), one);

        // And 2^127 mod (2^127 - 1) is 1, so 2^127 is its own answer.
        BOOST_TEST_EQ(powm(uint256{2U}, uint256{127U}, mersenne), one);
    }
}

// The modulus width is what selects the implementation, so each width gets its
// own draws.
void test_every_modulus_width()
{
    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 moduli[] {
            uint256{(rng() % 1000U) + 2U},                                        // tiny
            uint256{rng() | UINT64_C(1)},                                         // one word
            uint256{UINT64_C(0), UINT64_C(0), rng() | UINT64_C(1), rng()},        // two words
            uint256{UINT64_C(0), rng() | UINT64_C(1), rng(), rng()},              // three words
            uint256{rng() | UINT64_C(1), rng(), rng(), rng()}                     // four words
        };

        for (const auto& m : moduli)
        {
            if (m < 2U)
            {
                continue;
            }

            const auto base {random_uint256(rng, shape::word4)};
            const auto exp {random_uint256(rng, shape::word1)};

            check_powm(base, exp, m);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    // Corpus operands, including a base and modulus at the extremes.
    for (const auto& m : edge_values())
    {
        if (m < 2U)
        {
            continue;
        }

        check_powm(uint256{2U}, uint256{10U}, m);
        check_powm(uint256_max(), uint256{3U}, m);
        check_powm(uint256{3U}, uint256_max(), m);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
    #endif
}

void test_constexpr()
{
    static_assert(powm(uint256{2U}, uint256{10U}, uint256{1000U}) == uint256{24U}, "2^10 mod 1000");
    static_assert(powm(uint256{3U}, uint256{4U}, uint256{5U}) == uint256{1U}, "3^4 mod 5");
    static_assert(powm(uint256{2U}, uint256{}, uint256{7U}) == uint256{1U}, "the zero exponent");
    static_assert(powm(uint256{5U}, uint256{3U}, uint256{1U}) == uint256{}, "a modulus of one");
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

//=====================================
// int256: m <= 0 or exp < 0 gives 0; a negative base is reduced into [0, m)
// first, so the result is always the non-negative residue, never a
// negative one that a plain % of a negative base would have produced
//=====================================

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

// The same square and multiply reference, worked entirely in the wide signed
// domain: base is normalized into [0, m) up front so the loop itself only
// ever multiplies non-negative values, exactly like the library's own
// contract for what it hands back.
smp reference_powm_i(smp base, const smp& exp, const smp& m)
{
    if (m <= 0 || exp < 0)
    {
        return smp{0};
    }

    if (m == 1)
    {
        return smp{0};
    }

    base %= m;
    if (base < 0)
    {
        base += m;
    }

    smp result {1};
    smp e {exp};

    while (e != 0)
    {
        if ((e & smp{1}) != 0)
        {
            result = (result * base) % m;
        }

        base = (base * base) % m;
        e >>= 1;
    }

    return result;
}

void check_powm_i(const int256& base, const int256& exp, const int256& m)
{
    const auto got {powm(base, exp, m)};
    const auto expected {reference_powm_i(to_smp(base), to_smp(exp), to_smp(m))};

    BOOST_TEST(to_smp(got) == expected ||
               report_mismatch_i("powm", base, exp, got, expected));

    // The result is always the non-negative residue: 0 <= got < m whenever m
    // is a real (positive) modulus, and exactly 0 otherwise.
    if (m > 0)
    {
        BOOST_TEST(got >= int256{});
        BOOST_TEST(got < m);
    }
    else
    {
        BOOST_TEST(got == int256{});
    }
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE

void test_known_answers_i()
{
    BOOST_TEST(powm(int256{2}, int256{10}, int256{1000}) == int256{24});
    BOOST_TEST(powm(int256{3}, int256{4}, int256{5}) == int256{1});

    // A negative base: -2 mod 1000 (the residue, not the truncated remainder)
    // is 998, and 998^1 mod 1000 is 998.
    BOOST_TEST(powm(int256{-2}, int256{1}, int256{1000}) == int256{998});

    // (-3)^4 mod 5: -3 reduces to 2 modulo 5, and 2^4 = 16, 16 mod 5 = 1,
    // matching (-3)^4 = 81 = 16*5 + 1 mathematically.
    BOOST_TEST(powm(int256{-3}, int256{4}, int256{5}) == int256{1});

    // A negative exponent or a non-positive modulus both give 0, whatever the
    // base is.
    BOOST_TEST(powm(int256{5}, int256{-1}, int256{7}) == int256{});
    BOOST_TEST(powm(int256{-5}, int256{-1}, int256{7}) == int256{});
    BOOST_TEST(powm(int256{5}, int256{3}, int256{}) == int256{});
    BOOST_TEST(powm(int256{5}, int256{3}, int256{-7}) == int256{});

    // The zero exponent is 1 modulo anything greater than 1, whatever the
    // base's sign is.
    BOOST_TEST(powm(int256{-2}, int256{}, int256{7}) == int256{1});
    BOOST_TEST(powm(int256{2}, int256{}, int256{7}) == int256{1});

    // A modulus of one reduces everything to zero.
    BOOST_TEST(powm(int256{-12345}, int256{6789}, int256{1}) == int256{});

    // base == MIN (-2^255): -2^255 mod 1000 is 1000 - (2^255 mod 1000), i.e.
    // 1000 - 968 = 32, not 968 itself (2^255 mod 1000 is the magnitude's
    // residue, and MIN is negative). abs(MIN) wrapping back to MIN's own bit
    // pattern is exactly what lets the library compute that magnitude at
    // all; this is the same abs(MIN) contract the gcd/lcm and
    // saturating_cast tests pin, checked here from the other direction.
    const int256 min_i {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    BOOST_TEST(powm(min_i, int256{1}, int256{1000}) == int256{32});
}

void test_every_modulus_width_i()
{
    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    for (std::size_t i {0}; i < N; ++i)
    {
        const int256 moduli[] {
            int256{static_cast<std::int64_t>((rng() % 1000U) + 2U)},                                 // tiny
            int256{static_cast<std::int64_t>(rng() | UINT64_C(1))},                                  // one word (kept positive: top bit forced clear below)
            int256{INT64_C(0), UINT64_C(0), rng() | UINT64_C(1), rng()},                              // two words
            int256{INT64_C(0), rng() | UINT64_C(1), rng(), rng()}                                     // three words
        };

        for (const auto& m : moduli)
        {
            if (m < int256{2})
            {
                continue;
            }

            const auto base {random_int256(rng, shape::word4, sign::mixed)};
            const auto exp {random_int256(rng, shape::word1, sign::pos)};

            check_powm_i(base, exp, m);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    // Corpus operands, a positive modulus only (m <= 0 is already covered by
    // the known-answer cases above), including MIN and MAX as the base.
    for (const auto& m : signed_edge_values())
    {
        if (m < int256{2})
        {
            continue;
        }

        check_powm_i(int256{2}, int256{10}, m);
        check_powm_i(int256{-2}, int256{10}, m);

        for (const auto& base : {signed_edge_values().front(), signed_edge_values().back()})
        {
            check_powm_i(base, int256{3}, m);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
    #endif
}

void test_constexpr_i()
{
    static_assert(powm(int256{2}, int256{10}, int256{1000}) == int256{24}, "2^10 mod 1000");
    static_assert(powm(int256{-2}, int256{1}, int256{1000}) == int256{998}, "a negative base reduces into [0, m)");
    static_assert(powm(int256{5}, int256{-1}, int256{7}) == int256{}, "a negative exponent gives 0");
    static_assert(powm(int256{5}, int256{3}, int256{}) == int256{}, "a non-positive modulus gives 0");
    static_assert(powm(int256{5}, int256{3}, int256{-7}) == int256{}, "a negative modulus gives 0");
    static_assert(powm(int256{-2}, int256{}, int256{7}) == int256{1}, "the zero exponent, negative base");
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_constexpr();
    test_known_answers();
    test_every_modulus_width();

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_constexpr_i();
    test_known_answers_i();
    test_every_modulus_width_i();
    #endif

    return boost::report_errors();
}
