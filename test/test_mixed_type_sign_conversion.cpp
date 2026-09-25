// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Arithmetic between uint256 and a signed builtin. The signed operand converts
// to the 256-bit value the usual arithmetic conversions give it, so a negative
// scalar sign fills into all four words and the result follows from that. The
// library reaches the same answer without materializing the widened operand
// (it subtracts the magnitude instead), which is what this test pins.

#include "oracle.hpp"

#ifdef BOOST_INT256_ORACLE_UNAVAILABLE

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <cstdint>
#include <limits>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

bool report_signed(const char* what, const uint256& lhs, std::int64_t rhs,
                   const mp& got, const mp& expected)
{
    std::cerr << "int256 mixed sign mismatch in " << what << '\n'
              << "  lhs      = " << hex_words(lhs) << '\n'
              << "  rhs      = " << rhs << " (" << hex_words(widen(rhs)) << ")\n"
              << "  got      = " << hex_words(got) << '\n'
              << "  expected = " << hex_words(expected) << std::endl;

    return false;
}

#define CHECK_SIGNED(what, lhs, rhs, got, expected) \
    BOOST_TEST(to_mp(got) == (expected) || report_signed(what, lhs, rhs, to_mp(got), expected))

void check_one(const uint256& x, std::int64_t v)
{
    const mp mx {to_mp(x)};
    const mp mv {widen(v)};
    const uint256 v256 {v};

    {
        const uint256 got {x + v};
        CHECK_SIGNED("operator+(uint256, Signed)", x, v, got, mp{mx + mv});
        BOOST_TEST_EQ(got, x + v256);
    }
    {
        const uint256 got {v + x};
        CHECK_SIGNED("operator+(Signed, uint256)", x, v, got, mp{mv + mx});
        BOOST_TEST_EQ(got, v256 + x);
    }
    {
        const uint256 got {x - v};
        CHECK_SIGNED("operator-(uint256, Signed)", x, v, got, mp{mx - mv});
        BOOST_TEST_EQ(got, x - v256);
    }
    {
        const uint256 got {v - x};
        CHECK_SIGNED("operator-(Signed, uint256)", x, v, got, mp{mv - mx});
        BOOST_TEST_EQ(got, v256 - x);
    }
    {
        const uint256 got {x * v};
        CHECK_SIGNED("operator*(uint256, Signed)", x, v, got, mp{mx * mv});
        BOOST_TEST_EQ(got, x * v256);
    }
    {
        const uint256 got {v * x};
        CHECK_SIGNED("operator*(Signed, uint256)", x, v, got, mp{mv * mx});
        BOOST_TEST_EQ(got, v256 * x);
    }
    if (v != 0)
    {
        const uint256 quotient {x / v};
        CHECK_SIGNED("operator/(uint256, Signed)", x, v, quotient, mp{mx / mv});
        BOOST_TEST_EQ(quotient, x / v256);

        const uint256 remainder {x % v};
        CHECK_SIGNED("operator%(uint256, Signed)", x, v, remainder, mp{mx % mv});
        BOOST_TEST_EQ(remainder, x % v256);
    }
    if (x != 0U)
    {
        const uint256 quotient {v / x};
        CHECK_SIGNED("operator/(Signed, uint256)", x, v, quotient, mp{mv / mx});
        BOOST_TEST_EQ(quotient, v256 / x);

        const uint256 remainder {v % x};
        CHECK_SIGNED("operator%(Signed, uint256)", x, v, remainder, mp{mv % mx});
        BOOST_TEST_EQ(remainder, v256 % x);
    }
    {
        const uint256 got {x & v};
        CHECK_SIGNED("operator&(uint256, Signed)", x, v, got, mp{mx & mv});
    }
    {
        const uint256 got {x | v};
        CHECK_SIGNED("operator|(uint256, Signed)", x, v, got, mp{mx | mv});
    }
    {
        const uint256 got {x ^ v};
        CHECK_SIGNED("operator^(uint256, Signed)", x, v, got, mp{mx ^ mv});
    }
}

// The values whose magnitude cannot be negated inside their own type, plus a
// spread of ordinary ones.
void test_directed()
{
    const std::int64_t scalars[] {
        (std::numeric_limits<std::int64_t>::min)(),
        (std::numeric_limits<std::int64_t>::min)() + 1,
        INT64_C(-1000003), INT64_C(-42), INT64_C(-2), INT64_C(-1),
        INT64_C(0), INT64_C(1), INT64_C(2), INT64_C(42),
        (std::numeric_limits<std::int64_t>::max)() - 1,
        (std::numeric_limits<std::int64_t>::max)()
    };

    for (const auto& x : edge_values())
    {
        for (const auto v : scalars)
        {
            check_one(x, v);

            if (boost::detail::test_errors() != 0)
            {
                return;
            }
        }
    }
}

