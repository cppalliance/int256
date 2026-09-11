// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Each division block of detail/common_div.hpp on its own, rather than only
// through operator/. The dispatcher picks a different algorithm per divisor
// width and the reciprocal path is gated to run time, so a block that is only
// reachable in one configuration would otherwise go untested.
//
// Every directed vector here was recomputed against boost::multiprecision
// before being written down.

#include "oracle.hpp"

#ifdef BOOST_INT256_ORACLE_UNAVAILABLE

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <boost/int256/detail/common_div.hpp>

#include <cstddef>
#include <cstdint>

using boost::int256::uint256;

namespace detail = boost::int256::detail;

namespace
{

std::mt19937_64 rng {42};

mp mp_from_words(const std::uint64_t (&w)[4])
{
    mp r {w[3]};
    r <<= 64;
    r |= mp{w[2]};
    r <<= 64;
    r |= mp{w[1]};
    r <<= 64;
    r |= mp{w[0]};

    return r;
}

std::string words_string(const std::uint64_t (&w)[4])
{
    char buffer[96] {};
    static_cast<void>(std::snprintf(buffer, sizeof(buffer), "{0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX}",
                                    static_cast<unsigned long long>(w[0]), static_cast<unsigned long long>(w[1]),
                                    static_cast<unsigned long long>(w[2]), static_cast<unsigned long long>(w[3])));

    return std::string{buffer};
}

// Number of significant 64-bit words, which is the m and n Algorithm D wants.
std::size_t significant_words(const std::uint64_t (&w)[4])
{
    for (std::size_t i {4}; i > 0U; --i)
    {
        if (w[i - 1U] != 0U)
        {
            return i;
        }
    }

    return 0U;
}

//=====================================
// divlu and udiv_2by1 (128 / 64)
//=====================================

// Both have the precondition u1 < d, which is what keeps the quotient inside one
// word. divlu is the portable two digit routine; udiv_2by1 is the same division
// through a hardware instruction where one exists.
void check_2by1(std::uint64_t u1, std::uint64_t u0, std::uint64_t d)
{
    const mp dividend {(mp{u1} << 64) | mp{u0}};
    const mp expected_q {dividend / mp{d}};
    const mp expected_r {dividend % mp{d}};

    std::uint64_t portable_r {};
    const auto portable_q {detail::divlu(u1, u0, d, portable_r)};

    std::uint64_t dispatched_r {};
    const auto dispatched_q {detail::udiv_2by1(u1, u0, d, dispatched_r)};

    BOOST_TEST(mp{portable_q} == expected_q);
    BOOST_TEST(mp{portable_r} == expected_r);
    BOOST_TEST(mp{dispatched_q} == expected_q);
    BOOST_TEST(mp{dispatched_r} == expected_r);

    // The two must agree with each other, which is the point of having both.
    BOOST_TEST_EQ(portable_q, dispatched_q);
    BOOST_TEST_EQ(portable_r, dispatched_r);
}

void test_2by1()
{
    // Directed: the smallest and largest legal u1 for a given divisor, and a
    // divisor with and without its top bit set.
    const std::uint64_t divisors[] {
        UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(10),
        UINT64_C(0x100000000), UINT64_MAX, UINT64_MAX - 1,
        UINT64_C(0x8000000000000000), UINT64_C(0x8000000000000001),
        UINT64_C(0x7FFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFF)
    };

    for (const auto d : divisors)
    {
        check_2by1(UINT64_C(0), UINT64_C(0), d);
        check_2by1(UINT64_C(0), UINT64_MAX, d);
        check_2by1(d - 1U, UINT64_C(0), d);
        check_2by1(d - 1U, UINT64_MAX, d);
        check_2by1(d / 2U, UINT64_C(0x0123456789ABCDEF), d);
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        auto d {rng()};
        if (d == 0U)
        {
            d = 1U;
        }

        // The precondition is u1 < d, so the high word is reduced into range.
        const auto u1 {rng() % d};
        const auto u0 {rng()};

        check_2by1(u1, u0, d);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

//=====================================
// half_word_div (256 / 32)
//=====================================

void check_half_word(const uint256& dividend, std::uint32_t divisor)
{
    const mp expected_q {to_mp(dividend) / mp{divisor}};
    const mp expected_r {to_mp(dividend) % mp{divisor}};

    uint256 quotient {};
    std::uint64_t remainder {};
    detail::half_word_div(dividend, divisor, quotient, remainder);

    BOOST_TEST(to_mp(quotient) == expected_q ||
               report_mismatch("half_word_div quotient", dividend, quotient, expected_q));
    BOOST_TEST(mp{remainder} == expected_r);
}

void test_half_word_div()
{
    const std::uint32_t divisors[] {1U, 2U, 3U, 7U, 10U, 0xFFU, 0xFFFFU, 0x80000000U, 0xFFFFFFFFU};

    for (const auto& dividend : edge_values())
    {
        for (const auto d : divisors)
        {
            check_half_word(dividend, d);
        }
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        auto d {static_cast<std::uint32_t>(rng())};
        if (d == 0U)
        {
            d = 1U;
        }

        check_half_word(random_uint256(rng, shape::word4), d);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

//=====================================
// one_word_div (256 / 64)
//=====================================

void check_one_word(const uint256& dividend, std::uint64_t divisor)
{
    const mp expected_q {to_mp(dividend) / mp{divisor}};
    const mp expected_r {to_mp(dividend) % mp{divisor}};

    uint256 quotient {};
    std::uint64_t remainder {};
    detail::one_word_div(dividend, divisor, quotient, remainder);

    BOOST_TEST(to_mp(quotient) == expected_q ||
               report_mismatch("one_word_div quotient", dividend, quotient, expected_q));
    BOOST_TEST(mp{remainder} == expected_r);

    // The quotient only overload must agree.
    uint256 quotient_only {};
    detail::one_word_div(dividend, divisor, quotient_only);
    BOOST_TEST_EQ(quotient_only, quotient);
}

void test_one_word_div()
{
    // A divisor whose high half is zero forces the same value through both the
    // 64-bit and the 32-bit chain, so the two tiers can be compared.
    const std::uint64_t divisors[] {
        UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(0xFF), UINT64_C(10),
        UINT64_C(0xFFFFFFFF), UINT64_C(0x100000000),
        UINT64_C(0x8000000000000000), UINT64_MAX, UINT64_MAX - 1
    };

    for (const auto& dividend : edge_values())
    {
        for (const auto d : divisors)
        {
            check_one_word(dividend, d);

            // Where the divisor also fits 32 bits the two tiers must agree.
            if (d <= UINT32_MAX)
            {
                uint256 narrow_q {};
                std::uint64_t narrow_r {};
                detail::half_word_div(dividend, static_cast<std::uint32_t>(d), narrow_q, narrow_r);

                uint256 wide_q {};
                std::uint64_t wide_r {};
                detail::one_word_div(dividend, d, wide_q, wide_r);

                BOOST_TEST_EQ(narrow_q, wide_q);
                BOOST_TEST_EQ(narrow_r, wide_r);
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        auto d {rng()};
        if (d == 0U)
        {
            d = 1U;
        }

        check_one_word(random_uint256(rng, shape::word4), d);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

//=====================================
// knuth_divide (Algorithm D on 64-bit words)
//=====================================

bool check_knuth(const char* name, const std::uint64_t (&u_words)[4], const std::uint64_t (&v_words)[4])
{
    const auto m {significant_words(u_words)};
    const auto n {significant_words(v_words)};

    if (n < 2U || m < n)
    {
        return true;
    }

    const mp dividend {mp_from_words(u_words)};
    const mp divisor {mp_from_words(v_words)};
    const mp expected_q {dividend / divisor};
    const mp expected_r {dividend % divisor};

    std::uint64_t u[5] {u_words[0], u_words[1], u_words[2], u_words[3], UINT64_C(0)};
    const std::uint64_t v[4] {v_words[0], v_words[1], v_words[2], v_words[3]};
    std::uint64_t q[4] {};

    detail::knuth_divide<true>(u, m, v, n, q);

    const std::uint64_t remainder_words[4] {u[0], u[1], u[2], u[3]};

    const bool quotient_ok {mp_from_words(q) == expected_q};
    const bool remainder_ok {mp_from_words(remainder_words) == expected_r};

    if (!quotient_ok || !remainder_ok)
    {
        std::cerr << "knuth_divide mismatch in " << name << " (m = " << m << ", n = " << n << ")\n"
                  << "  u        = " << words_string(u_words) << '\n'
                  << "  v        = " << words_string(v_words) << '\n'
                  << "  q got    = " << words_string(q) << '\n'
                  << "  q expect = " << hex_words(from_mp(expected_q)) << '\n'
                  << "  r got    = " << words_string(remainder_words) << '\n'
                  << "  r expect = " << hex_words(from_mp(expected_r)) << std::endl;
    }

    BOOST_TEST(quotient_ok);
    BOOST_TEST(remainder_ok);

    // Without the remainder the quotient must be identical and u is scratch.
    std::uint64_t u2[5] {u_words[0], u_words[1], u_words[2], u_words[3], UINT64_C(0)};
    std::uint64_t q2[4] {};
    detail::knuth_divide<false>(u2, m, v, n, q2);

    for (std::size_t i {0}; i < 4U; ++i)
    {
        BOOST_TEST_EQ(q2[i], q[i]);
    }

    return quotient_ok && remainder_ok;
}

// The directed vectors from Appendix B.7. Each one drives a branch that random
// operands reach with probability around 2^-64: the add-back step of D.6, and
// the one and two step corrections of the qhat estimate in D.3.
void test_knuth_directed()
{
    struct vector_case
    {
        const char* name;
        std::uint64_t u[4];
        std::uint64_t v[4];
        std::uint64_t q[4];
        std::uint64_t r[4];
    };

    const vector_case cases[] {
        {"add-back n=3 m=4 s=0",
         {UINT64_C(0xDF9CA78E107486F9), UINT64_C(0xE815A127E9280B25), UINT64_C(0xB8A6F8B95B6BE70F), UINT64_C(0x7EED8D14F06D3FEE)},
         {UINT64_C(0x38C0C8FD8712B8BC), UINT64_C(0x701966A0C381E88F), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0)},
         {UINT64_C(0x7EED8D14F06D3FEE), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0xFD155CC3F98A8431), UINT64_C(0x68B624656B02160E), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0)}},
        {"add-back n=3 m=4 s=1",
         {UINT64_C(0x712E2ECE870316BB), UINT64_C(0x71D97619F1FD1567), UINT64_C(0xE8DCD3E4B30BD5E4), UINT64_C(0x4739E523F5485477)},
         {UINT64_C(0x05805975ED2F89D9), UINT64_C(0xD66B829E6A8AC4BA), UINT64_C(0x7FFFFFFFFFFFFFFF), UINT64_C(0)},
         {UINT64_C(0x8E73CA47EA90A8EF), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0x6112F222011AFD24), UINT64_C(0xD4EEC63250F1267A), UINT64_C(0x7FFFFFFFFFFFFFFF), UINT64_C(0)}},
        {"add-back n=4 m=4 s=63",
         {UINT64_C(0xCDEA7AF112146E41), UINT64_C(0xE2E41B269ACA8F7A), UINT64_C(0x5FF5FCBF714377FC), UINT64_C(0x304F17E4ED2129E3)},
         {UINT64_C(0x10B8FE223C116549), UINT64_C(0x0A57AF35B9B81635), UINT64_C(0x220D672B15AD9A9D), UINT64_C(1)},
         {UINT64_C(0x2AA3300B2B711342), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0x86F0F1D452DAE66F), UINT64_C(0x01C425A1833300C9), UINT64_C(0x220D672B15AD9A9D), UINT64_C(1)}},
        {"qhat corrected once n=2 m=2",
         {UINT64_C(0xF2A30B19629D6D6F), UINT64_C(0x5CB20247798F61EE), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0x42806AF673C815E3), UINT64_C(1), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0x4994BFBAFA02B11A), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0x1AB537266F634161), UINT64_C(1), UINT64_C(0), UINT64_C(0)}},
        {"qhat twice n=2 m=3",
         {UINT64_C(0xD9ADDB29B95A3816), UINT64_C(0xB760915E63523814), UINT64_C(0x6B851B79C6A4FCE6), UINT64_C(0)},
         {UINT64_C(0xC2D4404073D1DA3A), UINT64_C(0x8349E2715252E7A5), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0xD1A746FE259A8D85), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0xC0C9978F4F3DE5F4), UINT64_C(0x6C697624C5067102), UINT64_C(0), UINT64_C(0)}},
        {"qhat twice n=2 m=4",
         {UINT64_C(0xBCE8C423CDA85457), UINT64_C(0x79DD5D6E194051E5), UINT64_C(0x7043597B6978C10C), UINT64_C(0x87505ED75802EB74)},
         {UINT64_C(0xEA2E79B73FE44ACF), UINT64_C(0x83906B77E65F942C), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0xCBF9AA6DF589B2FC), UINT64_C(0x074BE3E6D0864E71), UINT64_C(1), UINT64_C(0)},
         {UINT64_C(0xAD188C6D0923C293), UINT64_C(0x7357A64C358CA64D), UINT64_C(0), UINT64_C(0)}},
        {"qhat twice n=3 m=3",
         {UINT64_C(0x486CF046FAF7368E), UINT64_C(0xCA149E0BC62154FB), UINT64_C(0xBE9596C9072A7E43), UINT64_C(0)},
         {UINT64_C(0xDAAF9BCC58159900), UINT64_C(0x04F6C6DE51EC28E5), UINT64_C(1), UINT64_C(0)},
         {UINT64_C(0xBAF5877E95A49046), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0xCB4816F7BAFF608E), UINT64_C(0xD1F7075DBCDD1E14), UINT64_C(0), UINT64_C(0)}},
        {"qhat twice n=3 m=4",
         {UINT64_C(0xAA1027FFF6C5BC36), UINT64_C(0x7641BA583B6BC914), UINT64_C(0x0602E705CA7271A1), UINT64_C(0x7FC3D1ABBFE9A0C5)},
         {UINT64_C(0xD8259701AEC6EAE9), UINT64_C(0xF7CA13BABA0A367B), UINT64_C(0x8AB084F6F9BD5AE6), UINT64_C(0)},
         {UINT64_C(0xEBD5B64420C3BF20), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0x2EDCFB9DA5298816), UINT64_C(0xA7E218270CF5E7FA), UINT64_C(0x38D71876E034981E), UINT64_C(0)}},
        {"qhat twice n=4 m=4",
         {UINT64_C(0x76980A53AF78D6C7), UINT64_C(0x37223898B7BB5134), UINT64_C(0xD8306564B515B990), UINT64_C(0xE5441F6167C5BBE6)},
         {UINT64_C(0xF9F16E0B96C65FAA), UINT64_C(0xAB893A9A80CA18B8), UINT64_C(0x0082A217CC1C7961), UINT64_C(1)},
         {UINT64_C(0xE4CF5D3395891695), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
         {UINT64_C(0xBDED07EABDD08CD5), UINT64_C(0x7C2A092EB1C9071F), UINT64_C(0x952C9E2FA3B6146D), UINT64_C(0)}}
    };

    for (const auto& c : cases)
    {
        // Against the reference, and against the quotient and remainder that were
        // written down with the vector.
        static_cast<void>(check_knuth(c.name, c.u, c.v));

        const uint256 dividend {c.u[3], c.u[2], c.u[1], c.u[0]};
        const uint256 divisor {c.v[3], c.v[2], c.v[1], c.v[0]};
        const uint256 expected_q {c.q[3], c.q[2], c.q[1], c.q[0]};
        const uint256 expected_r {c.r[3], c.r[2], c.r[1], c.r[0]};

        BOOST_TEST_EQ(dividend / divisor, expected_q);
        BOOST_TEST_EQ(dividend % divisor, expected_r);
    }
}

