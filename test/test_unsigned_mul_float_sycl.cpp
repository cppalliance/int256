// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// A mixed operation with a builtin floating point type on the device. long double does
// not exist on spir64, so only float and double participate there

#include "sycl_test.hpp"

using boost::int256::uint256;

int main()
{
    return int256_sycl_test::run<uint256, double>(
        [](uint256 a, uint256, int) { return a * 1.5; });
}
