// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The <bit> style functions for uint256. The reference here is a bit by bit walk
// over the 256 bits of the value, which shares no code with the library, so the
// two implementations agreeing means something. Every function is also pinned at
// the word boundaries (bits 0, 63, 64, 127, 128, 191, 192 and 255), where an off
// by one in a word index would otherwise hide.

#ifndef BOOST_INT256_BUILD_MODULE

#include <boost/int256.hpp>

#else

import boost.int256;
// A module import carries no macros, so the BOOST_INT256_* feature and endianness
// macros have to come from the header even in the module build.
#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/literal_macros.hpp>

#endif

#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <cstdint>
#include <random>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
constexpr uint256 all_ones {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

// The interesting bit positions: both sides of every word boundary, plus the ends.
const unsigned boundary_bits[] {0U, 1U, 62U, 63U, 64U, 65U, 126U, 127U, 128U,
                                129U, 190U, 191U, 192U, 193U, 254U, 255U};

//=====================================
// Bit by bit references
//=====================================

bool bit_at(const uint256& v, unsigned index)
{
    return ((v.words[index / 64U] >> (index % 64U)) & 1U) != 0U;
}

int reference_popcount(const uint256& v)
{
    int count {0};
    for (unsigned i {0}; i < 256U; ++i)
    {
        if (bit_at(v, i))
        {
            ++count;
        }
    }

    return count;
}

int reference_countl_zero(const uint256& v)
{
    for (int i {255}; i >= 0; --i)
    {
        if (bit_at(v, static_cast<unsigned>(i)))
        {
            return 255 - i;
        }
    }

    return 256;
}

int reference_countl_one(const uint256& v)
{
    for (int i {255}; i >= 0; --i)
    {
        if (!bit_at(v, static_cast<unsigned>(i)))
        {
            return 255 - i;
        }
    }

    return 256;
}

int reference_countr_zero(const uint256& v)
{
    for (unsigned i {0}; i < 256U; ++i)
    {
        if (bit_at(v, i))
        {
            return static_cast<int>(i);
        }
    }

    return 256;
}

int reference_countr_one(const uint256& v)
{
    for (unsigned i {0}; i < 256U; ++i)
    {
        if (!bit_at(v, i))
        {
            return static_cast<int>(i);
        }
    }

    return 256;
}

int reference_bit_width(const uint256& v)
{
    return 256 - reference_countl_zero(v);
}

uint256 reference_rotl(const uint256& v, unsigned count)
{
    const auto amount {count & 255U};

    uint256 result {};
    for (unsigned i {0}; i < 256U; ++i)
    {
        if (bit_at(v, i))
        {
            const auto target {(i + amount) & 255U};
            result.words[target / 64U] |= (UINT64_C(1) << (target % 64U));
        }
    }

    return result;
}

uint256 reference_byteswap(const uint256& v)
{
    unsigned char bytes[32] {};
    for (unsigned i {0}; i < 32U; ++i)
    {
        bytes[i] = static_cast<unsigned char>(v.words[i / 8U] >> ((i % 8U) * 8U));
    }

    uint256 result {};
    for (unsigned i {0}; i < 32U; ++i)
    {
        // Byte i of the result is byte 31 - i of the operand.
        const auto source {bytes[31U - i]};
        result.words[i / 8U] |= (static_cast<std::uint64_t>(source) << ((i % 8U) * 8U));
    }

    return result;
}

//=====================================
// Every function against the reference
//=====================================

void check_all(const uint256& v)
{
    BOOST_TEST_EQ(boost::int256::popcount(v), reference_popcount(v));
    BOOST_TEST_EQ(boost::int256::countl_zero(v), reference_countl_zero(v));
    BOOST_TEST_EQ(boost::int256::countl_one(v), reference_countl_one(v));
    BOOST_TEST_EQ(boost::int256::countr_zero(v), reference_countr_zero(v));
    BOOST_TEST_EQ(boost::int256::countr_one(v), reference_countr_one(v));
    BOOST_TEST_EQ(boost::int256::bit_width(v), reference_bit_width(v));
    BOOST_TEST_EQ(boost::int256::byteswap(v), reference_byteswap(v));
    BOOST_TEST_EQ(boost::int256::has_single_bit(v), reference_popcount(v) == 1);

    // bit_floor is the highest set bit, or zero.
    if (v == 0U)
    {
        BOOST_TEST_EQ(boost::int256::bit_floor(v), uint256{});
    }
    else
    {
        BOOST_TEST_EQ(boost::int256::bit_floor(v), one << static_cast<unsigned>(reference_bit_width(v) - 1));
    }

    // bit_ceil is the smallest power of two at or above the value, and one for
    // anything below two.
    if (v <= 1U)
    {
        BOOST_TEST_EQ(boost::int256::bit_ceil(v), one);
    }
    else if (boost::int256::has_single_bit(v))
    {
        BOOST_TEST_EQ(boost::int256::bit_ceil(v), v);
    }
    else if (reference_bit_width(v) < 256)
    {
        BOOST_TEST_EQ(boost::int256::bit_ceil(v), one << static_cast<unsigned>(reference_bit_width(v)));
    }

    // Rotation is a permutation of the bits, so it agrees with the bit by bit
    // reference and the two directions are inverses.
    for (const auto count : boundary_bits)
    {
        BOOST_TEST_EQ(boost::int256::rotl(v, static_cast<int>(count)), reference_rotl(v, count));
        BOOST_TEST_EQ(boost::int256::rotr(v, static_cast<int>(count)), reference_rotl(v, 256U - (count & 255U)));
        BOOST_TEST_EQ(boost::int256::rotr(boost::int256::rotl(v, static_cast<int>(count)), static_cast<int>(count)), v);
    }

    // Byteswap is an involution.
    BOOST_TEST_EQ(boost::int256::byteswap(boost::int256::byteswap(v)), v);
}

//=====================================
// The word boundaries
//=====================================

void test_single_bits()
{
    for (unsigned k {0}; k < 256U; ++k)
    {
        const uint256 value {one << k};

        BOOST_TEST_EQ(boost::int256::countl_zero(value), static_cast<int>(255U - k));
        BOOST_TEST_EQ(boost::int256::countr_zero(value), static_cast<int>(k));
        BOOST_TEST_EQ(boost::int256::popcount(value), 1);
        BOOST_TEST_EQ(boost::int256::bit_width(value), static_cast<int>(k) + 1);
        BOOST_TEST(boost::int256::has_single_bit(value));
        BOOST_TEST_EQ(boost::int256::bit_floor(value), value);
        BOOST_TEST_EQ(boost::int256::bit_ceil(value), value);
        BOOST_TEST_EQ(boost::int256::countl_one(value), 0 + (k == 255U ? 1 : 0));
        BOOST_TEST_EQ(boost::int256::countr_one(value), k == 0U ? 1 : 0);

        // 2^k - 1 has k low bits set.
        if (k > 0U)
        {
            const uint256 mask {value - 1U};
            BOOST_TEST_EQ(boost::int256::popcount(mask), static_cast<int>(k));
            BOOST_TEST_EQ(boost::int256::bit_width(mask), static_cast<int>(k));
            BOOST_TEST_EQ(boost::int256::countr_one(mask), static_cast<int>(k));
            BOOST_TEST_EQ(boost::int256::countr_zero(mask), 0);
            BOOST_TEST_EQ(boost::int256::countl_zero(mask), static_cast<int>(256U - k));
        }

        // 2^k + 1 needs the next power of two, except at the very top.
        if (k > 0U && k < 255U)
        {
            const uint256 above {value + 1U};
            BOOST_TEST_EQ(boost::int256::bit_ceil(above), one << (k + 1U));
            BOOST_TEST_EQ(boost::int256::bit_floor(above), value);
            BOOST_TEST_EQ(boost::int256::popcount(above), 2);
        }
    }
}

void test_zero_and_all_ones()
{
    BOOST_TEST_EQ(boost::int256::countl_zero(uint256{}), 256);
    BOOST_TEST_EQ(boost::int256::countr_zero(uint256{}), 256);
    BOOST_TEST_EQ(boost::int256::countl_one(uint256{}), 0);
    BOOST_TEST_EQ(boost::int256::countr_one(uint256{}), 0);
    BOOST_TEST_EQ(boost::int256::popcount(uint256{}), 0);
    BOOST_TEST_EQ(boost::int256::bit_width(uint256{}), 0);
    BOOST_TEST_EQ(boost::int256::bit_floor(uint256{}), uint256{});
    BOOST_TEST_EQ(boost::int256::bit_ceil(uint256{}), one);
    BOOST_TEST(!boost::int256::has_single_bit(uint256{}));

    BOOST_TEST_EQ(boost::int256::countl_zero(all_ones), 0);
    // Every bit is set, so there are no trailing zeros.
    BOOST_TEST_EQ(boost::int256::countr_zero(all_ones), 0);
    BOOST_TEST_EQ(boost::int256::countl_one(all_ones), 256);
    BOOST_TEST_EQ(boost::int256::countr_one(all_ones), 256);
    BOOST_TEST_EQ(boost::int256::popcount(all_ones), 256);
    BOOST_TEST_EQ(boost::int256::bit_width(all_ones), 256);
    BOOST_TEST_EQ(boost::int256::bit_floor(all_ones), one << 255U);
    BOOST_TEST(!boost::int256::has_single_bit(all_ones));

    // One is the only value whose bit_ceil is one along with zero.
    BOOST_TEST_EQ(boost::int256::bit_ceil(one), one);
    BOOST_TEST_EQ(boost::int256::bit_ceil(uint256{2U}), uint256{2U});
    BOOST_TEST_EQ(boost::int256::bit_ceil(uint256{3U}), uint256{4U});
}

void test_alternating_words()
{
    const uint256 fives {UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
                         UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)};
    const uint256 aas {UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xAAAAAAAAAAAAAAAA),
                       UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xAAAAAAAAAAAAAAAA)};
    const uint256 mixed {UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0x5555555555555555),
                         UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0x5555555555555555)};

    BOOST_TEST_EQ(boost::int256::popcount(fives), 128);
    BOOST_TEST_EQ(boost::int256::popcount(aas), 128);
    BOOST_TEST_EQ(boost::int256::popcount(mixed), 128);

    // 0x55.. has bit 254 as its top set bit, 0xAA.. has bit 255.
    BOOST_TEST_EQ(boost::int256::countl_zero(fives), 1);
    BOOST_TEST_EQ(boost::int256::countl_zero(aas), 0);
    BOOST_TEST_EQ(boost::int256::countr_zero(fives), 0);
    BOOST_TEST_EQ(boost::int256::countr_zero(aas), 1);
    BOOST_TEST_EQ(boost::int256::countr_one(fives), 1);
    BOOST_TEST_EQ(boost::int256::countr_one(aas), 0);

    BOOST_TEST_EQ(fives + aas, all_ones);
    BOOST_TEST_EQ(fives & aas, uint256{});

    check_all(fives);
    check_all(aas);
    check_all(mixed);

    // Only one word set, at each position.
    for (int w {0}; w < 4; ++w)
    {
        uint256 v {};
        v.words[w] = UINT64_MAX;
        check_all(v);

        uint256 alternating {};
        alternating.words[w] = UINT64_C(0x5555555555555555);
        check_all(alternating);
    }

    // Two non-adjacent words set, so a run of zeros spans a whole word.
    check_all(uint256(UINT64_MAX, UINT64_C(0), UINT64_MAX, UINT64_C(0)));
    check_all(uint256(UINT64_C(0), UINT64_MAX, UINT64_C(0), UINT64_MAX));
}