// Every legal operand shape, with the normalization shift driven to 0, 1 and 63.
// A shift of 0 skips the shifting branch entirely; a shift of 63 moves every word.
void test_knuth_shapes()
{
    for (std::size_t n {2}; n <= 4U; ++n)
    {
        for (std::size_t m {n}; m <= 4U; ++m)
        {
            for (std::size_t i {0}; i < N; ++i)
            {
                std::uint64_t u[4] {};
                std::uint64_t v[4] {};

                for (std::size_t w {0}; w < m; ++w)
                {
                    u[w] = rng();
                }
                for (std::size_t w {0}; w + 1U < n; ++w)
                {
                    v[w] = rng();
                }

                // The top word of the divisor decides the normalization shift.
                const auto which {i % 4U};
                if (which == 0U)
                {
                    v[n - 1U] = rng() | (UINT64_C(1) << 63);        // shift 0
                }
                else if (which == 1U)
                {
                    v[n - 1U] = (rng() >> 1) | (UINT64_C(1) << 62); // shift 1
                }
                else if (which == 2U)
                {
                    v[n - 1U] = UINT64_C(1);                        // shift 63
                }
                else
                {
                    v[n - 1U] = (rng() >> (rng() % 63U)) | UINT64_C(1);
                }

                // Algorithm D needs the top word of the dividend non-zero too.
                u[m - 1U] |= UINT64_C(1);

                if (significant_words(u) < significant_words(v))
                {
                    continue;
                }

                if (!check_knuth("random shape", u, v))
                {
                    std::cerr << "  requested m = " << m << ", n = " << n << std::endl;
                    return;
                }
            }
        }
    }

    // The clamp of D.3: the top word of the running remainder equals the top word
    // of the normalized divisor, so the true estimate is 2^64.
    for (std::size_t n {2}; n <= 4U; ++n)
    {
        std::uint64_t v[4] {};
        for (std::size_t w {0}; w + 1U < n; ++w)
        {
            v[w] = rng();
        }
        v[n - 1U] = UINT64_MAX;

        std::uint64_t u[4] {};
        for (std::size_t w {0}; w < 4U; ++w)
        {
            u[w] = UINT64_MAX;
        }

        static_cast<void>(check_knuth("qhat clamp", u, v));

        // And the shift 0 and shift 63 pairs the plan calls out explicitly.
        std::uint64_t all_ones[4] {};
        std::uint64_t power[4] {};
        for (std::size_t w {0}; w < n; ++w)
        {
            all_ones[w] = UINT64_MAX;
        }
        power[n - 1U] = UINT64_C(1);

        static_cast<void>(check_knuth("max / (2^(64n) - 1), shift 0", u, all_ones));
        static_cast<void>(check_knuth("max / 2^(64(n-1)), shift 63", u, power));
    }
}

