// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_FWD_HPP
#define BOOST_INT256_DETAIL_FWD_HPP

#include <boost/int256/detail/config.hpp>

namespace boost {
namespace int256 {

BOOST_INT256_EXPORT struct uint256;

// The signed type is a later addition; the declaration reserves the name so that the
// traits and the extended headers can be written against both from the start
BOOST_INT256_EXPORT struct int256;

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_DETAIL_FWD_HPP
