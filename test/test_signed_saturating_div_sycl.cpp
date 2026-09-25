// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "sycl_test.hpp"

using boost::int256::int256;

int main()
{
    // Force the one input saturating_div actually saturates on (MIN / -1, which the
    // plain operator wraps to MIN but saturating_div clamps to MAX) onto the device
    // path every run, not only when a random draw happens to land on it. Index 0's
    // divisor shift is (64 * (0 % 4)) == 0, so no pre-shift compensation is needed.
    const int256_sycl_test::directed_pair<int256> overrides[] {
        {(std::numeric_limits<int256>::min)(), int256{-1}},
    };

    return int256_sycl_test::run<int256, int256>(
        [](int256 a, int256 b, int i)
        {
            // Cycle the divisor through four, three, two and one significant words
            return boost::int256::saturating_div(a, int256_sycl_test::safe_divisor(a, b >> (64 * (i % 4))));
        },
        overrides, 1);
}
