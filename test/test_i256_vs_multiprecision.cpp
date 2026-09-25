// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Differential test of boost::int256::int256 against
// boost::multiprecision::int512_t (oracle.hpp's smp alias, wide enough that
// nothing int256 computes can overflow it). Multiprecision's division truncates
// toward zero, exactly like int256's, so it is a direct reference for / and %;
// its own >> on a negative value is NOT trusted (see oracle.hpp), so the
// arithmetic right shift is checked through
// int256_test::reference_arithmetic_shift_right instead, which only relies on
// the already-verified unsigned uint256 right shift.
//
// Operand pairs are drawn from every combination of the ten operand shapes,
// crossed with {pos, neg, mixed} signs, so that word-count, normalization and
// sign-boundary edges are all hit deliberately. Accepts --iterations N.

#include "oracle.hpp"

#if defined(BOOST_INT256_ORACLE_UNAVAILABLE) || !defined(BOOST_INT256_TEST_HAS_SIGNED)

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <cstdlib>
#include <cstring>
#include <vector>

using boost::int256::int256;
using boost::int256::uint256;

namespace
{

bool report_pair(const char* what, const int256& lhs, const int256& rhs)
{
    std::cerr << "int256 differential mismatch in " << what << '\n'
              << "  lhs = " << hex_words(lhs) << '\n'
              << "  rhs = " << hex_words(rhs) << std::endl;

    return false;
}

bool report_shift(const char* what, const int256& operand, unsigned count,
                  const int256& got, const int256& expected)
{
    std::cerr << "int256 differential mismatch in " << what << " by " << count << '\n'
              << "  operand  = " << hex_words(operand) << '\n'
              << "  got      = " << hex_words(got) << '\n'
              << "  expected = " << hex_words(expected) << std::endl;

    return false;
}

// Every comparison operator with a builtin operand, both orders. smp is wide
// enough to hold any builtin up to 64 bits directly (sign preserved), so this
// does not need oracle.hpp's widen_i machinery.
template <typename T>
void check_builtin_compare(const int256& a, T v)
{
    const smp ma {to_smp(a)};
    const smp mv {v};

    BOOST_TEST((a == v) == (ma == mv) || report_pair("operator== builtin", a, a));
    BOOST_TEST((v == a) == (mv == ma) || report_pair("operator== builtin reversed", a, a));
    BOOST_TEST((a != v) == (ma != mv) || report_pair("operator!= builtin", a, a));
    BOOST_TEST((v != a) == (mv != ma) || report_pair("operator!= builtin reversed", a, a));
    BOOST_TEST((a <  v) == (ma <  mv) || report_pair("operator< builtin",  a, a));
    BOOST_TEST((v <  a) == (mv <  ma) || report_pair("operator< builtin reversed",  a, a));
    BOOST_TEST((a <= v) == (ma <= mv) || report_pair("operator<= builtin", a, a));
    BOOST_TEST((v <= a) == (mv <= ma) || report_pair("operator<= builtin reversed", a, a));
    BOOST_TEST((a >  v) == (ma >  mv) || report_pair("operator> builtin",  a, a));
    BOOST_TEST((v >  a) == (mv >  ma) || report_pair("operator> builtin reversed",  a, a));
    BOOST_TEST((a >= v) == (ma >= mv) || report_pair("operator>= builtin", a, a));
    BOOST_TEST((v >= a) == (mv >= ma) || report_pair("operator>= builtin reversed", a, a));
}

// Every narrow width, 8 to 64 bits, both signedness, against a, both orders.
void check_all_builtin_widths(const int256& a, std::uint64_t magnitude)
{
    check_builtin_compare(a, static_cast<std::uint8_t>(magnitude));
    check_builtin_compare(a, static_cast<std::int8_t>(magnitude));
    check_builtin_compare(a, static_cast<std::uint16_t>(magnitude));
    check_builtin_compare(a, static_cast<std::int16_t>(magnitude));
    check_builtin_compare(a, static_cast<std::uint32_t>(magnitude));
    check_builtin_compare(a, static_cast<std::int32_t>(magnitude));
    check_builtin_compare(a, static_cast<std::uint64_t>(magnitude));
    check_builtin_compare(a, static_cast<std::int64_t>(magnitude));
}

// The directed regression the conductor's review found: a comparison between
// an unsigned builtin and a positive int256 whose words[1] or words[2] are
// nonzero while words[3] == 0 (a 2 or 3 word value) was wrong.
void test_directed_multiword_builtin_comparison_regression()
{
    const int256 two_64 {INT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)};
    const int256 two_128 {INT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)};

    BOOST_TEST(5U < two_64);
    BOOST_TEST(5U < two_128);
    BOOST_TEST(5U <= two_64);
    BOOST_TEST(two_64 > 5U);
    BOOST_TEST(!(5U > two_64));
    BOOST_TEST(!(5U >= two_64));

    check_all_builtin_widths(two_64, 5U);
    check_all_builtin_widths(two_128, 5U);
}

