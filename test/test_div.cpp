// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// boost::int256::div, the single call that returns both the quotient and the
// remainder. The invariants q * y + r == x and r < y hold for every divisor, and
// they do not depend on the reference being right, so they are checked on every
// operand pair alongside the comparison against multiprecision.

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
#include <type_traits>
#include <utility>

using boost::int256::uint256;
using boost::int256::div;
using boost::int256::u256div_t;
using boost::int256::int256;
using boost::int256::i256div_t;

namespace
{

std::mt19937_64 rng {42};

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

void check_div(const uint256& x, const uint256& y)
{
    const auto result {div(x, y)};

    const mp expected_q {to_mp(x) / to_mp(y)};
    const mp expected_r {to_mp(x) % to_mp(y)};

    BOOST_TEST(to_mp(result.quot) == expected_q ||
               report_mismatch("div quotient", x, y, result.quot, expected_q));
    BOOST_TEST(to_mp(result.rem) == expected_r ||
               report_mismatch("div remainder", x, y, result.rem, expected_r));

    // The two invariants, independent of the reference.
    BOOST_TEST(result.quot * y + result.rem == x || report_mismatch("q * y + r == x", x, y, result.quot, expected_q));
    BOOST_TEST(result.rem < y || report_mismatch("r < y", x, y, result.rem, expected_r));

    // div must agree with the operators.
    BOOST_TEST_EQ(result.quot, x / y);
    BOOST_TEST_EQ(result.rem, x % y);
}

// One divisor per algorithm tier, driven explicitly so a tier is never missed.
void test_divisor_classes()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 x {random_uint256(rng, shape::word4)};

