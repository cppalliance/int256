// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Pins the storage layout of uint256. The four words are std::uint64_t so that a
// loop over uint256 can be vectorized, words[0] is the least significant word on
// every platform, and there is no over-alignment: C++14 has no aligned
// operator new, so an over-aligned type in a std::vector would be undefined.

#include <boost/int256/int256.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

using boost::int256::uint256;

namespace
{

constexpr uint256 zero {};

using words_type = decltype(zero.words);

} // namespace

// The member is a plain array of four unsigned 64-bit words.
static_assert(std::is_same<std::remove_extent<words_type>::type, std::uint64_t>::value,
              "uint256::words must be an array of std::uint64_t");
static_assert(std::extent<words_type>::value == 4,
              "uint256::words must hold exactly four words");

static_assert(sizeof(uint256) == 32, "uint256 must be exactly 256 bits wide");
static_assert(alignof(uint256) == alignof(std::uint64_t),
              "uint256 must be naturally aligned; alignas(32) would break std::vector under C++14");

static_assert(std::is_trivially_copyable<uint256>::value, "uint256 must be trivially copyable");
static_assert(std::is_standard_layout<uint256>::value, "uint256 must be standard layout");

static_assert(std::numeric_limits<uint256>::digits == 256, "256 binary digits");
static_assert(std::numeric_limits<uint256>::digits10 == 77, "10^77 fits, 10^78 does not");

// words[0] is the least significant word, on every platform.
namespace
{

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
constexpr uint256 two_64 {one << 64U};
constexpr uint256 two_128 {one << 128U};
constexpr uint256 two_192 {one << 192U};

} // namespace

static_assert(two_64.words[1] == UINT64_C(1) && two_64.words[0] == UINT64_C(0),
              "1 << 64 sets words[1]");
static_assert(two_128.words[2] == UINT64_C(1), "1 << 128 sets words[2]");
static_assert(two_192.words[3] == UINT64_C(1), "1 << 192 sets words[3]");

// The four word constructor is most significant first.
static_assert(uint256{UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)}.words[0] == UINT64_C(4),
              "the last constructor argument is words[0]");
static_assert(uint256{UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)}.words[3] == UINT64_C(1),
              "the first constructor argument is words[3]");

// A memcpy of the object matches a memcpy of the four words in index order, which
// pins the absence of padding. The value order is fixed but the byte order inside
// each word is the platform's, so this is a mixed-endian representation on s390x.
void test_object_representation()
{
    const uint256 v {UINT64_C(0x0001020304050607), UINT64_C(0x08090A0B0C0D0E0F),
                     UINT64_C(0x1011121314151617), UINT64_C(0x18191A1B1C1D1E1F)};

    unsigned char from_object[sizeof(uint256)] {};
    std::memcpy(from_object, &v, sizeof(v));

    unsigned char from_words[sizeof(uint256)] {};
    for (std::size_t i {0}; i < 4U; ++i)
    {
        std::memcpy(from_words + i * sizeof(std::uint64_t), &v.words[i], sizeof(std::uint64_t));
    }

    BOOST_TEST_EQ(std::memcmp(from_object, from_words, sizeof(uint256)), 0);
}

// The word index to significance mapping holds at run time as well, on both a
// little-endian and a big-endian host.
void test_word_significance()
{
    uint256 v {};
    v.words[0] = UINT64_C(1);
    BOOST_TEST(v == 1U);

    v = uint256{};
    v.words[1] = UINT64_C(1);
    BOOST_TEST(v == (one << 64U));

    v = uint256{};
    v.words[2] = UINT64_C(1);
    BOOST_TEST(v == (one << 128U));

    v = uint256{};
    v.words[3] = UINT64_C(1);
    BOOST_TEST(v == (one << 192U));

    BOOST_TEST_EQ((one << 64U).words[1], UINT64_C(1));
    BOOST_TEST_EQ((one << 255U).words[3], UINT64_C(1) << 63);
}

// A default constructed value is all zero words, and copying moves the bits
// unchanged.
void test_value_initialization_and_copy()
{
    const uint256 v {};
    for (std::size_t i {0}; i < 4U; ++i)
    {
        BOOST_TEST_EQ(v.words[i], UINT64_C(0));
    }

    const uint256 pattern {UINT64_C(0xDEADBEEFCAFEF00D), UINT64_C(0x0123456789ABCDEF),
                           UINT64_C(0xFEDCBA9876543210), UINT64_C(0x1122334455667788)};

    const uint256 copy {pattern};
    for (std::size_t i {0}; i < 4U; ++i)
    {
        BOOST_TEST_EQ(copy.words[i], pattern.words[i]);
    }

    unsigned char bytes[sizeof(uint256)] {};
    std::memcpy(bytes, &pattern, sizeof(pattern));

    uint256 restored {};
    std::memcpy(&restored, bytes, sizeof(restored));
    BOOST_TEST(restored == pattern);
}

int main()
{
    test_object_representation();
    test_word_significance();
    test_value_initialization_and_copy();

    return boost::report_errors();
}
