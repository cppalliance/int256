// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_RANDOM_HPP
#define BOOST_INT256_RANDOM_HPP

#include <boost/int256/int256.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <type_traits>

#endif

// The Boost.Random trait templates are forward declared rather than included, so this
// header adds nothing to the dependency set.

namespace boost {
namespace random {
namespace traits {

template <class T, bool intrinsic>
struct make_unsigned_imp;

template <>
struct make_unsigned_imp<int256::uint256, false>
{
    using type = int256::uint256;
};

template <>
struct make_unsigned_imp<int256::int256, false>
{
    using type = int256::uint256;
};

template <class T>
struct make_unsigned;

template <>
struct make_unsigned<int256::uint256>
{
    using type = int256::uint256;
};

template <>
struct make_unsigned<int256::int256>
{
    using type = int256::uint256;
};

template <class T, bool intrinsic>
struct make_unsigned_or_unbounded_imp;

template <>
struct make_unsigned_or_unbounded_imp<int256::uint256, false>
{
    using type = int256::uint256;
};

template <>
struct make_unsigned_or_unbounded_imp<int256::int256, false>
{
    using type = int256::uint256;
};

template <class T>
struct make_unsigned_or_unbounded;

template <>
struct make_unsigned_or_unbounded<int256::uint256>
{
    using type = int256::uint256;
};

template <>
struct make_unsigned_or_unbounded<int256::int256>
{
    using type = int256::uint256;
};

template <class T>
struct is_integral;

template <>
struct is_integral<int256::uint256> : std::true_type {};

template <>
struct is_integral<int256::int256> : std::true_type {};

template <class T>
struct is_signed;

template <>
struct is_signed<int256::uint256> : std::false_type {};

template <>
struct is_signed<int256::int256> : std::true_type {};

} // namespace traits
} // namespace random
} // namespace boost

#endif // BOOST_INT256_RANDOM_HPP
