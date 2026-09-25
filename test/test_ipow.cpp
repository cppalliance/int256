// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// ipow, integer exponentiation by squaring. The reference is a naive repeated
// multiplication, which is a different algorithm reaching the same answer, and
// the results are also checked in 512 bits so that a wrapped answer is only
// accepted where the exact one genuinely does not fit.

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
using boost::int256::ipow;

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
using boost::int256::int256;
#endif

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 u256_one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
constexpr int256 min_i {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
constexpr int256 max_i {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
#endif

// Repeated multiplication, wrapping exactly as the library does.
uint256 reference_ipow(const uint256& base, std::uint64_t exp)
{
    uint256 result {1U};
    for (std::uint64_t i {0}; i < exp; ++i)
    {
        result = result * base;
    }

    return result;
}

void check_ipow(const uint256& base, std::uint64_t exp)
{
    const auto got {ipow(base, exp)};
    const auto expected {reference_ipow(base, exp)};

    if (got != expected)
    {
        std::cerr << "ipow mismatch\n"
                  << "  base     = " << hex_words(base) << '\n'
                  << "  exponent = " << exp << '\n'
                  << "  got      = " << hex_words(got) << '\n'
                  << "  expected = " << hex_words(expected) << std::endl;
    }

    BOOST_TEST_EQ(got, expected);

    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    // Where the exact answer fits 256 bits, it is the answer: no wrapping is
    // allowed to sneak in early.
    wide exact {1};
    bool overflowed {false};
    for (std::uint64_t i {0}; i < exp; ++i)
    {
        exact *= to_wide(base);
        if (!fits_256(exact))
        {
            overflowed = true;
            break;
        }
    }

    if (!overflowed)
    {
        BOOST_TEST(to_wide(got) == exact);
    }
    #endif
}

void test_known_answers()
{
    BOOST_TEST_EQ(ipow(uint256{2U}, 0U), u256_one);
    BOOST_TEST_EQ(ipow(uint256{2U}, 1U), uint256{2U});
    BOOST_TEST_EQ(ipow(uint256{2U}, 10U), uint256{1024U});
    BOOST_TEST_EQ(ipow(uint256{3U}, 5U), uint256{243U});
    BOOST_TEST_EQ(ipow(uint256{10U}, 19U), uint256{UINT64_C(10000000000000000000)});

    // Anything to the zero is one, including zero.
    BOOST_TEST_EQ(ipow(uint256{}, 0U), u256_one);
    BOOST_TEST_EQ(ipow(uint256_max(), 0U), u256_one);

    // Zero to a positive power is zero, and one to any power is one.
    BOOST_TEST_EQ(ipow(uint256{}, 1U), uint256{});
    BOOST_TEST_EQ(ipow(uint256{}, 1000U), uint256{});
    BOOST_TEST_EQ(ipow(u256_one, 0U), u256_one);
    BOOST_TEST_EQ(ipow(u256_one, UINT64_MAX), u256_one);

    // 2^k for every representable k, which is the exact boundary of the range.
    for (std::uint64_t k {0}; k < 256U; ++k)
    {
        BOOST_TEST_EQ(ipow(uint256{2U}, k), u256_one << static_cast<unsigned>(k));
    }

    // 2^256 wraps to zero, and so does every higher power.
    BOOST_TEST_EQ(ipow(uint256{2U}, 256U), uint256{});
    BOOST_TEST_EQ(ipow(uint256{2U}, 300U), uint256{});

    // 10^77 fits, 10^78 does not.
    {
        uint256 power {1U};
        for (int k {0}; k < 77; ++k)
        {
            power = power * 10U;
        }
        BOOST_TEST_EQ(ipow(uint256{10U}, 77U), power);
        BOOST_TEST_EQ(ipow(uint256{10U}, 78U), power * 10U);
    }

    // The maximum squared is one, since (2^256 - 1)^2 == 1 modulo 2^256.
    BOOST_TEST_EQ(ipow(uint256_max(), 2U), u256_one);
    BOOST_TEST_EQ(ipow(uint256_max(), 1U), uint256_max());
    BOOST_TEST_EQ(ipow(uint256_max(), 3U), uint256_max());
}

void test_small_bases_exhaustively()
{
    // Every base from 0 to 20 against every exponent up to 40, which covers both
    // the exact and the wrapping regime for each.
    for (std::uint64_t base {0}; base <= 20U; ++base)
    {
        for (std::uint64_t exp {0}; exp <= 40U; ++exp)
        {
            check_ipow(uint256{base}, exp);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_random()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        // A wide base with a small exponent, where the answer wraps almost at once.
        check_ipow(random_uint256(rng, shape::word4), static_cast<std::uint64_t>(rng() % 8U));

        // A narrow base with a larger exponent, where the exact answer often fits.
        check_ipow(uint256{(rng() % 1000U) + 2U}, static_cast<std::uint64_t>(rng() % 30U));

        // A power of two base, whose answer is a single bit until it wraps.
        {
            const auto shift {static_cast<unsigned>(rng() % 64U)};
            const auto exp {static_cast<std::uint64_t>(rng() % 10U)};
            check_ipow(u256_one << shift, exp);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_constexpr()
{
    static_assert(ipow(uint256{2U}, 10U) == uint256{1024U}, "2^10");
    static_assert(ipow(uint256{3U}, 5U) == uint256{243U}, "3^5");
    static_assert(ipow(uint256{2U}, 0U) == uint256{1U}, "the zero exponent");
    static_assert(ipow(uint256{}, 0U) == uint256{1U}, "zero to the zero is one");
    static_assert(ipow(uint256{}, 5U) == uint256{}, "zero to a positive power");
    static_assert(ipow(uint256{2U}, 255U) == (uint256{UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)}),
                  "the top bit");
    static_assert(ipow(uint256{2U}, 256U) == uint256{}, "one past the range wraps");
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

//=====================================
// int256: the same squaring algorithm, wrapping modulo 2^256 exactly like
// operator*, so a negative base's sign flips with the parity of the exponent
// right up until the point it wraps
//=====================================

// Repeated multiplication, wrapping exactly as the library does: a different
// algorithm from the library's squaring, sharing only int256's own (already
// independently tested) operator*.
int256 reference_ipow_i(const int256& base, std::uint64_t exp)
{
    int256 result {1};
    for (std::uint64_t i {0}; i < exp; ++i)
    {
        result = result * base;
    }

    return result;
}

void check_ipow_i(const int256& base, std::uint64_t exp)
{
    const auto got {ipow(base, exp)};
    const auto expected {reference_ipow_i(base, exp)};

    #ifndef BOOST_INT256_ORACLE_UNAVAILABLE
    BOOST_TEST(got == expected ||
               report_mismatch_i("ipow", base, base, got, to_smp(expected)));

    // Where the exact (unwrapped) answer fits int256's range, it is the
    // answer: no wrapping is allowed to sneak in early.
    const smp smp_min {-(smp{1} << 255)};
    const smp smp_max {(smp{1} << 255) - 1};

    smp exact {1};
    bool overflowed {false};
    for (std::uint64_t i {0}; i < exp; ++i)
    {
        exact *= to_smp(base);
        if (exact < smp_min || exact > smp_max)
        {
            overflowed = true;
            break;
        }
    }

    if (!overflowed)
    {
        BOOST_TEST(to_smp(got) == exact);
    }
    #else
    BOOST_TEST(got == expected);
    #endif
}

void test_known_answers_i()
{
    BOOST_TEST(ipow(int256{2}, 0U) == int256{1});
    BOOST_TEST(ipow(int256{-2}, 0U) == int256{1});
    BOOST_TEST(ipow(int256{2}, 1U) == int256{2});
    BOOST_TEST(ipow(int256{-2}, 1U) == int256{-2});
    BOOST_TEST(ipow(int256{-2}, 10U) == int256{1024});
    BOOST_TEST(ipow(int256{-2}, 3U) == int256{-8});
    BOOST_TEST(ipow(int256{-3}, 5U) == int256{-243});

    // Zero to a positive power is zero; one and minus one never grow.
    BOOST_TEST(ipow(int256{}, 1U) == int256{});
    BOOST_TEST(ipow(int256{}, 1000U) == int256{});
    BOOST_TEST(ipow(int256{1}, UINT64_MAX) == int256{1});
    // (-1)^exp alternates with the exponent's parity.
    BOOST_TEST(ipow(int256{-1}, 8U) == int256{1});
    BOOST_TEST(ipow(int256{-1}, 7U) == int256{-1});
    BOOST_TEST(ipow(int256{-1}, UINT64_MAX) == int256{-1});

    // 2^255 is exactly MIN's bit pattern, and once again the top bit.
    BOOST_TEST(ipow(int256{2}, 255U) == min_i);
    // 2^256 and beyond wraps to zero, same as the unsigned case (only the
    // bit pattern matters for the wraparound, not the sign interpretation).
    BOOST_TEST(ipow(int256{2}, 256U) == int256{});
    BOOST_TEST(ipow(int256{2}, 300U) == int256{});

    // MAX^2 == 1 and MAX^3 == MAX, mirroring uint256_max()'s own -1-like
    // behavior modulo 2^256.
    BOOST_TEST(ipow(max_i, 2U) == int256{1});
    BOOST_TEST(ipow(max_i, 1U) == max_i);
    BOOST_TEST(ipow(max_i, 3U) == max_i);

    // MIN squared already wraps to zero (MIN's magnitude, 2^255, squared is
    // a multiple of 2^256), so every higher power of MIN is zero too.
    BOOST_TEST(ipow(min_i, 2U) == int256{});
    BOOST_TEST(ipow(min_i, 3U) == int256{});
    BOOST_TEST(ipow(min_i, 1U) == min_i);
}

void test_small_bases_exhaustively_i()
{
    // Every base from -20 to 20 against every exponent up to 40, covering
    // both the exact and the wrapping regime, and every sign of base.
    for (std::int64_t base {-20}; base <= 20; ++base)
    {
        for (std::uint64_t exp {0}; exp <= 40U; ++exp)
        {
            check_ipow_i(int256{base}, exp);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_random_i()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        // A wide base with a small exponent, where the answer wraps almost
        // at once.
        check_ipow_i(random_int256(rng, shape::word4, sign::mixed), static_cast<std::uint64_t>(rng() % 8U));

        // A narrow base with a larger exponent, where the exact answer often
        // fits.
        check_ipow_i(int256{static_cast<std::int64_t>((rng() % 1000U) + 2U) * (rng() & 1U ? 1 : -1)},
                     static_cast<std::uint64_t>(rng() % 30U));

        // A power of two base (always non-negative here, since the sign
        // behavior of a negative base is already covered above), whose
        // answer is a single bit until it wraps.
        {
            const auto shift {static_cast<unsigned>(rng() % 64U)};
            const auto exp {static_cast<std::uint64_t>(rng() % 10U)};
            check_ipow_i(static_cast<int256>(u256_one << shift), exp);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_constexpr_i()
{
    static_assert(ipow(int256{-2}, 10U) == int256{1024}, "(-2)^10");
    static_assert(ipow(int256{-3}, 5U) == int256{-243}, "(-3)^5");
    static_assert(ipow(int256{2}, 0U) == int256{1}, "the zero exponent");
    static_assert(ipow(int256{}, 0U) == int256{1}, "zero to the zero is one");
    static_assert(ipow(int256{}, 5U) == int256{}, "zero to a positive power");
    static_assert(ipow(int256{-1}, 8U) == int256{1}, "an even power of -1");
    static_assert(ipow(int256{-1}, 7U) == int256{-1}, "an odd power of -1");
    static_assert(ipow(int256{2}, 255U) == min_i, "the top bit is MIN as signed");
    static_assert(ipow(int256{2}, 256U) == int256{}, "one past the range wraps");
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_constexpr();
    test_known_answers();
    test_small_bases_exhaustively();
    test_random();

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_constexpr_i();
    test_known_answers_i();
    test_small_bases_exhaustively_i();
    test_random_i();
    #endif

    return boost::report_errors();
}
