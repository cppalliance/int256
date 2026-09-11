// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_COMMON_ADD_HPP
#define BOOST_INT256_DETAIL_COMMON_ADD_HPP

#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/fwd.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstdint>

#endif

namespace boost {
namespace int256 {
namespace detail {

// One word of an add-with-carry chain. carry_in must be 0 or 1 and the result is 0 or 1,
// so the two partial carries can never both be set. The overflow builtins are constexpr
// from clang 7 and GCC 10, which is what the macro certifies, so this needs no gate
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr std::uint64_t
add_carry(const std::uint64_t a, const std::uint64_t b, const std::uint64_t carry_in, std::uint64_t& out) noexcept
{
    #ifdef BOOST_INT256_HAS_BUILTIN_ADD_OVERFLOW

    std::uint64_t sum {};
    const auto first_carry {static_cast<std::uint64_t>(__builtin_add_overflow(a, b, &sum))};
    const auto second_carry {static_cast<std::uint64_t>(__builtin_add_overflow(sum, carry_in, &out))};

    return first_carry + second_carry;

    #else

    const std::uint64_t sum {a + b};
    const std::uint64_t first_carry {static_cast<std::uint64_t>(sum < a)};
    const std::uint64_t total {sum + carry_in};

    out = total;

    return first_carry + static_cast<std::uint64_t>(total < sum);

    #endif
}

// One word of a subtract-with-borrow chain, with the same 0 or 1 contract
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr std::uint64_t
sub_borrow(const std::uint64_t a, const std::uint64_t b, const std::uint64_t borrow_in, std::uint64_t& out) noexcept
{
    #ifdef BOOST_INT256_HAS_BUILTIN_SUB_OVERFLOW

    std::uint64_t difference {};
    const auto first_borrow {static_cast<std::uint64_t>(__builtin_sub_overflow(a, b, &difference))};
    const auto second_borrow {static_cast<std::uint64_t>(__builtin_sub_overflow(difference, borrow_in, &out))};

    return first_borrow + second_borrow;

    #else

    const std::uint64_t difference {a - b};
    const std::uint64_t first_borrow {static_cast<std::uint64_t>(a < b)};
    const std::uint64_t total {difference - borrow_in};

    out = total;

    return first_borrow + static_cast<std::uint64_t>(difference < borrow_in);

    #endif
}

// The word functions are templates so that this header does not need the complete
// definition of uint256; T is deduced as uint256 at every call site.
//
// Each has a runtime only fast path.
//
// The x86-64 carry intrinsics (_addcarry_u64 and friends) take a pointer out-parameter of type
// unsigned long long, so the chain has to run through an array of them. On GCC and Clang that
// array is materialized on the stack, which costs more than it saves because the overflow
// builtins already lower the portable chain to one add plus three adc in registers. They are
// therefore only used where the overflow builtins do not exist, which today means MSVC on x64.
//
// The Clang carry builtins are free of that problem and produce the same code as the overflow
// builtins for the addition and a much shorter sbb chain for the subtraction, so they are used
// wherever they exist. Constant evaluation always runs the portable chain

// Full 256-bit addition, modulo 2^256
template <typename T>
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr T add_words(const T& lhs, const T& rhs) noexcept
{
    #if defined(BOOST_INT256_ADD_CARRY) && !defined(BOOST_INT256_HAS_BUILTIN_ADD_OVERFLOW) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        unsigned long long r[4] {};
        unsigned char carry {};

        carry = BOOST_INT256_ADD_CARRY(carry, lhs.words[0], rhs.words[0], &r[0]);
        carry = BOOST_INT256_ADD_CARRY(carry, lhs.words[1], rhs.words[1], &r[1]);
        carry = BOOST_INT256_ADD_CARRY(carry, lhs.words[2], rhs.words[2], &r[2]);
        BOOST_INT256_ADD_CARRY(carry, lhs.words[3], rhs.words[3], &r[3]);

        return T{static_cast<std::uint64_t>(r[3]), static_cast<std::uint64_t>(r[2]),
                 static_cast<std::uint64_t>(r[1]), static_cast<std::uint64_t>(r[0])};
    }

    #elif defined(BOOST_INT256_HAS_BUILTIN_ADDCLL) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        unsigned long long carry {};
        const auto r0 {__builtin_addcll(lhs.words[0], rhs.words[0], carry, &carry)};
        const auto r1 {__builtin_addcll(lhs.words[1], rhs.words[1], carry, &carry)};
        const auto r2 {__builtin_addcll(lhs.words[2], rhs.words[2], carry, &carry)};
        const auto r3 {__builtin_addcll(lhs.words[3], rhs.words[3], carry, &carry)};

        return T{static_cast<std::uint64_t>(r3), static_cast<std::uint64_t>(r2),
                 static_cast<std::uint64_t>(r1), static_cast<std::uint64_t>(r0)};
    }

    #endif

    std::uint64_t r0 {};
    std::uint64_t r1 {};
    std::uint64_t r2 {};
    std::uint64_t r3 {};

    auto carry {add_carry(lhs.words[0], rhs.words[0], UINT64_C(0), r0)};
    carry = add_carry(lhs.words[1], rhs.words[1], carry, r1);
    carry = add_carry(lhs.words[2], rhs.words[2], carry, r2);
    add_carry(lhs.words[3], rhs.words[3], carry, r3);