//=====================================
// The Moller and Granlund reciprocal path (runtime only)
//=====================================

#ifndef BOOST_INT256_NO_CONSTEVAL_DETECTION

// Moller and Granlund define the reciprocal of a normalized two word divisor d
// as v = floor((B^3 - 1) / d) - B with B = 2^64. Since d is in [2^127, 2^128),
// that quotient lands in (2^64, 2^65], so the term subtracted is 2^64 and not
// 2^128. Verified empirically against the implementation before being asserted.
void test_mg_reciprocal()
{
    const std::uint64_t low_words[] {
        UINT64_C(0), UINT64_C(1), UINT64_MAX, UINT64_C(0x0123456789ABCDEF)
    };

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto d1 {rng() | (UINT64_C(1) << 63)};
        const auto d0 {low_words[i % 4U]};

        const auto v {detail::mg_reciprocal_3by2(d1, d0)};

        const mp divisor {(mp{d1} << 64) | mp{d0}};
        const mp numerator {(mp{1} << 192) - 1};
        const mp expected {mp{numerator / divisor} - (mp{1} << 64)};

        BOOST_TEST(mp{v} == expected);

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  d1 = " << d1 << ", d0 = " << d0 << std::endl;
            return;
        }
    }
}

// One 3-by-2 step, with the preconditions the caller guarantees: the divisor is
// normalized and the top two words of the dividend are below it.
void test_mg_div_3by2()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto d1 {rng() | (UINT64_C(1) << 63)};
        const auto d0 {rng()};
        const auto v {detail::mg_reciprocal_3by2(d1, d0)};

        // (u2:u1) < (d1:d0) is the precondition, so the top two words are
        // reduced modulo the divisor.
        const mp divisor {(mp{d1} << 64) | mp{d0}};
        const mp top {mp{(mp{rng()} << 64) | mp{rng()}} % divisor};

        const auto u2 {static_cast<std::uint64_t>(mp{top >> 64})};
        const auto u1 {static_cast<std::uint64_t>(mp{top & mp{UINT64_MAX}})};
        const auto u0 {rng()};

        std::uint64_t r1 {};
        std::uint64_t r0 {};
        const auto q {detail::mg_div_3by2(u2, u1, u0, d1, d0, v, r1, r0)};

        const mp dividend {(mp{u2} << 128) | (mp{u1} << 64) | mp{u0}};
        BOOST_TEST(mp{q} == mp{dividend / divisor});
        BOOST_TEST(((mp{r1} << 64) | mp{r0}) == mp{dividend % divisor});

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// The whole 256 / 128 path, which is what the dispatcher calls for a two word
// divisor at run time. Normalizing into five words means nothing is rejected.
void check_mg_divide(const uint256& dividend, const uint256& divisor)
{
    const mp expected_q {to_mp(dividend) / to_mp(divisor)};
    const mp expected_r {to_mp(dividend) % to_mp(divisor)};

    uint256 quotient {};
    uint256 remainder {};
    detail::mg_divide_2word(dividend, divisor.words[1], divisor.words[0], quotient, remainder);

    BOOST_TEST(to_mp(quotient) == expected_q ||
               report_mismatch("mg_divide_2word quotient", dividend, divisor, quotient, expected_q));
    BOOST_TEST(to_mp(remainder) == expected_r ||
               report_mismatch("mg_divide_2word remainder", dividend, divisor, remainder, expected_r));
}

