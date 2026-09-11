// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_CSTDLIB_HPP
#define BOOST_INT256_CSTDLIB_HPP

#include <boost/int256/int256.hpp>
#include <boost/int256/detail/common_div.hpp>
#include <boost/int256/detail/config.hpp>

namespace boost {
namespace int256 {

BOOST_INT256_EXPORT struct u256div_t
{
    uint256 quot;
    uint256 rem;
};

// Quotient and remainder of a single division, in the style of std::div
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr u256div_t div(const uint256& x, const uint256& y) noexcept
{
    if (y == 0U)
    {
        BOOST_INT256_UNREACHABLE;
    }

    u256div_t res {};
    detail::divide<true>(x, y, res.quot, res.rem);
    return res;
}

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_CSTDLIB_HPP
