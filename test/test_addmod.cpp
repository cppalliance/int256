// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// addmod, and the mulmod helpers behind powm. The interesting case is a sum that
// overflows 256 bits before the reduction: (a + b) % m is wrong if computed that
// way, so the implementation has to notice the carry. The reference is done in
// 512 bits, where the sum is exact.

#include "oracle.hpp"

#ifdef BOOST_INT256_ORACLE_UNAVAILABLE

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <boost/int256/utilities.hpp>

#include <cstddef>
#include <cstdint>

using boost::int256::uint256;
using boost::int256::detail::addmod;
using boost::int256::detail::mulmod_shift;
using boost::int256::detail::mulmod_word;

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

void check_addmod(const uint256& a, const uint256& b, const uint256& m)
{
    const auto got {addmod(a, b, m)};

    // Exact in 512 bits, so an overflowing sum is still the right sum.
    const wide expected {wide{to_wide(a) + to_wide(b)} % to_wide(m)};

    if (to_wide(got) != expected)
    {
        std::cerr << "addmod mismatch\n"
                  << "  a        = " << hex_words(a) << '\n'
                  << "  b        = " << hex_words(b) << '\n'
                  << "  m        = " << hex_words(m) << '\n'
                  << "  got      = " << hex_words(got) << '\n'
                  << "  expected = " << expected << std::endl;
    }

    BOOST_TEST(to_wide(got) == expected);
    BOOST_TEST(got < m);
}

void check_mulmod(const uint256& a, const uint256& b, const uint256& m)
{
    const auto got {mulmod_shift(a, b, m)};
    const wide expected {wide{to_wide(a) * to_wide(b)} % to_wide(m)};

    if (to_wide(got) != expected)
    {
        std::cerr << "mulmod_shift mismatch\n"
                  << "  a        = " << hex_words(a) << '\n'
                  << "  b        = " << hex_words(b) << '\n'
                  << "  m        = " << hex_words(m) << '\n'
                  << "  got      = " << hex_words(got) << '\n'
                  << "  expected = " << expected << std::endl;
    }

    BOOST_TEST(to_wide(got) == expected);
    BOOST_TEST(got < m);
}

