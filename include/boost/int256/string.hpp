// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_STRING_HPP
#define BOOST_INT256_STRING_HPP

#include <boost/int256/int256.hpp>
#include <boost/int256/detail/mini_to_chars.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <string>
#include <type_traits>

#endif

namespace boost {
namespace int256 {

BOOST_INT256_EXPORT template <typename T>
auto to_string(const T& value) -> std::enable_if_t<std::is_same<T, uint256>::value, std::string>
{
    char buffer[detail::mini_to_chars_buffer_size] {};
    const auto last {detail::mini_to_chars(buffer, value, 10, false)};
    return std::string{last, buffer + sizeof(buffer) - 1};
}

BOOST_INT256_EXPORT template <typename T>
auto to_wstring(const T& value) -> std::enable_if_t<std::is_same<T, uint256>::value, std::wstring>
{
    char buffer[detail::mini_to_chars_buffer_size] {};
    const auto last {detail::mini_to_chars(buffer, value, 10, false)};
    return std::wstring{last, buffer + sizeof(buffer) - 1};
}

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_STRING_HPP
