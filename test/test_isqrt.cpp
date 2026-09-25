// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// isqrt, the integer square root. The defining property is checkable without any
// reference at all: r * r <= n < (r + 1) * (r + 1), with the upper half done in
// 512 bits so that (r + 1)^2 does not wrap for a value near the top of the range.
// That is the strongest statement available, so it is checked on every operand.

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
using boost::int256::isqrt;

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
using boost::int256::int256;
#endif

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
constexpr int256 min_i {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
constexpr int256 max_i {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
#endif

// The bracketing property, which fully determines the answer.
void check_isqrt(const uint256& n)
{
    const auto r {isqrt(n)};

    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    const wide root {to_wide(r)};
    const wide value {to_wide(n)};

    const bool lower_ok {wide{root * root} <= value};
    const bool upper_ok {wide{wide{root + 1} * wide{root + 1}} > value};

    if (!lower_ok || !upper_ok)
    {
        std::cerr << "isqrt out of bracket\n"
                  << "  n    = " << hex_words(n) << '\n'
                  << "  root = " << hex_words(r) << '\n'
                  << "  r^2  = " << wide{root * root} << '\n'
                  << "  next = " << wide{wide{root + 1} * wide{root + 1}} << std::endl;
    }

    BOOST_TEST(lower_ok);
    BOOST_TEST(upper_ok);

    // And against multiprecision's own square root, which is a different algorithm.
    BOOST_TEST(to_mp(r) == boost::multiprecision::sqrt(to_mp(n)) ||
               report_mismatch("isqrt", n, r, boost::multiprecision::sqrt(to_mp(n))));
    #endif

    // The root never exceeds 2^128 - 1, since (2^128)^2 is already 2^256.
    BOOST_TEST(r <= (one << 128U) - 1U);
}

void test_known_answers()
{
    BOOST_TEST_EQ(isqrt(uint256{}), uint256{});
    BOOST_TEST_EQ(isqrt(uint256{1U}), uint256{1U});
    BOOST_TEST_EQ(isqrt(uint256{2U}), uint256{1U});
    BOOST_TEST_EQ(isqrt(uint256{3U}), uint256{1U});
    BOOST_TEST_EQ(isqrt(uint256{4U}), uint256{2U});
    BOOST_TEST_EQ(isqrt(uint256{8U}), uint256{2U});
    BOOST_TEST_EQ(isqrt(uint256{9U}), uint256{3U});
    BOOST_TEST_EQ(isqrt(uint256{15U}), uint256{3U});
    BOOST_TEST_EQ(isqrt(uint256{16U}), uint256{4U});
    BOOST_TEST_EQ(isqrt(uint256{99U}), uint256{9U});
    BOOST_TEST_EQ(isqrt(uint256{100U}), uint256{10U});

    // The maximum: its root is 2^128 - 1, because (2^128 - 1)^2 < 2^256 - 1.
    BOOST_TEST_EQ(isqrt(uint256_max()), (one << 128U) - 1U);
}

// Perfect squares, and the values either side of them, which is where an off by
// one in the Newton iteration shows up.
void test_perfect_squares()
{
    // Every 2^(2k) has an exact root of 2^k.
    for (unsigned k {0}; k < 128U; ++k)
    {
        const uint256 square {one << (2U * k)};
        BOOST_TEST_EQ(isqrt(square), one << k);

        // One below a perfect square rounds down to root - 1.
        if (k > 0U)
        {
            BOOST_TEST_EQ(isqrt(square - 1U), (one << k) - 1U);
        }

        // And one above still gives root.
        if (2U * k < 255U)
        {
            BOOST_TEST_EQ(isqrt(square + 1U), one << k);
        }
    }

    // Squares of a spread of wide values.
    for (std::size_t i {0}; i < N; ++i)
    {
        // A root that fits 128 bits, so the square fits 256.
        const uint256 root {UINT64_C(0), UINT64_C(0), rng(), rng()};
        const uint256 square {root * root};

        // Only meaningful when the square did not wrap.
        #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
        if (!fits_256(wide{to_wide(root) * to_wide(root)}))
        {
            continue;
        }
        #endif

        BOOST_TEST_EQ(isqrt(square), root);

        if (root != 0U)
        {
            BOOST_TEST_EQ(isqrt(square - 1U), root - 1U);
        }
        BOOST_TEST_EQ(isqrt(square + 1U), root);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// 2^k - 1 for every k, called out in the plan.
void test_powers_of_two_minus_one()
{
    for (unsigned k {1}; k <= 256U; ++k)
    {
        const uint256 value {k == 256U ? uint256_max() : (one << k) - 1U};
        check_isqrt(value);

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  k = " << k << std::endl;
            return;
        }
    }
}

void test_corpus_and_random()
{
    for (const auto& v : edge_values())
    {
        check_isqrt(v);
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        for (const auto s : all_shapes())
        {
            check_isqrt(random_uint256(rng, s));
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_constexpr()
{
    static_assert(isqrt(uint256{}) == uint256{}, "isqrt(0)");
    static_assert(isqrt(uint256{1U}) == uint256{1U}, "isqrt(1)");
    static_assert(isqrt(uint256{15U}) == uint256{3U}, "isqrt(15)");
    static_assert(isqrt(uint256{16U}) == uint256{4U}, "isqrt(16)");
    static_assert(isqrt(uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}) ==
                  (uint256{UINT64_C(0), UINT64_C(0), UINT64_MAX, UINT64_MAX}),
                  "the root of the maximum is 2^128 - 1");
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

//=====================================
// int256: a negative input returns 0; a non-negative one runs the exact same
// bracketing algorithm as the unsigned overload, on the exact same bit
// pattern, so the interesting cases are all about the sign boundary
//=====================================

void check_isqrt_i(const int256& n)
{
    const auto r {isqrt(n)};

    if (n.signed_high() < 0)
    {
        BOOST_TEST(r == int256{});
        return;
    }

    // A non-negative input's root is never negative: it is at most the root
    // of MAX, far below the sign bit.
    BOOST_TEST(r >= int256{});

    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    const smp root {to_smp(r)};
    const smp value {to_smp(n)};

    const bool lower_ok {root * root <= value};
    const bool upper_ok {(root + 1) * (root + 1) > value};

    if (!lower_ok || !upper_ok)
    {
        std::cerr << "int256 isqrt out of bracket\n"
                  << "  n    = " << hex_words(n) << '\n'
                  << "  root = " << hex_words(r) << std::endl;
    }

    BOOST_TEST(lower_ok);
    BOOST_TEST(upper_ok);
    #endif
}

void test_known_answers_i()
{
    BOOST_TEST(isqrt(int256{}) == int256{});
    BOOST_TEST(isqrt(int256{1}) == int256{1});
    BOOST_TEST(isqrt(int256{4}) == int256{2});
    BOOST_TEST(isqrt(int256{15}) == int256{3});
    BOOST_TEST(isqrt(int256{16}) == int256{4});
    BOOST_TEST(isqrt(int256{99}) == int256{9});
    BOOST_TEST(isqrt(int256{100}) == int256{10});

    // Every negative input, whatever its magnitude, returns 0.
    BOOST_TEST(isqrt(int256{-1}) == int256{});
    BOOST_TEST(isqrt(int256{-4}) == int256{});
    BOOST_TEST(isqrt(int256{-100}) == int256{});
    BOOST_TEST(isqrt(min_i) == int256{});

    // MAX: its root is floor(sqrt(2^255 - 1)).
    check_isqrt_i(max_i);
}

// Perfect squares, and the values either side of them, restricted to the
// non-negative half of the range (everything below zero is already pinned
// to 0 above).
void test_perfect_squares_i()
{
    for (unsigned k {0}; k < 127U; ++k)
    {
        const int256 square {static_cast<int256>(one << (2U * k))};
        BOOST_TEST(isqrt(square) == static_cast<int256>(one << k));

        if (k > 0U)
        {
            BOOST_TEST(isqrt(square - 1) == static_cast<int256>((one << k) - 1U));
        }

        BOOST_TEST(isqrt(square + 1) == static_cast<int256>(one << k));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_corpus_and_random_i()
{
    for (const auto& v : signed_edge_values())
    {
        check_isqrt_i(v);
    }

    for (std::size_t i {0}; i < N; ++i)
    {
        check_isqrt_i(random_int256(rng, shape::word4, sign::mixed));
        check_isqrt_i(random_int256(rng, shape::word4, sign::pos));
        check_isqrt_i(random_int256(rng, shape::word4, sign::neg));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_constexpr_i()
{
    static_assert(isqrt(int256{}) == int256{}, "isqrt(0)");
    static_assert(isqrt(int256{15}) == int256{3}, "isqrt(15)");
    static_assert(isqrt(int256{16}) == int256{4}, "isqrt(16)");
    static_assert(isqrt(int256{-1}) == int256{}, "a negative input gives 0");
    static_assert(isqrt(min_i) == int256{}, "MIN gives 0 too");
    static_assert(isqrt(max_i) == static_cast<int256>(isqrt(static_cast<uint256>(max_i))),
                  "MAX matches the unsigned root of its own bit pattern");
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_constexpr();
    test_known_answers();
    test_perfect_squares();
    test_powers_of_two_minus_one();
    test_corpus_and_random();

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_constexpr_i();
    test_known_answers_i();
    test_perfect_squares_i();
    test_corpus_and_random_i();
    #endif

    return boost::report_errors();
}
