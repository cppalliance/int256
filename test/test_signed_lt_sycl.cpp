// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "sycl_test.hpp"

using boost::int256::int256;

int main()
{
    return int256_sycl_test::run_compare<int256>(
        [](int256 a, int256 b) { return a < b; });
}
