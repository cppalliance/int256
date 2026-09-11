// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_UTILITIES_HPP
#define BOOST_INT256_DETAIL_UTILITIES_HPP

#include <boost/int256/detail/config.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstddef>

#endif

namespace boost {
namespace int256 {
namespace detail {

// A constexpr strlen for both char and wchar_t, since neither std::strlen nor
// std::wcslen is required to be usable in a constant expression
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr std::size_t strlen(const T* str) noexcept
{
    std::size_t i {};
    while (*str != '\0')
    {
        ++str;
        ++i;
    }

    return i;
}

} // namespace detail
} // namespace int256
} // namespace boost

#endif // BOOST_INT256_DETAIL_UTILITIES_HPP