void test_mg_divide_2word()
{
    // The three curated 256 / 128 vectors from the Boost.Decimal test suite, which
    // were chosen to hit the hard to reach carry lines. Recomputed here.
    const uint256 curated_top[] {
        uint256{UINT64_C(0x01AC01E281D83F28), UINT64_C(0x698C19FD72AA8085), UINT64_C(0x78F0CD3B0CD2FF5D), UINT64_C(0xD6A0A4DB3233D019)},
        uint256{UINT64_C(0x009E1F4B3859275E), UINT64_C(0xE297AFBAB4ADB30B), UINT64_C(0xFAE7A9D4CAF5672E), UINT64_C(0xB279A59B9906070C)},
        uint256{UINT64_C(0x000B6F4866E326CC), UINT64_C(0x1321EAE5369D68E5), UINT64_C(0x824E7315340514AB), UINT64_C(0x6EF6D107ECB8BC38)}
    };

    const uint256 curated_bottom[] {
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x292FFA3C03F252D4), UINT64_C(0x42D1483A455B4281)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x2AA34E5021771CBC), UINT64_C(0x4EB1EFC17289FA09)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x718E8C8F6DBB6F76), UINT64_C(0x597BD68B19ACF237)}
    };

    for (std::size_t i {0}; i < 3U; ++i)
    {
        check_mg_divide(curated_top[i], curated_bottom[i]);

        // The same division through the operator must agree.
        BOOST_TEST_EQ(curated_top[i] / curated_bottom[i], from_mp(to_mp(curated_top[i]) / to_mp(curated_bottom[i])));
        BOOST_TEST_EQ(curated_top[i] % curated_bottom[i], from_mp(to_mp(curated_top[i]) % to_mp(curated_bottom[i])));
    }

    // Every normalization shift, including 0 and 63, and a divisor whose low word
    // is zero.
    const std::uint64_t high_words[] {
        UINT64_C(1),                        // shift 63
        UINT64_C(2),                        // shift 62
        UINT64_C(0x4000000000000000),       // shift 1
        UINT64_C(0x8000000000000000),       // shift 0
        UINT64_MAX                          // shift 0, largest
    };

    for (const auto d1 : high_words)
    {
        for (const auto& dividend : edge_values())
        {
            const uint256 divisor {UINT64_C(0), UINT64_C(0), d1, UINT64_C(0)};
            if (to_mp(dividend) >= to_mp(divisor))
            {
                check_mg_divide(dividend, divisor);
            }

            const uint256 odd_divisor {UINT64_C(0), UINT64_C(0), d1, UINT64_MAX};
            if (to_mp(dividend) >= to_mp(odd_divisor))
            {
                check_mg_divide(dividend, odd_divisor);
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 dividend {random_uint256(rng, shape::word4)};
        const uint256 divisor {UINT64_C(0), UINT64_C(0), rng() | UINT64_C(1), rng()};

        if (to_mp(dividend) >= to_mp(divisor))
        {
            check_mg_divide(dividend, divisor);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// The reciprocal path and Algorithm D must give the same answer for every two
// word divisor, which is the only thing keeping the gated fast path honest.
void test_mg_agrees_with_knuth()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 dividend {random_uint256(rng, shape::word4)};
        const uint256 divisor {UINT64_C(0), UINT64_C(0), rng() | UINT64_C(1), rng()};

        if (to_mp(dividend) < to_mp(divisor))
        {
            continue;
        }

        uint256 mg_q {};
        uint256 mg_r {};
        detail::mg_divide_2word(dividend, divisor.words[1], divisor.words[0], mg_q, mg_r);

        std::uint64_t u[5] {dividend.words[0], dividend.words[1], dividend.words[2], dividend.words[3], UINT64_C(0)};
        const std::uint64_t v[4] {divisor.words[0], divisor.words[1], UINT64_C(0), UINT64_C(0)};
        std::uint64_t q[4] {};

        std::uint64_t u_words[4] {dividend.words[0], dividend.words[1], dividend.words[2], dividend.words[3]};
        detail::knuth_divide<true>(u, significant_words(u_words), v, 2U, q);

        BOOST_TEST_EQ(mg_q, uint256(q[3], q[2], q[1], q[0]));
        BOOST_TEST_EQ(mg_r, uint256(u[3], u[2], u[1], u[0]));

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  dividend = " << hex_words(dividend) << ", divisor = " << hex_words(divisor) << std::endl;
            return;
        }
    }
}

#else

void test_mg_reciprocal() {}
void test_mg_div_3by2() {}
void test_mg_divide_2word() {}
void test_mg_agrees_with_knuth() {}

#endif // BOOST_INT256_NO_CONSTEVAL_DETECTION

//=====================================
// The dispatcher
//=====================================

void check_divide(const uint256& dividend, const uint256& divisor)
{
    const mp expected_q {to_mp(dividend) / to_mp(divisor)};
    const mp expected_r {to_mp(dividend) % to_mp(divisor)};

    uint256 quotient {};
    uint256 remainder {};
    detail::divide<true>(dividend, divisor, quotient, remainder);

    BOOST_TEST(to_mp(quotient) == expected_q ||
               report_mismatch("divide<true> quotient", dividend, divisor, quotient, expected_q));
    BOOST_TEST(to_mp(remainder) == expected_r ||
               report_mismatch("divide<true> remainder", dividend, divisor, remainder, expected_r));

    // Without the remainder the quotient must be unchanged.
    uint256 quotient_only {};
    uint256 untouched {};
    detail::divide<false>(dividend, divisor, quotient_only, untouched);
    BOOST_TEST_EQ(quotient_only, quotient);
}

void test_dispatcher_tiers()
{
    for (const auto& dividend : edge_values())
    {
        for (const auto& divisor : edge_values())
        {
            if (divisor != 0U)
            {
                check_divide(dividend, divisor);
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    // One divisor per tier, driven explicitly rather than sampled.
    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 dividend {random_uint256(rng, shape::word4)};

        const uint256 divisors[] {
            uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), (rng() & UINT64_C(0xFFFFFFFF)) | UINT64_C(1)},
            uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), rng() | (UINT64_C(1) << 63)},
            uint256{UINT64_C(0), UINT64_C(0), rng() | UINT64_C(1), rng()},
            uint256{UINT64_C(0), rng() | UINT64_C(1), rng(), rng()},
            uint256{rng() | UINT64_C(1), rng(), rng(), rng()},
            dividend,
            uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)}
        };

        for (const auto& divisor : divisors)
        {
            if (divisor != 0U)
            {
                check_divide(dividend, divisor);
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    // A divisor with a zero low word is not a one word divisor.
    const uint256 dividend {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    check_divide(dividend, uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)));
    check_divide(dividend, uint256(UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)));
    check_divide(dividend, uint256(UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)));
}

