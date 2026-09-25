// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstdint>
#include <limits>

using boost::int256::uint256;
using boost::int256::int256;

int main()
{
    static_assert(BOOST_INT256_UINT256_MAX == (std::numeric_limits<uint256>::max)(), "Should be equal");
    static_assert(BOOST_INT256_UINT256_MAX == uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX),
                  "Every word is set");
    static_assert(BOOST_INT256_UINT256_MAX + uint256{1U} == uint256{}, "One past the maximum wraps");

    // The macro is usable where a constant is required.
    constexpr uint256 maximum {BOOST_INT256_UINT256_MAX};
    BOOST_TEST_EQ(maximum, (std::numeric_limits<uint256>::max)());

    static_assert(BOOST_INT256_INT256_MAX == (std::numeric_limits<int256>::max)(), "Should be equal");
    static_assert(BOOST_INT256_INT256_MIN == (std::numeric_limits<int256>::min)(), "Should be equal");
    static_assert(BOOST_INT256_INT256_MAX == int256(INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX),
                  "The top word is the largest positive int64_t, the rest all ones");
    static_assert(BOOST_INT256_INT256_MIN == int256(INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)),
                  "The top word is INT64_MIN, the rest zero");
    static_assert(BOOST_INT256_INT256_MAX + int256{1} == BOOST_INT256_INT256_MIN, "One past the maximum wraps to MIN");

    constexpr int256 signed_maximum {BOOST_INT256_INT256_MAX};
    constexpr int256 signed_minimum {BOOST_INT256_INT256_MIN};
    BOOST_TEST_EQ(signed_maximum, (std::numeric_limits<int256>::max)());
    BOOST_TEST_EQ(signed_minimum, (std::numeric_limits<int256>::min)());

    return boost::report_errors();
}
