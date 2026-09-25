// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_COMMON_DIV_HPP
#define BOOST_INT256_DETAIL_COMMON_DIV_HPP

#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/fwd.hpp>
#include <boost/int256/detail/clz.hpp>
#include <boost/int256/detail/common_add.hpp>
#include <boost/int256/detail/common_mul.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstdint>
#include <cstddef>

#endif

namespace boost {
namespace int256 {
namespace detail {

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wassume"
#endif

// Portable 128-bit by 64-bit unsigned division producing a 64-bit quotient and remainder.
// This is the classic Hacker's Delight divlu (two 32-bit digit steps over 64-bit words).
// Precondition: u1 < d so the quotient is guaranteed to fit in 64 bits. It is constexpr-safe
// and serves as the fallback for udiv_2by1 on every target without a hardware 128/64 divide.
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr std::uint64_t divlu(std::uint64_t u1, std::uint64_t u0, std::uint64_t d, std::uint64_t& r) noexcept
{
    constexpr std::uint64_t b {UINT64_C(1) << 32U}; // Number base (2^32)

    BOOST_INT256_ASSUME(u1 < d); // LCOV_EXCL_LINE

    // D.1: normalize so that the divisor's most significant bit is set
    const auto s {countl_zero(d)};
    d <<= s;

    const auto vn1 {d >> 32U};
    const auto vn0 {d & UINT32_MAX};

    // Shift the dividend left by s. The (64 - s) shift is undefined when s == 0, so guard it.
    const auto un32 {s == 0 ? u1 : ((u1 << s) | (u0 >> (64 - s)))};
    const auto un10 {u0 << s};

    const auto un1 {un10 >> 32U};
    const auto un0 {un10 & UINT32_MAX};

    // First quotient digit
    auto q1 {un32 / vn1};
    auto rhat {un32 - (q1 * vn1)};

    while (q1 >= b || (q1 * vn0) > ((b * rhat) + un1))
    {
        --q1;
        rhat += vn1;
        if (rhat >= b)
        {
            break;
        }
    }

    const auto un21 {(un32 * b) + un1 - (q1 * d)};

    // Second quotient digit
    auto q0 {un21 / vn1};
    rhat = un21 - (q0 * vn1);

    while (q0 >= b || (q0 * vn0) > ((b * rhat) + un0))
    {
        --q0;
        rhat += vn1;
        if (rhat >= b)
        {
            break;
        }
    }

    // The remainder is shifted back down by the normalization amount
    r = ((un21 * b) + un0 - (q0 * d)) >> s;
    return (q1 * b) + q0;
}

#if defined(BOOST_INT256_HAS_X86_64_DIVQ)

// Inline asm cannot appear in a constexpr function body before C++20, so the x86-64 DIV
// instruction is wrapped in a non-constexpr helper that udiv_2by1 only calls at runtime.
BOOST_INT256_FORCE_INLINE std::uint64_t udiv_2by1_divq(const std::uint64_t u1, const std::uint64_t u0, const std::uint64_t d, std::uint64_t& r) noexcept
{
    std::uint64_t q {};
    __asm__("divq %[d]" : "=a"(q), "=d"(r) : [d] "r"(d), "a"(u0), "d"(u1) : "cc");
    return q;
}

#endif // BOOST_INT256_HAS_X86_64_DIVQ

// Divides the 128-bit value (u1:u0) by d, returning a 64-bit quotient and the true remainder.
// Precondition: u1 < d. Mirrors common_mul.hpp umul: a hardware instruction at runtime where
// one exists, and the portable divlu in constant evaluation and everywhere else.
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr std::uint64_t udiv_2by1(const std::uint64_t u1, const std::uint64_t u0, const std::uint64_t d, std::uint64_t& r) noexcept
{
    BOOST_INT256_ASSUME(u1 < d); // LCOV_EXCL_LINE

    #if defined(BOOST_INT256_HAS_FAST_DIV128) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(u1))
    {
        #if defined(BOOST_INT256_HAS_X86_64_DIVQ)

        return udiv_2by1_divq(u1, u0, d, r);

        #else

        return _udiv128(u1, u0, d, &r);

        #endif
    }

    #endif

    return divlu(u1, u0, d, r);
}

#ifndef BOOST_INT256_NO_CONSTEVAL_DETECTION

// Moller and Granlund, "Improved Division by Invariant Integers" (2010), algorithms 2 to 6.
// These replace the per-word hardware division of Algorithm D with multiplications against a
// precomputed reciprocal. They are runtime only, so constant evaluation always runs Algorithm D

// Precomputes the reciprocal v of the normalized one word divisor d, d >= 2^63, such that
// floor((2^128 - 1) / d) = 2^64 + v. Algorithm 2, evaluated with one 128/64 division: the
// dividend's high word ~d is below d because d >= 2^63, which is the udiv_2by1 precondition
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE std::uint64_t mg_reciprocal_2by1(const std::uint64_t d) noexcept
{
    std::uint64_t ignored {};
    return udiv_2by1(~d, UINT64_MAX, d, ignored);
}

// One 2-by-1 word division step: (u1:u0) / d with the precomputed reciprocal v, returning the
// one word quotient and writing the remainder to r. Algorithm 4.
// Preconditions: d >= 2^63 and u1 < d. Every intermediate is modulo 2^64 by design
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE std::uint64_t mg_udiv_2by1(const std::uint64_t u1, const std::uint64_t u0,
                                                                             const std::uint64_t d, const std::uint64_t v,
                                                                             std::uint64_t& r) noexcept
{
    // (q1:q0) = v * u1 + (u1:u0), then one more on the high word
    std::uint64_t q1 {};
    auto q0 {umul(v, u1, q1)};

    const auto carry {add_carry(q0, u0, UINT64_C(0), q0)};
    q1 += u1 + carry + UINT64_C(1);

    auto remainder {u0 - q1 * d};

    // First correction, taken about half the time
    if (remainder > q0)
    {
        --q1;
        remainder += d;
    }

    // Second correction, rare
    if (BOOST_INT256_UNLIKELY(remainder >= d))
    {
        ++q1;                       // LCOV_EXCL_LINE
        remainder -= d;             // LCOV_EXCL_LINE
    }

    r = remainder;

    return q1;
}

// 256/64 division through four chained 2-by-1 steps against a single reciprocal. This replaces
// four software divlu calls with one, plus two multiplications per word, which is the whole
// point of the reciprocal on targets without a hardware 128/64 divide
template <typename T>
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE void mg_divide_1word(const T& lhs, const std::uint64_t divisor,
                                                                       T& quotient, std::uint64_t& remainder) noexcept
{
    BOOST_INT256_ASSUME(divisor != 0); // LCOV_EXCL_LINE

    const auto s {static_cast<unsigned>(countl_zero(divisor))};
    const auto complement {64U - s};
    const bool needs_shift {s > 0U};

    const auto d {divisor << s};
    const auto v {mg_reciprocal_2by1(d)};

    // The normalized dividend is one word wider, and that word is below 2^s <= d, which is
    // the precondition of the first step. Normalizing both operands leaves the quotient
    // unchanged and scales the remainder by 2^s
    const auto u0 {needs_shift ? (lhs.words[0] << s) : lhs.words[0]};
    const auto u1 {needs_shift ? ((lhs.words[1] << s) | (lhs.words[0] >> complement)) : lhs.words[1]};
    const auto u2 {needs_shift ? ((lhs.words[2] << s) | (lhs.words[1] >> complement)) : lhs.words[2]};
    const auto u3 {needs_shift ? ((lhs.words[3] << s) | (lhs.words[2] >> complement)) : lhs.words[3]};

    auto r {needs_shift ? (lhs.words[3] >> complement) : UINT64_C(0)};

    std::uint64_t q[4] {};
    q[3] = mg_udiv_2by1(r, u3, d, v, r);
    q[2] = mg_udiv_2by1(r, u2, d, v, r);
    q[1] = mg_udiv_2by1(r, u1, d, v, r);
    q[0] = mg_udiv_2by1(r, u0, d, v, r);

    quotient = T{q[3], q[2], q[1], q[0]};
    remainder = r >> s;
}

// Precomputes the reciprocal v of the normalized two word divisor (d1:d0), d1 >= 2^63, such
// that floor((2^192 - 1) / d) = 2^64 + v
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE std::uint64_t mg_reciprocal_3by2(const std::uint64_t d1, const std::uint64_t d0) noexcept
{
    // ~d1 < d1 because d1 >= 2^63, which is the udiv_2by1 precondition
    std::uint64_t ignored {};
    auto v {udiv_2by1(~d1, UINT64_MAX, d1, ignored)};

    auto p {d1 * v + d0};
    if (p < d0)
    {
        --v;
        if (p >= d1)
        {
            --v;
            p -= d1;
        }
        p -= d1;
    }

    std::uint64_t t_hi {};
    const auto t_lo {umul(v, d0, t_hi)};

    const auto p_sum {p + t_hi};
    if (p_sum < p)
    {
        --v;
        if (p_sum > d1 || (p_sum == d1 && t_lo >= d0))
        {
            --v;
        }
    }

    return v;
}

// One 3-by-2 word division step: (u2:u1:u0) / (d1:d0) with the reciprocal v, returning the
// one word quotient and writing the two word remainder to (r1:r0).
// Preconditions: d1 >= 2^63 and (u2:u1) < (d1:d0)
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE std::uint64_t mg_div_3by2(const std::uint64_t u2, const std::uint64_t u1, const std::uint64_t u0,
                                                                            const std::uint64_t d1, const std::uint64_t d0, const std::uint64_t v,
                                                                            std::uint64_t& r1, std::uint64_t& r0) noexcept
{
    // (q1:q0) = u2 * v + (u2:u1)
    std::uint64_t q1 {};
    auto q0 {umul(u2, v, q1)};

    const auto low_sum {q0 + u1};
    q1 += u2 + static_cast<std::uint64_t>(low_sum < q0);
    q0 = low_sum;

    // (rem1:rem0) = (u1 - q1 * d1 : u0) - q1 * d0 - d
    const auto estimate_hi {u1 - q1 * d1};

    std::uint64_t t_hi {};
    const auto t_lo {umul(q1, d0, t_hi)};

    std::uint64_t rem0 {};
    std::uint64_t rem1 {};
    auto borrow {sub_borrow(u0, t_lo, UINT64_C(0), rem0)};
    borrow = sub_borrow(estimate_hi, t_hi, borrow, rem1);
    borrow = sub_borrow(rem0, d0, UINT64_C(0), rem0);
    sub_borrow(rem1, d1, borrow, rem1);

    auto quotient {q1 + UINT64_C(1)};

    // First correction, taken about half the time
    if (rem1 >= q0)
    {
        --quotient;

        const auto carry {add_carry(rem0, d0, UINT64_C(0), rem0)};
        add_carry(rem1, d1, carry, rem1);
    }

    // Second correction, rare
    if (BOOST_INT256_UNLIKELY(rem1 > d1 || (rem1 == d1 && rem0 >= d0)))
    {
        ++quotient;                                                     // LCOV_EXCL_LINE

        const auto carry {sub_borrow(rem0, d0, UINT64_C(0), rem0)};     // LCOV_EXCL_LINE
        sub_borrow(rem1, d1, carry, rem1);                              // LCOV_EXCL_LINE
    }

    r1 = rem1;
    r0 = rem0;

    return quotient;
}

// 256/128 division through three chained 3-by-2 steps. Normalizing the dividend into five
// words means the top two words are always below the normalized divisor, so no input is
// rejected and the caller never has to fall back
template <typename T>
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE void mg_divide_2word(const T& lhs, const std::uint64_t divisor_high, const std::uint64_t divisor_low,
                                                                       T& quotient, T& remainder) noexcept
{
    BOOST_INT256_ASSUME(divisor_high != 0); // LCOV_EXCL_LINE

    const auto s {static_cast<unsigned>(countl_zero(divisor_high))};
    const auto complement {64U - s};
    const bool needs_shift {s > 0U};

    const auto d1 {needs_shift ? ((divisor_high << s) | (divisor_low >> complement)) : divisor_high};
    const auto d0 {needs_shift ? (divisor_low << s) : divisor_low};

    std::uint64_t un[5] {};
    un[0] = needs_shift ? (lhs.words[0] << s) : lhs.words[0];
    un[1] = needs_shift ? ((lhs.words[1] << s) | (lhs.words[0] >> complement)) : lhs.words[1];
    un[2] = needs_shift ? ((lhs.words[2] << s) | (lhs.words[1] >> complement)) : lhs.words[2];
    un[3] = needs_shift ? ((lhs.words[3] << s) | (lhs.words[2] >> complement)) : lhs.words[3];
    un[4] = needs_shift ? (lhs.words[3] >> complement) : UINT64_C(0);

    const auto v {mg_reciprocal_3by2(d1, d0)};

    // The quotient of a 256-bit value by a divisor of at least 2^64 needs at most three words
    const auto q2 {mg_div_3by2(un[4], un[3], un[2], d1, d0, v, un[3], un[2])};
    const auto q1 {mg_div_3by2(un[3], un[2], un[1], d1, d0, v, un[2], un[1])};
    const auto q0 {mg_div_3by2(un[2], un[1], un[0], d1, d0, v, un[1], un[0])};

    quotient = T{UINT64_C(0), q2, q1, q0};

    remainder = needs_shift ? T{UINT64_C(0), UINT64_C(0), un[1] >> s, (un[0] >> s) | (un[1] << complement)}
                            : T{UINT64_C(0), UINT64_C(0), un[1], un[0]};
}

#endif // BOOST_INT256_NO_CONSTEVAL_DETECTION

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4127) // Pre C++17 the need_remainder branch is a constant condition
#endif

// 256/32 -> 256-bit quotient and 64-bit remainder by eight chained 64/32 divides. This is the
// narrow path for targets without a hardware 128/64 divide, where a 64/64 divide of a value
// that fits in 32 bits is the cheapest primitive available
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr void half_word_div(const T& lhs, const std::uint32_t rhs, T& quotient, std::uint64_t& remainder) noexcept
{
    BOOST_INT256_ASSUME(rhs != 0); // LCOV_EXCL_LINE

    const std::uint64_t divisor {rhs};
    std::uint64_t r {};
    std::uint64_t q[4] {};

    for (int i {3}; i >= 0; --i)
    {
        const auto word {lhs.words[i]};

        r = (r << 32U) | (word >> 32U);
        const auto q_hi {r / divisor};
        r %= divisor;

        r = (r << 32U) | (word & UINT32_MAX);
        const auto q_lo {r / divisor};
        r %= divisor;

        q[i] = (q_hi << 32U) | q_lo;
    }

    quotient = T{q[3], q[2], q[1], q[0]};
    remainder = r;
}

// The one word divisor tier. Four chained udiv_2by1 calls, where the running remainder is
// always below the divisor, which is exactly the udiv_2by1 precondition.
//
// The runtime choice is measured per target (1e6 divisions of a four word dividend, -O2):
//
//                            arm64 real   i386 emulated   arm32 emulated
//   four chained udiv_2by1     34.8 ns        336 ns          592 ns
//   reciprocal (2-by-1 MG)     13.7 ns        347 ns          567 ns
//   half_word_div, 32-bit d      6.3 ns        302 ns         1639 ns
//
// So where there is no hardware 128/64 divide the four software divisions dominate and the
// Moller and Granlund reciprocal, which needs only one, replaces them. A divisor that fits in
// 32 bits goes to the eight 64/32 divides of half_word_div instead, but only where a 64-bit
// division is a single instruction: on 32-bit ARM it is a library call, which makes that path
// three times slower than the reciprocal. On the targets with a hardware 128/64 divide
// (x86-64 divq, MSVC _udiv128) the chained form is already four instructions and is left
// alone. Constant evaluation always runs the chained form
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr void one_word_div(const T& lhs, const std::uint64_t rhs, T& quotient) noexcept
{
    BOOST_INT256_ASSUME(rhs != 0); // LCOV_EXCL_LINE

    #if !defined(BOOST_INT256_HAS_FAST_DIV128) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(rhs))
    {
        std::uint64_t ignored {};

        #if !defined(__arm__) && !defined(__thumb__) && !defined(_M_ARM)

        if (rhs <= UINT32_MAX)
        {
            half_word_div(lhs, static_cast<std::uint32_t>(rhs), quotient, ignored);
            return;
        }

        #endif

        mg_divide_1word(lhs, rhs, quotient, ignored);
        return;
    }

    #endif

    std::uint64_t r {};
    std::uint64_t q[4] {};

    q[3] = udiv_2by1(r, lhs.words[3], rhs, r);
    q[2] = udiv_2by1(r, lhs.words[2], rhs, r);
    q[1] = udiv_2by1(r, lhs.words[1], rhs, r);
    q[0] = udiv_2by1(r, lhs.words[0], rhs, r);

    quotient = T{q[3], q[2], q[1], q[0]};
}

// The same division, keeping the 64-bit remainder
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr void one_word_div(const T& lhs, const std::uint64_t rhs, T& quotient, std::uint64_t& remainder) noexcept
{
    BOOST_INT256_ASSUME(rhs != 0); // LCOV_EXCL_LINE

    #if !defined(BOOST_INT256_HAS_FAST_DIV128) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(rhs))
    {
        // See the note on the three way runtime choice above
        #if !defined(__arm__) && !defined(__thumb__) && !defined(_M_ARM)

        if (rhs <= UINT32_MAX)
        {
            half_word_div(lhs, static_cast<std::uint32_t>(rhs), quotient, remainder);
            return;
        }

        #endif

        mg_divide_1word(lhs, rhs, quotient, remainder);
        return;
    }

    #endif

    std::uint64_t r {};
    std::uint64_t q[4] {};

    q[3] = udiv_2by1(r, lhs.words[3], rhs, r);
    q[2] = udiv_2by1(r, lhs.words[2], rhs, r);
    q[1] = udiv_2by1(r, lhs.words[1], rhs, r);
    q[0] = udiv_2by1(r, lhs.words[0], rhs, r);

    quotient = T{q[3], q[2], q[1], q[0]};
    remainder = r;
}

// See: The Art of Computer Programming Volume 2 (Seminumerical algorithms) section 4.3.1
// Algorithm D: Division of non-negative integers, on 64-bit words.
//
// Divides the m word dividend u by the n word divisor v (m >= n >= 2, v[n - 1] != 0) and writes
// the m - n + 1 word quotient to q. With need_remainder the n word remainder is left in u[0..n-1]
// and the words above it are cleared, otherwise u is scratch. u needs one word more than m for
// the normalization, which is why it is five words wide.
template <bool need_remainder>
BOOST_INT256_HOST_DEVICE constexpr void knuth_divide(std::uint64_t (&u)[5], const std::size_t m,
                                                     const std::uint64_t (&v)[4], const std::size_t n,
                                                     std::uint64_t (&q)[4]) noexcept
{
    BOOST_INT256_ASSERT_MSG(n >= 2U && m >= n && m <= 4U, "Algorithm D operand shape");

    q[0] = UINT64_C(0);
    q[1] = UINT64_C(0);
    q[2] = UINT64_C(0);
    q[3] = UINT64_C(0);

    // D.1: normalize so the top word of the divisor has its most significant bit set.
    // The complementary shift is only taken when s is non-zero, since a shift by 64 is undefined
    const auto s {static_cast<unsigned>(countl_zero(v[n - 1U]))};
    const auto complement {64U - s};
    const bool needs_shift {s > 0U};

    std::uint64_t un[5] {};
    std::uint64_t vn[4] {};

    for (std::size_t i {n - 1U}; i > 0U; --i)
    {
        vn[i] = needs_shift ? ((v[i] << s) | (v[i - 1U] >> complement)) : v[i];
    }
    vn[0] = needs_shift ? (v[0] << s) : v[0];

    un[m] = needs_shift ? (u[m - 1U] >> complement) : UINT64_C(0);
    for (std::size_t i {m - 1U}; i > 0U; --i)
    {
        un[i] = needs_shift ? ((u[i] << s) | (u[i - 1U] >> complement)) : u[i];
    }
    un[0] = needs_shift ? (u[0] << s) : u[0];

    // D.2: loop over the quotient words, most significant first
    for (std::size_t j {m - n}; j != static_cast<std::size_t>(-1); --j)
    {
        // D.3: estimate one quotient word from the top two words of the running remainder.
        // The normalization guarantees un[j + n] <= vn[n - 1]; when they are equal the true
        // estimate is 2^64 and is clamped, which leaves a remainder that can exceed one word
        std::uint64_t qhat {};
        std::uint64_t rhat {};
        bool rhat_overflow {false};

        if (un[j + n] < vn[n - 1U])
        {
            qhat = udiv_2by1(un[j + n], un[j + n - 1U], vn[n - 1U], rhat);
        }
        else
        {
            qhat = UINT64_MAX;
            rhat = un[j + n - 1U] + vn[n - 1U];
            rhat_overflow = rhat < un[j + n - 1U];
        }

        // Refine the estimate against the second divisor word. Knuth's theorem B bounds the
        // top word estimate by q + 2, and this brings it down to at most one too large, which
        // the add-back below corrects. At most two iterations run
        if (!rhat_overflow)
        {
            std::uint64_t product_hi {};
            auto product_lo {umul(qhat, vn[n - 2U], product_hi)};

            while (product_hi > rhat || (product_hi == rhat && product_lo > un[j + n - 2U]))
            {
                --qhat;
                rhat += vn[n - 1U];

                const bool rhat_carry {rhat < vn[n - 1U]};
                product_lo = umul(qhat, vn[n - 2U], product_hi);

                if (rhat_carry)
                {
                    break;
                }
            }
        }

        // D.4: multiply and subtract qhat * vn from the running remainder
        std::uint64_t borrow {};
        std::uint64_t carry {};

        for (std::size_t i {}; i < n; ++i)
        {
            std::uint64_t product_hi {};
            const auto product_lo {umul(qhat, vn[i], product_hi)};

            std::uint64_t subtrahend {};
            const auto product_carry {add_carry(product_lo, carry, UINT64_C(0), subtrahend)};
            carry = product_hi + product_carry;

            std::uint64_t difference {};
            borrow = sub_borrow(un[j + i], subtrahend, borrow, difference);
            un[j + i] = difference;
        }

        std::uint64_t top {};
        const auto top_borrow {sub_borrow(un[j + n], carry, borrow, top)};
        un[j + n] = top;

        // D.5 and D.6: a borrow out of the top word means qhat was one too large, so the
        // divisor is added back. The probability of this branch is about 2^-64
        if (BOOST_INT256_UNLIKELY(top_borrow != UINT64_C(0)))
        {
            --qhat;                                                     // LCOV_EXCL_LINE

            std::uint64_t add_back_carry {};                            // LCOV_EXCL_LINE
            for (std::size_t i {}; i < n; ++i)                          // LCOV_EXCL_LINE
            {                                                           // LCOV_EXCL_LINE
                std::uint64_t sum {};                                   // LCOV_EXCL_LINE
                add_back_carry = add_carry(un[j + i], vn[i], add_back_carry, sum); // LCOV_EXCL_LINE
                un[j + i] = sum;                                        // LCOV_EXCL_LINE
            }                                                           // LCOV_EXCL_LINE

            // The carry out cancels the borrow taken above
            un[j + n] += add_back_carry;                                // LCOV_EXCL_LINE
        }

        q[j] = qhat;
    }

    // D.8: unnormalize the remainder. Only the modulo case needs it
    BOOST_INT256_IF_CONSTEXPR (need_remainder)
    {
        if (needs_shift)
        {
            for (std::size_t i {}; i + 1U < n; ++i)
            {
                u[i] = (un[i] >> s) | (un[i + 1U] << complement);
            }
            u[n - 1U] = un[n - 1U] >> s;
        }
        else
        {
            for (std::size_t i {}; i < n; ++i)
            {
                u[i] = un[i];
            }
        }

        // Anything above the remainder is zero
        for (std::size_t i {n}; i < m; ++i)
        {
            u[i] = UINT64_C(0);
        }
    }
}


namespace impl {

// Number of significant 64-bit words of a 256-bit value, 0 only for zero
template <typename T>
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr std::size_t significant_words(const T& x) noexcept
{
    return x.words[3] != UINT64_C(0) ? 4U :
           x.words[2] != UINT64_C(0) ? 3U :
           x.words[1] != UINT64_C(0) ? 2U :
           x.words[0] != UINT64_C(0) ? 1U : 0U;
}

// lhs < rhs on the words alone, so that this header does not depend on the operators
template <typename T>
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr bool less_than_words(const T& lhs, const T& rhs) noexcept
{
    return lhs.words[3] != rhs.words[3] ? lhs.words[3] < rhs.words[3] :
           lhs.words[2] != rhs.words[2] ? lhs.words[2] < rhs.words[2] :
           lhs.words[1] != rhs.words[1] ? lhs.words[1] < rhs.words[1] :
                                          lhs.words[0] < rhs.words[0];
}

// The multi-word tier of the dispatcher: Algorithm D over the words of lhs and rhs
template <bool need_remainder, typename T>
BOOST_INT256_HOST_DEVICE constexpr void knuth_divide_words(const T& lhs, const T& rhs, const std::size_t m, const std::size_t n,
                                                           T& quotient, T& remainder) noexcept
{
    std::uint64_t u[5] {lhs.words[0], lhs.words[1], lhs.words[2], lhs.words[3], UINT64_C(0)};
    const std::uint64_t v[4] {rhs.words[0], rhs.words[1], rhs.words[2], rhs.words[3]};
    std::uint64_t q[4] {};

    knuth_divide<need_remainder>(u, m, v, n, q);

    quotient = T{q[3], q[2], q[1], q[0]};

    BOOST_INT256_IF_CONSTEXPR (need_remainder)
    {
        remainder = T{u[3], u[2], u[1], u[0]};
    }
    else
    {
        static_cast<void>(remainder);
    }
}

} // namespace impl

// The general 256/256 division. The quotient is always written; the remainder only when
// need_remainder is true, since computing it costs cycles that a plain division does not need.
// Three tiers: a one word divisor divides word by word, a two word divisor takes the
// Moller and Granlund reciprocal path at runtime, and everything else runs Algorithm D
template <bool need_remainder, typename T>
BOOST_INT256_HOST_DEVICE constexpr void divide(const T& lhs, const T& rhs, T& quotient, T& remainder) noexcept
{
    BOOST_INT256_ASSERT_MSG(impl::significant_words(rhs) != 0U, "Division by 0");

    if (impl::less_than_words(lhs, rhs))
    {
        quotient = T{};

        BOOST_INT256_IF_CONSTEXPR (need_remainder)
        {
            remainder = lhs;
        }
        else
        {
            static_cast<void>(remainder);
        }

        return;
    }

    const auto n {impl::significant_words(rhs)};

    // n == 0 is division by zero (asserted above). Sending it here too keeps GCC's
    // -Warray-bounds from following a zero word divisor into Algorithm D at -O3
    if (n <= 1U)
    {
        // one_word_div owns the whole one word tier: the hardware 128/64 divide where there is
        // one, and otherwise the reciprocal or the half word chain, whichever the target measures
        // faster. Constant evaluation runs the chained udiv_2by1 there
        std::uint64_t word_remainder {};

        one_word_div(lhs, rhs.words[0], quotient, word_remainder);

        BOOST_INT256_IF_CONSTEXPR (need_remainder)
        {
            remainder = T{UINT64_C(0), UINT64_C(0), UINT64_C(0), word_remainder};
        }
        else
        {
            static_cast<void>(remainder);
        }

        return;
    }

    #ifndef BOOST_INT256_NO_CONSTEVAL_DETECTION

    if (n == 2U && !BOOST_INT256_IS_CONSTANT_EVALUATED(lhs.words[0]))
    {
        mg_divide_2word(lhs, rhs.words[1], rhs.words[0], quotient, remainder);
        return;
    }

    #endif

    impl::knuth_divide_words<need_remainder>(lhs, rhs, impl::significant_words(lhs), n, quotient, remainder);
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

} // namespace detail
} // namespace int256
} // namespace boost

#endif // BOOST_INT256_DETAIL_COMMON_DIV_HPP
