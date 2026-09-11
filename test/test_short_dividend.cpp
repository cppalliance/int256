// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// A dividend narrower than the divisor. Algorithm D computes m - n to size the
// quotient loop, so a dividend with fewer significant words than the divisor
// underflows that subtraction if the dispatcher does not take the lhs < rhs exit
// first. Boost.Decimal hit exactly this with 1230 / 10^20, which is the case
// this file is built around.

#include <boost/int256.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstdint>

using boost::int256::uint256;
using boost::int256::div;

namespace
{

// 10^20 = {7766279631452241920, 5, 0, 0} and 10^37 = {68739955140067328,
// 542101086242752217, 0, 0}, both recomputed against multiprecision.
constexpr uint256 ten_to_20 {UINT64_C(0), UINT64_C(0), UINT64_C(5), UINT64_C(7766279631452241920)};
constexpr uint256 ten_to_37 {UINT64_C(0), UINT64_C(0), UINT64_C(542101086242752217), UINT64_C(68739955140067328)};
constexpr uint256 ten_to_17 {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(100000000000000000)};
constexpr uint256 small {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1230)};

// The regression itself: a one word dividend over a two word divisor.
static_assert(small / ten_to_20 == uint256{}, "1230 / 10^20 is zero");
static_assert(small % ten_to_20 == small, "1230 % 10^20 is 1230");

// And the other direction, where the quotient is a single word.
static_assert(ten_to_20 / small == uint256{UINT64_C(81300813008130081)}, "10^20 / 1230");
static_assert(ten_to_20 % small == uint256{UINT64_C(370)}, "10^20 % 1230");

// Two word over two word, exact.
static_assert(ten_to_37 / ten_to_20 == ten_to_17, "10^37 / 10^20 is 10^17");
static_assert(ten_to_37 % ten_to_20 == uint256{}, "10^37 / 10^20 is exact");

uint256 opaque(const uint256& v)
{
    volatile std::uint64_t w3 {v.words[3]};
    volatile std::uint64_t w2 {v.words[2]};
    volatile std::uint64_t w1 {v.words[1]};
    volatile std::uint64_t w0 {v.words[0]};

    return uint256{w3, w2, w1, w0};
}

// The same cases at run time, where the dispatcher may take the reciprocal path.
void test_runtime()
{
    BOOST_TEST_EQ(opaque(small) / opaque(ten_to_20), uint256{});
    BOOST_TEST_EQ(opaque(small) % opaque(ten_to_20), small);

    BOOST_TEST_EQ(opaque(ten_to_20) / opaque(small), uint256{UINT64_C(81300813008130081)});
    BOOST_TEST_EQ(opaque(ten_to_20) % opaque(small), uint256{UINT64_C(370)});

    BOOST_TEST_EQ(opaque(ten_to_37) / opaque(ten_to_20), ten_to_17);
    BOOST_TEST_EQ(opaque(ten_to_37) % opaque(ten_to_20), uint256{});

    const auto result {div(opaque(small), opaque(ten_to_20))};
    BOOST_TEST_EQ(result.quot, uint256{});
    BOOST_TEST_EQ(result.rem, small);

    const auto other {div(opaque(ten_to_37), opaque(ten_to_20))};
    BOOST_TEST_EQ(other.quot, ten_to_17);
    BOOST_TEST_EQ(other.rem, uint256{});
}

// Every combination of a narrower dividend and a wider divisor, so the m - n
// underflow is driven for each word count rather than only for 1 over 2.
void test_every_narrower_pair()
{
    const uint256 by_word_count[4] {
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0x0123456789ABCDEF)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x0123456789ABCDEF), UINT64_C(0xFEDCBA9876543210)},
        uint256{UINT64_C(0), UINT64_C(0x0123456789ABCDEF), UINT64_C(0xFEDCBA9876543210), UINT64_C(0x1122334455667788)},
        uint256{UINT64_C(0x0123456789ABCDEF), UINT64_C(0xFEDCBA9876543210), UINT64_C(0x1122334455667788), UINT64_C(0x99AABBCCDDEEFF00)}
    };

    for (int dividend_words {0}; dividend_words < 4; ++dividend_words)
    {
        for (int divisor_words {dividend_words + 1}; divisor_words < 4; ++divisor_words)
        {
            const auto dividend {opaque(by_word_count[dividend_words])};
            const auto divisor {opaque(by_word_count[divisor_words])};

            BOOST_TEST(dividend < divisor);
            BOOST_TEST_EQ(dividend / divisor, uint256{});
            BOOST_TEST_EQ(dividend % divisor, dividend);

            const auto result {div(dividend, divisor)};
            BOOST_TEST_EQ(result.quot, uint256{});
            BOOST_TEST_EQ(result.rem, dividend);
        }
    }

    // A dividend one less than the divisor, for every divisor width. This is the
    // largest dividend that still yields a zero quotient.
    for (int i {0}; i < 4; ++i)
    {
        const auto divisor {opaque(by_word_count[i])};
        const auto dividend {opaque(by_word_count[i] - 1U)};

        BOOST_TEST_EQ(dividend / divisor, uint256{});
        BOOST_TEST_EQ(dividend % divisor, dividend);
    }

    // Zero over anything.
    for (int i {0}; i < 4; ++i)
    {
        const auto divisor {opaque(by_word_count[i])};
        BOOST_TEST_EQ(opaque(uint256{}) / divisor, uint256{});
        BOOST_TEST_EQ(opaque(uint256{}) % divisor, uint256{});
    }
}

// A dividend equal to the divisor is the boundary of the early exit.
void test_equal_operands()
{
    const uint256 values[] {
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)},
        ten_to_20,
        ten_to_37,
        uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX},
        uint256{UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)}
    };

    for (const auto& v : values)
    {
        const auto x {opaque(v)};
        BOOST_TEST_EQ(x / x, uint256{UINT64_C(1)});
        BOOST_TEST_EQ(x % x, uint256{});

        const auto result {div(x, x)};
        BOOST_TEST_EQ(result.quot, uint256{UINT64_C(1)});
        BOOST_TEST_EQ(result.rem, uint256{});
    }
}

} // namespace

int main()
{
    test_runtime();
    test_every_narrower_pair();
    test_equal_operands();

    return boost::report_errors();
}
