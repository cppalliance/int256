// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// This header injects additional functionality into the Boost.Charconv namespace
// If this library is accepted into boost this functionality could be moved there

#ifndef BOOST_INT256_CHARCONV_HPP
#define BOOST_INT256_CHARCONV_HPP

#if __has_include(<boost/charconv.hpp>)

// Define for the user automatically,
// otherwise we'll have an ever-increasing number of these required as we go down the dependency chain
#if defined(BOOST_INT256_ENABLE_CUDA) && !defined(BOOST_CHARCONV_ENABLE_CUDA)
#  define BOOST_CHARCONV_ENABLE_CUDA
#endif

#if defined(BOOST_INT256_ENABLE_SYCL) && !defined(BOOST_CHARCONV_ENABLE_SYCL)
#  define BOOST_CHARCONV_ENABLE_SYCL
#endif

#include <boost/int256/int256.hpp>
#include <boost/int256/literals.hpp>
#include <boost/int256/detail/clz.hpp>
#include <boost/int256/detail/config.hpp>
#include <boost/charconv.hpp>
#include <boost/core/detail/string_view.hpp>

namespace boost {
namespace charconv {

namespace detail {

template <>
struct is_signed<int256::uint256> { static constexpr bool value = false; };

template <>
struct is_signed<int256::int256> { static constexpr bool value = true; };

template <>
struct make_unsigned<int256::uint256> { using type = int256::uint256; };

template <>
struct make_unsigned<int256::int256> { using type = int256::uint256; };

template <>
struct make_signed<int256::uint256> { using type = int256::int256; };

template <>
struct make_signed<int256::int256> { using type = int256::int256; };

#if defined(BOOST_INT256_HAS_GPU_SUPPORT)

template <>
BOOST_INT256_HOST_DEVICE constexpr int256::uint256 get_max_value<int256::uint256>()
{
    return (std::numeric_limits<int256::uint256>::max)();
}

template <>
BOOST_INT256_HOST_DEVICE constexpr int256::int256 get_max_value<int256::int256>()
{
    return (std::numeric_limits<int256::int256>::max)();
}

#endif

#if !defined(BOOST_INT256_HAS_GPU_SUPPORT)

BOOST_INT256_INLINE_CONSTEXPR int256::uint256 uint256_pow10[78] =
{
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x1)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xa)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x64)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x3e8)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x2710)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x186a0)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xf4240)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x989680)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x5f5e100)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x3b9aca00)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x2540be400)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x174876e800)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xe8d4a51000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x9184e72a000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x5af3107a4000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x38d7ea4c68000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x2386f26fc10000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x16345785d8a0000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xde0b6b3a7640000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x8ac7230489e80000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x5), UINT64_C(0x6bc75e2d63100000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x36), UINT64_C(0x35c9adc5dea00000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x21e), UINT64_C(0x19e0c9bab2400000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x152d), UINT64_C(0x2c7e14af6800000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xd3c2), UINT64_C(0x1bcecceda1000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x84595), UINT64_C(0x161401484a000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x52b7d2), UINT64_C(0xdcc80cd2e4000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x33b2e3c), UINT64_C(0x9fd0803ce8000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x204fce5e), UINT64_C(0x3e25026110000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x1431e0fae), UINT64_C(0x6d7217caa0000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xc9f2c9cd0), UINT64_C(0x4674edea40000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x7e37be2022), UINT64_C(0xc0914b2680000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x4ee2d6d415b), UINT64_C(0x85acef8100000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x314dc6448d93), UINT64_C(0x38c15b0a00000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x1ed09bead87c0), UINT64_C(0x378d8e6400000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x13426172c74d82), UINT64_C(0x2b878fe800000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xc097ce7bc90715), UINT64_C(0xb34b9f1000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x785ee10d5da46d9), UINT64_C(0xf436a000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x4b3b4ca85a86c47a), UINT64_C(0x98a224000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x2), UINT64_C(0xf050fe938943acc4), UINT64_C(0x5f65568000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x1d), UINT64_C(0x6329f1c35ca4bfab), UINT64_C(0xb9f5610000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x125), UINT64_C(0xdfa371a19e6f7cb5), UINT64_C(0x4395ca0000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0xb7a), UINT64_C(0xbc627050305adf14), UINT64_C(0xa3d9e40000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x72cb), UINT64_C(0x5bd86321e38cb6ce), UINT64_C(0x6682e80000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x47bf1), UINT64_C(0x9673df52e37f2410), UINT64_C(0x11d100000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x2cd76f), UINT64_C(0xe086b93ce2f768a0), UINT64_C(0xb22a00000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x1c06a5e), UINT64_C(0xc5433c60ddaa1640), UINT64_C(0x6f5a400000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x118427b3), UINT64_C(0xb4a05bc8a8a4de84), UINT64_C(0x5986800000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0xaf298d05), UINT64_C(0xe4395d69670b12b), UINT64_C(0x7f41000000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x6d79f8232), UINT64_C(0x8ea3da61e066ebb2), UINT64_C(0xf88a000000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x446c3b15f9), UINT64_C(0x926687d2c40534fd), UINT64_C(0xb564000000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x2ac3a4edbbf), UINT64_C(0xb8014e3ba83411e9), UINT64_C(0x15e8000000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x1aba4714957d), UINT64_C(0x300d0e549208b31a), UINT64_C(0xdb10000000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x10b46c6cdd6e3), UINT64_C(0xe0828f4db456ff0c), UINT64_C(0x8ea0000000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0xa70c3c40a64e6), UINT64_C(0xc51999090b65f67d), UINT64_C(0x9240000000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x6867a5a867f103), UINT64_C(0xb2fffa5a71fba0e7), UINT64_C(0xb680000000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x4140c78940f6a24), UINT64_C(0xfdffc78873d4490d), UINT64_C(0x2100000000000000)},
    int256::uint256{UINT64_C(0x0), UINT64_C(0x28c87cb5c89a2571), UINT64_C(0xebfdcb54864ada83), UINT64_C(0x4a00000000000000)},
    int256::uint256{UINT64_C(0x1), UINT64_C(0x97d4df19d6057673), UINT64_C(0x37e9f14d3eec8920), UINT64_C(0xe400000000000000)},
    int256::uint256{UINT64_C(0xf), UINT64_C(0xee50b7025c36a080), UINT64_C(0x2f236d04753d5b48), UINT64_C(0xe800000000000000)},
    int256::uint256{UINT64_C(0x9f), UINT64_C(0x4f2726179a224501), UINT64_C(0xd762422c946590d9), UINT64_C(0x1000000000000000)},
    int256::uint256{UINT64_C(0x639), UINT64_C(0x17877cec0556b212), UINT64_C(0x69d695bdcbf7a87a), UINT64_C(0xa000000000000000)},
    int256::uint256{UINT64_C(0x3e3a), UINT64_C(0xeb4ae1383562f4b8), UINT64_C(0x2261d969f7ac94ca), UINT64_C(0x4000000000000000)},
    int256::uint256{UINT64_C(0x26e4d), UINT64_C(0x30eccc3215dd8f31), UINT64_C(0x57d27e23acbdcfe6), UINT64_C(0x8000000000000000)},
    int256::uint256{UINT64_C(0x184f03), UINT64_C(0xe93ff9f4daa797ed), UINT64_C(0x6e38ed64bf6a1f01), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0xf31627), UINT64_C(0x1c7fc3908a8bef46), UINT64_C(0x4e3945ef7a25360a), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0x97edd87), UINT64_C(0x1cfda3a5697758bf), UINT64_C(0xe3cbb5ac5741c64), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0x5ef4a747), UINT64_C(0x21e864761ea97776), UINT64_C(0x8e5f518bb6891be8), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0x3b58e88c7), UINT64_C(0x5313ec9d329eaaa1), UINT64_C(0x8fb92f75215b1710), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0x25179157c9), UINT64_C(0x3ec73e23fa32aa4f), UINT64_C(0x9d3bda934d8ee6a0), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0x172ebad6ddc), UINT64_C(0x73c86d67c5faa71c), UINT64_C(0x245689c107950240), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0xe7d34c64a9c), UINT64_C(0x85d4460dbbca8719), UINT64_C(0x6b61618a4bd21680), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0x90e40fbeea1d), UINT64_C(0x3a4abc8955e946fe), UINT64_C(0x31cdcf66f634e100), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0x5a8e89d752524), UINT64_C(0x46eb5d5d5b1cc5ed), UINT64_C(0xf20a1a059e10ca00), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0x3899162693736a), UINT64_C(0xc531a5a58f1fbb4b), UINT64_C(0x746504382ca7e400), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0x235fadd81c2822b), UINT64_C(0xb3f07877973d50f2), UINT64_C(0x8bf22a31be8ee800), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0x161bcca7119915b5), UINT64_C(0x764b4abe8652979), UINT64_C(0x7775a5f171951000), UINT64_C(0x0)},
    int256::uint256{UINT64_C(0xdd15fe86affad912), UINT64_C(0x49ef0eb713f39ebe), UINT64_C(0xaa987b6e6fd2a000), UINT64_C(0x0)}
};