// Every comparison operator with a builtin operand, against int256 values of
// every shape (1, 2, 3 and 4 significant words) and every sign.
void test_builtin_comparisons_every_shape(std::mt19937_64& rng, std::size_t per_shape)
{
    for (const auto s : all_shapes())
    {
        for (const auto sg : {sign::pos, sign::neg, sign::mixed})
        {
            for (std::size_t i {0}; i < per_shape; ++i)
            {
                const int256 a {random_int256(rng, s, sg)};
                const auto magnitude {rng()};

                check_all_builtin_widths(a, magnitude);

                if (boost::detail::test_errors() != 0)
                {
                    std::cerr << "  shape = " << shape_name(s) << std::endl;
                    return;
                }
            }
        }
    }
}

void check_arithmetic(const int256& a, const int256& b)
{
    const smp ma {to_smp(a)};
    const smp mb {to_smp(b)};

    {
        const int256 got {a + b};
        BOOST_INT256_TEST_BINARY_I("operator+", a, b, got, ma + mb);
    }
    {
        const int256 got {a - b};
        BOOST_INT256_TEST_BINARY_I("operator-", a, b, got, ma - mb);
    }
    {
        const int256 got {a * b};
        BOOST_INT256_TEST_BINARY_I("operator*", a, b, got, ma * mb);
    }
    if (b != 0)
    {
        const int256 q {a / b};
        BOOST_INT256_TEST_BINARY_I("operator/", a, b, q, ma / mb);

        const int256 r {a % b};
        BOOST_INT256_TEST_BINARY_I("operator%", a, b, r, ma % mb);

        // q * b + r == a is an identity computed entirely in int256's own
        // wrapping arithmetic, so it holds regardless of the oracle and is a
        // strong statement about the divide path on its own.
        BOOST_TEST(q * b + r == a || report_pair("q * b + r == a", a, b));
    }
}

void check_compound(const int256& a, const int256& b)
{
    const smp ma {to_smp(a)};
    const smp mb {to_smp(b)};

    {
        int256 t {a};
        t += b;
        BOOST_INT256_TEST_BINARY_I("operator+=", a, b, t, ma + mb);
    }
    {
        int256 t {a};
        t -= b;
        BOOST_INT256_TEST_BINARY_I("operator-=", a, b, t, ma - mb);
    }
    {
        int256 t {a};
        t *= b;
        BOOST_INT256_TEST_BINARY_I("operator*=", a, b, t, ma * mb);
    }
    if (b != 0)
    {
        int256 t {a};
        t /= b;
        BOOST_INT256_TEST_BINARY_I("operator/=", a, b, t, ma / mb);

        int256 u {a};
        u %= b;
        BOOST_INT256_TEST_BINARY_I("operator%=", a, b, u, ma % mb);
    }
}

void check_compare(const int256& a, const int256& b)
{
    const smp ma {to_smp(a)};
    const smp mb {to_smp(b)};

    BOOST_TEST((a == b) == (ma == mb) || report_pair("operator==", a, b));
    BOOST_TEST((a != b) == (ma != mb) || report_pair("operator!=", a, b));
    BOOST_TEST((a <  b) == (ma <  mb) || report_pair("operator<",  a, b));
    BOOST_TEST((a <= b) == (ma <= mb) || report_pair("operator<=", a, b));
    BOOST_TEST((a >  b) == (ma >  mb) || report_pair("operator>",  a, b));
    BOOST_TEST((a >= b) == (ma >= mb) || report_pair("operator>=", a, b));

    BOOST_TEST((b == a) == (mb == ma) || report_pair("operator== reversed", a, b));
    BOOST_TEST((b <  a) == (mb <  ma) || report_pair("operator< reversed",  a, b));
    BOOST_TEST((b <= a) == (mb <= ma) || report_pair("operator<= reversed", a, b));
}

