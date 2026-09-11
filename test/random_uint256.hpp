// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Random operand generation for the tests, including the CUDA and SYCL suites.
// A plain std::mt19937_64 is used rather than Boost.Random because the device
// translation units are single source: the device pass would still have to parse
// whatever the host pass includes, and the Boost.Random headers are host-only.

#ifndef BOOST_INT256_TEST_RANDOM_UINT256_HPP
#define BOOST_INT256_TEST_RANDOM_UINT256_HPP

#include <boost/int256/int256.hpp>

#include <cstdint>
#include <random>

// Produces a full-range random value of the 256-bit type T from a 64-bit engine.
// The four words are drawn into named locals first because the order in which
// function arguments are evaluated is unspecified, and a reproducible sequence
// from a fixed seed is what makes a failure re-runnable.
template <typename T>
T random_value(std::mt19937_64& rng)
{
    const auto w3 {static_cast<std::uint64_t>(rng())};
    const auto w2 {static_cast<std::uint64_t>(rng())};
    const auto w1 {static_cast<std::uint64_t>(rng())};
    const auto w0 {static_cast<std::uint64_t>(rng())};

    return T{w3, w2, w1, w0};
}

// Returns a divisor that is never zero so division stays well defined. Applied
// identically on the host and on the device.
template <typename T>
inline T safe_divisor(const T& b) noexcept
{
    return b == T{0} ? T{1} : b;
}

#endif // BOOST_INT256_TEST_RANDOM_UINT256_HPP