#endif // Namespace scope tables are unavailable on the device

// Number of decimal digits of x, from a log10 estimate of the most significant bit
// position corrected against the table of powers of ten
BOOST_INT256_HOST_DEVICE constexpr int num_digits(const int256::uint256& x) noexcept
{
    #if defined(BOOST_INT256_HAS_GPU_SUPPORT)

    constexpr int256::uint256 uint256_pow10[78] =
    {
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x1)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xa)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x64)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x3e8)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x2710)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x186a0)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xf4240)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x989680)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x5f5e100)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x3b9aca00)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x2540be400)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x174876e800)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xe8d4a51000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x9184e72a000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x5af3107a4000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x38d7ea4c68000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x2386f26fc10000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x16345785d8a0000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xde0b6b3a7640000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x8ac7230489e80000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x5), UINT64_C(0x6bc75e2d63100000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x36), UINT64_C(0x35c9adc5dea00000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x21e), UINT64_C(0x19e0c9bab2400000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x152d), UINT64_C(0x2c7e14af6800000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xd3c2), UINT64_C(0x1bcecceda1000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x84595), UINT64_C(0x161401484a000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x52b7d2), UINT64_C(0xdcc80cd2e4000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x33b2e3c), UINT64_C(0x9fd0803ce8000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x204fce5e), UINT64_C(0x3e25026110000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x1431e0fae), UINT64_C(0x6d7217caa0000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xc9f2c9cd0), UINT64_C(0x4674edea40000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x7e37be2022), UINT64_C(0xc0914b2680000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x4ee2d6d415b), UINT64_C(0x85acef8100000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x314dc6448d93), UINT64_C(0x38c15b0a00000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x1ed09bead87c0), UINT64_C(0x378d8e6400000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x13426172c74d82), UINT64_C(0x2b878fe800000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0xc097ce7bc90715), UINT64_C(0xb34b9f1000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x785ee10d5da46d9), UINT64_C(0xf436a000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x0), UINT64_C(0x4b3b4ca85a86c47a), UINT64_C(0x98a224000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x2), UINT64_C(0xf050fe938943acc4), UINT64_C(0x5f65568000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x1d), UINT64_C(0x6329f1c35ca4bfab), UINT64_C(0xb9f5610000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x125), UINT64_C(0xdfa371a19e6f7cb5), UINT64_C(0x4395ca0000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0xb7a), UINT64_C(0xbc627050305adf14), UINT64_C(0xa3d9e40000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x72cb), UINT64_C(0x5bd86321e38cb6ce), UINT64_C(0x6682e80000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x47bf1), UINT64_C(0x9673df52e37f2410), UINT64_C(0x11d100000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x2cd76f), UINT64_C(0xe086b93ce2f768a0), UINT64_C(0xb22a00000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x1c06a5e), UINT64_C(0xc5433c60ddaa1640), UINT64_C(0x6f5a400000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x118427b3), UINT64_C(0xb4a05bc8a8a4de84), UINT64_C(0x5986800000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0xaf298d05), UINT64_C(0xe4395d69670b12b), UINT64_C(0x7f41000000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x6d79f8232), UINT64_C(0x8ea3da61e066ebb2), UINT64_C(0xf88a000000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x446c3b15f9), UINT64_C(0x926687d2c40534fd), UINT64_C(0xb564000000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x2ac3a4edbbf), UINT64_C(0xb8014e3ba83411e9), UINT64_C(0x15e8000000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x1aba4714957d), UINT64_C(0x300d0e549208b31a), UINT64_C(0xdb10000000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x10b46c6cdd6e3), UINT64_C(0xe0828f4db456ff0c), UINT64_C(0x8ea0000000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0xa70c3c40a64e6), UINT64_C(0xc51999090b65f67d), UINT64_C(0x9240000000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x6867a5a867f103), UINT64_C(0xb2fffa5a71fba0e7), UINT64_C(0xb680000000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x4140c78940f6a24), UINT64_C(0xfdffc78873d4490d), UINT64_C(0x2100000000000000)},
        int256::uint256{UINT64_C(0x0), UINT64_C(0x28c87cb5c89a2571), UINT64_C(0xebfdcb54864ada83), UINT64_C(0x4a00000000000000)},
        int256::uint256{UINT64_C(0x1), UINT64_C(0x97d4df19d6057673), UINT64_C(0x37e9f14d3eec8920), UINT64_C(0xe400000000000000)},
        int256::uint256{UINT64_C(0xf), UINT64_C(0xee50b7025c36a080), UINT64_C(0x2f236d04753d5b48), UINT64_C(0xe800000000000000)},
        int256::uint256{UINT64_C(0x9f), UINT64_C(0x4f2726179a224501), UINT64_C(0xd762422c946590d9), UINT64_C(0x1000000000000000)},
        int256::uint256{UINT64_C(0x639), UINT64_C(0x17877cec0556b212), UINT64_C(0x69d695bdcbf7a87a), UINT64_C(0xa000000000000000)},
        int256::uint256{UINT64_C(0x3e3a), UINT64_C(0xeb4ae1383562f4b8), UINT64_C(0x2261d969f7ac94ca), UINT64_C(0x4000000000000000)},
        int256::uint256{UINT64_C(0x26e4d), UINT64_C(0x30eccc3215dd8f31), UINT64_C(0x57d27e23acbdcfe6), UINT64_C(0x8000000000000000)},
        int256::uint256{UINT64_C(0x184f03), UINT64_C(0xe93ff9f4daa797ed), UINT64_C(0x6e38ed64bf6a1f01), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0xf31627), UINT64_C(0x1c7fc3908a8bef46), UINT64_C(0x4e3945ef7a25360a), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0x97edd87), UINT64_C(0x1cfda3a5697758bf), UINT64_C(0xe3cbb5ac5741c64), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0x5ef4a747), UINT64_C(0x21e864761ea97776), UINT64_C(0x8e5f518bb6891be8), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0x3b58e88c7), UINT64_C(0x5313ec9d329eaaa1), UINT64_C(0x8fb92f75215b1710), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0x25179157c9), UINT64_C(0x3ec73e23fa32aa4f), UINT64_C(0x9d3bda934d8ee6a0), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0x172ebad6ddc), UINT64_C(0x73c86d67c5faa71c), UINT64_C(0x245689c107950240), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0xe7d34c64a9c), UINT64_C(0x85d4460dbbca8719), UINT64_C(0x6b61618a4bd21680), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0x90e40fbeea1d), UINT64_C(0x3a4abc8955e946fe), UINT64_C(0x31cdcf66f634e100), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0x5a8e89d752524), UINT64_C(0x46eb5d5d5b1cc5ed), UINT64_C(0xf20a1a059e10ca00), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0x3899162693736a), UINT64_C(0xc531a5a58f1fbb4b), UINT64_C(0x746504382ca7e400), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0x235fadd81c2822b), UINT64_C(0xb3f07877973d50f2), UINT64_C(0x8bf22a31be8ee800), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0x161bcca7119915b5), UINT64_C(0x764b4abe8652979), UINT64_C(0x7775a5f171951000), UINT64_C(0x0)},
        int256::uint256{UINT64_C(0xdd15fe86affad912), UINT64_C(0x49ef0eb713f39ebe), UINT64_C(0xaa987b6e6fd2a000), UINT64_C(0x0)}
    };

    #endif // Namespace scope tables are unavailable on the device

    if ((x.words[1] | x.words[2] | x.words[3]) == UINT64_C(0))
    {
        return num_digits(x.words[0]);
    }

    // Use the most significant bit position to approximate log10
    // log10(x) ~= log2(x) / log2(10) ~= log2(x) / 3.32

    int msb {};
    if (x.words[3] != UINT64_C(0))
    {
        msb = 192 + (63 - int256::detail::countl_zero(x.words[3]));
    }
    else if (x.words[2] != UINT64_C(0))
    {
        msb = 128 + (63 - int256::detail::countl_zero(x.words[2]));
    }
    else
    {
        msb = 64 + (63 - int256::detail::countl_zero(x.words[1]));
    }

    // Approximate log10
    const auto estimated_digits {(msb * 1000) / 3322 + 1};

    if (estimated_digits < 78 && x >= uint256_pow10[static_cast<std::size_t>(estimated_digits)])
    {
        return estimated_digits + 1;
    }

    // Estimated digits can't be less than 20 digits (65-bits minimum)
    if (x < uint256_pow10[static_cast<std::size_t>(estimated_digits - 1)])
    {
        return estimated_digits - 1;
    }

    return estimated_digits;
}

} // namespace detail

// Every base, base 10 included, goes through the base-parameterized four argument
// to_chars_integer_impl, whose scratch buffer is sizeof(Unsigned_Integer) * CHAR_BIT
// and so holds all 256 binary digits. It must never be routed through to_chars_int or
// to_chars128, whose specialized decimal paths use a char buffer[5][10] sized for a
// 128-bit value and cannot hold the 78 decimal digits of a 256-bit maximum.
BOOST_INT256_HOST_DEVICE BOOST_CHARCONV_CONSTEXPR to_chars_result to_chars(char* first, char* last, const int256::uint256& value, const int base = 10) noexcept
{
    // The generic Boost.Charconv base path guards with (first <= last) and then
    // writes '0' unconditionally for a zero value, so an empty range overruns by
    // one byte. Reject it here.
    if (first >= last)
    {
        return {last, std::errc::value_too_large};
    }

    return detail::to_chars_integer_impl<int256::uint256, int256::uint256>(first, last, value, base);
}

BOOST_INT256_HOST_DEVICE BOOST_CHARCONV_GCC5_CONSTEXPR from_chars_result from_chars(const char* first, const char* last, int256::uint256& value, const int base = 10) noexcept
{
    return detail::from_chars_integer_impl<int256::uint256, int256::uint256>(first, last, value, base);
}