    return T{r3, r2, r1, r0};
}

// 256-bit plus one word
template <typename T>
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr T add_words(const T& lhs, const std::uint64_t rhs) noexcept
{
    #if defined(BOOST_INT256_ADD_CARRY) && !defined(BOOST_INT256_HAS_BUILTIN_ADD_OVERFLOW) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        unsigned long long r[4] {};
        unsigned char carry {};

        carry = BOOST_INT256_ADD_CARRY(carry, lhs.words[0], rhs, &r[0]);
        carry = BOOST_INT256_ADD_CARRY(carry, lhs.words[1], 0ULL, &r[1]);
        carry = BOOST_INT256_ADD_CARRY(carry, lhs.words[2], 0ULL, &r[2]);
        BOOST_INT256_ADD_CARRY(carry, lhs.words[3], 0ULL, &r[3]);

        return T{static_cast<std::uint64_t>(r[3]), static_cast<std::uint64_t>(r[2]),
                 static_cast<std::uint64_t>(r[1]), static_cast<std::uint64_t>(r[0])};
    }

    #endif

    std::uint64_t r0 {};
    std::uint64_t r1 {};
    std::uint64_t r2 {};
    std::uint64_t r3 {};

    auto carry {add_carry(lhs.words[0], rhs, UINT64_C(0), r0)};
    carry = add_carry(lhs.words[1], UINT64_C(0), carry, r1);
    carry = add_carry(lhs.words[2], UINT64_C(0), carry, r2);
    add_carry(lhs.words[3], UINT64_C(0), carry, r3);

    return T{r3, r2, r1, r0};
}

// Full 256-bit subtraction, modulo 2^256
template <typename T>
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr T sub_words(const T& lhs, const T& rhs) noexcept
{
    #if defined(BOOST_INT256_SUB_BORROW) && !defined(BOOST_INT256_HAS_BUILTIN_SUB_OVERFLOW) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        unsigned long long r[4] {};
        unsigned char borrow {};

        borrow = BOOST_INT256_SUB_BORROW(borrow, lhs.words[0], rhs.words[0], &r[0]);
        borrow = BOOST_INT256_SUB_BORROW(borrow, lhs.words[1], rhs.words[1], &r[1]);
        borrow = BOOST_INT256_SUB_BORROW(borrow, lhs.words[2], rhs.words[2], &r[2]);
        BOOST_INT256_SUB_BORROW(borrow, lhs.words[3], rhs.words[3], &r[3]);

        return T{static_cast<std::uint64_t>(r[3]), static_cast<std::uint64_t>(r[2]),
                 static_cast<std::uint64_t>(r[1]), static_cast<std::uint64_t>(r[0])};
    }

    #elif defined(BOOST_INT256_HAS_BUILTIN_ADDCLL) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        unsigned long long borrow {};
        const auto r0 {__builtin_subcll(lhs.words[0], rhs.words[0], borrow, &borrow)};
        const auto r1 {__builtin_subcll(lhs.words[1], rhs.words[1], borrow, &borrow)};
        const auto r2 {__builtin_subcll(lhs.words[2], rhs.words[2], borrow, &borrow)};
        const auto r3 {__builtin_subcll(lhs.words[3], rhs.words[3], borrow, &borrow)};

        return T{static_cast<std::uint64_t>(r3), static_cast<std::uint64_t>(r2),
                 static_cast<std::uint64_t>(r1), static_cast<std::uint64_t>(r0)};
    }

    #endif

    std::uint64_t r0 {};
    std::uint64_t r1 {};
    std::uint64_t r2 {};
    std::uint64_t r3 {};

    auto borrow {sub_borrow(lhs.words[0], rhs.words[0], UINT64_C(0), r0)};
    borrow = sub_borrow(lhs.words[1], rhs.words[1], borrow, r1);
    borrow = sub_borrow(lhs.words[2], rhs.words[2], borrow, r2);
    sub_borrow(lhs.words[3], rhs.words[3], borrow, r3);

    return T{r3, r2, r1, r0};
}

// 256-bit minus one word
template <typename T>
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr T sub_words(const T& lhs, const std::uint64_t rhs) noexcept
{
    #if defined(BOOST_INT256_SUB_BORROW) && !defined(BOOST_INT256_HAS_BUILTIN_SUB_OVERFLOW) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        unsigned long long r[4] {};
        unsigned char borrow {};

        borrow = BOOST_INT256_SUB_BORROW(borrow, lhs.words[0], rhs, &r[0]);
        borrow = BOOST_INT256_SUB_BORROW(borrow, lhs.words[1], 0ULL, &r[1]);
        borrow = BOOST_INT256_SUB_BORROW(borrow, lhs.words[2], 0ULL, &r[2]);
        BOOST_INT256_SUB_BORROW(borrow, lhs.words[3], 0ULL, &r[3]);

        return T{static_cast<std::uint64_t>(r[3]), static_cast<std::uint64_t>(r[2]),
                 static_cast<std::uint64_t>(r[1]), static_cast<std::uint64_t>(r[0])};
    }

    #endif

    std::uint64_t r0 {};
    std::uint64_t r1 {};
    std::uint64_t r2 {};
    std::uint64_t r3 {};

    auto borrow {sub_borrow(lhs.words[0], rhs, UINT64_C(0), r0)};
    borrow = sub_borrow(lhs.words[1], UINT64_C(0), borrow, r1);
    borrow = sub_borrow(lhs.words[2], UINT64_C(0), borrow, r2);
    sub_borrow(lhs.words[3], UINT64_C(0), borrow, r3);

    return T{r3, r2, r1, r0};
}

} // namespace detail
} // namespace int256
} // namespace boost

#endif // BOOST_INT256_DETAIL_COMMON_ADD_HPP
