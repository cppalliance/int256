// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The byte conversion surface, and the reconciled semantics behind it.
//
// to_be_bytes and to_le_bytes are value encodings: the byte order they produce is
// the same on every host. to_ne_bytes is the object representation, so a memcpy of
// the struct reproduces it. Because the word order of uint256 is fixed at
// words[0] least significant on every platform, the object representation on a
// big-endian host is mixed endian: the words ascend in significance while the
// bytes inside each word descend. That is deliberate, and it is asserted here for
// both host byte orders so that a change to the layout has to change this test.
//
// The invariants that hold on every host:
//   memcpy(&to_be(x)) == to_be_bytes(x)
//   memcpy(&to_le(x)) == to_le_bytes(x)
//   from_X_bytes(to_X_bytes(x)) == x   for X in be, le, ne

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

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <type_traits>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

// The 0x00 through 0x1F pattern, whose big-endian encoding is the bytes in order.
constexpr uint256 pattern {UINT64_C(0x0001020304050607), UINT64_C(0x08090A0B0C0D0E0F),
                           UINT64_C(0x1011121314151617), UINT64_C(0x18191A1B1C1D1E1F)};

constexpr uint256 all_ones {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

//=====================================
// Independent references for the value encodings
//=====================================

// Byte i of the big-endian encoding is byte 31 - i of the value, counting from
// the least significant byte.
void reference_be_bytes(const uint256& v, unsigned char (&out)[32])
{
    for (unsigned i {0}; i < 32U; ++i)
    {
        const auto source {31U - i};
        out[i] = static_cast<unsigned char>(v.words[source / 8U] >> ((source % 8U) * 8U));
    }
}

// Byte i of the little-endian encoding is byte i of the value.
void reference_le_bytes(const uint256& v, unsigned char (&out)[32])
{
    for (unsigned i {0}; i < 32U; ++i)
    {
        out[i] = static_cast<unsigned char>(v.words[i / 8U] >> ((i % 8U) * 8U));
    }
}

// The word order reversed: result.words[0] is v.words[3] and so on. The four word
// constructor is most significant first, so passing the words in ascending order
// reverses them.
uint256 reverse_word_order(const uint256& v)
{
    return uint256{v.words[0], v.words[1], v.words[2], v.words[3]};
}

// Each word byte swapped in place, with the word order left alone.
uint256 byteswap_each_word(const uint256& v)
{
    uint256 result {};
    for (unsigned w {0}; w < 4U; ++w)
    {
        std::uint64_t swapped {0};
        for (unsigned b {0}; b < 8U; ++b)
        {
            swapped |= (((v.words[w] >> (b * 8U)) & UINT64_C(0xFF)) << ((7U - b) * 8U));
        }
        result.words[w] = swapped;
    }

    return result;
}

//=====================================
// The documented byte array behavior
//=====================================

void test_known_answer_encodings()
{
    const auto be {boost::int256::to_be_bytes(pattern)};
    const auto le {boost::int256::to_le_bytes(pattern)};

    // Big endian: the bytes come out in ascending order, 0x00 first.
    for (unsigned i {0}; i < 32U; ++i)
    {
        BOOST_TEST_EQ(be[i], static_cast<std::uint8_t>(i));
    }

    // Little endian: the same bytes reversed.
    for (unsigned i {0}; i < 32U; ++i)
    {
        BOOST_TEST_EQ(le[i], static_cast<std::uint8_t>(31U - i));
    }

    // A value of one has its only set byte at the end of the big-endian encoding
    // and at the start of the little-endian one.
    const auto be_one {boost::int256::to_be_bytes(one)};
    const auto le_one {boost::int256::to_le_bytes(one)};
    BOOST_TEST_EQ(be_one[31], static_cast<std::uint8_t>(1));
    BOOST_TEST_EQ(be_one[0], static_cast<std::uint8_t>(0));
    BOOST_TEST_EQ(le_one[0], static_cast<std::uint8_t>(1));
    BOOST_TEST_EQ(le_one[31], static_cast<std::uint8_t>(0));

    // And every byte of the maximum is set in both.
    const auto be_max {boost::int256::to_be_bytes(all_ones)};
    const auto le_max {boost::int256::to_le_bytes(all_ones)};
    for (unsigned i {0}; i < 32U; ++i)
    {
        BOOST_TEST_EQ(be_max[i], static_cast<std::uint8_t>(0xFF));
        BOOST_TEST_EQ(le_max[i], static_cast<std::uint8_t>(0xFF));
    }
}

// The value encodings must match the hand-written references, whatever the host
// byte order is.
void check_encodings(const uint256& v)
{
    unsigned char expected_be[32] {};
    unsigned char expected_le[32] {};
    reference_be_bytes(v, expected_be);
    reference_le_bytes(v, expected_le);

    const auto be {boost::int256::to_be_bytes(v)};
    const auto le {boost::int256::to_le_bytes(v)};

    for (unsigned i {0}; i < 32U; ++i)
    {
        BOOST_TEST_EQ(be[i], static_cast<std::uint8_t>(expected_be[i]));
        BOOST_TEST_EQ(le[i], static_cast<std::uint8_t>(expected_le[i]));
    }

    // The two encodings are byte reversals of each other.
    for (unsigned i {0}; i < 32U; ++i)
    {
        BOOST_TEST_EQ(be[i], le[31U - i]);
    }

    // to_ne_bytes is the object representation.
    const auto ne {boost::int256::to_ne_bytes(v)};
    unsigned char from_object[32] {};
    std::memcpy(from_object, &v, sizeof(v));
    for (unsigned i {0}; i < 32U; ++i)
    {
        BOOST_TEST_EQ(ne[i], static_cast<std::uint8_t>(from_object[i]));
    }

    // On a little-endian host the object representation is the little-endian
    // encoding; on a big-endian host it is neither, because the word order is
    // fixed while the byte order inside each word is not.
    #if BOOST_INT256_ENDIAN_LITTLE_BYTE
    for (unsigned i {0}; i < 32U; ++i)
    {
        BOOST_TEST_EQ(ne[i], le[i]);
    }
    #else
    // Word w of the value occupies bytes 8w through 8w + 7 of the native
    // representation, most significant byte of the word first.
    for (unsigned w {0}; w < 4U; ++w)
    {
        for (unsigned b {0}; b < 8U; ++b)
        {
            const auto expected {static_cast<std::uint8_t>(v.words[w] >> ((7U - b) * 8U))};
            BOOST_TEST_EQ(ne[w * 8U + b], expected);
        }
    }
    #endif

    // Every round trip returns the original value.
    BOOST_TEST_EQ(boost::int256::from_be_bytes<uint256>(be), v);
    BOOST_TEST_EQ(boost::int256::from_le_bytes<uint256>(le), v);
    BOOST_TEST_EQ(boost::int256::from_ne_bytes<uint256>(ne), v);

    // And so does the pointer form.
    BOOST_TEST_EQ(boost::int256::from_be_bytes<uint256>(be.data()), v);
    BOOST_TEST_EQ(boost::int256::from_le_bytes<uint256>(le.data()), v);
    BOOST_TEST_EQ(boost::int256::from_ne_bytes<uint256>(ne.data()), v);
}

//=====================================
// to_be and to_le, the value returning forms
//=====================================

// to_be(x) returns an object whose representation IS the big-endian encoding of
// x, so a memcpy of it equals to_be_bytes(x). Same for to_le. This holds on every
// host, which is the invariant the reconciled design is built on.
void check_swapped_forms(const uint256& v)
{
    const auto be_bytes {boost::int256::to_be_bytes(v)};
    const auto le_bytes {boost::int256::to_le_bytes(v)};

    const auto as_be {boost::int256::to_be(v)};
    const auto as_le {boost::int256::to_le(v)};

    unsigned char be_object[32] {};
    unsigned char le_object[32] {};
    std::memcpy(be_object, &as_be, sizeof(as_be));
    std::memcpy(le_object, &as_le, sizeof(as_le));

    for (unsigned i {0}; i < 32U; ++i)
    {
        BOOST_TEST_EQ(static_cast<std::uint8_t>(be_object[i]), be_bytes[i]);
        BOOST_TEST_EQ(static_cast<std::uint8_t>(le_object[i]), le_bytes[i]);
    }

    // from_be and from_le are the inverses.
    BOOST_TEST_EQ(boost::int256::from_be(as_be), v);
    BOOST_TEST_EQ(boost::int256::from_le(as_le), v);

    // The value each conversion produces follows from the invariant above, and it
    // is NOT the naive one on a big-endian host. The word order of uint256 is fixed
    // at words[0] least significant everywhere, while the byte order inside a word
    // is the host's, so the two halves of the encoding are handled by different
    // steps and only one of them changes with the host.
    //
    // On a little-endian host the object representation already ascends by byte, so
    // to_le is the identity and to_be is the full byte reversal.
    //
    // On a big-endian host the bytes inside each word already descend, so producing
    // the big-endian encoding needs only the word order reversed, and producing the
    // little-endian encoding needs only each word swapped in place. Confirmed on
    // s390x: to_be(1) is 2^192 (word reversal) and to_le(1) is 2^56 (per-word swap).
    #if BOOST_INT256_ENDIAN_LITTLE_BYTE
    BOOST_TEST_EQ(as_le, v);
    BOOST_TEST_EQ(as_be, boost::int256::byteswap(v));
    #else
    BOOST_TEST_EQ(as_be, reverse_word_order(v));
    BOOST_TEST_EQ(as_le, byteswap_each_word(v));
    #endif

    // Either way, a full byteswap is the two steps composed, which ties the two
    // branches above to one another on every host.
    BOOST_TEST_EQ(boost::int256::byteswap(v), reverse_word_order(byteswap_each_word(v)));
}

//=====================================
// Every byte-like element type
//=====================================

template <typename ByteType>
void test_byte_type()
{
    const uint256 values[] {uint256{}, one, all_ones, pattern,
                            uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0))};

    for (const auto& v : values)
    {
        const auto be {boost::int256::to_be_bytes<ByteType>(v)};
        const auto le {boost::int256::to_le_bytes<ByteType>(v)};
        const auto ne {boost::int256::to_ne_bytes<ByteType>(v)};

        static_assert(std::tuple_size<decltype(be)>::value == 32, "32 bytes");
        static_assert(std::is_same<typename decltype(be)::value_type, ByteType>::value,
                      "the element type follows the request");

        BOOST_TEST_EQ(boost::int256::from_be_bytes<uint256>(be), v);
        BOOST_TEST_EQ(boost::int256::from_le_bytes<uint256>(le), v);
        BOOST_TEST_EQ(boost::int256::from_ne_bytes<uint256>(ne), v);

        // The raw array overloads, which are the ones device code uses.
        ByteType raw_be[32] {};
        ByteType raw_le[32] {};
        ByteType raw_ne[32] {};
        boost::int256::to_be_bytes<ByteType>(v, raw_be);
        boost::int256::to_le_bytes<ByteType>(v, raw_le);
        boost::int256::to_ne_bytes<ByteType>(v, raw_ne);

        for (unsigned i {0}; i < 32U; ++i)
        {
            BOOST_TEST(raw_be[i] == be[i]);
            BOOST_TEST(raw_le[i] == le[i]);
            BOOST_TEST(raw_ne[i] == ne[i]);
        }

        BOOST_TEST_EQ(boost::int256::from_be_bytes<uint256>(raw_be), v);
        BOOST_TEST_EQ(boost::int256::from_le_bytes<uint256>(raw_le), v);
        BOOST_TEST_EQ(boost::int256::from_ne_bytes<uint256>(raw_ne), v);
    }
}