//=====================================
// Rotation edge cases
//=====================================

// The count is masked to 0 through 255, and a negative count rotates the other
// way, so rotl(x, -1) is rotr(x, 1).
void test_rotation_counts()
{
    const uint256 v {UINT64_C(0x0123456789ABCDEF), UINT64_C(0xFEDCBA9876543210),
                     UINT64_C(0x1122334455667788), UINT64_C(0x99AABBCCDDEEFF00)};

    BOOST_TEST_EQ(boost::int256::rotl(v, 0), v);
    BOOST_TEST_EQ(boost::int256::rotr(v, 0), v);
    BOOST_TEST_EQ(boost::int256::rotl(v, 256), v);
    BOOST_TEST_EQ(boost::int256::rotr(v, 256), v);
    BOOST_TEST_EQ(boost::int256::rotl(v, 512), v);

    BOOST_TEST_EQ(boost::int256::rotl(v, 64), reference_rotl(v, 64U));
    BOOST_TEST_EQ(boost::int256::rotl(v, 128), reference_rotl(v, 128U));
    BOOST_TEST_EQ(boost::int256::rotl(v, 192), reference_rotl(v, 192U));
    BOOST_TEST_EQ(boost::int256::rotl(v, 255), reference_rotl(v, 255U));

    // A negative count reverses the direction.
    BOOST_TEST_EQ(boost::int256::rotl(v, -1), boost::int256::rotr(v, 1));
    BOOST_TEST_EQ(boost::int256::rotr(v, -1), boost::int256::rotl(v, 1));
    BOOST_TEST_EQ(boost::int256::rotl(v, -64), boost::int256::rotr(v, 64));
    BOOST_TEST_EQ(boost::int256::rotl(v, -255), boost::int256::rotr(v, 255));

    // A rotation of a value with every bit set changes nothing.
    for (int count {-260}; count <= 260; ++count)
    {
        BOOST_TEST_EQ(boost::int256::rotl(all_ones, count), all_ones);
        BOOST_TEST_EQ(boost::int256::rotr(all_ones, count), all_ones);
        BOOST_TEST_EQ(boost::int256::rotl(uint256{}, count), uint256{});
    }

    // Rotating a single bit walks it around the whole width.
    for (unsigned k {0}; k < 256U; ++k)
    {
        BOOST_TEST_EQ(boost::int256::rotl(one, static_cast<int>(k)), one << k);
        BOOST_TEST_EQ(boost::int256::rotr(one << k, static_cast<int>(k)), one);
    }
}