// A sum that carries out of the top word is the whole reason this function is not
// just (a + b) % m.
void test_overflowing_sums()
{
    const auto max {uint256_max()};

    // Two operands just below the maximum, with a modulus that does not divide
    // their sum. The exact sum is close to 2^257.
    const uint256 moduli[] {
        uint256{3U}, uint256{1000U}, uint256{UINT64_MAX},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)},
        max, max - 1U,
        uint256{UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)}
    };

    for (const auto& m : moduli)
    {
        if (m < 2U)
        {
            continue;
        }

        // Reduce first so the operands are legal inputs, then add.
        const uint256 a {max % m};
        const uint256 b {(max - 1U) % m};

        check_addmod(a, b, m);
        check_addmod(b, a, m);

        // And the extreme: m - 1 plus m - 1, which is the largest legal sum.
        check_addmod(m - 1U, m - 1U, m);
        check_addmod(m - 1U, one % m, m);
        check_addmod(uint256{}, m - 1U, m);
        check_addmod(m - 1U, uint256{}, m);
    }

    // A modulus above 2^255, so m - 1 plus m - 1 is definitely above 2^256.
    {
        const uint256 m {UINT64_C(0xF000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
        check_addmod(m - 1U, m - 1U, m);
        check_addmod(m - 1U, m / 2U, m);
        check_addmod(m / 2U, m / 2U, m);
    }
}

void test_known_answers()
{
    BOOST_TEST_EQ(addmod(uint256{5U}, uint256{7U}, uint256{10U}), uint256{2U});
    BOOST_TEST_EQ(addmod(uint256{}, uint256{}, uint256{10U}), uint256{});
    BOOST_TEST_EQ(addmod(uint256{9U}, uint256{1U}, uint256{10U}), uint256{});
    BOOST_TEST_EQ(addmod(uint256{9U}, uint256{9U}, uint256{10U}), uint256{8U});

    // The maximum plus the maximum, modulo the maximum, is the maximum minus one
    // reduced: 2 * (m - 1) mod m is m - 2.
    const auto max {uint256_max()};
    BOOST_TEST_EQ(addmod(max - 1U, max - 1U, max), max - 2U);

    BOOST_TEST_EQ(mulmod_shift(uint256{5U}, uint256{7U}, uint256{10U}), uint256{5U});
    BOOST_TEST_EQ(mulmod_shift(uint256{}, uint256{7U}, uint256{10U}), uint256{});
    BOOST_TEST_EQ(mulmod_shift(uint256{7U}, uint256{}, uint256{10U}), uint256{});

    // mulmod_word is the single word path.
    BOOST_TEST_EQ(mulmod_word(UINT64_C(5), UINT64_C(7), UINT64_C(10)), UINT64_C(5));
    BOOST_TEST_EQ(mulmod_word(UINT64_MAX, UINT64_MAX, UINT64_C(1000000007)),
                  static_cast<std::uint64_t>((static_cast<unsigned long long>(UINT64_MAX) % UINT64_C(1000000007)) *
                                             (static_cast<unsigned long long>(UINT64_MAX) % UINT64_C(1000000007)) %
                                             UINT64_C(1000000007)));
}

// The word-level modular multiply, whose product needs 128 bits.
void test_mulmod_word()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        auto m {rng()};
        if (m < 2U)
        {
            m = 2U;
        }

        const auto a {rng() % m};
        const auto b {rng() % m};

        const auto got {mulmod_word(a, b, m)};
        const wide expected {wide{wide{a} * wide{b}} % wide{m}};

        BOOST_TEST(wide{got} == expected);
        BOOST_TEST(got < m);

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  a = " << a << ", b = " << b << ", m = " << m << std::endl;
            return;
        }
    }

    // The extremes, where the product is close to 2^128.
    BOOST_TEST_EQ(mulmod_word(UINT64_MAX - 1U, UINT64_MAX - 1U, UINT64_MAX), UINT64_C(1));
    BOOST_TEST_EQ(mulmod_word(UINT64_C(0), UINT64_MAX, UINT64_MAX), UINT64_C(0));
    BOOST_TEST_EQ(mulmod_word(UINT64_C(1), UINT64_MAX - 1U, UINT64_MAX), UINT64_MAX - 1U);
}

void test_random()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 moduli[] {
            uint256{(rng() % 1000U) + 2U},
            uint256{rng() | UINT64_C(2)},
            uint256{UINT64_C(0), UINT64_C(0), rng() | UINT64_C(1), rng()},
            uint256{rng() | UINT64_C(1), rng(), rng(), rng()}
        };

        for (const auto& m : moduli)
        {
            if (m < 2U)
            {
                continue;
            }

            const uint256 a {random_uint256(rng, shape::word4) % m};
            const uint256 b {random_uint256(rng, shape::word4) % m};

            check_addmod(a, b, m);
            check_mulmod(a, b, m);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_constexpr()
{
    static_assert(addmod(uint256{5U}, uint256{7U}, uint256{10U}) == uint256{2U}, "5 + 7 mod 10");
    static_assert(addmod(uint256{9U}, uint256{1U}, uint256{10U}) == uint256{}, "the sum reaches the modulus");
    static_assert(mulmod_shift(uint256{5U}, uint256{7U}, uint256{10U}) == uint256{5U}, "5 * 7 mod 10");

    // An overflowing sum at compile time.
    constexpr uint256 max {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    static_assert(addmod(max - 1U, max - 1U, max) == max - 2U, "the sum carries past 2^256");
}

} // namespace

int main()
{
    test_constexpr();
    test_known_answers();
    test_overflowing_sums();
    test_mulmod_word();
    test_random();

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE
