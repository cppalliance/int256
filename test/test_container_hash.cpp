// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Boost.ContainerHash and Boost.Unordered interoperation. boost::hash picks up the
// hash_value overload by ADL, so boost::hash, std::hash and hash_value must all
// agree, and a uint256 must work as a key in the Boost containers as well as the
// standard ones.

// Boost.ContainerHash and Boost.Unordered are not clean under the strict warning
// set the test Jamfile turns on, and b2 sorts <cxxflags> so a -Wno- switch on the
// command line cannot override the -W switch that precedes it. The suppression
// therefore lives here, around the includes.
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wold-style-cast"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wsign-conversion"
#  pragma clang diagnostic ignored "-Wundef"
#  pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wundef"
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wuseless-cast"
#elif defined(_MSC_VER)
#  pragma warning(push)
// Padding warnings propagate out of <utility>.
#  pragma warning(disable : 4324)
#  pragma warning(disable : 4127)
#endif

#include <boost/int256.hpp>

#include <boost/container_hash/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <set>
#include <vector>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

// boost::hash finds hash_value through ADL, so the three spellings are one value.
void test_all_three_agree()
{
    const uint256 values[] {
        uint256{},
        uint256{1U},
        uint256{42U},
        uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)),
        uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX),
        uint256(UINT64_C(0x0123456789ABCDEF), UINT64_C(0xFEDCBA9876543210),
                UINT64_C(0x1122334455667788), UINT64_C(0x99AABBCCDDEEFF00))
    };

    for (const auto& v : values)
    {
        const auto by_boost {boost::hash<uint256>{}(v)};
        const auto by_std {std::hash<uint256>{}(v)};
        const auto by_adl {boost::int256::hash_value(v)};

        BOOST_TEST_EQ(by_boost, by_std);
        BOOST_TEST_EQ(by_boost, by_adl);
    }

    for (int i {0}; i < 512; ++i)
    {
        uint256 v {};
        for (int w {0}; w < 4; ++w)
        {
            v.words[w] = rng();
        }

        BOOST_TEST_EQ(boost::hash<uint256>{}(v), std::hash<uint256>{}(v));
        BOOST_TEST_EQ(boost::hash<uint256>{}(v), boost::int256::hash_value(v));
    }
}

// hash_combine and hash_range are the reason the ADL hook exists: they let a
// uint256 be a member of a larger hashed aggregate.
void test_hash_combine_and_range()
{
    const uint256 a {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    const uint256 b {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(2)};

    std::size_t seed_ab {0};
    boost::hash_combine(seed_ab, a);
    boost::hash_combine(seed_ab, b);

    std::size_t seed_ba {0};
    boost::hash_combine(seed_ba, b);
    boost::hash_combine(seed_ba, a);

    // Order matters, so the two seeds differ.
    BOOST_TEST(seed_ab != seed_ba);

    // The same sequence gives the same seed.
    std::size_t seed_again {0};
    boost::hash_combine(seed_again, a);
    boost::hash_combine(seed_again, b);
    BOOST_TEST_EQ(seed_ab, seed_again);

    // hash_range over a vector of values.
    const std::vector<uint256> first {a, b, uint256{3U}};
    const std::vector<uint256> second {a, b, uint256{3U}};
    const std::vector<uint256> different {a, b, uint256{4U}};

    BOOST_TEST_EQ(boost::hash_range(first.begin(), first.end()),
                  boost::hash_range(second.begin(), second.end()));
    BOOST_TEST(boost::hash_range(first.begin(), first.end()) !=
               boost::hash_range(different.begin(), different.end()));

    // And a vector of uint256 is hashable as a whole.
    BOOST_TEST_EQ(boost::hash<std::vector<uint256>>{}(first),
                  boost::hash<std::vector<uint256>>{}(second));
    BOOST_TEST(boost::hash<std::vector<uint256>>{}(first) !=
               boost::hash<std::vector<uint256>>{}(different));
}

// The Boost containers, which use boost::hash by default.
void test_boost_unordered_containers()
{
    boost::unordered_set<uint256> keys;

    for (std::uint64_t i {0}; i < 512U; ++i)
    {
        keys.insert(uint256{i});
    }

    BOOST_TEST_EQ(keys.size(), static_cast<std::size_t>(512));
    BOOST_TEST(keys.find(uint256{7U}) != keys.end());
    BOOST_TEST(keys.find(uint256{1000U}) == keys.end());

    // Keys that differ only in an upper word must not collide into one bucket in
    // a way that loses them.
    for (std::uint64_t i {0}; i < 512U; ++i)
    {
        keys.insert(uint256(i, UINT64_C(0), UINT64_C(0), UINT64_C(0)));
    }
    BOOST_TEST_EQ(keys.size(), static_cast<std::size_t>(1023));

    boost::unordered_map<uint256, std::uint64_t> squares;
    for (std::uint64_t i {0}; i < 256U; ++i)
    {
        squares[uint256{i}] = i * i;
    }

    BOOST_TEST_EQ(squares.size(), static_cast<std::size_t>(256));
    for (std::uint64_t i {0}; i < 256U; ++i)
    {
        BOOST_TEST_EQ(squares[uint256{i}], i * i);
    }

    // A uint256 as the mapped type as well as the key.
    boost::unordered_map<std::uint64_t, uint256> wide_values;
    wide_values[1] = uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX);
    BOOST_TEST_EQ(wide_values[1], (uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}));
}

// The distribution has to hold for boost::hash too, since it is a different entry
// point into the same finalizer.
void test_distribution()
{
    std::set<std::size_t> hashes;
    for (std::uint64_t i {0}; i < 1024U; ++i)
    {
        hashes.insert(boost::hash<uint256>{}(uint256{i}));
    }
    BOOST_TEST_EQ(hashes.size(), static_cast<std::size_t>(1024));

    hashes.clear();
    for (std::uint64_t i {0}; i < 1024U; ++i)
    {
        hashes.insert(boost::hash<uint256>{}(uint256(i, UINT64_C(0), UINT64_C(0), UINT64_C(0))));
    }
    BOOST_TEST_EQ(hashes.size(), static_cast<std::size_t>(1024));
}

} // namespace

int main()
{
    test_all_three_agree();
    test_hash_combine_and_range();
    test_boost_unordered_containers();
    test_distribution();

    return boost::report_errors();
}
