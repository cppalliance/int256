// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "sycl_test.hpp"

using boost::int256::uint256;

int main()
{
    return int256_sycl_test::run<uint256, uint256>(
        [](uint256 a, uint256 b, int) { return boost::int256::gcd(a, b); });
}
