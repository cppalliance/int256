// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_CONVERSIONS_HPP
#define BOOST_INT256_DETAIL_CONVERSIONS_HPP

#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/uint256_imp.hpp>
#include <boost/int256/detail/int256_imp.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <type_traits>

#endif

namespace boost {
namespace int256 {

namespace detail {

// The single definition shared by every header that needs to accept both 256-bit types
// (byte_conversions.hpp used to keep its own uint256-only copy of this)
template <typename T>
struct valid_overload
{
    static constexpr bool value = std::is_same<T, uint256>::value || std::is_same<T, int256>::value;
};

template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_valid_overload_v = valid_overload<T>::value;

} // namespace detail

// Bit copies both ways
BOOST_INT256_HOST_DEVICE constexpr int256::int256(const uint256& v) noexcept :
    words {v.words[0], v.words[1], v.words[2], v.words[3]} {}

BOOST_INT256_HOST_DEVICE constexpr uint256::uint256(const int256& v) noexcept :
    words {v.words[0], v.words[1], v.words[2], v.words[3]} {}

//=====================================
// Comparison Operators
//
// The usual arithmetic conversions: both operands become uint256, so int256{-1} < uint256{1}
// is false, exactly as long long{-1} < (unsigned long long){1} is false
//=====================================

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127)
#endif

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool operator==(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) == static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool operator!=(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) != static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool operator<(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) < static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool operator<=(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) <= static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool operator>(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) > static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr bool operator>=(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) >= static_cast<uint256>(rhs);
}

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr std::strong_ordering operator<=>(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) <=> static_cast<uint256>(rhs);
}

#endif

//=====================================
// Arithmetic and Bitwise Operators
//
// The usual arithmetic conversions again: both operands become uint256 and the result is
// uint256
//=====================================

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator+(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) + static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator-(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) - static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator*(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) * static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator/(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) / static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator%(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) % static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator|(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) | static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator&(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) & static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT template <typename T, typename U, std::enable_if_t<detail::is_valid_overload_v<T> && detail::is_valid_overload_v<U> && !std::is_same<T, U>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr uint256 operator^(const T& lhs, const U& rhs) noexcept
{
    return static_cast<uint256>(lhs) ^ static_cast<uint256>(rhs);
}

//=====================================
// Cross-type Shift Operators
//
// A shift takes its value and its result type from the left operand, and only the count
// from the right, exactly as for every other mixed shift in this library. int256's own
// operator>> is arithmetic, so routing through it (rather than through the uint256
// domain) is what keeps a negative int256 shifted by a uint256 count arithmetic too
//=====================================

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator<<(const int256& lhs, const uint256& rhs) noexcept
{
    return lhs << static_cast<int256>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator<<(const uint256& lhs, const int256& rhs) noexcept
{
    return lhs << static_cast<uint256>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 operator>>(const int256& lhs, const uint256& rhs) noexcept
{
    return lhs >> static_cast<int256>(rhs);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 operator>>(const uint256& lhs, const int256& rhs) noexcept
{
    return lhs >> static_cast<uint256>(rhs);
}

//=====================================
// Cross-type Compound Assignment
//=====================================

#define BOOST_INT256_DETAIL_I256_U256_COMPOUND_OP(op, compound_op)                                          \
    BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256& operator compound_op(int256& lhs, const uint256& rhs) noexcept \
    {                                                                                                       \
        lhs = static_cast<int256>(static_cast<uint256>(lhs) op rhs);                                        \
        return lhs;                                                                                         \
    }

BOOST_INT256_DETAIL_I256_U256_COMPOUND_OP(|, |=)
BOOST_INT256_DETAIL_I256_U256_COMPOUND_OP(&, &=)
BOOST_INT256_DETAIL_I256_U256_COMPOUND_OP(^, ^=)
BOOST_INT256_DETAIL_I256_U256_COMPOUND_OP(+, +=)
BOOST_INT256_DETAIL_I256_U256_COMPOUND_OP(-, -=)
BOOST_INT256_DETAIL_I256_U256_COMPOUND_OP(*, *=)
BOOST_INT256_DETAIL_I256_U256_COMPOUND_OP(/, /=)
BOOST_INT256_DETAIL_I256_U256_COMPOUND_OP(%, %=)

#undef BOOST_INT256_DETAIL_I256_U256_COMPOUND_OP

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256& operator<<=(int256& lhs, const uint256& rhs) noexcept
{
    lhs = lhs << rhs;
    return lhs;
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256& operator>>=(int256& lhs, const uint256& rhs) noexcept
{
    lhs = lhs >> rhs;
    return lhs;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_DETAIL_CONVERSIONS_HPP
