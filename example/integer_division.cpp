// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>
#include <boost/int256/numeric.hpp>
#include <boost/int256/iostream.hpp>
#include <boost/int256/limits.hpp>
#include <iostream>
#include <limits>

// tag::exclude[]
// Only present with MSVC 14.1
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4307) // integral constant overflow
#  pragma warning(disable : 4146) // unary minus operator applied to unsigned type, result still unsigned
#endif
// end::exclude[]

int main()
{
    using boost::int256::uint256;

    std::cout << "=== Every rounding mode on 12 / 5 ===" << std::endl;

    // The exact quotient is 2.4, so the nearest integer is 2 and no mode ties
    constexpr uint256 x {12U};
    constexpr uint256 y {5U};

    std::cout << "div_to_zero          = " << boost::int256::div_to_zero(x, y) << std::endl;
    std::cout << "div_away_zero        = " << boost::int256::div_away_zero(x, y) << std::endl;
    std::cout << "div_to_pos_inf       = " << boost::int256::div_to_pos_inf(x, y) << std::endl;
    std::cout << "div_to_neg_inf       = " << boost::int256::div_to_neg_inf(x, y) << std::endl;
    std::cout << "div_euclid           = " << boost::int256::div_euclid(x, y) << std::endl;
    std::cout << "div_ties_to_zero     = " << boost::int256::div_ties_to_zero(x, y) << std::endl;

    // For an unsigned type several of the modes necessarily agree: nothing is ever
    // negative, so rounding towards zero, towards negative infinity, and Euclidean
    // division are the same operation, and rounding away from zero is rounding
    // towards positive infinity.
    std::cout << "\n=== Which modes coincide for an unsigned type ===" << std::endl;
    std::cout << std::boolalpha;
    std::cout << "to_zero == to_neg_inf == euclid: "
              << (boost::int256::div_to_zero(x, y) == boost::int256::div_to_neg_inf(x, y) &&
                  boost::int256::div_to_zero(x, y) == boost::int256::div_euclid(x, y)) << std::endl;
    std::cout << "away_zero == to_pos_inf:         "
              << (boost::int256::div_away_zero(x, y) == boost::int256::div_to_pos_inf(x, y)) << std::endl;
    std::cout << "operator% == rem_euclid:         "
              << ((x % y) == boost::int256::rem_euclid(x, y)) << std::endl;

    std::cout << "\n=== Tie breaking on 7 / 2 ===" << std::endl;

    // The exact quotient is 3.5, so the tie-breaking rules disagree
    constexpr uint256 tie_x {7U};
    constexpr uint256 tie_y {2U};

    std::cout << "div_ties_to_zero     = " << boost::int256::div_ties_to_zero(tie_x, tie_y) << std::endl;
    std::cout << "div_ties_away_zero   = " << boost::int256::div_ties_away_zero(tie_x, tie_y) << std::endl;
    std::cout << "div_ties_to_pos_inf  = " << boost::int256::div_ties_to_pos_inf(tie_x, tie_y) << std::endl;
    std::cout << "div_ties_to_neg_inf  = " << boost::int256::div_ties_to_neg_inf(tie_x, tie_y) << std::endl;
    std::cout << "div_ties_to_odd      = " << boost::int256::div_ties_to_odd(tie_x, tie_y) << std::endl;
    std::cout << "div_ties_to_even     = " << boost::int256::div_ties_to_even(tie_x, tie_y) << std::endl;

    std::cout << "\n=== Quotient and remainder from one division ===" << std::endl;

    // Each div_rem_ function performs a single division and returns both halves
    constexpr auto truncated {boost::int256::div_rem_to_zero(x, y)};
    std::cout << "div_rem_to_zero(12, 5):     quotient = " << truncated.quotient
              << ", remainder = " << truncated.remainder << std::endl;

    // A quotient rounded up leaves a negative remainder, which is returned reduced
    // modulo 2^256 so that the identity below still holds in the type
    constexpr auto rounded_up {boost::int256::div_rem_to_pos_inf(x, y)};
    std::cout << "div_rem_to_pos_inf(12, 5):  quotient = " << rounded_up.quotient
              << ", remainder = " << rounded_up.remainder << std::endl;

    // The remainder always satisfies x == quotient * y + remainder
    std::cout << "quotient * y + remainder = " << (truncated.quotient * y + truncated.remainder)
              << " and " << (rounded_up.quotient * y + rounded_up.remainder) << std::endl;

    std::cout << "\n=== Ceiling division without overflow ===" << std::endl;

    // Counting fixed size blocks needed to cover a length is the classic use for
    // rounding towards positive infinity. The usual (length + block - 1) / block
    // overflows here, while div_to_pos_inf does not.
    constexpr auto length {(std::numeric_limits<uint256>::max)()};
    constexpr uint256 block {1000U};

    std::cout << "length = " << length << std::endl;
    std::cout << "div_to_pos_inf(length, 1000) = " << boost::int256::div_to_pos_inf(length, block) << std::endl;
    std::cout << "div_to_zero(length, 1000)    = " << boost::int256::div_to_zero(length, block) << std::endl;
    std::cout << "(length + 999) / 1000        = " << ((length + 999U) / block) << " (wrapped)" << std::endl;

    std::cout << "\n=== Unbiased rounding of a scaled value ===" << std::endl;

    // Rounding half to even keeps a long running sum from drifting upwards, which is
    // what operator/ plus a manual half-adjustment would do
    constexpr uint256 scale {1000U};
    const uint256 samples[] {uint256{1500U}, uint256{2500U}, uint256{3500U}, uint256{4500U}};

    for (const auto& sample : samples)
    {
        std::cout << sample << " / 1000: ties_to_even = " << boost::int256::div_ties_to_even(sample, scale)
                  << ", ties_away_zero = " << boost::int256::div_ties_away_zero(sample, scale) << std::endl;
    }

    std::cout << "\n=== Every rounding mode on -12 / 5, with int256 ===" << std::endl;

    using boost::int256::int256;

    // The exact quotient is -2.4, so the nearest integer is -2 and no mode ties.
    // Unlike uint256 above, every mode can now disagree with every other.
    constexpr int256 sx {-12};
    constexpr int256 sy {5};

    std::cout << "div_to_zero          = " << boost::int256::div_to_zero(sx, sy) << std::endl;
    std::cout << "div_away_zero        = " << boost::int256::div_away_zero(sx, sy) << std::endl;
    std::cout << "div_to_pos_inf       = " << boost::int256::div_to_pos_inf(sx, sy) << std::endl;
    std::cout << "div_to_neg_inf       = " << boost::int256::div_to_neg_inf(sx, sy) << std::endl;
    std::cout << "div_euclid           = " << boost::int256::div_euclid(sx, sy) << std::endl;
    std::cout << "div_ties_to_zero     = " << boost::int256::div_ties_to_zero(sx, sy) << std::endl;

    constexpr auto s_floored {boost::int256::div_rem_to_neg_inf(sx, sy)};
    std::cout << "\ndiv_rem_to_neg_inf(-12, 5): quotient = " << s_floored.quotient
              << ", remainder = " << s_floored.remainder << std::endl;

    constexpr auto s_truncated {boost::int256::div_rem_to_zero(sx, sy)};
    std::cout << "div_rem_to_zero(-12, 5):    quotient = " << s_truncated.quotient
              << ", remainder = " << s_truncated.remainder << std::endl;

    std::cout << "quotient * y + remainder = " << (s_floored.quotient * sy + s_floored.remainder) << std::endl;

    std::cout << "\n=== Euclidean remainder is never negative ===" << std::endl;

    // operator% takes its sign from the dividend, which makes it a poor fit for
    // wrapping an offset into a range. rem_euclid always lands in [0, abs(y)).
    constexpr int256 modulus {7};
    for (int256 offset {-9}; offset <= -6; ++offset)
    {
        std::cout << offset << " % 7 = " << (offset % modulus)
                  << ", rem_euclid(" << offset << ", 7) = " << boost::int256::rem_euclid(offset, modulus) << std::endl;
    }

    std::cout << "\n=== MIN / -1, the one signed overflow this library defines ===" << std::endl;

    // The paper's other division rules are unaffected: only the truncating
    // quotient overflows here, and this library wraps it back to MIN rather
    // than leaving it undefined, matching every other overflowing signed op.
    constexpr auto min_value {(std::numeric_limits<int256>::min)()};
    std::cout << "div_to_zero(MIN, -1)      = " << boost::int256::div_to_zero(min_value, int256{-1}) << std::endl;
    std::cout << "div_rem_to_zero(MIN, -1)  = quotient "
              << boost::int256::div_rem_to_zero(min_value, int256{-1}).quotient
              << ", remainder " << boost::int256::div_rem_to_zero(min_value, int256{-1}).remainder << std::endl;

    return 0;
}