void check_shifts(const int256& a, unsigned count)
{
    const smp ma {to_smp(a)};

    {
        const int256 got {a << count};
        BOOST_INT256_TEST_BINARY_I("operator<<", a, a, got, ma * (smp{1} << count));
    }
    {
        const int256 got {a >> count};
        const int256 expected {reference_arithmetic_shift_right(a, count)};
        BOOST_TEST(got == expected || report_shift("operator>>", a, count, got, expected));
    }
    {
        int256 t {a};
        t <<= count;
        BOOST_INT256_TEST_BINARY_I("operator<<=", a, a, t, ma * (smp{1} << count));
    }
    {
        int256 t {a};
        t >>= count;
        const int256 expected {reference_arithmetic_shift_right(a, count)};
        BOOST_TEST(t == expected || report_shift("operator>>=", a, count, t, expected));
    }
}

void check_unary(const int256& a)
{
    const smp ma {to_smp(a)};

    {
        const int256 got {-a};
        BOOST_INT256_TEST_UNARY_I("unary operator-", a, got, -ma);
    }
    {
        const int256 got {+a};
        BOOST_TEST(got == a || report_pair("unary operator+", a, a));
    }
    {
        const int256 got {boost::int256::abs(a)};
        const smp expected {ma < 0 ? -ma : ma};
        BOOST_INT256_TEST_UNARY_I("abs", a, got, expected);
    }
}

void check_incdec(const int256& a)
{
    const smp ma {to_smp(a)};

    {
        int256 t {a};
        const int256 previous {t++};
        BOOST_TEST(previous == a || report_pair("operator++ post value", a, a));
        BOOST_INT256_TEST_UNARY_I("operator++ post", a, t, ma + 1);
    }
    {
        int256 t {a};
        const int256 current {++t};
        BOOST_INT256_TEST_UNARY_I("operator++ pre", a, current, ma + 1);
    }
    {
        int256 t {a};
        const int256 previous {t--};
        BOOST_TEST(previous == a || report_pair("operator-- post value", a, a));
        BOOST_INT256_TEST_UNARY_I("operator-- post", a, t, ma - 1);
    }
    {
        int256 t {a};
        const int256 current {--t};
        BOOST_INT256_TEST_UNARY_I("operator-- pre", a, current, ma - 1);
    }
}

void check_pair(const int256& a, const int256& b, unsigned count)
{
    check_arithmetic(a, b);
    check_compound(a, b);
    check_compare(a, b);
    check_shifts(a, count);
}

