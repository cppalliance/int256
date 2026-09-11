// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_BIT_HPP
#define BOOST_INT256_BIT_HPP

#include <boost/int256/int256.hpp>
#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/clz.hpp>
#include <boost/int256/detail/ctz.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <cstdint>

#endif

namespace boost {
namespace int256 {

// Counts the leading zero bits, returning 256 for a zero argument. There is no
// 256-bit clz builtin, so the first nonzero word from the top decides.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int countl_zero(const uint256& x) noexcept
{
    if (x.words[3] != UINT64_C(0))
    {
        return detail::countl_zero(x.words[3]);
    }

    if (x.words[2] != UINT64_C(0))
    {
        return 64 + detail::countl_zero(x.words[2]);
    }

    if (x.words[1] != UINT64_C(0))
    {
        return 128 + detail::countl_zero(x.words[1]);
    }

    return 192 + detail::countl_zero(x.words[0]);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int countl_one(const uint256& x) noexcept
{
    return countl_zero(~x);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int bit_width(const uint256& x) noexcept
{
    return x ? 256 - countl_zero(x) : 0;
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 bit_ceil(const uint256& x) noexcept
{
    // __builtin_stdc_bit_ceil not available, but this is equivalent
    return x <= 1U ? uint256{1U} : uint256{2U} << (255 - countl_zero(x - 1U));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 bit_floor(const uint256& x) noexcept
{
    // __builtin_stdc_bit_floor not available, but this is equivalent
    return x == 0U ? uint256{} : uint256{1U} << (255 - countl_zero(x));
}

// Counts the trailing zero bits, returning 256 for a zero argument
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int countr_zero(const uint256& x) noexcept
{
    if (x.words[0] != UINT64_C(0))
    {
        return detail::countr_zero(x.words[0]);
    }

    if (x.words[1] != UINT64_C(0))
    {
        return 64 + detail::countr_zero(x.words[1]);
    }

    if (x.words[2] != UINT64_C(0))
    {
        return 128 + detail::countr_zero(x.words[2]);
    }

    return 192 + detail::countr_zero(x.words[3]);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int countr_one(const uint256& x) noexcept
{
    return countr_zero(~x);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 rotl(const uint256& x, const int s) noexcept
{
    // __builtin_stdc_rotate_left not available
    constexpr auto mask {255U};
    return x << (static_cast<unsigned>(s) & mask) | x >> (static_cast<unsigned>(-s) & mask);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 rotr(const uint256& x, const int s) noexcept
{
    // __builtin_stdc_rotate_right not available
    constexpr auto mask {255U};
    return x >> (static_cast<unsigned>(s) & mask) | x << (static_cast<unsigned>(-s) & mask);
}

namespace impl {

BOOST_INT256_TEST_EXPORT BOOST_INT256_HOST_DEVICE constexpr int popcount_impl(std::uint64_t x) noexcept
{
    x = x - ((x >> 1U) & UINT64_C(0x5555555555555555));
    x = (x & UINT64_C(0x3333333333333333)) + ((x >> 2U) & UINT64_C(0x3333333333333333));
    x = (x + (x >> 4U)) & UINT64_C(0x0F0F0F0F0F0F0F0F);

    return static_cast<int>((x * UINT64_C(0x0101010101010101)) >> 56U);
}

// The exact-match overload above is selected for the 64-bit words
BOOST_INT256_TEST_EXPORT BOOST_INT256_HOST_DEVICE constexpr int popcount_impl(const uint256& x) noexcept
{
    return popcount_impl(x.words[0]) + popcount_impl(x.words[1]) +
           popcount_impl(x.words[2]) + popcount_impl(x.words[3]);
}

} // namespace impl

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int popcount(const uint256& x) noexcept
{
    #if BOOST_INT256_HAS_BUILTIN(__builtin_popcountll) && !(defined(__CUDACC__) && defined(BOOST_INT256_ENABLE_CUDA))

    return __builtin_popcountll(x.words[0]) + __builtin_popcountll(x.words[1]) +
           __builtin_popcountll(x.words[2]) + __builtin_popcountll(x.words[3]);

    #elif defined(_M_AMD64) && !defined(__GNUC__) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (BOOST_INT256_IS_CONSTANT_EVALUATED(x))
    {
        return impl::popcount_impl(x); // LCOV_EXCL_LINE
    }
    else
    {
        #ifdef __AVX__

        return static_cast<int>(_mm_popcnt_u64(x.words[0]) + _mm_popcnt_u64(x.words[1]) +
                                _mm_popcnt_u64(x.words[2]) + _mm_popcnt_u64(x.words[3]));

        #else

        return static_cast<int>(__popcnt64(x.words[0]) + __popcnt64(x.words[1]) +
                                __popcnt64(x.words[2]) + __popcnt64(x.words[3]));

        #endif
    }

    #elif defined(_M_IX86) && !defined(__GNUC__) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (BOOST_INT256_IS_CONSTANT_EVALUATED(x))
    {
        return impl::popcount_impl(x); // LCOV_EXCL_LINE
    }
    else
    {
        #ifdef __AVX__

        return static_cast<int>(
               _mm_popcnt_u32(static_cast<unsigned>(x.words[0])) +
               _mm_popcnt_u32(static_cast<unsigned>(x.words[0] >> 32U)) +
               _mm_popcnt_u32(static_cast<unsigned>(x.words[1])) +
               _mm_popcnt_u32(static_cast<unsigned>(x.words[1] >> 32U)) +
               _mm_popcnt_u32(static_cast<unsigned>(x.words[2])) +
               _mm_popcnt_u32(static_cast<unsigned>(x.words[2] >> 32U)) +
               _mm_popcnt_u32(static_cast<unsigned>(x.words[3])) +
               _mm_popcnt_u32(static_cast<unsigned>(x.words[3] >> 32U)));

        #else

        return static_cast<int>(
               __popcnt(static_cast<unsigned>(x.words[0])) +
               __popcnt(static_cast<unsigned>(x.words[0] >> 32U)) +
               __popcnt(static_cast<unsigned>(x.words[1])) +
               __popcnt(static_cast<unsigned>(x.words[1] >> 32U)) +
               __popcnt(static_cast<unsigned>(x.words[2])) +
               __popcnt(static_cast<unsigned>(x.words[2] >> 32U)) +
               __popcnt(static_cast<unsigned>(x.words[3])) +
               __popcnt(static_cast<unsigned>(x.words[3] >> 32U)));

        #endif
    }

    #else

    return impl::popcount_impl(x);

    #endif
}

namespace impl {

BOOST_INT256_TEST_EXPORT BOOST_INT256_HOST_DEVICE constexpr std::uint64_t byteswap_impl(const std::uint64_t x) noexcept
{
    const auto step32 {x << 32U | x >> 32U};
    const auto step16 {(step32 & UINT64_C(0x0000FFFF0000FFFF)) << 16U | (step32 & UINT64_C(0xFFFF0000FFFF0000)) >> 16U};
    return (step16 & UINT64_C(0x00FF00FF00FF00FF)) << 8U | (step16 & UINT64_C(0xFF00FF00FF00FF00)) >> 8U;
}

} // namespace impl

// Reverses the byte order of the whole 256-bit value. The constructor takes the most
// significant word first, so the words appear here in ascending order.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 byteswap(const uint256& x) noexcept
{
    #if BOOST_INT256_HAS_BUILTIN(__builtin_bswap64) && !(defined(__CUDACC__) && defined(BOOST_INT256_ENABLE_CUDA))

    return uint256{__builtin_bswap64(x.words[0]), __builtin_bswap64(x.words[1]),
                   __builtin_bswap64(x.words[2]), __builtin_bswap64(x.words[3])};

    #elif defined(_MSC_VER) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION)

    if (BOOST_INT256_IS_CONSTANT_EVALUATED(x))
    {
        // LCOV_EXCL_START
        return uint256{impl::byteswap_impl(x.words[0]), impl::byteswap_impl(x.words[1]),
                       impl::byteswap_impl(x.words[2]), impl::byteswap_impl(x.words[3])};
        // LCOV_EXCL_STOP
    }
    else
    {
        return uint256{_byteswap_uint64(x.words[0]), _byteswap_uint64(x.words[1]),
                       _byteswap_uint64(x.words[2]), _byteswap_uint64(x.words[3])};
    }

    #else

    return uint256{impl::byteswap_impl(x.words[0]), impl::byteswap_impl(x.words[1]),
                   impl::byteswap_impl(x.words[2]), impl::byteswap_impl(x.words[3])};

    #endif
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr bool has_single_bit(const uint256& x) noexcept
{
    return popcount(x) == 1;
}

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_BIT_HPP
