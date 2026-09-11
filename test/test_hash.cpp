// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// std::hash<uint256> and the hash_value ADL hook. Every word has to reach the
// result: an implementation that only mixes the low word would pass an equality
// test but collide catastrophically, so the distribution checks below are the
// point of this file rather than an extra.

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
#include <functional>
#include <random>
#include <set>
#include <unordered_map>
#include <unordered_set>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

std::size_t hash_of(const uint256& v)
{
    return std::hash<uint256>{}(v);
}

// Equal values hash equally, which is the only hard requirement.
void test_equal_values_hash_equally()
{
    for (int i {0}; i < 1024; ++i)
    {
        uint256 a {};
        for (int w {0}; w < 4; ++w)
        {
            a.words[w] = rng();
        }

        const uint256 b {a};
        BOOST_TEST_EQ(hash_of(a), hash_of(b));

        // And the ADL hook agrees with the specialization.
        BOOST_TEST_EQ(boost::int256::hash_value(a), hash_of(a));
    }

    BOOST_TEST_EQ(hash_of(uint256{}), hash_of(uint256{}));
    BOOST_TEST_EQ(hash_of(uint256{42U}), hash_of(uint256{42U}));
}

// A permutation of the words is a different value, so it must not produce the
// same hash. This is what catches a hash that adds or exclusive-ors the words
// without mixing in their position.
void test_word_permutations_differ()
{
    const std::uint64_t a {UINT64_C(0x0123456789ABCDEF)};
    const std::uint64_t b {UINT64_C(0xFEDCBA9876543210)};
    const std::uint64_t c {UINT64_C(0x1122334455667788)};
    const std::uint64_t d {UINT64_C(0x99AABBCCDDEEFF00)};

    const uint256 forward {a, b, c, d};
    const uint256 reversed {d, c, b, a};

    BOOST_TEST(forward != reversed);
    BOOST_TEST(hash_of(forward) != hash_of(reversed));

    // A single word moved between positions.
    const uint256 in_word_0 {UINT64_C(0), UINT64_C(0), UINT64_C(0), a};
    const uint256 in_word_1 {UINT64_C(0), UINT64_C(0), a, UINT64_C(0)};
    const uint256 in_word_2 {UINT64_C(0), a, UINT64_C(0), UINT64_C(0)};
    const uint256 in_word_3 {a, UINT64_C(0), UINT64_C(0), UINT64_C(0)};

    std::set<std::size_t> hashes;
    hashes.insert(hash_of(in_word_0));
    hashes.insert(hash_of(in_word_1));
    hashes.insert(hash_of(in_word_2));
    hashes.insert(hash_of(in_word_3));
    BOOST_TEST_EQ(hashes.size(), static_cast<std::size_t>(4));

    // Swapping any adjacent pair changes the hash.
    BOOST_TEST(hash_of(uint256(a, b, c, d)) != hash_of(uint256(b, a, c, d)));
    BOOST_TEST(hash_of(uint256(a, b, c, d)) != hash_of(uint256(a, c, b, d)));
    BOOST_TEST(hash_of(uint256(a, b, c, d)) != hash_of(uint256(a, b, d, c)));
}

// Every bit of the value has to be able to change the hash, including bits in the
// upper words, which is where a truncating implementation gives itself away. The
// finalizer mixes all 64 bits of each word before any narrowing to size_t, so
// this holds even where size_t is 32 bits.
void test_every_word_reaches_the_result()
{
    for (int w {0}; w < 4; ++w)
    {
        std::set<std::size_t> hashes;

        for (int bit {0}; bit < 64; ++bit)
        {
            uint256 v {};
            v.words[w] = (UINT64_C(1) << bit);
            hashes.insert(hash_of(v));
        }

        // 64 distinct single bit values in one word, and no more than a couple of
        // collisions is acceptable even for a 32-bit size_t.
        BOOST_TEST(hashes.size() >= 60U);
    }

    // A value differing only in the top word hashes differently.
    const uint256 base {UINT64_C(0), UINT64_C(1), UINT64_C(2), UINT64_C(3)};
    const uint256 top_changed {UINT64_C(1), UINT64_C(1), UINT64_C(2), UINT64_C(3)};
    BOOST_TEST(hash_of(base) != hash_of(top_changed));
}