// Draws pairs from every combination of the operand shapes crossed with the
// three sign classes.
void test_random_pairs(std::mt19937_64& rng, std::size_t total)
{
    const auto& shapes {all_shapes()};
    const sign signs[] {sign::pos, sign::neg, sign::mixed};
    const auto combinations {shapes.size() * shapes.size() * 3U * 3U};
    const std::size_t per_combination {total / combinations > 0U ? total / combinations : 1U};

    for (const auto lhs_shape : shapes)
    {
        for (const auto rhs_shape : shapes)
        {
            for (const auto lhs_sign : signs)
            {
                for (const auto rhs_sign : signs)
                {
                    for (std::size_t i {0}; i < per_combination; ++i)
                    {
                        const int256 a {random_int256(rng, lhs_shape, lhs_sign)};
                        const int256 b {random_int256(rng, rhs_shape, rhs_sign)};
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
    }
}

// Every shift count from 0 to 255 on every value in the signed edge corpus. A
// count of 256 or more is undefined by contract and is not tested.
void test_exhaustive_shifts()
{
    for (const auto& v : signed_edge_values())
    {
        for (unsigned count {0}; count < 256U; ++count)
        {
            check_shifts(v, count);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// The dispatcher picks a different code path for each divisor width and for
// the MIN and -1 special cases, so every class is driven explicitly rather
// than hoping the random draws land on it. Correctness for every class is
// verified against the oracle in check_arithmetic, so the specific magnitude
// relationship between dividend and divisor does not need to be asserted here.
void test_divisor_classes(std::mt19937_64& rng, std::size_t per_class)
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};

    for (std::size_t i {0}; i < per_class; ++i)
    {
        const int256 dividend {random_int256(rng, shape::word4, sign::mixed)};

        std::vector<int256> divisors;

        divisors.push_back(min_v);
        divisors.push_back(int256{-1});

        // one word: fits in a single 64-bit lane, nonzero.
        {
            const auto w {static_cast<std::int64_t>((rng() & UINT64_C(0x7FFFFFFFFFFFFFFE)) | UINT64_C(1))};
            divisors.push_back(int256{w});
            divisors.push_back(int256{static_cast<std::int64_t>(-w)});
        }
        // two words.
        {
            const auto lo {rng()};
            const auto hi {(rng() % UINT64_C(0x7FFFFFFFFFFFFFFF)) + 1U};
            const int256 mag {to_signed_bits(uint256{UINT64_C(0), UINT64_C(0), hi, lo})};
            divisors.push_back(mag);
            divisors.push_back(-mag);
        }
        // four words, magnitude below MIN so it stays a normal negative value
        // once negated.
        {
            const auto w0 {rng()};
            const auto w1 {rng()};
            const auto w2 {rng()};
            const auto w3 {(rng() % UINT64_C(0x7FFFFFFFFFFFFFFF)) + 1U};
            const int256 mag {to_signed_bits(uint256{w3, w2, w1, w0})};
            divisors.push_back(mag);
            divisors.push_back(-mag);
        }
        // larger in magnitude than the dividend, both signs.
        {
            const smp md {to_smp(dividend)};
            const smp bigger {(md < 0 ? -md : md) + 1};
            divisors.push_back(from_smp_wrapped(bigger));
            divisors.push_back(from_smp_wrapped(-bigger));
        }
        // equal to the dividend: quotient 1, remainder 0.
        divisors.push_back(dividend);

        for (const auto& d : divisors)
        {
            if (d == 0)
            {
                continue;
            }

            check_arithmetic(dividend, d);

            if (boost::detail::test_errors() != 0)
            {
                return;
            }
        }
    }
}

// Every ordered pair from the hard-coded signed corpus, so the boundary values
// meet each other rather than only meeting random operands.
void test_edge_pairs()
{
    const auto& values {signed_edge_values()};

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

// The int128 defect A regression, restated with a wide divisor sweep: an
// unsigned or lightly-negative dividend divided or reduced by MIN must not
// divide by a zero intermediate, and the result must be mathematically exact
// (quotient 0, remainder the dividend, since |dividend| < |MIN| in every case
// here).
void test_min_divisor_sweep(std::mt19937_64& rng, std::size_t total)
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};

    for (std::size_t i {0}; i < total; ++i)
    {
        const int256 dividend {random_int256(rng, shape::word1, sign::mixed)};

        check_arithmetic(dividend, min_v);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// Paper examples and known identities, scaled so MIN/-1 and a handful of other
// hand-verified cases are pinned by name rather than only by random sweep.
void test_directed_identities()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    // BOOST_TEST(a == b) rather than BOOST_TEST_EQ: int256 has no operator<<
    // until D2 lands, and BOOST_TEST_EQ would try to stream the operands on a
    // failure (currently ambiguous).
    BOOST_TEST(min_v / int256{-1} == min_v);
    BOOST_TEST(min_v % int256{-1} == int256{0});
    BOOST_TEST(max_v / int256{-1} == -max_v);
    BOOST_TEST(max_v % int256{-1} == int256{0});

    BOOST_TEST(int256{-7} / int256{2} == int256{-3});
    BOOST_TEST(int256{-7} % int256{2} == int256{-1});
    BOOST_TEST(int256{7} / int256{-2} == int256{-3});
    BOOST_TEST(int256{7} % int256{-2} == int256{1});

    BOOST_TEST(max_v + int256{1} == min_v);
    BOOST_TEST(min_v - int256{1} == max_v);
    BOOST_TEST(min_v * int256{-1} == min_v);
    BOOST_TEST(boost::int256::abs(min_v) == min_v);
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
    const std::size_t default_total {N * all_shapes().size() * all_shapes().size() * 3U * 3U};
    const std::size_t total {parse_iterations(argc, argv, default_total)};

    std::mt19937_64 rng {42};

    test_directed_identities();
    test_directed_multiword_builtin_comparison_regression();
    test_exhaustive_shifts();
    test_edge_pairs();
    test_divisor_classes(rng, N);
    test_min_divisor_sweep(rng, N);
    test_builtin_comparisons_every_shape(rng, N);
    test_random_pairs(rng, total);

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE || !BOOST_INT256_TEST_HAS_SIGNED
