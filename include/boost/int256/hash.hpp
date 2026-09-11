// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_HASH_HPP
#define BOOST_INT256_HASH_HPP

#include <boost/int256/int256.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstddef>
#include <cstdint>
#include <functional>

#endif

namespace boost {
namespace int256 {
namespace detail {

// The cast is only useless for 64-bit platforms
// Without we get an implicit conversion warning which is arguably worse
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wuseless-cast"
#endif

// splitmix64 finalizer: mixes all 64 input bits into the result before any narrowing to size_t.
// This is required for correctness on platforms where size_t is 32 bits
inline std::size_t hash_finalize_64(std::uint64_t v) noexcept
{
    v ^= v >> 30;
    v *= UINT64_C(0xbf58476d1ce4e5b9);
    v ^= v >> 27;
    v *= UINT64_C(0x94d049bb133111eb);
    v ^= v >> 31;
    return static_cast<std::size_t>(v);
}

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif

// boost::hash_combine style mixing of one finalized word into the running seed
inline std::size_t hash_combine_64(const std::size_t seed, const std::uint64_t word) noexcept
{
    constexpr std::size_t golden_ratio {0x9e3779b9U};
    const std::size_t word_hash {hash_finalize_64(word)};
    return seed ^ (word_hash + golden_ratio + (seed << 6) + (seed >> 2));
}

} // namespace detail
} // namespace int256
} // namespace boost

namespace std {

template <>
struct hash<boost::int256::uint256>
{
    auto operator()(const boost::int256::uint256& v) const noexcept -> std::size_t
    {
        std::size_t seed {boost::int256::detail::hash_finalize_64(v.words[0])};
        seed = boost::int256::detail::hash_combine_64(seed, v.words[1]);
        seed = boost::int256::detail::hash_combine_64(seed, v.words[2]);
        seed = boost::int256::detail::hash_combine_64(seed, v.words[3]);
        return seed;
    }
};

} // namespace std

namespace boost {
namespace int256 {

BOOST_INT256_EXPORT inline std::size_t hash_value(const uint256& v) noexcept
{
    return std::hash<uint256>{}(v);
}

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_HASH_HPP
