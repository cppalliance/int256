// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Adapted from Boost.Int128 (Copyright 2022 Peter Dimov, Copyright 2025 Matt Borland)

#ifndef BOOST_INT256_LITERALS_HPP
#define BOOST_INT256_LITERALS_HPP

#include <boost/int256/int256.hpp>
#include <boost/int256/detail/mini_from_chars.hpp>
#include <boost/int256/detail/utilities.hpp>
#include <boost/int256/detail/config.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstddef>

#endif

namespace boost {
namespace int256 {
namespace literals {

// A malformed or out-of-range literal is a compile-time error wherever the literal is a
// constant expression. Reached at run time there is no value to return, so the operators
// are noexcept and the failure terminates the program.

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator ""_u256(const char* str) noexcept
{
    return detail::parse_literal<uint256>(str, str + detail::strlen(str));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator ""_U256(const char* str) noexcept
{
    return detail::parse_literal<uint256>(str, str + detail::strlen(str));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator ""_u256(const char* str, std::size_t len) noexcept
{
    return detail::parse_literal<uint256>(str, str + len);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator ""_U256(const char* str, std::size_t len) noexcept
{
    return detail::parse_literal<uint256>(str, str + len);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator ""_i256(const char* str) noexcept
{
    return detail::parse_literal<int256>(str, str + detail::strlen(str));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator ""_I256(const char* str) noexcept
{
    return detail::parse_literal<int256>(str, str + detail::strlen(str));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator ""_i256(const char* str, std::size_t len) noexcept
{
    return detail::parse_literal<int256>(str, str + len);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator ""_I256(const char* str, std::size_t len) noexcept
{
    return detail::parse_literal<int256>(str, str + len);
}

} // namespace literals
} // namespace int256
} // namespace boost

#include <boost/int256/detail/literal_macros.hpp>

#endif // BOOST_INT256_LITERALS_HPP
