// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The eleven P3724 integer division families plus rem_euclid.
//
// For an unsigned type several of the rounding modes coincide, and pinning those
// identities is the point: to_zero, to_neg_inf and euclid all truncate because a
// non-negative quotient is never rounded down, while away_zero and to_pos_inf
// both round up. If a future signed int256 shares this code, a mode that has
// quietly drifted will show up as one of these identities breaking.
//
// The reference is a slow rounding done on the reference type, so the rounding
// decision is made independently of the library.

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
using boost::int256::div_result;

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
using boost::int256::int256;
#endif

namespace int256_ns = boost::int256;

namespace
{

std::mt19937_64 rng {42};

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
constexpr int256 min_i {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
constexpr int256 max_i {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
#endif

// Failure reporter for the invariants that do not need the reference, so they stay
// checked in the module build too, where the reference is unavailable. hex_words is
// exported by oracle.hpp unconditionally.
bool report_triple(const char* what, const uint256& x, const uint256& y, const uint256& got)
{
    std::cerr << "int256 integer division mismatch in " << what << '\n'
              << "  x   = " << hex_words(x) << '\n'
              << "  y   = " << hex_words(y) << '\n'
              << "  got = " << hex_words(got) << std::endl;

    return false;
}

//=====================================
// The identities that must hold for an unsigned type
//=====================================

void check_identities(const uint256& x, const uint256& y)
{
    // Truncation, rounding down and Euclidean division agree: the quotient is
    // never negative, so there is nothing to round down to.
    BOOST_TEST(int256_ns::div_rem_to_zero(x, y) == int256_ns::div_rem_to_neg_inf(x, y));
    BOOST_TEST(int256_ns::div_rem_to_zero(x, y) == int256_ns::div_rem_euclid(x, y));
    BOOST_TEST_EQ(int256_ns::div_to_zero(x, y), int256_ns::div_to_neg_inf(x, y));
    BOOST_TEST_EQ(int256_ns::div_to_zero(x, y), int256_ns::div_euclid(x, y));

    // Rounding away from zero and rounding up agree for the same reason.
    BOOST_TEST(int256_ns::div_rem_away_zero(x, y) == int256_ns::div_rem_to_pos_inf(x, y));
    BOOST_TEST_EQ(int256_ns::div_away_zero(x, y), int256_ns::div_to_pos_inf(x, y));

    // And so do the two pairs of tie breaking modes.
    BOOST_TEST(int256_ns::div_rem_ties_to_zero(x, y) == int256_ns::div_rem_ties_to_neg_inf(x, y));
    BOOST_TEST(int256_ns::div_rem_ties_away_zero(x, y) == int256_ns::div_rem_ties_to_pos_inf(x, y));
    BOOST_TEST_EQ(int256_ns::div_ties_to_zero(x, y), int256_ns::div_ties_to_neg_inf(x, y));
    BOOST_TEST_EQ(int256_ns::div_ties_away_zero(x, y), int256_ns::div_ties_to_pos_inf(x, y));

    // rem_euclid is the remainder of the Euclidean division, and is always below
    // the divisor for an unsigned operand.
    BOOST_TEST_EQ(int256_ns::rem_euclid(x, y), int256_ns::div_rem_euclid(x, y).remainder);
    BOOST_TEST(int256_ns::rem_euclid(x, y) < y);

    // Every div_X agrees with the quotient of the matching div_rem_X.
    BOOST_TEST_EQ(int256_ns::div_to_zero(x, y), int256_ns::div_rem_to_zero(x, y).quotient);
    BOOST_TEST_EQ(int256_ns::div_away_zero(x, y), int256_ns::div_rem_away_zero(x, y).quotient);
    BOOST_TEST_EQ(int256_ns::div_to_pos_inf(x, y), int256_ns::div_rem_to_pos_inf(x, y).quotient);
    BOOST_TEST_EQ(int256_ns::div_to_neg_inf(x, y), int256_ns::div_rem_to_neg_inf(x, y).quotient);
    BOOST_TEST_EQ(int256_ns::div_euclid(x, y), int256_ns::div_rem_euclid(x, y).quotient);
    BOOST_TEST_EQ(int256_ns::div_ties_to_zero(x, y), int256_ns::div_rem_ties_to_zero(x, y).quotient);
    BOOST_TEST_EQ(int256_ns::div_ties_away_zero(x, y), int256_ns::div_rem_ties_away_zero(x, y).quotient);
    BOOST_TEST_EQ(int256_ns::div_ties_to_pos_inf(x, y), int256_ns::div_rem_ties_to_pos_inf(x, y).quotient);
    BOOST_TEST_EQ(int256_ns::div_ties_to_neg_inf(x, y), int256_ns::div_rem_ties_to_neg_inf(x, y).quotient);
    BOOST_TEST_EQ(int256_ns::div_ties_to_odd(x, y), int256_ns::div_rem_ties_to_odd(x, y).quotient);
    BOOST_TEST_EQ(int256_ns::div_ties_to_even(x, y), int256_ns::div_rem_ties_to_even(x, y).quotient);

    // The remainder of a rounded up quotient is the truncated remainder minus the
    // divisor, reduced modulo 2^256, so adding the two back gives the dividend in
    // every mode.
    const auto modes = {
        int256_ns::div_rem_to_zero(x, y),
        int256_ns::div_rem_away_zero(x, y),
        int256_ns::div_rem_to_pos_inf(x, y),
        int256_ns::div_rem_to_neg_inf(x, y),
        int256_ns::div_rem_euclid(x, y),
        int256_ns::div_rem_ties_to_zero(x, y),
        int256_ns::div_rem_ties_away_zero(x, y),
        int256_ns::div_rem_ties_to_pos_inf(x, y),
        int256_ns::div_rem_ties_to_neg_inf(x, y),
        int256_ns::div_rem_ties_to_odd(x, y),
        int256_ns::div_rem_ties_to_even(x, y)
    };

    for (const auto& m : modes)
    {
        BOOST_TEST(m.quotient * y + m.remainder == x ||
                   report_triple("q * y + r == x", x, y, m.quotient));
    }
}

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

//=====================================
// A slow reference for each rounding mode
//=====================================

// The truncated quotient and the exact remainder, on the reference type.
struct exact
{
    mp quotient;
    mp remainder;
};

exact reference_truncated(const uint256& x, const uint256& y)
{
    return exact{mp{to_mp(x) / to_mp(y)}, mp{to_mp(x) % to_mp(y)}};
}

void check_against_reference(const uint256& x, const uint256& y)
{
    const auto truncated {reference_truncated(x, y)};
    const bool is_exact {truncated.remainder == 0};

    const mp rounded_up {is_exact ? truncated.quotient : mp{truncated.quotient + 1}};

    // Truncating modes.
    BOOST_TEST(to_mp(int256_ns::div_to_zero(x, y)) == truncated.quotient);
    BOOST_TEST(to_mp(int256_ns::div_to_neg_inf(x, y)) == truncated.quotient);
    BOOST_TEST(to_mp(int256_ns::div_euclid(x, y)) == truncated.quotient);

    // Rounding up modes.
    BOOST_TEST(to_mp(int256_ns::div_away_zero(x, y)) == rounded_up);
    BOOST_TEST(to_mp(int256_ns::div_to_pos_inf(x, y)) == rounded_up);

    // Round to nearest. The remainder is compared against half the divisor, and
    // the tie is only reachable for an even divisor.
    const mp half {to_mp(y) / 2};
    const bool y_is_odd {(y.words[0] & 1U) != 0U};
    const bool above_half {truncated.remainder > half};
    const bool exactly_half {!y_is_odd && truncated.remainder == half};

    // Ties towards zero: round up only when strictly above the halfway point.
    BOOST_TEST(to_mp(int256_ns::div_ties_to_zero(x, y)) == (above_half ? rounded_up : truncated.quotient));

    // Ties away from zero: round up at the halfway point too.
    BOOST_TEST(to_mp(int256_ns::div_ties_away_zero(x, y)) ==
               ((above_half || exactly_half) ? rounded_up : truncated.quotient));

    // Ties to even and ties to odd look at the parity of the truncated quotient.
    const bool quotient_is_odd {(truncated.quotient % 2) != 0};
    const mp ties_to_even {above_half ? rounded_up
                                      : (exactly_half && quotient_is_odd ? rounded_up : truncated.quotient)};
    const mp ties_to_odd {above_half ? rounded_up
                                     : (exactly_half && !quotient_is_odd ? rounded_up : truncated.quotient)};

    BOOST_TEST(to_mp(int256_ns::div_ties_to_even(x, y)) == ties_to_even);
    BOOST_TEST(to_mp(int256_ns::div_ties_to_odd(x, y)) == ties_to_odd);

    // The Euclidean remainder is the plain remainder for unsigned operands.
    BOOST_TEST(to_mp(int256_ns::rem_euclid(x, y)) == truncated.remainder);
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE

//=====================================
// Known answers
//=====================================

void test_known_answers()
{
    const uint256 seven {7U};
    const uint256 two {2U};
    const uint256 three {3U};
    const uint256 four {4U};
    const uint256 six {6U};

    // 7 / 2 is 3 remainder 1, and half of 2 is 1, so 3.5 is an exact tie.
    BOOST_TEST_EQ(int256_ns::div_to_zero(seven, two), three);
    BOOST_TEST_EQ(int256_ns::div_away_zero(seven, two), four);
    BOOST_TEST_EQ(int256_ns::div_ties_to_zero(seven, two), three);
    BOOST_TEST_EQ(int256_ns::div_ties_away_zero(seven, two), four);
    BOOST_TEST_EQ(int256_ns::div_ties_to_even(seven, two), four);      // 3 is odd, so round up
    BOOST_TEST_EQ(int256_ns::div_ties_to_odd(seven, two), three);      // 3 is already odd

    // 9 / 2 is 4 remainder 1, the same tie with an even truncated quotient.
    const uint256 nine {9U};
    BOOST_TEST_EQ(int256_ns::div_ties_to_even(nine, two), four);       // 4 is already even
    BOOST_TEST_EQ(int256_ns::div_ties_to_odd(nine, two), uint256{5U});

    // 6 / 4 is 1 remainder 2, an exact tie: 1.5 rounds either way.
    BOOST_TEST_EQ(int256_ns::div_to_zero(six, four), uint256{1U});
    BOOST_TEST_EQ(int256_ns::div_away_zero(six, four), two);
    BOOST_TEST_EQ(int256_ns::div_ties_to_zero(six, four), uint256{1U});
    BOOST_TEST_EQ(int256_ns::div_ties_away_zero(six, four), two);
    BOOST_TEST_EQ(int256_ns::div_ties_to_even(six, four), two);          // 1 is odd, so round up to 2
    BOOST_TEST_EQ(int256_ns::div_ties_to_odd(six, four), uint256{1U});   // 1 is already odd

    // 10 / 4 is 2 remainder 2, also an exact tie, with an even truncated quotient.
    const uint256 ten {10U};
    BOOST_TEST_EQ(int256_ns::div_ties_to_even(ten, four), two);          // 2 is already even
    BOOST_TEST_EQ(int256_ns::div_ties_to_odd(ten, four), three);         // round up to 3

    // An exact division never rounds.
    const uint256 eight {8U};
    BOOST_TEST_EQ(int256_ns::div_to_zero(eight, four), two);
    BOOST_TEST_EQ(int256_ns::div_away_zero(eight, four), two);
    BOOST_TEST_EQ(int256_ns::div_ties_to_zero(eight, four), two);
    BOOST_TEST_EQ(int256_ns::div_ties_away_zero(eight, four), two);
    BOOST_TEST_EQ(int256_ns::div_ties_to_even(eight, four), two);
    BOOST_TEST_EQ(int256_ns::div_ties_to_odd(eight, four), two);
    BOOST_TEST_EQ(int256_ns::rem_euclid(eight, four), uint256{});

    // An odd divisor cannot produce a tie, so both ties modes truncate at the
    // halfway-ish point. 5 / 3 is 1 remainder 2, and 2 > 1 so it rounds up.
    const uint256 five {5U};
    BOOST_TEST_EQ(int256_ns::div_ties_to_zero(five, three), two);
    BOOST_TEST_EQ(int256_ns::div_ties_away_zero(five, three), two);
    // 4 / 3 is 1 remainder 1, and 1 <= 1 so it truncates.
    BOOST_TEST_EQ(int256_ns::div_ties_to_zero(four, three), uint256{1U});
    BOOST_TEST_EQ(int256_ns::div_ties_away_zero(four, three), uint256{1U});

    // Zero over anything is zero in every mode.
    for (const auto& y : {two, three, uint256_max()})
    {
        BOOST_TEST_EQ(int256_ns::div_to_zero(uint256{}, y), uint256{});
        BOOST_TEST_EQ(int256_ns::div_away_zero(uint256{}, y), uint256{});
        BOOST_TEST_EQ(int256_ns::div_ties_to_even(uint256{}, y), uint256{});
        BOOST_TEST_EQ(int256_ns::rem_euclid(uint256{}, y), uint256{});
    }

    // A dividend below the divisor: the truncated quotient is zero and rounding up
    // gives one.
    BOOST_TEST_EQ(int256_ns::div_to_zero(uint256{1U}, uint256_max()), uint256{});
    BOOST_TEST_EQ(int256_ns::div_away_zero(uint256{1U}, uint256_max()), uint256{1U});
    BOOST_TEST_EQ(int256_ns::rem_euclid(uint256{1U}, uint256_max()), uint256{1U});

    // At the very top of the range, where rounding up wraps to zero.
    const auto at_max {int256_ns::div_rem_away_zero(uint256_max(), uint256{2U})};
    BOOST_TEST_EQ(at_max.quotient, (uint256_max() / 2U) + 1U);
    BOOST_TEST(at_max.quotient * uint256{2U} + at_max.remainder == uint256_max());
}

// div_result is comparable, which is what makes the identities above expressible.
void test_div_result_comparisons()
{
    const div_result<uint256> a {uint256{1U}, uint256{2U}};
    const div_result<uint256> same {uint256{1U}, uint256{2U}};
    const div_result<uint256> other_quotient {uint256{2U}, uint256{2U}};
    const div_result<uint256> other_remainder {uint256{1U}, uint256{3U}};

    BOOST_TEST(a == same);
    BOOST_TEST(!(a != same));
    BOOST_TEST(a != other_quotient);
    BOOST_TEST(a != other_remainder);

    #ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR
    BOOST_TEST((a <=> same) == std::strong_ordering::equal);
    BOOST_TEST((a <=> other_quotient) == std::strong_ordering::less);
    BOOST_TEST((other_quotient <=> a) == std::strong_ordering::greater);
    // The quotient is compared first, then the remainder.
    BOOST_TEST((a <=> other_remainder) == std::strong_ordering::less);
    #endif
}

void test_constexpr()
{
    constexpr uint256 x {7U};
    constexpr uint256 y {2U};

    static_assert(int256_ns::div_to_zero(x, y) == uint256{3U}, "7 / 2 truncates to 3");
    static_assert(int256_ns::div_away_zero(x, y) == uint256{4U}, "7 / 2 rounds away to 4");
    static_assert(int256_ns::div_to_neg_inf(x, y) == uint256{3U}, "unsigned rounds down like truncation");
    static_assert(int256_ns::div_to_pos_inf(x, y) == uint256{4U}, "unsigned rounds up like away from zero");
    static_assert(int256_ns::div_euclid(x, y) == uint256{3U}, "Euclidean matches truncation");
    static_assert(int256_ns::rem_euclid(x, y) == uint256{1U}, "the Euclidean remainder");
    static_assert(int256_ns::div_rem_to_zero(x, y).remainder == uint256{1U}, "the truncated remainder");
    static_assert(int256_ns::div_rem_to_zero(x, y) == int256_ns::div_rem_euclid(x, y),
                  "the identity holds at compile time");
    static_assert(int256_ns::div_ties_to_even(uint256{6U}, uint256{4U}) == uint256{2U}, "1.5 ties to even");
    static_assert(int256_ns::div_ties_to_odd(uint256{10U}, uint256{4U}) == uint256{3U}, "2.5 ties to odd");
}

void test_corpus_and_random()
{
    for (const auto& x : edge_values())
    {
        for (const auto& y : edge_values())
        {
            if (y != 0U)
            {
                check_identities(x, y);
                #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
                check_against_reference(x, y);
                #endif
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto x {random_uint256(rng, shape::word4)};

        // Small divisors are where the rounding decisions bite, so they get their
        // own draws alongside the wide ones.
        const uint256 divisors[] {
            uint256{2U}, uint256{3U}, uint256{4U},
            uint256{(rng() % 1000U) + 1U},
            uint256{rng() | UINT64_C(1)},
            uint256{rng() & ~UINT64_C(1) ? rng() & ~UINT64_C(1) : UINT64_C(2)},
            random_uint256(rng, shape::word2),
            random_uint256(rng, shape::word4)
        };

        for (const auto& y : divisors)
        {
            if (y != 0U)
            {
                check_identities(x, y);
                #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
                check_against_reference(x, y);
                #endif
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

//=====================================
// int256: the same eleven modes, now with sign to worry about. Unlike the
// unsigned type, to_zero/to_neg_inf/euclid (and away_zero/to_pos_inf) only
// coincide when the exact quotient is non-negative; a mode that has quietly
// drifted for a negative quotient will not show up in an identity built for
// the unsigned case, so this section checks against an independent
// mathematical reference instead of reusing those coincidences.
//=====================================

bool report_triple_i(const char* what, const int256& x, const int256& y, const int256& got)
{
    std::cerr << "int256 signed integer division mismatch in " << what << '\n'
              << "  x   = " << hex_words(x) << '\n'
              << "  y   = " << hex_words(y) << '\n'
              << "  got = " << hex_words(got) << std::endl;

    return false;
}

void check_identities_i(const int256& x, const int256& y)
{
    // Every div_X agrees with the quotient of the matching div_rem_X.
    BOOST_TEST(int256_ns::div_to_zero(x, y) == int256_ns::div_rem_to_zero(x, y).quotient);
    BOOST_TEST(int256_ns::div_away_zero(x, y) == int256_ns::div_rem_away_zero(x, y).quotient);
    BOOST_TEST(int256_ns::div_to_pos_inf(x, y) == int256_ns::div_rem_to_pos_inf(x, y).quotient);
    BOOST_TEST(int256_ns::div_to_neg_inf(x, y) == int256_ns::div_rem_to_neg_inf(x, y).quotient);
    BOOST_TEST(int256_ns::div_euclid(x, y) == int256_ns::div_rem_euclid(x, y).quotient);
    BOOST_TEST(int256_ns::div_ties_to_zero(x, y) == int256_ns::div_rem_ties_to_zero(x, y).quotient);
    BOOST_TEST(int256_ns::div_ties_away_zero(x, y) == int256_ns::div_rem_ties_away_zero(x, y).quotient);
    BOOST_TEST(int256_ns::div_ties_to_pos_inf(x, y) == int256_ns::div_rem_ties_to_pos_inf(x, y).quotient);
    BOOST_TEST(int256_ns::div_ties_to_neg_inf(x, y) == int256_ns::div_rem_ties_to_neg_inf(x, y).quotient);
    BOOST_TEST(int256_ns::div_ties_to_odd(x, y) == int256_ns::div_rem_ties_to_odd(x, y).quotient);
    BOOST_TEST(int256_ns::div_ties_to_even(x, y) == int256_ns::div_rem_ties_to_even(x, y).quotient);

    // rem_euclid is the remainder half of div_rem_euclid, and its magnitude is
    // always below |y| (compared as bit patterns so a y of MIN, whose own
    // magnitude does not fit as a positive int256, still compares correctly).
    BOOST_TEST(int256_ns::rem_euclid(x, y) == int256_ns::div_rem_euclid(x, y).remainder);
    BOOST_TEST(int256_ns::rem_euclid(x, y) >= int256{});
    BOOST_TEST(to_unsigned_bits(int256_ns::rem_euclid(x, y)) < to_unsigned_bits(int256_ns::abs(y)));

    // q * y + r == x in every mode: this is the one identity that does not
    // depend on rounding direction or sign at all.
    const auto modes = {
        int256_ns::div_rem_to_zero(x, y),
        int256_ns::div_rem_away_zero(x, y),
        int256_ns::div_rem_to_pos_inf(x, y),
        int256_ns::div_rem_to_neg_inf(x, y),
        int256_ns::div_rem_euclid(x, y),
        int256_ns::div_rem_ties_to_zero(x, y),
        int256_ns::div_rem_ties_away_zero(x, y),
        int256_ns::div_rem_ties_to_pos_inf(x, y),
        int256_ns::div_rem_ties_to_neg_inf(x, y),
        int256_ns::div_rem_ties_to_odd(x, y),
        int256_ns::div_rem_ties_to_even(x, y)
    };

    for (const auto& m : modes)
    {
        BOOST_TEST(m.quotient * y + m.remainder == x ||
                   report_triple_i("q * y + r == x", x, y, m.quotient));
    }
}

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

//=====================================
// An independent mathematical reference for every mode, worked out directly
// from the truncating quotient/remainder rather than from the library's own
// offset_quotient machinery
//=====================================

struct signed_modes
{
    smp to_zero;
    smp away_zero;
    smp to_pos_inf;
    smp to_neg_inf;
    smp euclid;
    smp ties_to_zero;
    smp ties_away_zero;
    smp ties_to_pos_inf;
    smp ties_to_neg_inf;
    smp ties_to_even;
    smp ties_to_odd;
};

signed_modes reference_signed_modes(const smp& xs, const smp& ys)
{
    // Native smp division truncates towards zero and its remainder takes the
    // dividend's sign, exactly like operator/ and operator% on a builtin
    // signed type.
    const smp tq {xs / ys};
    const smp tr {xs - tq * ys};
    const bool is_exact {tr == 0};

    // The sign of the exact (unrounded) quotient: growing the magnitude by one
    // means adding this to tq.
    const int qsign {is_exact ? 0 : (((xs > 0) == (ys > 0)) ? 1 : -1)};
    const smp away_candidate {tq + qsign};

    signed_modes m{};
    m.to_zero = tq;
    m.away_zero = is_exact ? tq : away_candidate;
    m.to_pos_inf = is_exact ? tq : (qsign > 0 ? away_candidate : tq);
    m.to_neg_inf = is_exact ? tq : (qsign < 0 ? away_candidate : tq);

    // Euclidean: the unique q with 0 <= xs - q * ys < |ys|. tr already
    // satisfies this when tr >= 0; otherwise shift by one whole divisor in
    // the direction that makes the remainder non-negative.
    if (tr >= 0)
    {
        m.euclid = tq;
    }
    else if (ys > 0)
    {
        m.euclid = tq - 1;
    }
    else
    {
        m.euclid = tq + 1;
    }

    // Round to nearest: compare 2 * |tr| against |ys| exactly (no rounding of
    // the comparison itself, since both sides are exact integers here).
    const smp two_r {2 * (tr < 0 ? -tr : tr)};
    const smp aby {ys < 0 ? -ys : ys};

    if (is_exact || two_r < aby)
    {
        m.ties_to_zero = tq;
        m.ties_away_zero = tq;
        m.ties_to_pos_inf = tq;
        m.ties_to_neg_inf = tq;
        m.ties_to_even = tq;
        m.ties_to_odd = tq;
    }
    else if (two_r > aby)
    {
        m.ties_to_zero = away_candidate;
        m.ties_away_zero = away_candidate;
        m.ties_to_pos_inf = away_candidate;
        m.ties_to_neg_inf = away_candidate;
        m.ties_to_even = away_candidate;
        m.ties_to_odd = away_candidate;
    }
    else
    {
        // An exact tie: each variant picks whichever side its name says.
        m.ties_to_zero = tq;
        m.ties_away_zero = away_candidate;
        m.ties_to_pos_inf = qsign > 0 ? away_candidate : tq;
        m.ties_to_neg_inf = qsign < 0 ? away_candidate : tq;

        const bool tq_is_even {(tq % 2) == 0};
        m.ties_to_even = tq_is_even ? tq : away_candidate;
        m.ties_to_odd = tq_is_even ? away_candidate : tq;
    }

    return m;
}

void check_against_reference_i(const int256& x, const int256& y)
{
    const smp xs {to_smp(x)};
    const smp ys {to_smp(y)};
    const auto ref {reference_signed_modes(xs, ys)};

    // from_smp_wrapped reduces the reference into int256's range: MIN / -1 is
    // the one input where even the truncated quotient (2^255) does not fit,
    // and it wraps to MIN there exactly like operator/ already does.
    const auto check = [&](const char* what, const int256& got, const smp& expected)
    {
        BOOST_TEST(got == from_smp_wrapped(expected) ||
                   report_mismatch_i(what, x, y, got, expected));
    };

    check("div_to_zero", int256_ns::div_to_zero(x, y), ref.to_zero);
    check("div_away_zero", int256_ns::div_away_zero(x, y), ref.away_zero);
    check("div_to_pos_inf", int256_ns::div_to_pos_inf(x, y), ref.to_pos_inf);
    check("div_to_neg_inf", int256_ns::div_to_neg_inf(x, y), ref.to_neg_inf);
    check("div_euclid", int256_ns::div_euclid(x, y), ref.euclid);
    check("div_ties_to_zero", int256_ns::div_ties_to_zero(x, y), ref.ties_to_zero);
    check("div_ties_away_zero", int256_ns::div_ties_away_zero(x, y), ref.ties_away_zero);
    check("div_ties_to_pos_inf", int256_ns::div_ties_to_pos_inf(x, y), ref.ties_to_pos_inf);
    check("div_ties_to_neg_inf", int256_ns::div_ties_to_neg_inf(x, y), ref.ties_to_neg_inf);
    check("div_ties_to_even", int256_ns::div_ties_to_even(x, y), ref.ties_to_even);
    check("div_ties_to_odd", int256_ns::div_ties_to_odd(x, y), ref.ties_to_odd);
    check("rem_euclid via euclid", int256_ns::div_euclid(x, y), ref.euclid);
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE

//=====================================
// Known answers, every sign combination
//=====================================

void test_known_answers_i()
{
    // 7 / 2 = 3 r 1 (exact tie at the halfway point). Every sign combination.
    BOOST_TEST(int256_ns::div_to_zero(int256{7}, int256{2}) == int256{3});
    BOOST_TEST(int256_ns::div_away_zero(int256{7}, int256{2}) == int256{4});
    BOOST_TEST(int256_ns::div_to_zero(int256{-7}, int256{2}) == int256{-3});
    BOOST_TEST(int256_ns::div_to_pos_inf(int256{-7}, int256{2}) == int256{-3});
    BOOST_TEST(int256_ns::div_to_neg_inf(int256{-7}, int256{2}) == int256{-4});
    BOOST_TEST(int256_ns::div_away_zero(int256{-7}, int256{2}) == int256{-4});
    BOOST_TEST(int256_ns::div_euclid(int256{-7}, int256{2}) == int256{-4});
    BOOST_TEST(int256_ns::rem_euclid(int256{-7}, int256{2}) == int256{1});

    BOOST_TEST(int256_ns::div_to_zero(int256{7}, int256{-2}) == int256{-3});
    BOOST_TEST(int256_ns::div_to_pos_inf(int256{7}, int256{-2}) == int256{-3});
    BOOST_TEST(int256_ns::div_to_neg_inf(int256{7}, int256{-2}) == int256{-4});
    BOOST_TEST(int256_ns::div_away_zero(int256{7}, int256{-2}) == int256{-4});
    // x >= 0, so the Euclidean remainder is already non-negative: no shift.
    BOOST_TEST(int256_ns::div_euclid(int256{7}, int256{-2}) == int256{-3});
    BOOST_TEST(int256_ns::rem_euclid(int256{7}, int256{-2}) == int256{1});

    // -7 / -2 = 3 r -1 truncated; the true quotient is positive (3.5), so
    // away/pos_inf round up to 4, and Euclid needs the remainder fixed too.
    BOOST_TEST(int256_ns::div_to_zero(int256{-7}, int256{-2}) == int256{3});
    BOOST_TEST(int256_ns::div_away_zero(int256{-7}, int256{-2}) == int256{4});
    BOOST_TEST(int256_ns::div_to_pos_inf(int256{-7}, int256{-2}) == int256{4});
    BOOST_TEST(int256_ns::div_to_neg_inf(int256{-7}, int256{-2}) == int256{3});
    BOOST_TEST(int256_ns::div_euclid(int256{-7}, int256{-2}) == int256{4});
    BOOST_TEST(int256_ns::rem_euclid(int256{-7}, int256{-2}) == int256{1});

    // An inexact, non-tie case: -5 / 2 = -2.5 truncates to -2, r = -1; 2*1 < 2
    // is false (2*1 == 2), so this is actually a tie too. Use -4 / 3 instead:
    // -1.333, truncates to -1, r = -1, 2*1 = 2 < 3, so ties_to_zero and
    // ties_away_zero both just report the truncated value here.
    BOOST_TEST(int256_ns::div_ties_to_zero(int256{-4}, int256{3}) == int256{-1});
    BOOST_TEST(int256_ns::div_ties_away_zero(int256{-4}, int256{3}) == int256{-1});

    // -5 / 3 = -1.666, truncates to -1, r = -2, 2*2 = 4 > 3: rounds away, to -2.
    BOOST_TEST(int256_ns::div_ties_to_zero(int256{-5}, int256{3}) == int256{-2});
    BOOST_TEST(int256_ns::div_ties_away_zero(int256{-5}, int256{3}) == int256{-2});

    // Exact division never rounds, whatever the signs are.
    BOOST_TEST(int256_ns::div_to_zero(int256{-8}, int256{4}) == int256{-2});
    BOOST_TEST(int256_ns::div_away_zero(int256{-8}, int256{4}) == int256{-2});
    BOOST_TEST(int256_ns::div_euclid(int256{-8}, int256{4}) == int256{-2});
    BOOST_TEST(int256_ns::rem_euclid(int256{-8}, int256{4}) == int256{});

    // MIN / -1: the one input where the exact quotient (2^255) does not fit,
    // and division is exact (remainder 0), so every mode reports the same
    // wrapped value as operator/ (already pinned in test_div.cpp), with no
    // rounding to compound the overflow.
    const auto wrapped {min_i / int256{-1}};
    BOOST_TEST(wrapped == min_i);
    BOOST_TEST(int256_ns::div_to_zero(min_i, int256{-1}) == wrapped);
    BOOST_TEST(int256_ns::div_away_zero(min_i, int256{-1}) == wrapped);
    BOOST_TEST(int256_ns::div_to_pos_inf(min_i, int256{-1}) == wrapped);
    BOOST_TEST(int256_ns::div_to_neg_inf(min_i, int256{-1}) == wrapped);
    BOOST_TEST(int256_ns::div_euclid(min_i, int256{-1}) == wrapped);
    BOOST_TEST(int256_ns::div_ties_to_even(min_i, int256{-1}) == wrapped);
    BOOST_TEST(int256_ns::div_ties_to_odd(min_i, int256{-1}) == wrapped);
    BOOST_TEST(int256_ns::rem_euclid(min_i, int256{-1}) == int256{});

    // Zero over anything is zero in every mode, any sign of divisor.
    for (const auto& y : {int256{2}, int256{-2}, int256{3}, int256{-3}, max_i, min_i})
    {
        BOOST_TEST(int256_ns::div_to_zero(int256{}, y) == int256{});
        BOOST_TEST(int256_ns::div_away_zero(int256{}, y) == int256{});
        BOOST_TEST(int256_ns::div_ties_to_even(int256{}, y) == int256{});
        BOOST_TEST(int256_ns::rem_euclid(int256{}, y) == int256{});
    }
}

// div_result<int256> is comparable, mirroring div_result<uint256>.
void test_div_result_comparisons_i()
{
    const div_result<int256> a {int256{1}, int256{-2}};
    const div_result<int256> same {int256{1}, int256{-2}};
    const div_result<int256> other_quotient {int256{-2}, int256{-2}};
    const div_result<int256> other_remainder {int256{1}, int256{2}};

    BOOST_TEST(a == same);
    BOOST_TEST(!(a != same));
    BOOST_TEST(a != other_quotient);
    BOOST_TEST(a != other_remainder);

    #ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR
    BOOST_TEST((a <=> same) == std::strong_ordering::equal);
    BOOST_TEST((other_quotient <=> a) == std::strong_ordering::less);
    BOOST_TEST((a <=> other_quotient) == std::strong_ordering::greater);
    #endif
}

void test_constexpr_i()
{
    static_assert(int256_ns::div_to_zero(int256{-7}, int256{2}) == int256{-3}, "-7 / 2 truncates towards zero");
    static_assert(int256_ns::div_to_neg_inf(int256{-7}, int256{2}) == int256{-4}, "-7 / 2 rounds down");
    static_assert(int256_ns::div_euclid(int256{-7}, int256{2}) == int256{-4}, "Euclidean matches rounding down here");
    static_assert(int256_ns::rem_euclid(int256{-7}, int256{2}) == int256{1}, "the Euclidean remainder is non-negative");
    static_assert(int256_ns::div_rem_to_zero(int256{7}, int256{2}).remainder == int256{1}, "the truncated remainder");
    static_assert((min_i / int256{-1}) == min_i, "the wrapped MIN / -1 case");
    static_assert(int256_ns::div_euclid(min_i, int256{-1}) == min_i, "exact division needs no rounding, even here");
}

void test_corpus_and_random_i()
{
    for (const auto& x : signed_edge_values())
    {
        for (const auto& y : signed_edge_values())
        {
            if (y != int256{})
            {
                check_identities_i(x, y);
                #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
                check_against_reference_i(x, y);
                #endif
            }
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto x {random_int256(rng, shape::word4, sign::mixed)};

        // Small divisors of both signs, plus wide ones, so the rounding
        // decisions (which are most interesting for a narrow divisor) get
        // exercised alongside multi-word division.
        const int256 divisors[] {
            int256{2}, int256{-2}, int256{3}, int256{-3}, int256{4}, int256{-4},
            static_cast<int256>(static_cast<std::int64_t>((rng() % 1000U) + 1U)),
            random_int256(rng, shape::word2, sign::mixed),
            random_int256(rng, shape::word4, sign::mixed)
        };

        for (const auto& y : divisors)
        {
            if (y != int256{})
            {
                check_identities_i(x, y);
                #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
                check_against_reference_i(x, y);
                #endif
            }
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
    test_constexpr();
    test_known_answers();
    test_div_result_comparisons();
    test_corpus_and_random();

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_constexpr_i();
    test_known_answers_i();
    test_div_result_comparisons_i();
    test_corpus_and_random_i();
    #endif

    return boost::report_errors();
}
