// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_CONSTANTS_HPP
#define BOOST_INT256_DETAIL_CONSTANTS_HPP

#include <boost/int256/detail/config.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstdint>
#include <limits>

#endif

namespace boost {
namespace int256 {
namespace detail {

BOOST_INT256_INLINE_CONSTEXPR std::uint64_t low_word_mask {(std::numeric_limits<std::uint64_t>::max)()};

// 2^64 exactly, as a T. Every supported floating point type holds it exactly
template <typename T>
BOOST_INT256_INLINE_CONSTEXPR T offset_value_v = static_cast<T>(18446744073709551616.0); // UINT64_MAX + 1 as a double

} // namespace detail
} // namespace int256
} // namespace boost

#endif // BOOST_INT256_DETAIL_CONSTANTS_HPP