BOOST_CHARCONV_GCC5_CONSTEXPR from_chars_result from_chars(core::string_view sv, int256::uint256& value, const int base = 10) noexcept
{
    return detail::from_chars_integer_impl<int256::uint256, int256::uint256>(sv.data(), sv.data() + sv.size(), value, base);
}

// The signed twins. Same note as above about to_chars_integer_impl's buffer size: this
// must never be routed through a 128-bit-specialized decimal fast path
BOOST_INT256_HOST_DEVICE BOOST_CHARCONV_CONSTEXPR to_chars_result to_chars(char* first, char* last, const int256::int256& value, const int base = 10) noexcept
{
    if (first >= last)
    {
        return {last, std::errc::value_too_large};
    }

    return detail::to_chars_integer_impl<int256::int256, int256::uint256>(first, last, value, base);
}

BOOST_INT256_HOST_DEVICE BOOST_CHARCONV_GCC5_CONSTEXPR from_chars_result from_chars(const char* first, const char* last, int256::int256& value, const int base = 10) noexcept
{
    return detail::from_chars_integer_impl<int256::int256, int256::uint256>(first, last, value, base);
}

BOOST_CHARCONV_GCC5_CONSTEXPR from_chars_result from_chars(core::string_view sv, int256::int256& value, const int base = 10) noexcept
{
    return detail::from_chars_integer_impl<int256::int256, int256::uint256>(sv.data(), sv.data() + sv.size(), value, base);
}

} // namespace charconv
} // namespace boost

#else

#error "This header requires Boost.Charconv to be present"

#endif // __has_include(<boost/charconv.hpp>)

#endif // BOOST_INT256_CHARCONV_HPP