        const uint256 divisors[] {
            // a 32-bit divisor: the 64 over 32 chain
            uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), (rng() & UINT64_C(0xFFFFFFFF)) | UINT64_C(1)},
            // a full 64-bit divisor: the chained 128 over 64 path
            uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), rng() | (UINT64_C(1) << 63)},
            // two words: the reciprocal path at run time, Algorithm D at compile time
            uint256{UINT64_C(0), UINT64_C(0), rng() | (UINT64_C(1) << 63), rng()},
            // two words with a small top word, so the normalization shift is large
            uint256{UINT64_C(0), UINT64_C(0), UINT64_C(1), rng()},
            // three and four words: Algorithm D
            uint256{UINT64_C(0), rng() | (UINT64_C(1) << 63), rng(), rng()},
            uint256{UINT64_C(0), UINT64_C(1), rng(), rng()},
            uint256{rng() | (UINT64_C(1) << 63), rng(), rng(), rng()},
            uint256{UINT64_C(1), rng(), rng(), rng()},
            // larger than the dividend: quotient 0, remainder the dividend
            x == uint256_max() ? uint256_max() : x + 1U,
            // equal to the dividend: quotient 1, remainder 0
            x,
            // a power of two
            uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)} << static_cast<unsigned>(rng() % 256U),
            // one
            uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)}
        };

        for (const auto& y : divisors)
        {
            if (y != 0U)
            {
                check_div(x, y);
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_corpus()
{
    for (const auto& x : edge_values())
    {
        for (const auto& y : edge_values())
        {
            if (y != 0U)
            {
                check_div(x, y);
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

#else

// Without the reference the invariant check is still meaningful on its own.
void check_div(const uint256& x, const uint256& y)
{
    const auto result {div(x, y)};

    BOOST_TEST(result.quot * y + result.rem == x);
    BOOST_TEST(result.rem < y);
    BOOST_TEST_EQ(result.quot, x / y);
    BOOST_TEST_EQ(result.rem, x % y);
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE

// The known answers, recomputed against multiprecision before being written down.
void test_directed_identities()
{
    const uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    const uint256 max {uint256_max()};
    const uint256 two_64 {one << 64U};
    const uint256 two_128 {one << 128U};
    const uint256 two_192 {one << 192U};

    {
        const auto r {div(max, two_64 - 1U)};
        BOOST_TEST_EQ(r.quot, uint256(UINT64_C(1), UINT64_C(1), UINT64_C(1), UINT64_C(1)));
        BOOST_TEST_EQ(r.rem, uint256{});
    }
    {
        const auto r {div(max, uint256(UINT64_C(1), UINT64_C(1), UINT64_C(1), UINT64_C(1)))};
        BOOST_TEST_EQ(r.quot, two_64 - 1U);
    }
    {
        // 2^256 - 1 over 2^128 - 1 is 2^128 + 1
        const auto r {div(max, two_128 - 1U)};
        BOOST_TEST_EQ(r.quot, uint256(UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(1)));
    }
    {
        const auto r {div(max, two_128 + 1U)};
        BOOST_TEST_EQ(r.quot, two_128 - 1U);
    }
    {
        const auto r {div(max, two_192 - 1U)};
        BOOST_TEST_EQ(r.quot, two_64);
        BOOST_TEST_EQ(r.rem, two_64 - 1U);
    }
    {
        const auto r {div(max, uint256{3U})};
        BOOST_TEST_EQ(r.quot, uint256(UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
                                      UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)));
        BOOST_TEST_EQ(r.rem, uint256{});
    }
    {
        const auto r {div(max, uint256{0xFFU})};
        BOOST_TEST_EQ(r.quot, uint256(UINT64_C(0x0101010101010101), UINT64_C(0x0101010101010101),
                                      UINT64_C(0x0101010101010101), UINT64_C(0x0101010101010101)));
        BOOST_TEST_EQ(r.rem, uint256{});
    }
    {
        const auto r {div(two_128, two_64)};
        BOOST_TEST_EQ(r.quot, two_64);
        BOOST_TEST_EQ(r.rem, uint256{});
    }
    {
        // x / x and x / (x + 1)
        const auto r {div(max, max)};
        BOOST_TEST_EQ(r.quot, one);
        BOOST_TEST_EQ(r.rem, uint256{});

        const auto s {div(two_128, two_128 + 1U)};
        BOOST_TEST_EQ(s.quot, uint256{});
        BOOST_TEST_EQ(s.rem, two_128);
    }
}

// The same divisions that drive the Algorithm D corrections, through div rather
// than through the primitive, so the dispatcher reaches them too.
void test_knuth_directed_through_div()
{
    struct vector_case
    {
        uint256 u;
        uint256 v;
        uint256 q;
        uint256 r;
    };

    const vector_case cases[] {
        // add-back, n = 3, m = 4, normalization shift 0
        {uint256{UINT64_C(0x7EED8D14F06D3FEE), UINT64_C(0xB8A6F8B95B6BE70F), UINT64_C(0xE815A127E9280B25), UINT64_C(0xDF9CA78E107486F9)},
         uint256{UINT64_C(0), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x701966A0C381E88F), UINT64_C(0x38C0C8FD8712B8BC)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0x7EED8D14F06D3FEE)},
         uint256{UINT64_C(0), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x68B624656B02160E), UINT64_C(0xFD155CC3F98A8431)}},
        // add-back, n = 3, m = 4, normalization shift 1
        {uint256{UINT64_C(0x4739E523F5485477), UINT64_C(0xE8DCD3E4B30BD5E4), UINT64_C(0x71D97619F1FD1567), UINT64_C(0x712E2ECE870316BB)},
         uint256{UINT64_C(0), UINT64_C(0x7FFFFFFFFFFFFFFF), UINT64_C(0xD66B829E6A8AC4BA), UINT64_C(0x05805975ED2F89D9)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0x8E73CA47EA90A8EF)},
         uint256{UINT64_C(0), UINT64_C(0x7FFFFFFFFFFFFFFF), UINT64_C(0xD4EEC63250F1267A), UINT64_C(0x6112F222011AFD24)}},
        // add-back, n = 4, m = 4, normalization shift 63
        {uint256{UINT64_C(0x304F17E4ED2129E3), UINT64_C(0x5FF5FCBF714377FC), UINT64_C(0xE2E41B269ACA8F7A), UINT64_C(0xCDEA7AF112146E41)},
         uint256{UINT64_C(1), UINT64_C(0x220D672B15AD9A9D), UINT64_C(0x0A57AF35B9B81635), UINT64_C(0x10B8FE223C116549)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0x2AA3300B2B711342)},
         uint256{UINT64_C(1), UINT64_C(0x220D672B15AD9A9D), UINT64_C(0x01C425A1833300C9), UINT64_C(0x86F0F1D452DAE66F)}},
        // one qhat correction, n = 2, m = 2
        {uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x5CB20247798F61EE), UINT64_C(0xF2A30B19629D6D6F)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0x42806AF673C815E3)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0x4994BFBAFA02B11A)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0x1AB537266F634161)}},
        // two qhat corrections, n = 2, m = 3
        {uint256{UINT64_C(0), UINT64_C(0x6B851B79C6A4FCE6), UINT64_C(0xB760915E63523814), UINT64_C(0xD9ADDB29B95A3816)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x8349E2715252E7A5), UINT64_C(0xC2D4404073D1DA3A)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0xD1A746FE259A8D85)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x6C697624C5067102), UINT64_C(0xC0C9978F4F3DE5F4)}},
        // two qhat corrections, n = 2, m = 4, quotient spills into a third word
        {uint256{UINT64_C(0x87505ED75802EB74), UINT64_C(0x7043597B6978C10C), UINT64_C(0x79DD5D6E194051E5), UINT64_C(0xBCE8C423CDA85457)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x83906B77E65F942C), UINT64_C(0xEA2E79B73FE44ACF)},
         uint256{UINT64_C(0), UINT64_C(1), UINT64_C(0x074BE3E6D0864E71), UINT64_C(0xCBF9AA6DF589B2FC)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x7357A64C358CA64D), UINT64_C(0xAD188C6D0923C293)}},
        // two qhat corrections, n = 3, m = 3
        {uint256{UINT64_C(0), UINT64_C(0xBE9596C9072A7E43), UINT64_C(0xCA149E0BC62154FB), UINT64_C(0x486CF046FAF7368E)},
         uint256{UINT64_C(0), UINT64_C(1), UINT64_C(0x04F6C6DE51EC28E5), UINT64_C(0xDAAF9BCC58159900)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0xBAF5877E95A49046)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0xD1F7075DBCDD1E14), UINT64_C(0xCB4816F7BAFF608E)}},
        // two qhat corrections, n = 3, m = 4
        {uint256{UINT64_C(0x7FC3D1ABBFE9A0C5), UINT64_C(0x0602E705CA7271A1), UINT64_C(0x7641BA583B6BC914), UINT64_C(0xAA1027FFF6C5BC36)},
         uint256{UINT64_C(0), UINT64_C(0x8AB084F6F9BD5AE6), UINT64_C(0xF7CA13BABA0A367B), UINT64_C(0xD8259701AEC6EAE9)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0xEBD5B64420C3BF20)},
         uint256{UINT64_C(0), UINT64_C(0x38D71876E034981E), UINT64_C(0xA7E218270CF5E7FA), UINT64_C(0x2EDCFB9DA5298816)}},
        // two qhat corrections, n = 4, m = 4
        {uint256{UINT64_C(0xE5441F6167C5BBE6), UINT64_C(0xD8306564B515B990), UINT64_C(0x37223898B7BB5134), UINT64_C(0x76980A53AF78D6C7)},
         uint256{UINT64_C(1), UINT64_C(0x0082A217CC1C7961), UINT64_C(0xAB893A9A80CA18B8), UINT64_C(0xF9F16E0B96C65FAA)},
         uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0xE4CF5D3395891695)},
         uint256{UINT64_C(0), UINT64_C(0x952C9E2FA3B6146D), UINT64_C(0x7C2A092EB1C9071F), UINT64_C(0xBDED07EABDD08CD5)}}
    };

    for (const auto& c : cases)
    {
        const auto r {div(c.u, c.v)};

        if (r.quot != c.q || r.rem != c.r)
        {
            std::cerr << "directed division mismatch\n"
                      << "  u        = " << hex_words(c.u) << '\n'
                      << "  v        = " << hex_words(c.v) << '\n'
                      << "  q got    = " << hex_words(r.quot) << '\n'
                      << "  q expect = " << hex_words(c.q) << '\n'
                      << "  r got    = " << hex_words(r.rem) << '\n'
                      << "  r expect = " << hex_words(c.r) << std::endl;
        }

        BOOST_TEST(r.quot == c.q);
        BOOST_TEST(r.rem == c.r);

        check_div(c.u, c.v);
    }
}

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