//=====================================
// Runtime against constant evaluation
//=====================================

// The reciprocal path exists only at run time, so a division evaluated at compile
// time runs Algorithm D and the same division evaluated at run time runs Moller
// and Granlund. Doing both in one translation unit is what proves the two agree.
// The runtime operands go through volatile so nothing is folded.

namespace constant_division
{

constexpr uint256 dividend_a {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
constexpr uint256 divisor_a {UINT64_C(0), UINT64_C(0), UINT64_C(0x8349E2715252E7A5), UINT64_C(0xC2D4404073D1DA3A)};
constexpr uint256 quotient_a {dividend_a / divisor_a};
constexpr uint256 remainder_a {dividend_a % divisor_a};

// A two word divisor whose top word is 1, so the normalization shift is 63.
constexpr uint256 dividend_b {UINT64_C(0x87505ED75802EB74), UINT64_C(0x7043597B6978C10C),
                              UINT64_C(0x79DD5D6E194051E5), UINT64_C(0xBCE8C423CDA85457)};
constexpr uint256 divisor_b {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0xEA2E79B73FE44ACF)};
constexpr uint256 quotient_b {dividend_b / divisor_b};
constexpr uint256 remainder_b {dividend_b % divisor_b};

// The two correction vector, whose divisor is also two words.
constexpr uint256 dividend_c {UINT64_C(0), UINT64_C(0x6B851B79C6A4FCE6), UINT64_C(0xB760915E63523814), UINT64_C(0xD9ADDB29B95A3816)};
constexpr uint256 divisor_c {UINT64_C(0), UINT64_C(0), UINT64_C(0x8349E2715252E7A5), UINT64_C(0xC2D4404073D1DA3A)};
constexpr uint256 quotient_c {dividend_c / divisor_c};
constexpr uint256 remainder_c {dividend_c % divisor_c};

static_assert(quotient_c == uint256(UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0xD1A746FE259A8D85)),
              "the two correction quotient, computed by Algorithm D at compile time");
