// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_TRAITS_HPP
#define BOOST_INT256_DETAIL_TRAITS_HPP

#include <boost/int256/detail/config.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <type_traits>
#include <cstdint>

#endif

namespace boost {
namespace int256 {
namespace detail {

template <typename T>
struct signed_integer
{
    static constexpr bool value = (std::is_signed<T>::value && std::is_integral<T>::value)
    #ifdef BOOST_INT256_HAS_INT128
    || std::is_same<T, builtin_i128>::value;
    #else
    ;
    #endif
};

template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_signed_integer_v = signed_integer<T>::value;

template <typename T>
struct unsigned_integer
{
    static constexpr bool value = (std::is_unsigned<T>::value && std::is_integral<T>::value)
    #ifdef BOOST_INT256_HAS_INT128
    || std::is_same<T, builtin_u128>::value;
    #else
    ;
    #endif
};

template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_unsigned_integer_v = unsigned_integer<T>::value;

template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_any_integer_v = signed_integer<T>::value || unsigned_integer<T>::value;

template <typename T>
struct floating_point
{
    static constexpr bool value = std::is_same<T, float>::value || std::is_same<T, double>::value
    #ifndef BOOST_INT256_HAS_GPU_SUPPORT
    || std::is_same<T, long double>::value
    #if defined(BOOST_INT256_HAS_STDFLOAT) && defined(__STDCPP_FLOAT16_T__)
    || std::is_same<T, std::float16_t>::value
    #endif
    #if defined(BOOST_INT256_HAS_STDFLOAT) && defined(__STDCPP_FLOAT32_T__)
    || std::is_same<T, std::float32_t>::value
    #endif
    #if defined(BOOST_INT256_HAS_STDFLOAT) && defined(__STDCPP_FLOAT64_T__)
    || std::is_same<T, std::float64_t>::value
    #endif
    #if defined(BOOST_INT256_HAS_STDFLOAT) && defined(__STDCPP_FLOAT128_T__)
    || std::is_same<T, std::float128_t>::value
    #endif
    #if defined(BOOST_INT256_HAS_STDFLOAT) && defined(__STDCPP_BFLOAT16_T__)
    || std::is_same<T, std::bfloat16_t>::value
    #endif
    ;
    #else
    ;
    #endif
};

template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_floating_point_v = floating_point<T>::value;

// True only for a C++23 <stdfloat> types
template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_extended_floating_point_v = floating_point<T>::value
    && !std::is_same<T, float>::value && !std::is_same<T, double>::value && !std::is_same<T, long double>::value;

// The type integral promotion gives an operand of type T, which is the result type of a
// shift with T on the left. The rule depends on the rank of T and not only on its size, so
// long and char32_t differ where both are the width of an int; asking the compiler is exact
template <typename T>
struct promoted
{
    using type = decltype(+T{});
};

// Unary plus on a bool draws a warning from MSVC, and the answer is always int
template <>
struct promoted<bool>
{
    using type = int;
};

template <typename T>
using promoted_t = typename promoted<T>::type;

// Boost.Int128's uint128 and int128. This library never includes <boost/int128.hpp>
// itself: detail/int128_interop.hpp specializes this for both types only when that
// header was included first, so every Int128Type overload in this library stays inert otherwise
template <typename T>
struct int128_interop
{
    static constexpr bool value {false};
    static constexpr bool is_signed {false};
};

template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_int128_interop_v = int128_interop<T>::value;

// The upper words of the 256-bit image of a Boost.Int128 value: all ones for a
// negative int128, and zero otherwise
template <typename Int128Type>
BOOST_INT256_HOST_DEVICE constexpr std::uint64_t int128_interop_fill(const Int128Type v) noexcept
{
    return int128_interop<Int128Type>::is_signed && (v.high >> 63U) != UINT64_C(0) ? UINT64_MAX : UINT64_C(0);
}

// Converts a 256-bit value to T. Under C++14 static_cast to a Boost.Int128 type is
// ambiguous with that type's own builtin 128-bit constructors, so it is built from the
// low words instead
template <typename T, typename Wide, std::enable_if_t<!is_int128_interop_v<T> || std::is_same<T, Wide>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr T convert_to(const Wide& v) noexcept
{
    return static_cast<T>(v);
}

template <typename T, typename Wide, std::enable_if_t<is_int128_interop_v<T> && !std::is_same<T, Wide>::value, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr T convert_to(const Wide& v) noexcept
{
    return int128_interop<T>::from_words(v.words[1], v.words[0]);
}

// Decides if we can use a u32, u64 or builtin 128-bit implementation for some operations

#ifdef BOOST_INT256_HAS_INT128

template <typename T>
using evaluation_type_t = std::conditional_t<sizeof(T) <= sizeof(std::uint32_t), std::uint32_t,
                            std::conditional_t<sizeof(T) <= sizeof(std::uint64_t), std::uint64_t, builtin_u128>>;

#else

template <typename T>
using evaluation_type_t = std::conditional_t<sizeof(T) <= sizeof(std::uint32_t), std::uint32_t, std::uint64_t>;

#endif

} // namespace detail
} // namespace int256
} // namespace boost

