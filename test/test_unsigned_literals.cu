// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The user-defined literal operators on the device.
// Adapted from the Boost.Int128 CUDA test suite.

#include <iostream>
#include <vector>
#include <random>
#include <limits>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <boost/int256.hpp>
#include <boost/int256/literals.hpp>
#include "random_uint256.hpp"
#include "cuda_managed_ptr.hpp"
#include "stopwatch.hpp"

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>

using test_type = boost::int256::uint256;

using namespace boost::int256::literals;

// Each thread evaluates one literal case on the device. The groups cover the raw literal
// operator with values too large for unsigned long long, the cooked string literal
// operator, and the raw literal operator with small values, in both spellings.
constexpr int num_tests {24};

__global__ void cuda_test(test_type* out)
{
    const int i {static_cast<int>(threadIdx.x)};

    switch (i)
    {
        // Raw literal operator, with values too large for unsigned long long
        case 0: out[0] = 18446744073709551616_u256; break;
        case 1: out[1] = 340282366920938463463374607431768211456_u256; break;
        case 2: out[2] = 6277101735386680763835789423207666416102355444464034512896_u256; break;
        case 3: out[3] = 115792089237316195423570985008687907853269984665640564039457584007913129639935_u256; break;
        case 4: out[4] = 18446744073709551616_U256; break;
        case 5: out[5] = 340282366920938463463374607431768211456_U256; break;
        case 6: out[6] = 6277101735386680763835789423207666416102355444464034512896_U256; break;
        case 7: out[7] = 115792089237316195423570985008687907853269984665640564039457584007913129639935_U256; break;

        // Cooked string literal operator
        case 8: out[8] = "0"_u256; break;
        case 9: out[9] = "1"_u256; break;
        case 10: out[10] = "340282366920938463463374607431768211455"_u256; break;
        case 11: out[11] = "115792089237316195423570985008687907853269984665640564039457584007913129639935"_u256; break;
        case 12: out[12] = "0"_U256; break;
        case 13: out[13] = "1"_U256; break;
        case 14: out[14] = "340282366920938463463374607431768211455"_U256; break;
        case 15: out[15] = "115792089237316195423570985008687907853269984665640564039457584007913129639935"_U256; break;

        // Raw literal operator, with small values
        case 16: out[16] = 0_u256; break;
        case 17: out[17] = 1_u256; break;
        case 18: out[18] = 42_u256; break;
        case 19: out[19] = 18446744073709551615_u256; break;
        case 20: out[20] = 0_U256; break;
        case 21: out[21] = 1_U256; break;
        case 22: out[22] = 42_U256; break;
        case 23: out[23] = 18446744073709551615_U256; break;

        default: break;
    }
}

int main()
{
    std::cout << "[Unsigned literal tests: " << num_tests << " cases]" << std::endl;

    cuda_managed_ptr<test_type> output(num_tests);

    watch w;

    cuda_test<<<1, num_tests>>>(output.get());
    cudaDeviceSynchronize();

    std::cout << "CUDA kernel done in: " << w.elapsed() << "s" << std::endl;

    const cudaError_t err {cudaGetLastError()};
    if (err != cudaSuccess)
    {
        std::cerr << "Failed to launch kernel (error code " << cudaGetErrorString(err) << ")!" << std::endl;
        return EXIT_FAILURE;
    }

    // The same literals, evaluated on the host
    test_type expected[num_tests];
    // Raw literal operator, with values too large for unsigned long long
    expected[0] = 18446744073709551616_u256;
    expected[1] = 340282366920938463463374607431768211456_u256;
    expected[2] = 6277101735386680763835789423207666416102355444464034512896_u256;
    expected[3] = 115792089237316195423570985008687907853269984665640564039457584007913129639935_u256;
    expected[4] = 18446744073709551616_U256;
    expected[5] = 340282366920938463463374607431768211456_U256;
    expected[6] = 6277101735386680763835789423207666416102355444464034512896_U256;
    expected[7] = 115792089237316195423570985008687907853269984665640564039457584007913129639935_U256;

    // Cooked string literal operator
    expected[8] = "0"_u256;
    expected[9] = "1"_u256;
    expected[10] = "340282366920938463463374607431768211455"_u256;
    expected[11] = "115792089237316195423570985008687907853269984665640564039457584007913129639935"_u256;
    expected[12] = "0"_U256;
    expected[13] = "1"_U256;
    expected[14] = "340282366920938463463374607431768211455"_U256;
    expected[15] = "115792089237316195423570985008687907853269984665640564039457584007913129639935"_U256;

    // Raw literal operator, with small values
    expected[16] = 0_u256;
    expected[17] = 1_u256;
    expected[18] = 42_u256;
    expected[19] = 18446744073709551615_u256;
    expected[20] = 0_U256;
    expected[21] = 1_U256;
    expected[22] = 42_U256;
    expected[23] = 18446744073709551615_U256;

    for (int i {0}; i < num_tests; ++i)
    {
        if (output[i] != expected[i])
        {
            std::cerr << "Result verification failed at test case " << i << "!" << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "Test PASSED" << std::endl;
    std::cout << "Done" << std::endl;

    return 0;
}