static_assert(remainder_c == uint256(UINT64_C(0), UINT64_C(0), UINT64_C(0x6C697624C5067102), UINT64_C(0xC0C9978F4F3DE5F4)),
              "the two correction remainder");

static_assert(quotient_a * divisor_a + remainder_a == dividend_a, "the division identity holds at compile time");
static_assert(quotient_b * divisor_b + remainder_b == dividend_b, "the division identity holds at compile time");

} // namespace constant_division

uint256 opaque(const uint256& v)
{
    volatile std::uint64_t w3 {v.words[3]};
    volatile std::uint64_t w2 {v.words[2]};
    volatile std::uint64_t w1 {v.words[1]};
    volatile std::uint64_t w0 {v.words[0]};

    return uint256{w3, w2, w1, w0};
}

void test_runtime_matches_constant_evaluation()
{
    {
        const auto dividend {opaque(constant_division::dividend_a)};
        const auto divisor {opaque(constant_division::divisor_a)};
        BOOST_TEST_EQ(dividend / divisor, constant_division::quotient_a);
        BOOST_TEST_EQ(dividend % divisor, constant_division::remainder_a);
    }
    {
        const auto dividend {opaque(constant_division::dividend_b)};
        const auto divisor {opaque(constant_division::divisor_b)};
        BOOST_TEST_EQ(dividend / divisor, constant_division::quotient_b);
        BOOST_TEST_EQ(dividend % divisor, constant_division::remainder_b);
    }
    {
        const auto dividend {opaque(constant_division::dividend_c)};
        const auto divisor {opaque(constant_division::divisor_c)};
        BOOST_TEST_EQ(dividend / divisor, constant_division::quotient_c);
        BOOST_TEST_EQ(dividend % divisor, constant_division::remainder_c);
    }

    // And over random two word divisors, where the compile time answer is not
    // available but the two tiers of the dispatcher still have to agree with the
    // reference from both directions.
    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 dividend {random_uint256(rng, shape::word4)};
        const uint256 divisor {UINT64_C(0), UINT64_C(0), rng() | UINT64_C(1), rng()};

        const auto runtime_q {opaque(dividend) / opaque(divisor)};
        const auto runtime_r {opaque(dividend) % opaque(divisor)};

        BOOST_TEST(to_mp(runtime_q) == mp{to_mp(dividend) / to_mp(divisor)} ||
                   report_mismatch("runtime quotient", dividend, divisor, runtime_q,
                                   mp{to_mp(dividend) / to_mp(divisor)}));
        BOOST_TEST(to_mp(runtime_r) == mp{to_mp(dividend) % to_mp(divisor)} ||
                   report_mismatch("runtime remainder", dividend, divisor, runtime_r,
                                   mp{to_mp(dividend) % to_mp(divisor)}));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

} // namespace

int main()
{
    test_2by1();
    test_half_word_div();
    test_one_word_div();
    test_knuth_directed();
    test_knuth_shapes();
    test_mg_reciprocal();
    test_mg_div_3by2();
    test_mg_divide_2word();
    test_mg_agrees_with_knuth();
    test_dispatcher_tiers();
    test_runtime_matches_constant_evaluation();

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE
