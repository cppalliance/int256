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

BOOST_INT256_EXPORT struct i256div_t
{
    int256 quot;
    int256 rem;
};

// Quotient and remainder of a single division, in the style of std::div
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr i256div_t div(const int256& x, const int256& y) noexcept
{
    if (y == 0)
    {
        BOOST_INT256_UNREACHABLE;
    }

    const auto abs_x {detail::to_unsigned_bits(abs(x))};
    const auto abs_y {detail::to_unsigned_bits(abs(y))};
    const auto negative_quot {(x.signed_high() < 0) != (y.signed_high() < 0)};
    const auto negative_rem {x.signed_high() < 0};

    i256div_t res {};

    if (abs_x < abs_y)
    {
        res.rem = x;
        return res;
    }

    uint256 quotient {};
    uint256 remainder {};
    detail::divide<true>(abs_x, abs_y, quotient, remainder);

    const auto signed_quotient {detail::to_signed_bits(quotient)};
    const auto signed_remainder {detail::to_signed_bits(remainder)};

    res.quot = negative_quot ? -signed_quotient : signed_quotient;
    res.rem = negative_rem ? -signed_remainder : signed_remainder;

    return res;
}

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_CSTDLIB_HPP
