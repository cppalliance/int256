// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Differential test of boost::int256::uint256 against
// boost::multiprecision::uint256_t. The reference is a fixed 256-bit unchecked
// type, so +, -, *, << and the compound forms all wrap modulo 2^256 exactly as
// the library does (verified: max + 1 == 0, max * 2 == max - 1, ~0 == max).
// The reference has no unary minus for an unsigned type, so negation is checked
// against 0 - x.
//
// Operand pairs are drawn from every combination of the ten operand shapes so
// that word-count and normalization edges are hit deliberately. Accepts
// --iterations N to set the total number of random pairs.
//
// Each operator family sits behind its own switch so that the file still builds
// while an operator is still being written. All of them are on for the finished
// library; turn one off on the command line to isolate a regression.

#ifndef BOOST_INT256_TEST_OP_ADD
#  define BOOST_INT256_TEST_OP_ADD 1
#endif
#ifndef BOOST_INT256_TEST_OP_SUB
#  define BOOST_INT256_TEST_OP_SUB 1
#endif
#ifndef BOOST_INT256_TEST_OP_MUL
#  define BOOST_INT256_TEST_OP_MUL 1
#endif
#ifndef BOOST_INT256_TEST_OP_DIV
#  define BOOST_INT256_TEST_OP_DIV 1
#endif
#ifndef BOOST_INT256_TEST_OP_MOD
#  define BOOST_INT256_TEST_OP_MOD 1
#endif
#ifndef BOOST_INT256_TEST_OP_SHIFT
#  define BOOST_INT256_TEST_OP_SHIFT 1
#endif
#ifndef BOOST_INT256_TEST_OP_BITWISE
#  define BOOST_INT256_TEST_OP_BITWISE 1
#endif
#ifndef BOOST_INT256_TEST_OP_UNARY
#  define BOOST_INT256_TEST_OP_UNARY 1
#endif
#ifndef BOOST_INT256_TEST_OP_COMPARE
#  define BOOST_INT256_TEST_OP_COMPARE 1
#endif
#ifndef BOOST_INT256_TEST_OP_COMPOUND
#  define BOOST_INT256_TEST_OP_COMPOUND 1
#endif
#ifndef BOOST_INT256_TEST_OP_INCDEC
#  define BOOST_INT256_TEST_OP_INCDEC 1
#endif

#include "oracle.hpp"

#ifdef BOOST_INT256_ORACLE_UNAVAILABLE

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <cstdlib>
#include <cstring>

using boost::int256::uint256;