//=====================================
// Byteswap
//=====================================

// A byteswap reverses the 32 bytes of the value, which on a word basis reverses
// the word order and swaps the bytes inside each word.
void test_byteswap_known_answers()
{
    const uint256 pattern {UINT64_C(0x0001020304050607), UINT64_C(0x08090A0B0C0D0E0F),
                           UINT64_C(0x1011121314151617), UINT64_C(0x18191A1B1C1D1E1F)};

    const uint256 swapped {UINT64_C(0x1F1E1D1C1B1A1918), UINT64_C(0x1716151413121110),
                           UINT64_C(0x0F0E0D0C0B0A0908), UINT64_C(0x0706050403020100)};

    BOOST_TEST_EQ(boost::int256::byteswap(pattern), swapped);
    BOOST_TEST_EQ(boost::int256::byteswap(swapped), pattern);
    BOOST_TEST_EQ(boost::int256::byteswap(uint256{}), uint256{});
    BOOST_TEST_EQ(boost::int256::byteswap(all_ones), all_ones);

    // Byte 0 becomes byte 31, so a value of one becomes 2^248.
    BOOST_TEST_EQ(boost::int256::byteswap(one), one << 248U);
    BOOST_TEST_EQ(boost::int256::byteswap(one << 248U), one);
}

