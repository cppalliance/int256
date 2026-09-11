// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_COMMON_MUL_HPP
#define BOOST_INT256_DETAIL_COMMON_MUL_HPP

#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/fwd.hpp>
#include <boost/int256/detail/common_add.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstdint>

#endif

namespace boost {
namespace int256 {
namespace detail {

// High 64 bits of the 64x64 -> 128 product, computed with four 32-bit partial products
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr std::uint64_t umulh_generic(const std::uint64_t a, const std::uint64_t b) noexcept
{
    const std::uint64_t a_lo {a & UINT32_MAX};
    const std::uint64_t a_hi {a >> 32U};
    const std::uint64_t b_lo {b & UINT32_MAX};
    const std::uint64_t b_hi {b >> 32U};

    const std::uint64_t lo_lo {a_lo * b_lo};
    const std::uint64_t hi_lo {a_hi * b_lo};
    const std::uint64_t lo_hi {a_lo * b_hi};
    const std::uint64_t hi_hi {a_hi * b_hi};

    const std::uint64_t cross {(lo_lo >> 32U) + (hi_lo & UINT32_MAX) + (lo_hi & UINT32_MAX)};

    return hi_hi + (hi_lo >> 32U) + (lo_hi >> 32U) + (cross >> 32U);
}

// Full 64x64 -> 128 product. One instruction on every 64-bit target, including s390x
// where the 64x64 product is a single mlgr
BOOST_INT256_HOST_DEVICE BOOST_INT256_FORCE_INLINE constexpr std::uint64_t umul(const std::uint64_t a, const std::uint64_t b, std::uint64_t& hi) noexcept
{
    #ifndef BOOST_INT256_NO_CONSTEVAL_DETECTION

    if (!BOOST_INT256_IS_CONSTANT_EVALUATED(a))
    {
        #if defined(BOOST_INT256_HAS_INT128)

        // The shift takes an exactly typed right operand, as every shift of a builtin 128-bit
        // value in this library does; see the note in uint256_imp.hpp
        const detail::builtin_u128 product {static_cast<detail::builtin_u128>(a) * static_cast<detail::builtin_u128>(b)};
        hi = static_cast<std::uint64_t>(product >> static_cast<detail::builtin_u128>(64));
        return static_cast<std::uint64_t>(product);

        #elif defined(_M_AMD64) && !defined(__GNUC__) && !defined(__CUDA_ARCH__) && !defined(__SYCL_DEVICE_ONLY__)

        return _umul128(a, b, &hi);

        #elif defined(_M_ARM64) && !defined(__GNUC__) && !defined(__CUDA_ARCH__) && !defined(__SYCL_DEVICE_ONLY__)

        hi = __umulh(a, b);
        return a * b;

        #endif
    }

    #endif

    hi = umulh_generic(a, b);
    return a * b;
}

// The word functions are templates so that this header does not need the complete
// definition of uint256; T is deduced as uint256 at every call site

// Low 256 bits of a 256x256 product. Six full products contribute to words 1 and 2 and the
// four products that only reach word 3 are taken modulo 2^64. Each column is summed with
// add_carry because a column of six terms can carry more than one
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T mul_words(const T& lhs, const T& rhs) noexcept
{
    const auto a0 {lhs.words[0]};
    const auto a1 {lhs.words[1]};
    const auto a2 {lhs.words[2]};
    const auto a3 {lhs.words[3]};

    const auto b0 {rhs.words[0]};
    const auto b1 {rhs.words[1]};
    const auto b2 {rhs.words[2]};
    const auto b3 {rhs.words[3]};

    std::uint64_t p00_hi {};
    const auto p00_lo {umul(a0, b0, p00_hi)};

    std::uint64_t p01_hi {};
    const auto p01_lo {umul(a0, b1, p01_hi)};
    std::uint64_t p10_hi {};
    const auto p10_lo {umul(a1, b0, p10_hi)};

    std::uint64_t p02_hi {};
    const auto p02_lo {umul(a0, b2, p02_hi)};
    std::uint64_t p11_hi {};
    const auto p11_lo {umul(a1, b1, p11_hi)};
    std::uint64_t p20_hi {};
    const auto p20_lo {umul(a2, b0, p20_hi)};

    // Column 1
    std::uint64_t r1 {};
    auto carry_1 {add_carry(p00_hi, p01_lo, UINT64_C(0), r1)};
    carry_1 += add_carry(r1, p10_lo, UINT64_C(0), r1);

    // Column 2
    std::uint64_t r2 {};
    auto carry_2 {add_carry(p01_hi, p10_hi, UINT64_C(0), r2)};
    carry_2 += add_carry(r2, p02_lo, UINT64_C(0), r2);
    carry_2 += add_carry(r2, p11_lo, UINT64_C(0), r2);
    carry_2 += add_carry(r2, p20_lo, UINT64_C(0), r2);
    carry_2 += add_carry(r2, carry_1, UINT64_C(0), r2);

    // Column 3 is the top of the result, so everything above it is discarded
    const std::uint64_t r3 {p02_hi + p11_hi + p20_hi + carry_2 +
                            a0 * b3 + a1 * b2 + a2 * b1 + a3 * b0};

    return T{r3, r2, r1, p00_lo};
}

// Low 256 bits of a 256x64 product: three full products and one taken modulo 2^64
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T mul_words(const T& lhs, const std::uint64_t rhs) noexcept
{
    std::uint64_t p0_hi {};
    const auto p0_lo {umul(lhs.words[0], rhs, p0_hi)};
    std::uint64_t p1_hi {};
    const auto p1_lo {umul(lhs.words[1], rhs, p1_hi)};
    std::uint64_t p2_hi {};
    const auto p2_lo {umul(lhs.words[2], rhs, p2_hi)};

    std::uint64_t r1 {};
    const auto carry_1 {add_carry(p0_hi, p1_lo, UINT64_C(0), r1)};

    std::uint64_t r2 {};
    auto carry_2 {add_carry(p1_hi, p2_lo, UINT64_C(0), r2)};
    carry_2 += add_carry(r2, carry_1, UINT64_C(0), r2);

    const std::uint64_t r3 {p2_hi + carry_2 + lhs.words[3] * rhs};

    return T{r3, r2, r1, p0_lo};
}

// A half word divisor is the 64-bit case with a narrower operand
template <typename T>
BOOST_INT256_HOST_DEVICE constexpr T mul_words(const T& lhs, const std::uint32_t rhs) noexcept
{
    return mul_words(lhs, static_cast<std::uint64_t>(rhs));
}

} // namespace detail
} // namespace int256
} // namespace boost

#endif // BOOST_INT256_DETAIL_COMMON_MUL_HPP
