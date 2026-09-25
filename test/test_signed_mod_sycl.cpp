// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "sycl_test.hpp"

using boost::int256::int256;

int main()
{
    // Force the one defined signed overflow (MIN % -1 wraps to 0) and its
    // non-overflowing neighbor onto the device path every run, not only when a
    // random draw happens to land on them. The lambda below shifts the divisor
    // right by (64 * (i % 4)) to cycle its significant width, so the override at
    // index 1 is pre-shifted left by 64 (its i % 4 == 1) to still divide by exactly
    // 1 once that shift is applied; index 0 needs no adjustment (i % 4 == 0).
    const int256_sycl_test::directed_pair<int256> overrides[] {
        {(std::numeric_limits<int256>::min)(), int256{-1}},
        {(std::numeric_limits<int256>::min)(), int256{1} << 64},
    };

    return int256_sycl_test::run<int256, int256>(
        [](int256 a, int256 b, int i)
        {
            // Cycle the divisor through four, three, two and one significant words
            return a % int256_sycl_test::safe_divisor(a, b >> (64 * (i % 4)));
        },
        overrides, 2);
}