//=====================================
// Constant evaluation
//=====================================

void test_constexpr()
{
    static_assert(boost::int256::countl_zero(uint256{}) == 256, "countl_zero(0)");
    static_assert(boost::int256::countl_zero(one) == 255, "countl_zero(1)");
    static_assert(boost::int256::countl_zero(all_ones) == 0, "countl_zero(max)");
    static_assert(boost::int256::countr_zero(one) == 0, "countr_zero(1)");
    static_assert(boost::int256::countr_zero(uint256{}) == 256, "countr_zero(0)");
    static_assert(boost::int256::popcount(all_ones) == 256, "popcount(max)");
    static_assert(boost::int256::popcount(uint256{}) == 0, "popcount(0)");
    static_assert(boost::int256::bit_width(all_ones) == 256, "bit_width(max)");
    static_assert(boost::int256::bit_width(uint256{}) == 0, "bit_width(0)");
    static_assert(boost::int256::has_single_bit(one), "has_single_bit(1)");
    static_assert(!boost::int256::has_single_bit(uint256{3U}), "has_single_bit(3)");
    static_assert(boost::int256::bit_ceil(uint256{3U}) == uint256{4U}, "bit_ceil(3)");
    static_assert(boost::int256::bit_floor(uint256{3U}) == uint256{2U}, "bit_floor(3)");
    static_assert(boost::int256::rotl(one, 1) == uint256{2U}, "rotl(1, 1)");
    static_assert(boost::int256::rotr(one, 1) == (one << 255U), "rotr(1, 1)");
    static_assert(boost::int256::countl_one(all_ones) == 256, "countl_one(max)");
    static_assert(boost::int256::countr_one(all_ones) == 256, "countr_one(max)");
    static_assert(boost::int256::byteswap(one) == (one << 248U), "byteswap(1)");
}

void test_random()
{
    for (int i {0}; i < 2048; ++i)
    {
        uint256 v {};

        // Every width, plus values with long runs of ones and zeros.
        const int kind {i % 4};
        if (kind == 0)
        {
            for (int w {0}; w < 4; ++w)
            {
                v.words[w] = rng();
            }
        }
        else if (kind == 1)
        {
            v.words[0] = rng();
        }
        else if (kind == 2)
        {
            v = all_ones >> static_cast<unsigned>(rng() % 256U);
        }
        else
        {
            v = all_ones << static_cast<unsigned>(rng() % 256U);
        }

        check_all(v);

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
    test_zero_and_all_ones();
    test_single_bits();
    test_alternating_words();
    test_rotation_counts();
    test_byteswap_known_answers();
    test_random();

    return boost::report_errors();
}