namespace
{

// Failure reporter for the boolean comparisons, which have no 256-bit result to
// print. Always returns false so it can sit on the right of a short-circuit.
bool report_pair(const char* what, const uint256& lhs, const uint256& rhs)
{
    std::cerr << "int256 differential mismatch in " << what << '\n'
              << "  lhs = " << hex_words(lhs) << '\n'
              << "  rhs = " << hex_words(rhs) << std::endl;

    return false;
}

bool report_shift(const char* what, const uint256& operand, unsigned count,
                  const uint256& got, const mp& expected)
{
    std::cerr << "int256 differential mismatch in " << what << " by " << count << '\n'
              << "  operand  = " << hex_words(operand) << '\n'
              << "  got      = " << hex_words(got) << '\n'
              << "  expected = " << hex_words(expected) << std::endl;

    return false;
}

void check_arithmetic(const uint256& a, const uint256& b)
{
    const mp ma {to_mp(a)};
    const mp mb {to_mp(b)};

    #if BOOST_INT256_TEST_OP_ADD
    {
        const uint256 got {a + b};
        BOOST_INT256_TEST_BINARY("operator+", a, b, got, mp{ma + mb});
    }
    #endif

    #if BOOST_INT256_TEST_OP_SUB
    {
        const uint256 got {a - b};
        BOOST_INT256_TEST_BINARY("operator-", a, b, got, mp{ma - mb});
    }
    #endif

    #if BOOST_INT256_TEST_OP_MUL
    {
        const uint256 got {a * b};
        BOOST_INT256_TEST_BINARY("operator*", a, b, got, mp{ma * mb});
    }
    #endif

    #if BOOST_INT256_TEST_OP_DIV
    if (b != 0U)
    {
        const uint256 got {a / b};
        BOOST_INT256_TEST_BINARY("operator/", a, b, got, mp{ma / mb});
    }
    #endif

    #if BOOST_INT256_TEST_OP_MOD
    if (b != 0U)
    {
        const uint256 got {a % b};
        BOOST_INT256_TEST_BINARY("operator%", a, b, got, mp{ma % mb});
    }
    #endif

    // The division identity is the strongest single statement about the divide
    // path, and it does not depend on the reference being right.
    #if BOOST_INT256_TEST_OP_DIV && BOOST_INT256_TEST_OP_MOD && BOOST_INT256_TEST_OP_MUL && BOOST_INT256_TEST_OP_ADD
    if (b != 0U)
    {
        const uint256 q {a / b};
        const uint256 r {a % b};
        BOOST_TEST(q * b + r == a || report_pair("q * b + r == a", a, b));
        BOOST_TEST(r < b || report_pair("r < b", a, b));
    }
    #endif
}

void check_bitwise(const uint256& a, const uint256& b)
{
    #if BOOST_INT256_TEST_OP_BITWISE
    const mp ma {to_mp(a)};
    const mp mb {to_mp(b)};

    {
        const uint256 got {a & b};
        BOOST_INT256_TEST_BINARY("operator&", a, b, got, mp{ma & mb});
    }
    {
        const uint256 got {a | b};
        BOOST_INT256_TEST_BINARY("operator|", a, b, got, mp{ma | mb});
    }
    {
        const uint256 got {a ^ b};
        BOOST_INT256_TEST_BINARY("operator^", a, b, got, mp{ma ^ mb});
    }
    #else
    static_cast<void>(a);
    static_cast<void>(b);
    #endif
}

void check_unary(const uint256& a)
{
    #if BOOST_INT256_TEST_OP_UNARY
    const mp ma {to_mp(a)};

    {
        const uint256 got {~a};
        BOOST_INT256_TEST_UNARY("operator~", a, got, mp{~ma});
    }
    {
        // The reference rejects unary minus on an unsigned type, so the two's
        // complement result is compared against 0 - x.
        const uint256 got {-a};
        BOOST_INT256_TEST_UNARY("unary operator-", a, got, mp{mp{0} - ma});
    }
    {
        const uint256 got {+a};
        BOOST_INT256_TEST_UNARY("unary operator+", a, got, ma);
    }
    #else
    static_cast<void>(a);
    #endif
}

void check_shifts(const uint256& a, unsigned count)
{
    #if BOOST_INT256_TEST_OP_SHIFT
    const mp ma {to_mp(a)};

    {
        const uint256 got {a << count};
        BOOST_TEST(to_mp(got) == mp{ma << count} || report_shift("operator<<", a, count, got, mp{ma << count}));
    }
    {
        const uint256 got {a >> count};
        BOOST_TEST(to_mp(got) == mp{ma >> count} || report_shift("operator>>", a, count, got, mp{ma >> count}));
    }
    #else
    static_cast<void>(a);
    static_cast<void>(count);
    #endif
}

void check_compare(const uint256& a, const uint256& b)
{
    #if BOOST_INT256_TEST_OP_COMPARE
    const mp ma {to_mp(a)};
    const mp mb {to_mp(b)};

    BOOST_TEST((a == b) == (ma == mb) || report_pair("operator==", a, b));
    BOOST_TEST((a != b) == (ma != mb) || report_pair("operator!=", a, b));
    BOOST_TEST((a <  b) == (ma <  mb) || report_pair("operator<",  a, b));
    BOOST_TEST((a <= b) == (ma <= mb) || report_pair("operator<=", a, b));
    BOOST_TEST((a >  b) == (ma >  mb) || report_pair("operator>",  a, b));
    BOOST_TEST((a >= b) == (ma >= mb) || report_pair("operator>=", a, b));

    // The reversed forms exercise the other member of each operator pair.
    BOOST_TEST((b == a) == (mb == ma) || report_pair("operator== reversed", a, b));
    BOOST_TEST((b <  a) == (mb <  ma) || report_pair("operator< reversed",  a, b));
    BOOST_TEST((b <= a) == (mb <= ma) || report_pair("operator<= reversed", a, b));
    #else
    static_cast<void>(a);
    static_cast<void>(b);
    #endif
}

void check_compound(const uint256& a, const uint256& b, unsigned count)
{
    #if BOOST_INT256_TEST_OP_COMPOUND
    const mp ma {to_mp(a)};
    const mp mb {to_mp(b)};

    #if BOOST_INT256_TEST_OP_ADD
    {
        uint256 t {a};
        t += b;
        BOOST_INT256_TEST_BINARY("operator+=", a, b, t, mp{ma + mb});
    }
    #endif
    #if BOOST_INT256_TEST_OP_SUB
    {
        uint256 t {a};
        t -= b;
        BOOST_INT256_TEST_BINARY("operator-=", a, b, t, mp{ma - mb});
    }
    #endif
    #if BOOST_INT256_TEST_OP_MUL
    {
        uint256 t {a};
        t *= b;
        BOOST_INT256_TEST_BINARY("operator*=", a, b, t, mp{ma * mb});
    }
    #endif
    #if BOOST_INT256_TEST_OP_DIV
    if (b != 0U)
    {
        uint256 t {a};
        t /= b;
        BOOST_INT256_TEST_BINARY("operator/=", a, b, t, mp{ma / mb});
    }
    #endif
    #if BOOST_INT256_TEST_OP_MOD
    if (b != 0U)
    {
        uint256 t {a};
        t %= b;
        BOOST_INT256_TEST_BINARY("operator%=", a, b, t, mp{ma % mb});
    }
    #endif
    #if BOOST_INT256_TEST_OP_BITWISE
    {
        uint256 t {a};
        t &= b;
        BOOST_INT256_TEST_BINARY("operator&=", a, b, t, mp{ma & mb});
    }
    {
        uint256 t {a};
        t |= b;
        BOOST_INT256_TEST_BINARY("operator|=", a, b, t, mp{ma | mb});
    }
    {
        uint256 t {a};
        t ^= b;
        BOOST_INT256_TEST_BINARY("operator^=", a, b, t, mp{ma ^ mb});
    }
    #endif
    #if BOOST_INT256_TEST_OP_SHIFT
    {
        uint256 t {a};
        t <<= count;
        BOOST_TEST(to_mp(t) == mp{ma << count} || report_shift("operator<<=", a, count, t, mp{ma << count}));
    }
    {
        uint256 t {a};
        t >>= count;
        BOOST_TEST(to_mp(t) == mp{ma >> count} || report_shift("operator>>=", a, count, t, mp{ma >> count}));
    }
    #endif
    #else
    static_cast<void>(a);
    static_cast<void>(b);
    static_cast<void>(count);
    #endif
}

void check_incdec(const uint256& a)
{
    #if BOOST_INT256_TEST_OP_INCDEC
    const mp ma {to_mp(a)};

    {
        uint256 t {a};
        const uint256 previous {t++};
        BOOST_INT256_TEST_UNARY("operator++ post value", a, previous, ma);
        BOOST_INT256_TEST_UNARY("operator++ post", a, t, mp{ma + 1});
    }
    {
        uint256 t {a};
        const uint256 current {++t};
        BOOST_INT256_TEST_UNARY("operator++ pre", a, current, mp{ma + 1});
    }
    {
        uint256 t {a};
        const uint256 previous {t--};
        BOOST_INT256_TEST_UNARY("operator-- post value", a, previous, ma);
        BOOST_INT256_TEST_UNARY("operator-- post", a, t, mp{ma - 1});
    }
    {
        uint256 t {a};
        const uint256 current {--t};
        BOOST_INT256_TEST_UNARY("operator-- pre", a, current, mp{ma - 1});
    }
    #else
    static_cast<void>(a);
    #endif
}

void check_pair(const uint256& a, const uint256& b, unsigned count)
{
    check_arithmetic(a, b);
    check_bitwise(a, b);
    check_shifts(a, count);
    check_compare(a, b);
    check_compound(a, b, count);
}

// Draws pairs from every combination of the operand shapes.
void test_random_pairs(std::mt19937_64& rng, std::size_t total)
{
    const auto& shapes {all_shapes()};
    const auto combinations {shapes.size() * shapes.size()};
    const std::size_t per_combination {total / combinations > 0U ? total / combinations : 1U};

    for (const auto lhs_shape : shapes)
    {
        for (const auto rhs_shape : shapes)
        {
            for (std::size_t i {0}; i < per_combination; ++i)
            {
                const uint256 a {random_uint256(rng, lhs_shape)};
                const uint256 b {random_uint256(rng, rhs_shape)};
                const auto count {static_cast<unsigned>(i % 256U)};

                check_pair(a, b, count);

                if (boost::detail::test_errors() != 0)
                {
                    std::cerr << "  lhs shape = " << shape_name(lhs_shape)
                              << ", rhs shape = " << shape_name(rhs_shape) << std::endl;
                    return;
                }
            }
        }
    }
}

// Every shift count is valid, so all 256 of them are exercised rather than
// sampled. A count of 256 or more is undefined by contract and is not tested.
void test_exhaustive_shifts()
{
    const uint256 values[] {
        uint256_max(),
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)},
        uint256{UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
                UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)},
        uint256{UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(1)},
        uint256{UINT64_C(0x0001020304050607), UINT64_C(0x08090A0B0C0D0E0F),
                UINT64_C(0x1011121314151617), UINT64_C(0x18191A1B1C1D1E1F)}
    };

    for (const auto& v : values)
    {
        for (unsigned count {0}; count < 256U; ++count)
        {
            check_shifts(v, count);
        }
    }
}

// The dispatcher picks a different division algorithm for each divisor width, so
// every class is driven explicitly rather than hoping the random draws land on it.
void test_divisor_classes(std::mt19937_64& rng, std::size_t per_class)
{
    for (std::size_t i {0}; i < per_class; ++i)
    {
        const uint256 dividend {random_uint256(rng, shape::word4)};

        uint256 divisors[8] {};

        // one half word: the 64/32 chain
        divisors[0] = uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), (rng() & UINT64_C(0xFFFFFFFF)) | UINT64_C(1)};
        // one full word: the chained 128/64 path
        divisors[1] = uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), rng() | (UINT64_C(1) << 63)};
        // two words: Moller-Granlund at run time, Knuth in constant evaluation
        divisors[2] = uint256{UINT64_C(0), UINT64_C(0), rng() | (UINT64_C(1) << 63), rng()};
        // three words: Knuth
        divisors[3] = uint256{UINT64_C(0), rng() | (UINT64_C(1) << 63), rng(), rng()};
        // four words: Knuth
        divisors[4] = uint256{rng() | (UINT64_C(1) << 63), rng(), rng(), rng()};
        // larger than the dividend: quotient 0, remainder the dividend
        divisors[5] = dividend == uint256_max() ? uint256_max() : dividend + 1U;
        // equal to the dividend: quotient 1, remainder 0
        divisors[6] = dividend;
        // a power of two: the shift path if there is one
        divisors[7] = uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)} << static_cast<unsigned>(rng() % 256U);

        for (const auto& d : divisors)
        {
            if (d == 0U)
            {
                continue;
            }

            const mp expected_q {to_mp(dividend) / to_mp(d)};
            const mp expected_r {to_mp(dividend) % to_mp(d)};

            const uint256 q {dividend / d};
            const uint256 r {dividend % d};

            BOOST_INT256_TEST_BINARY("divisor class operator/", dividend, d, q, expected_q);
            BOOST_INT256_TEST_BINARY("divisor class operator%", dividend, d, r, expected_r);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// Every ordered pair from the hard-coded corpus, so the boundary values meet each
// other rather than only meeting random operands.
void test_edge_pairs()
{
    const auto& values {edge_values()};

    for (const auto& a : values)
    {
        check_unary(a);
        check_incdec(a);

        for (const auto& b : values)
        {
            check_pair(a, b, static_cast<unsigned>(b.words[0] % 256U));

            if (boost::detail::test_errors() != 0)
            {
                return;
            }
        }
    }
}

// The identities that were recomputed against multiprecision before being written
// down. They pin the specific quotients the division dispatcher must produce.
void test_directed_identities()
{
    const uint256 one_v {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    const uint256 max_v {uint256_max()};
    const uint256 two_64 {one_v << 64U};
    const uint256 two_128 {one_v << 128U};
    const uint256 two_192 {one_v << 192U};
    const uint256 two_255 {one_v << 255U};

    BOOST_TEST(max_v / (two_64 - 1U) == uint256(UINT64_C(1), UINT64_C(1), UINT64_C(1), UINT64_C(1)));
    BOOST_TEST(max_v / uint256(UINT64_C(1), UINT64_C(1), UINT64_C(1), UINT64_C(1)) == (two_64 - 1U));
    // 2^256 - 1 divided by 2^128 - 1 is 2^128 + 1, so words[0] and words[2] are set.
    BOOST_TEST(max_v / (two_128 - 1U) == uint256(UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(1)));
    BOOST_TEST(max_v / (two_128 + 1U) == (two_128 - 1U));
    BOOST_TEST(max_v / (two_192 - 1U) == two_64);
    BOOST_TEST(max_v % (two_192 - 1U) == (two_64 - 1U));
    BOOST_TEST(max_v / 3U == uint256(UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
                                     UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)));
    BOOST_TEST(max_v / 0xFFU == uint256(UINT64_C(0x0101010101010101), UINT64_C(0x0101010101010101),
                                        UINT64_C(0x0101010101010101), UINT64_C(0x0101010101010101)));
    BOOST_TEST(two_128 / two_64 == two_64);

    // Multiplication cascades.
    BOOST_TEST(max_v * max_v == one_v);
    BOOST_TEST((two_128 - 1U) * (two_128 - 1U) == uint256(UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFE),
                                                          UINT64_C(0), UINT64_C(1)));
    BOOST_TEST((two_192 + 1U) * (two_64 + 1U) == uint256(UINT64_C(1), UINT64_C(0), UINT64_C(1), UINT64_C(1)));
    BOOST_TEST((two_64 - 1U) * (two_64 - 1U) == uint256(UINT64_C(0), UINT64_C(0),
                                                        UINT64_C(0xFFFFFFFFFFFFFFFE), UINT64_C(1)));
    BOOST_TEST(max_v * 2U == max_v - 1U);
    BOOST_TEST(two_255 * 2U == uint256{});
    BOOST_TEST((one_v << 127U) * two_128 == two_255);

    // Wraparound at the boundaries.
    BOOST_TEST(max_v + 1U == uint256{});
    BOOST_TEST(uint256{} - 1U == max_v);
}

std::size_t parse_iterations(int argc, char* argv[], std::size_t fallback)
{
    for (int i {1}; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--iterations") == 0 && i + 1 < argc)
        {
            const auto requested {std::strtoull(argv[i + 1], nullptr, 10)};
            if (requested > 0U)
            {
                return static_cast<std::size_t>(requested);
            }
        }
    }

    return fallback;
}

} // namespace

int main(int argc, char* argv[])
{
    // The default budget gives N pairs per shape combination.
    const std::size_t default_total {N * all_shapes().size() * all_shapes().size()};
    const std::size_t total {parse_iterations(argc, argv, default_total)};

    std::mt19937_64 rng {42};

    test_directed_identities();
    test_exhaustive_shifts();
    test_edge_pairs();
    test_divisor_classes(rng, N);
    test_random_pairs(rng, total);

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE
