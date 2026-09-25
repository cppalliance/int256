// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_CLIMITS_HPP
#define BOOST_INT256_CLIMITS_HPP

#include <boost/int256/int256.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <climits>
#include <cstdint>

#endif

#define BOOST_INT256_UINT256_MAX boost::int256::uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}
#define BOOST_INT256_INT256_MAX  boost::int256::int256{INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}
#define BOOST_INT256_INT256_MIN  boost::int256::int256{INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)}

#endif // BOOST_INT256_CLIMITS_HPP