void check_div_i(const int256& x, const int256& y)
{
    const auto result {div(x, y)};

    const smp mx {to_smp(x)};
    const smp my {to_smp(y)};

    BOOST_TEST(result.quot == from_smp_wrapped(mx / my) ||
               report_mismatch_i("i256div quotient", x, y, result.quot, mx / my));
    BOOST_TEST(result.rem == from_smp_wrapped(mx % my) ||
               report_mismatch_i("i256div remainder", x, y, result.rem, mx % my));

    // The identity, independent of the reference.
    BOOST_TEST(result.quot * y + result.rem == x);

    // div must agree with the operators.
    BOOST_TEST(result.quot == x / y);
    BOOST_TEST(result.rem == x % y);
}

// The quotient's sign is sign(x) != sign(y); the remainder takes the sign of
// the dividend. MIN as either operand is exercised explicitly: MIN / -1 wraps
// back to MIN (the mathematical quotient 2^255 is not representable), and a
// small dividend against MIN as the divisor is the int128 defect A shape
// (a careless abs(MIN) that is still MIN must not divide by a zero
// intermediate).
void test_signed_directed_identities()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    {
        const auto r {div(min_v, int256{-1})};
        BOOST_TEST(r.quot == min_v);
        BOOST_TEST(r.rem == int256{0});
    }
    {
        const auto r {div(max_v, int256{-1})};
        BOOST_TEST(r.quot == -max_v);
        BOOST_TEST(r.rem == int256{0});
    }
    {
        const auto r {div(int256{-7}, int256{2})};
        BOOST_TEST(r.quot == int256{-3});
        BOOST_TEST(r.rem == int256{-1});
    }
    {
        const auto r {div(int256{7}, int256{-2})};
        BOOST_TEST(r.quot == int256{-3});
        BOOST_TEST(r.rem == int256{1});
    }
    {
        const auto r {div(int256{-7}, int256{-2})};
        BOOST_TEST(r.quot == int256{3});
        BOOST_TEST(r.rem == int256{-1});
    }

    // A small dividend divided by MIN: quotient 0, remainder the dividend.
    const int256 small_dividends[] {
        int256{0}, int256{1}, int256{-1}, int256{5}, int256{-5}, int256{INT64_MAX}, int256{INT64_MIN + 1}
    };
    for (const auto& d : small_dividends)
    {
        const auto r {div(d, min_v)};
        BOOST_TEST(r.quot == int256{0});
        BOOST_TEST(r.rem == d);
    }

    // MIN against itself: quotient 1, remainder 0.
    {
        const auto r {div(min_v, min_v)};
        BOOST_TEST(r.quot == int256{1});
        BOOST_TEST(r.rem == int256{0});
    }
}