#define BOOST_INT256_DEFAULTED_SIGNED_INTEGER_CONCEPT typename SignedInteger, std::enable_if_t<detail::is_signed_integer_v<SignedInteger>, bool> = true
#define BOOST_INT256_DEFAULTED_UNSIGNED_INTEGER_CONCEPT typename UnsignedInteger, std::enable_if_t<detail::is_unsigned_integer_v<UnsignedInteger>, bool> = true
#define BOOST_INT256_DEFAULTED_INTEGER_CONCEPT typename Integer, std::enable_if_t<detail::is_any_integer_v<Integer>, bool> = true
#define BOOST_INT256_DEFAULTED_FLOATING_POINT_CONCEPT typename Float, std::enable_if_t<detail::is_floating_point_v<Float>, bool> = true

#define BOOST_INT256_SIGNED_INTEGER_CONCEPT typename SignedInteger, std::enable_if_t<detail::is_signed_integer_v<SignedInteger>, bool>
#define BOOST_INT256_UNSIGNED_INTEGER_CONCEPT typename UnsignedInteger, std::enable_if_t<detail::is_unsigned_integer_v<UnsignedInteger>, bool>
#define BOOST_INT256_INTEGER_CONCEPT typename Integer, std::enable_if_t<detail::is_any_integer_v<Integer>, bool>
#define BOOST_INT256_FLOATING_POINT_CONCEPT typename Float, std::enable_if_t<detail::is_floating_point_v<Float>, bool>

#define BOOST_INT256_DEFAULTED_INT128_INTEROP_CONCEPT typename Int128Type, std::enable_if_t<detail::is_int128_interop_v<Int128Type>, bool> = true
#define BOOST_INT256_INT128_INTEROP_CONCEPT typename Int128Type, std::enable_if_t<detail::is_int128_interop_v<Int128Type>, bool>

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

#define BOOST_INT256_DEFAULTED_SIGNED_128BIT_INTEGER_CONCEPT typename SignedInteger, std::enable_if_t<std::is_same<SignedInteger, detail::builtin_i128>::value, bool> = true
#define BOOST_INT256_DEFAULTED_UNSIGNED_128BIT_INTEGER_CONCEPT typename UnsignedInteger, std::enable_if_t<std::is_same<UnsignedInteger, detail::builtin_u128>::value, bool> = true
#define BOOST_INT256_DEFAULTED_128BIT_INTEGER_CONCEPT typename Integer, std::enable_if_t<std::is_same<Integer, detail::builtin_u128>::value || std::is_same<Integer, detail::builtin_i128>::value, bool> = true

#define BOOST_INT256_SIGNED_128BIT_INTEGER_CONCEPT typename SignedInteger, std::enable_if_t<std::is_same<SignedInteger, detail::builtin_i128>::value, bool>
#define BOOST_INT256_UNSIGNED_128BIT_INTEGER_CONCEPT typename UnsignedInteger, std::enable_if_t<std::is_same<UnsignedInteger, detail::builtin_u128>::value, bool>
#define BOOST_INT256_128BIT_INTEGER_CONCEPT typename Integer, std::enable_if_t<std::is_same<Integer, detail::builtin_u128>::value || std::is_same<Integer, detail::builtin_i128>::value, bool>

#endif

#endif // BOOST_INT256_DETAIL_TRAITS_HPP
