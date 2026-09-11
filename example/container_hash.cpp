// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// This example demonstrates Boost.ContainerHash integration with uint256.
// Including <boost/int256/hash.hpp> injects a hash_value overload for the library
// type, so boost::hash, boost::hash_combine, boost::hash_range, and the
// boost::unordered containers (which default to boost::hash) all work with no
// extra configuration.

#include <boost/int256/int256.hpp>
#include <boost/int256/hash.hpp>
#include <boost/int256/iostream.hpp>
// tag::exclude[]
// The Boost.ContainerHash and Boost.Unordered headers trip strict warnings the
// test build enables (old-style casts, and a float-to-double promotion in
// std::ceil on older clang); silence them for these third-party headers only.
// This is not needed in normal use.
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable: 4848)
#endif
// end::exclude[]
#include <boost/container_hash/hash.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
// tag::exclude[]
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif
// end::exclude[]
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>

using boost::int256::uint256;

// A user-defined composite key that holds 256-bit fields. Providing a hash_value
// overload in the type's own namespace lets Boost.ContainerHash find it via ADL,
// and boost::hash_combine reuses the uint256 hash supplied by hash.hpp.
struct point
{
    uint256 x;
    uint256 y;
};

bool operator==(const point& lhs, const point& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

std::size_t hash_value(const point& p)
{
    std::size_t seed {0};
    boost::hash_combine(seed, p.x);
    boost::hash_combine(seed, p.y);
    return seed;
}

int main()
{
    std::cout << "=== boost::hash on uint256 ===" << std::endl;

    // boost::hash<T> dispatches to the hash_value overload from hash.hpp, which
    // delegates to std::hash, so the two functors always agree.
    const uint256 big {UINT64_C(0xDEADBEEF), UINT64_C(0xCAFEBABE12345678),
                       UINT64_C(0x0F1E2D3C4B5A6978), UINT64_C(0x8796A5B4C3D2E1F0)};

    std::cout << std::boolalpha;
    std::cout << "boost::hash matches std::hash: "
              << (boost::hash<uint256>{}(big) == std::hash<uint256>{}(big)) << std::endl;

    // Every word takes part in the hash, so a permutation of the words is a
    // different key
    const uint256 permuted {UINT64_C(0x8796A5B4C3D2E1F0), UINT64_C(0x0F1E2D3C4B5A6978),
                            UINT64_C(0xCAFEBABE12345678), UINT64_C(0xDEADBEEF)};
    std::cout << "Reversed words hash differently: "
              << (boost::hash<uint256>{}(big) != boost::hash<uint256>{}(permuted)) << std::endl;

    std::cout << "\n=== boost::unordered_map<uint256, ...> ===" << std::endl;

    // boost::unordered_map defaults to boost::hash<Key>, so uint256 keys need
    // no explicit hasher.
    boost::unordered_map<uint256, std::string> labels {};
    labels[uint256{1U} << 64U] = "two to the sixty-fourth";
    labels[uint256{1U} << 128U] = "two to the one hundred twenty-eighth";
    labels[uint256{1U} << 255U] = "two to the two hundred fifty-fifth";
    labels[uint256{42U}] = "forty-two";

    std::cout << "Entries: " << labels.size() << std::endl;
    std::cout << "Label at 2^128: " << labels[uint256{1U} << 128U] << std::endl;
    std::cout << "Contains 42: " << (labels.find(uint256{42U}) != labels.end()) << std::endl;

    std::cout << "\n=== hash_combine for a composite key ===" << std::endl;

    // The point hasher combines two uint256 fields; boost::hash<point> finds it
    // via ADL, letting point be used as a key directly.
    boost::unordered_map<point, long> populations {};
    populations[point{uint256{10U}, uint256{20U}}] = 5000000;
    populations[point{uint256{1U} << 200U, uint256{40U}}] = 250000;

    std::cout << "Points stored: " << populations.size() << std::endl;
    std::cout << "Value at (10, 20): " << populations[point{uint256{10U}, uint256{20U}}] << std::endl;
    std::cout << "Same coordinate hashes equal: "
              << (boost::hash<point>{}(point{uint256{10U}, uint256{20U}}) ==
                  boost::hash<point>{}(point{uint256{10U}, uint256{20U}})) << std::endl;

    std::cout << "\n=== boost::unordered_flat_map<uint256, ...> ===" << std::endl;

    // The modern flat container also defaults to boost::hash.
    boost::unordered_flat_map<uint256, int> counts {};
    counts[uint256{0U}] = 1;
    counts[uint256{1U}] = 2;
    counts[~uint256{}] = 3;

    std::cout << "Flat map size: " << counts.size() << std::endl;
    std::cout << "counts[max] = " << counts[~uint256{}] << std::endl;

    std::cout << "\n=== Using a 256-bit hash as a key ===" << std::endl;

    // The obvious use for a 256-bit integer key: a digest, stored and looked up as
    // one integer rather than as a byte array
    boost::unordered_map<uint256, std::string> by_digest {};
    const uint256 digest {UINT64_C(0xE3B0C44298FC1C14), UINT64_C(0x9AFBF4C8996FB924),
                          UINT64_C(0x27AE41E4649B934C), UINT64_C(0xA495991B7852B855)};
    by_digest[digest] = "the empty string";

    std::cout << "Lookup by digest: " << by_digest[digest] << std::endl;
    std::cout << "Digest as an integer: " << digest << std::endl;

    return 0;
}