void test_signed_corpus()
{
    for (const auto& x : signed_edge_values())
    {
        for (const auto& y : signed_edge_values())
        {
            if (y != 0)
            {
                check_div_i(x, y);
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_signed_random()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const int256 x {random_int256(rng, shape::word4, sign::mixed)};
        const int256 y {random_int256(rng, shape::word4, sign::mixed)};

        if (y != 0)
        {
            check_div_i(x, y);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE

// i256div_t is the same shape as u256div_t, with int256 members.
void test_signed_struct_shape()
{
    static_assert(std::is_same<decltype(std::declval<i256div_t>().quot), int256>::value, "quot is int256");
    static_assert(std::is_same<decltype(std::declval<i256div_t>().rem), int256>::value, "rem is int256");
}

// div is usable in a constant expression for int256 too.
void test_signed_constexpr()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr auto result {div(min_v, int256{-1})};

    static_assert(result.quot == min_v, "MIN / -1 wraps to MIN at compile time");
    static_assert(result.rem == int256{0}, "MIN % -1 == 0 at compile time");
}

// div is usable in a constant expression, which routes through Algorithm D.
void test_constexpr()
{
    constexpr uint256 x {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    constexpr uint256 y {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)};
    constexpr auto result {div(x, y)};

    // (2^256 - 1) / 2^64 is 2^192 - 1, so the low three words are all ones.
    static_assert(result.quot == uint256(UINT64_C(0), UINT64_MAX, UINT64_MAX, UINT64_MAX), "MAX / 2^64");
    static_assert(result.rem == uint256(UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_MAX), "MAX % 2^64");
    static_assert(result.quot * y + result.rem == x, "the identity holds at compile time");

    BOOST_TEST_EQ(result.quot, x / y);
    BOOST_TEST_EQ(result.rem, x % y);
}

} // namespace

int main()
{
    test_directed_identities();
    test_knuth_directed_through_div();
    test_constexpr();

    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    test_corpus();
    test_divisor_classes();
    #endif

    test_signed_struct_shape();
    test_signed_constexpr();

    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    test_signed_directed_identities();
    test_signed_corpus();
    test_signed_random();
    #endif

    return boost::report_errors();
}