//=====================================
// Constant evaluation
//=====================================

void test_constexpr()
{
    // std::array::operator[] is not constexpr in C++14, so the arrays are built
    // as braced lists and read back through a constexpr copy of the value.
    constexpr auto be {boost::int256::to_be_bytes(pattern)};
    constexpr auto le {boost::int256::to_le_bytes(pattern)};
    constexpr auto round_tripped_be {boost::int256::from_be_bytes<uint256>(be)};
    constexpr auto round_tripped_le {boost::int256::from_le_bytes<uint256>(le)};

    static_assert(round_tripped_be == pattern, "the big-endian round trip is exact");
    static_assert(round_tripped_le == pattern, "the little-endian round trip is exact");

    constexpr auto swapped {boost::int256::to_be(pattern)};
    static_assert(boost::int256::from_be(swapped) == pattern, "to_be and from_be are inverses");

    constexpr auto identity {boost::int256::to_le(pattern)};
    static_assert(boost::int256::from_le(identity) == pattern, "to_le and from_le are inverses");

    // to_be and to_le compose into a byteswap on either host.
    static_assert(boost::int256::to_be(boost::int256::to_be(pattern)) == pattern,
                  "applying the same conversion twice is the identity");
    static_assert(boost::int256::to_le(boost::int256::to_le(pattern)) == pattern,
                  "applying the same conversion twice is the identity");
}

void test_random()
{
    for (int i {0}; i < 1024; ++i)
    {
        uint256 v {};
        const int words {(i % 4) + 1};
        for (int w {0}; w < words; ++w)
        {
            v.words[w] = rng();
        }

        check_encodings(v);
        check_swapped_forms(v);

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
    test_known_answer_encodings();

    const uint256 directed[] {uint256{}, one, all_ones, pattern,
                              uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)),
                              uint256(UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)),
                              uint256(UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)),
                              all_ones - 1U};

    for (const auto& v : directed)
    {
        check_encodings(v);
        check_swapped_forms(v);
    }

    test_byte_type<std::uint8_t>();
    test_byte_type<char>();
    test_byte_type<unsigned char>();
    test_byte_type<signed char>();

    #if defined(__cpp_lib_byte) && __cpp_lib_byte >= 201603L
    test_byte_type<std::byte>();
    #endif

    test_random();

    return boost::report_errors();
}
