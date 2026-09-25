// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The Boost.Int128 interoperability is opt-in by include order. Including this library
// alone must never drag <boost/int128.hpp> in, and including <boost/int128.hpp> only
// afterwards leaves the two libraries unaware of each other.

#include <boost/int256.hpp>

#if defined(BOOST_INT128_HPP) || defined(BOOST_INT128_INT128_HPP)
#  error "Boost.Int256 must not include Boost.Int128 on its own"
#endif

#if defined(__has_include)
#  if __has_include(<boost/int128.hpp>)

#include <boost/int128.hpp>
#include <type_traits>

static_assert(!boost::int256::detail::is_int128_interop_v<boost::int128::uint128>, "No interop after the fact");
static_assert(!boost::int256::detail::is_int128_interop_v<boost::int128::int128>, "No interop after the fact");
static_assert(!std::is_convertible<boost::int128::uint128, boost::int256::uint256>::value, "No implicit conversion");
static_assert(!std::is_convertible<boost::int128::int128, boost::int256::int256>::value, "No implicit conversion");

#  endif
#endif

int main()
{
    return 0;
}
