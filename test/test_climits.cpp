// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstdint>
#include <limits>

using boost::int256::uint256;

int main()
{
    static_assert(BOOST_INT256_UINT256_MAX == (std::numeric_limits<uint256>::max)(), "Should be equal");
    static_assert(BOOST_INT256_UINT256_MAX == uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX),
                  "Every word is set");
    static_assert(BOOST_INT256_UINT256_MAX + uint256{1U} == uint256{}, "One past the maximum wraps");

    // The macro is usable where a constant is required.
    constexpr uint256 maximum {BOOST_INT256_UINT256_MAX};
    BOOST_TEST_EQ(maximum, (std::numeric_limits<uint256>::max)());

    return boost::report_errors();
}
