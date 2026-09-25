// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "sycl_test.hpp"

using boost::int256::int256;

int main()
{
    int result {0};
    result |= int256_sycl_test::run_to_chars<int256>(2);
    result |= int256_sycl_test::run_to_chars<int256>(8);
    result |= int256_sycl_test::run_to_chars<int256>(16);
    result |= int256_sycl_test::run_to_chars<int256>(36);
    return result;
}
