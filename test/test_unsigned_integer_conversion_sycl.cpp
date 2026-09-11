// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "sycl_test.hpp"

using boost::int256::uint256;

int main()
{
    int result {0};
    result |= int256_sycl_test::run<uint256, std::uint64_t>(
        [](uint256 a, uint256, int) { return static_cast<std::uint64_t>(a); });
    result |= int256_sycl_test::run<uint256, uint256>(
        [](uint256 a, uint256, int) { return uint256{static_cast<std::uint64_t>(a)}; });
    return result;
}