// A thousand consecutive values must not pile up. Consecutive integers are the
// worst realistic input for a weak integer hash.
void test_distribution()
{
    {
        std::set<std::size_t> hashes;
        for (std::uint64_t i {0}; i < 1024U; ++i)
        {
            hashes.insert(hash_of(uint256{i}));
        }
        BOOST_TEST_EQ(hashes.size(), static_cast<std::size_t>(1024));
    }

    // The same for consecutive values in the top word.
    {
        std::set<std::size_t> hashes;
        for (std::uint64_t i {0}; i < 1024U; ++i)
        {
            hashes.insert(hash_of(uint256(i, UINT64_C(0), UINT64_C(0), UINT64_C(0))));
        }
        BOOST_TEST_EQ(hashes.size(), static_cast<std::size_t>(1024));
    }

    // And for random values.
    {
        std::set<std::size_t> hashes;
        for (int i {0}; i < 1024; ++i)
        {
            uint256 v {};
            for (int w {0}; w < 4; ++w)
            {
                v.words[w] = rng();
            }
            hashes.insert(hash_of(v));
        }
        BOOST_TEST(hashes.size() >= 1020U);
    }
}

// The specialization is what makes uint256 usable as a key, so the containers are
// exercised directly.
void test_unordered_containers()
{
    std::unordered_set<uint256> keys;

    for (std::uint64_t i {0}; i < 512U; ++i)
    {
        keys.insert(uint256{i});
        keys.insert(uint256(i, UINT64_C(0), UINT64_C(0), i));
    }

    BOOST_TEST_EQ(keys.size(), static_cast<std::size_t>(1023));  // uint256{0} collides with {0,0,0,0}
    BOOST_TEST(keys.find(uint256{7U}) != keys.end());
    BOOST_TEST(keys.find(uint256(UINT64_C(3), UINT64_C(0), UINT64_C(0), UINT64_C(3))) != keys.end());
    BOOST_TEST(keys.find(uint256{100000U}) == keys.end());

    std::unordered_map<uint256, int> values;
    values[uint256{1U}] = 1;
    values[uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX)] = 2;
    values[uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0))] = 3;

    BOOST_TEST_EQ(values.size(), static_cast<std::size_t>(3));
    BOOST_TEST_EQ(values[uint256{1U}], 1);
    BOOST_TEST_EQ(values[uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX)], 2);
    BOOST_TEST_EQ(values[uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0))], 3);

    // Erasing by key works, which needs both the hash and the equality.
    BOOST_TEST_EQ(values.erase(uint256{1U}), static_cast<std::size_t>(1));
    BOOST_TEST_EQ(values.size(), static_cast<std::size_t>(2));
}

// The specialization has to be usable where std::hash is required to be, and it
// must be noexcept so the containers can rely on it.
void test_traits()
{
    static_assert(noexcept(std::hash<uint256>{}(uint256{})), "the hash is noexcept");
    static_assert(std::is_same<decltype(std::hash<uint256>{}(uint256{})), std::size_t>::value,
                  "the hash returns size_t");
    static_assert(std::is_same<decltype(boost::int256::hash_value(uint256{})), std::size_t>::value,
                  "hash_value returns size_t");
    static_assert(std::is_default_constructible<std::hash<uint256>>::value,
                  "the hash is default constructible");
}

} // namespace

int main()
{
    test_traits();
    test_equal_values_hash_equally();
    test_word_permutations_differ();
    test_every_word_reaches_the_result();
    test_distribution();
    test_unordered_containers();

    return boost::report_errors();
}
