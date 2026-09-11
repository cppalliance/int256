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

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

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
    BOOST_TEST_EQ(ipow(uint256{2U}, 0U), one);
    BOOST_TEST_EQ(ipow(uint256{2U}, 1U), uint256{2U});
    BOOST_TEST_EQ(ipow(uint256{2U}, 10U), uint256{1024U});
    BOOST_TEST_EQ(ipow(uint256{3U}, 5U), uint256{243U});
    BOOST_TEST_EQ(ipow(uint256{10U}, 19U), uint256{UINT64_C(10000000000000000000)});

    // Anything to the zero is one, including zero.
    BOOST_TEST_EQ(ipow(uint256{}, 0U), one);
    BOOST_TEST_EQ(ipow(uint256_max(), 0U), one);

    // Zero to a positive power is zero, and one to any power is one.
    BOOST_TEST_EQ(ipow(uint256{}, 1U), uint256{});
    BOOST_TEST_EQ(ipow(uint256{}, 1000U), uint256{});
    BOOST_TEST_EQ(ipow(one, 0U), one);
    BOOST_TEST_EQ(ipow(one, UINT64_MAX), one);

    // 2^k for every representable k, which is the exact boundary of the range.
    for (std::uint64_t k {0}; k < 256U; ++k)
    {
        BOOST_TEST_EQ(ipow(uint256{2U}, k), one << static_cast<unsigned>(k));
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
    BOOST_TEST_EQ(ipow(uint256_max(), 2U), one);
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
            check_ipow(one << shift, exp);
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

} // namespace

int main()
{
    test_constexpr();
    test_known_answers();
    test_small_bases_exhaustively();
    test_random();

    return boost::report_errors();
}
