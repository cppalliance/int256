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

namespace int256_ns = boost::int256;

namespace
{

std::mt19937_64 rng {42};

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

} // namespace

int main()
{
    test_constexpr();
    test_known_answers();
    test_div_result_comparisons();
    test_corpus_and_random();

    return boost::report_errors();
}