void test_random()
{
    std::uniform_int_distribution<std::int64_t> dist {
        (std::numeric_limits<std::int64_t>::min)(), (std::numeric_limits<std::int64_t>::max)()};

    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 x {random_uint256(rng, shape::word4)};
        check_one(x, dist(rng));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

// int128 defect B: int128 /= uint128 (and %=) had no cross-type compound
// overload, so the call resolved to the member operator op=(int128), which
// divides SIGNED, where the correct (and int256's contracted) behavior divides
// UNSIGNED. This pins int256 /= uint256 and %= (and the other compound ops)
// as computing in the uint256 domain and converting back, not in the int256
// domain.
void test_int256_compound_assign_from_uint256_uses_unsigned_semantics()
{
    const int256 negative_ten {-10};
    const uint256 three {3U};
    const uint256 unsigned_bits {to_unsigned_bits(negative_ten)};

    // BOOST_TEST(a == b) is used instead of BOOST_TEST_EQ for int256 operands
    // throughout this file: Boost.Int256's iostream support (D2) has not
    // landed yet, so int256 has no operator<<, and BOOST_TEST_EQ would try to
    // stream the operands on a failure (currently ambiguous, since int256
    // converts implicitly to several builtin types).
    int256 t {negative_ten};
    t /= three;
    BOOST_TEST(t == to_signed_bits(unsigned_bits / three));

    // The signed-division answer (-10 / 3 == -3) is a different bit pattern
    // from the unsigned one computed above; this is exactly the int128 defect,
    // so it must NOT be what int256 produces.
    BOOST_TEST(t != int256{-3});

    int256 u {negative_ten};
    u %= three;
    BOOST_TEST(u == to_signed_bits(unsigned_bits % three));

    int256 v {negative_ten};
    v += three;
    BOOST_TEST(v == to_signed_bits(unsigned_bits + three));

    int256 w {negative_ten};
    w -= three;
    BOOST_TEST(w == to_signed_bits(unsigned_bits - three));

    int256 x {negative_ten};
    x *= three;
    BOOST_TEST(x == to_signed_bits(unsigned_bits * three));

    int256 y {negative_ten};
    y &= three;
    BOOST_TEST(y == to_signed_bits(unsigned_bits & three));

    int256 z {negative_ten};
    z |= three;
    BOOST_TEST(z == to_signed_bits(unsigned_bits | three));

    int256 xo {negative_ten};
    xo ^= three;
    BOOST_TEST(xo == to_signed_bits(unsigned_bits ^ three));

    // Shifts are NOT computed in the uint256 domain the way the eight ops
    // above are: a shift's value and sign come from the LEFT operand, exactly
    // like `long long >>= unsigned long long`. The right-hand uint256 supplies
    // only the count. So <<= is logical (which happens to be bit-identical to
    // shifting the unsigned bit image, since a logical shift does not depend
    // on sign), but >>= is ARITHMETIC: it sign-extends, and must NOT match the
    // uint256-domain (logical) shift on a negative left-hand side.
    int256 sh_l {negative_ten};
    sh_l <<= uint256{2U};
    BOOST_TEST(sh_l == to_signed_bits(unsigned_bits << 2U));

    int256 sh_r {negative_ten};
    sh_r >>= uint256{2U};
    BOOST_TEST(sh_r == int256{-3});                          // -10 >> 2 == -3, arithmetic
    BOOST_TEST(sh_r != to_signed_bits(unsigned_bits >> 2U));  // NOT the logical (uint256-domain) shift

    // The exact case the conductor's review found: -16 >>= 2 must be -4.
    {
        int256 minus_sixteen {-16};
        minus_sixteen >>= uint256{2U};
        BOOST_TEST(minus_sixteen == int256{-4});
    }
}

// All 10 compound forms, negative lhs, against the "builtin model":
// +-*/%|&^ compute in the uint256 domain and convert back (the defect B fix);
// << is logical (bit-identical either way); >> is arithmetic, sign and type
// from the left operand, exactly like `long long >>= unsigned long long`.
void test_int256_compound_assign_from_uint256_random()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const int256 lhs {random_int256(rng, shape::word4, sign::mixed)};
        const uint256 rhs {random_uint256(rng, shape::word4)};
        const uint256 lhs_bits {to_unsigned_bits(lhs)};

        {
            int256 t {lhs};
            t += rhs;
            BOOST_TEST(t == to_signed_bits(lhs_bits + rhs));
        }
        {
            int256 t {lhs};
            t -= rhs;
            BOOST_TEST(t == to_signed_bits(lhs_bits - rhs));
        }
        {
            int256 t {lhs};
            t *= rhs;
            BOOST_TEST(t == to_signed_bits(lhs_bits * rhs));
        }
        if (rhs != 0U)
        {
            int256 t {lhs};
            t /= rhs;
            BOOST_TEST(t == to_signed_bits(lhs_bits / rhs));

            int256 u {lhs};
            u %= rhs;
            BOOST_TEST(u == to_signed_bits(lhs_bits % rhs));
        }
        {
            int256 t {lhs};
            t &= rhs;
            BOOST_TEST(t == to_signed_bits(lhs_bits & rhs));

            int256 u {lhs};
            u |= rhs;
            BOOST_TEST(u == to_signed_bits(lhs_bits | rhs));

            int256 v {lhs};
            v ^= rhs;
            BOOST_TEST(v == to_signed_bits(lhs_bits ^ rhs));
        }

        // The shift count is the right operand's low word, clamped into range
        // so the test does not depend on the undefined behavior at >= 256.
        const auto count {static_cast<unsigned>(rhs.words[0] % 256U)};
        {
            int256 t {lhs};
            t <<= uint256{count};
            BOOST_TEST(t == to_signed_bits(lhs_bits << count));
        }
        {
            int256 t {lhs};
            t >>= uint256{count};
            BOOST_TEST(t == reference_arithmetic_shift_right(lhs, count));
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// The other direction: uint256 op= int256 resolves through
// uint256::operator@=(const uint256&) after a bit-copy conversion of the
// right-hand side, so every one of the 10 ops (including the shifts, which
// are always logical for a uint256 left-hand side regardless of the
// int256 operand's sign) is checked against a nonnegative int256 rhs (a
// negative rhs bit-copies to a huge magnitude, which is a different, already
//-covered path: ordinary uint256 arithmetic/bitwise, and an out-of-range
// shift count, which is undefined by contract).
void test_uint256_compound_assign_from_int256_random()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 lhs {random_uint256(rng, shape::word4)};
        const int256 rhs {random_int256(rng, shape::word4, sign::mixed)};
        const uint256 rhs_bits {to_unsigned_bits(rhs)};

        {
            uint256 t {lhs};
            t += rhs;
            BOOST_TEST_EQ(t, uint256{lhs + rhs_bits});
        }
        {
            uint256 t {lhs};
            t -= rhs;
            BOOST_TEST_EQ(t, uint256{lhs - rhs_bits});
        }
        {
            uint256 t {lhs};
            t *= rhs;
            BOOST_TEST_EQ(t, uint256{lhs * rhs_bits});
        }
        if (rhs_bits != 0U)
        {
            uint256 t {lhs};
            t /= rhs;
            BOOST_TEST_EQ(t, uint256{lhs / rhs_bits});

            uint256 u {lhs};
            u %= rhs;
            BOOST_TEST_EQ(u, uint256{lhs % rhs_bits});
        }
        {
            uint256 t {lhs};
            t &= rhs;
            BOOST_TEST_EQ(t, uint256{lhs & rhs_bits});

            uint256 u {lhs};
            u |= rhs;
            BOOST_TEST_EQ(u, uint256{lhs | rhs_bits});

            uint256 v {lhs};
            v ^= rhs;
            BOOST_TEST_EQ(v, uint256{lhs ^ rhs_bits});
        }

        // Only a nonnegative, in-range rhs is a defined shift count here; the
        // count actually used for the shift has to be rhs itself (clamped by
        // this guard), not some other in-range value, or the operation itself
        // (not just the reference) would be out of range.
        if (rhs >= 0 && rhs < 256)
        {
            const auto count {static_cast<unsigned>(rhs_bits.words[0])};

            uint256 t {lhs};
            t <<= rhs;
            BOOST_TEST_EQ(t, uint256{lhs << count});

            uint256 u {lhs};
            u >>= rhs;
            BOOST_TEST_EQ(u, uint256{lhs >> count});
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_directed();
    test_random();

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_int256_compound_assign_from_uint256_uses_unsigned_semantics();
    test_int256_compound_assign_from_uint256_random();
    test_uint256_compound_assign_from_int256_random();
#endif

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE
