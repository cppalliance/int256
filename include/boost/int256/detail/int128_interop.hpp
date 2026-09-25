// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_INT128_INTEROP_HPP
#define BOOST_INT256_DETAIL_INT128_INTEROP_HPP

#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/traits.hpp>
#include <boost/int256/detail/uint256_imp.hpp>
#include <boost/int256/detail/int256_imp.hpp>
#include <boost/int256/detail/conversions.hpp>

// Interoperability with Boost.Int128's uint128 and int128. This header never includes
// <boost/int128.hpp>: everything below exists only when the user already included it
// before this library, so neither library drags the other in. A module build cannot
// see the other library's macros, so it is skipped there
#if defined(BOOST_INT128_INT128_HPP) && !defined(BOOST_INT256_BUILD_MODULE)

namespace boost {
namespace int256 {
namespace detail {

template <>
struct int128_interop<boost::int128::uint128>
{
    static constexpr bool value {true};
    static constexpr bool is_signed {false};

    static BOOST_INT256_HOST_DEVICE constexpr boost::int128::uint128 from_words(const std::uint64_t hi, const std::uint64_t lo) noexcept
    {
        return boost::int128::uint128{hi, lo};
    }
};

template <>
struct int128_interop<boost::int128::int128>
{
    static constexpr bool value {true};
    static constexpr bool is_signed {true};

    static BOOST_INT256_HOST_DEVICE constexpr boost::int128::int128 from_words(const std::uint64_t hi, const std::uint64_t lo) noexcept
    {
        return boost::int128::int128{static_cast<std::int64_t>(hi), lo};
    }
};

} // namespace detail

//=====================================
// Binary Operators
//=====================================

// Each of these converts the Boost.Int128 operand to the 256-bit type and re-dispatches,
// which is the usual arithmetic conversion a builtin 128-bit operand gets as well: the
// signed int128 sign fills, and the result has the type of the 256-bit operand
#define BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(op, wide_type, return_type)                                               \
    template <BOOST_INT256_DEFAULTED_INT128_INTEROP_CONCEPT>                                                                   \
    BOOST_INT256_HOST_DEVICE constexpr return_type operator op(const wide_type& lhs, const Int128Type rhs) noexcept            \
    {                                                                                                                          \
        return lhs op wide_type{rhs};                                                                                          \
    }                                                                                                                          \
                                                                                                                               \
    template <BOOST_INT256_DEFAULTED_INT128_INTEROP_CONCEPT>                                                                   \
    BOOST_INT256_HOST_DEVICE constexpr return_type operator op(const Int128Type lhs, const wide_type& rhs) noexcept            \
    {                                                                                                                          \
        return wide_type{lhs} op rhs;                                                                                          \
    }

#define BOOST_INT256_DETAIL_INT128_INTEROP_ALL_BINARY_OPS(wide_type)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(==, wide_type, bool)                                                          \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(!=, wide_type, bool)                                                          \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(<, wide_type, bool)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(<=, wide_type, bool)                                                          \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(>, wide_type, bool)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(>=, wide_type, bool)                                                          \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(|, wide_type, wide_type)                                                      \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(&, wide_type, wide_type)                                                      \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(^, wide_type, wide_type)                                                      \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(+, wide_type, wide_type)                                                      \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(-, wide_type, wide_type)                                                      \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(*, wide_type, wide_type)                                                      \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(/, wide_type, wide_type)                                                      \
    BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(%, wide_type, wide_type)

BOOST_INT256_DETAIL_INT128_INTEROP_ALL_BINARY_OPS(uint256)
BOOST_INT256_DETAIL_INT128_INTEROP_ALL_BINARY_OPS(int256)

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR

BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(<=>, uint256, std::strong_ordering)
BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP(<=>, int256, std::strong_ordering)

#endif // BOOST_INT256_HAS_SPACESHIP_OPERATOR

#undef BOOST_INT256_DETAIL_INT128_INTEROP_ALL_BINARY_OPS
#undef BOOST_INT256_DETAIL_INT128_INTEROP_BINARY_OP

//=====================================
// Shift Operators
//=====================================

// As for the builtins, a shift takes its value and its result type from the left operand
// alone, and only the count from the right. A count of 2^64 or more is undefined anyway,
// so only the low word of it is used
#define BOOST_INT256_DETAIL_INT128_INTEROP_SHIFT_OP(op, wide_type)                                                             \
    template <BOOST_INT256_DEFAULTED_INT128_INTEROP_CONCEPT>                                                                   \
    BOOST_INT256_HOST_DEVICE constexpr wide_type operator op(const wide_type& lhs, const Int128Type rhs) noexcept              \
    {                                                                                                                          \
        return lhs op rhs.low;                                                                                                 \
    }                                                                                                                          \
                                                                                                                               \
    template <BOOST_INT256_DEFAULTED_INT128_INTEROP_CONCEPT>                                                                   \
    BOOST_INT256_HOST_DEVICE constexpr Int128Type operator op(const Int128Type lhs, const wide_type& rhs) noexcept             \
    {                                                                                                                          \
        return lhs op rhs.words[0];                                                                                            \
    }

BOOST_INT256_DETAIL_INT128_INTEROP_SHIFT_OP(<<, uint256)
BOOST_INT256_DETAIL_INT128_INTEROP_SHIFT_OP(>>, uint256)
BOOST_INT256_DETAIL_INT128_INTEROP_SHIFT_OP(<<, int256)
BOOST_INT256_DETAIL_INT128_INTEROP_SHIFT_OP(>>, int256)

#undef BOOST_INT256_DETAIL_INT128_INTEROP_SHIFT_OP

//=====================================
// Compound Assignment
//=====================================

// Both operand orders, each applying the binary operator above. With the Boost.Int128 type
// on the left, the result keeps its low 128 bits as the builtin compound operators do. With
// uint256 or int256 on the left an exact match is needed: int256 op= uint128 would otherwise
// tie int256's own member against the mixed operator taking a uint256
#define BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(op, compound_op, wide_type)                                             \
    template <BOOST_INT256_DEFAULTED_INT128_INTEROP_CONCEPT>                                                                   \
    BOOST_INT256_HOST_DEVICE constexpr wide_type& operator compound_op(wide_type& lhs, const Int128Type rhs) noexcept          \
    {                                                                                                                          \
        lhs = lhs op rhs;                                                                                                      \
        return lhs;                                                                                                            \
    }                                                                                                                          \
                                                                                                                               \
    template <BOOST_INT256_DEFAULTED_INT128_INTEROP_CONCEPT>                                                                   \
    BOOST_INT256_HOST_DEVICE constexpr Int128Type& operator compound_op(Int128Type& lhs, const wide_type& rhs) noexcept        \
    {                                                                                                                          \
        lhs = detail::convert_to<Int128Type>(lhs op rhs);                                                                      \
        return lhs;                                                                                                            \
    }

#define BOOST_INT256_DETAIL_INT128_INTEROP_ALL_COMPOUND_OPS(wide_type)                                                         \
    BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(|, |=, wide_type)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(&, &=, wide_type)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(^, ^=, wide_type)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(+, +=, wide_type)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(-, -=, wide_type)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(*, *=, wide_type)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(/, /=, wide_type)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(%, %=, wide_type)                                                           \
    BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(<<, <<=, wide_type)                                                         \
    BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP(>>, >>=, wide_type)

BOOST_INT256_DETAIL_INT128_INTEROP_ALL_COMPOUND_OPS(uint256)
BOOST_INT256_DETAIL_INT128_INTEROP_ALL_COMPOUND_OPS(int256)

#undef BOOST_INT256_DETAIL_INT128_INTEROP_ALL_COMPOUND_OPS
#undef BOOST_INT256_DETAIL_INT128_INTEROP_COMPOUND_OP

} // namespace int256
} // namespace boost

#endif // BOOST_INT128_INT128_HPP && !BOOST_INT256_BUILD_MODULE

#endif // BOOST_INT256_DETAIL_INT128_INTEROP_HPP
