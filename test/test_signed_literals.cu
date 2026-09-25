// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The user-defined literal operators on the device.
// Adapted from the Boost.Int128 CUDA test suite.

#include <iostream>
#include <boost/int256.hpp>
#include <boost/int256/literals.hpp>
#include "cuda_managed_ptr.hpp"
#include "stopwatch.hpp"

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>

using boost::int256::int256;
using namespace boost::int256::literals;

// Each thread evaluates one signed literal case on the device: the raw literal
// operator with small values, the cooked string operator with decimal MIN/MAX, and
// the prefixed hex form whose magnitude parses as uint256 (MIN via -0x8 followed
// by 63 zero hex digits).
constexpr int num_tests {18};

__global__ void cuda_test(int256* out)
{
    const int i {static_cast<int>(threadIdx.x)};

    switch (i)
    {
        case 0: out[0] = 0_i256; break;
        case 1: out[1] = 1_i256; break;
        case 2: out[2] = -1_i256; break;
        case 3: out[3] = 42_i256; break;
        case 4: out[4] = 0_I256; break;
        case 5: out[5] = 1_I256; break;
        case 6: out[6] = -42_I256; break;
        case 7: out[7] = 9223372036854775807_I256; break;
        case 8: out[8] = "0"_i256; break;
        case 9: out[9] = "-1"_i256; break;
        case 10: out[10] = "57896044618658097711785492504343953926634992332820282019728792003956564819967"_i256; break;
        case 11: out[11] = "-57896044618658097711785492504343953926634992332820282019728792003956564819968"_i256; break;
        case 12: out[12] = "0"_I256; break;
        case 13: out[13] = "-999999999999999999"_I256; break;
        case 14: out[14] = "57896044618658097711785492504343953926634992332820282019728792003956564819967"_I256; break;
        case 15: out[15] = "-57896044618658097711785492504343953926634992332820282019728792003956564819968"_I256; break;
        case 16: out[16] = "-0x8000000000000000000000000000000000000000000000000000000000000000"_i256; break;
        case 17: out[17] = -0xff_i256; break;
        default: break;
    }
}

int main()
{
    std::cout << "[Signed literal tests: " << num_tests << " cases]" << std::endl;

    cuda_managed_ptr<int256> output(num_tests);

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
    int256 expected[num_tests];
    expected[0] = 0_i256;
    expected[1] = 1_i256;
    expected[2] = -1_i256;
    expected[3] = 42_i256;
    expected[4] = 0_I256;
    expected[5] = 1_I256;
    expected[6] = -42_I256;
    expected[7] = 9223372036854775807_I256;
    expected[8] = "0"_i256;
    expected[9] = "-1"_i256;
    expected[10] = "57896044618658097711785492504343953926634992332820282019728792003956564819967"_i256;
    expected[11] = "-57896044618658097711785492504343953926634992332820282019728792003956564819968"_i256;
    expected[12] = "0"_I256;
    expected[13] = "-999999999999999999"_I256;
    expected[14] = "57896044618658097711785492504343953926634992332820282019728792003956564819967"_I256;
    expected[15] = "-57896044618658097711785492504343953926634992332820282019728792003956564819968"_I256;
    expected[16] = "-0x8000000000000000000000000000000000000000000000000000000000000000"_i256;
    expected[17] = -0xff_i256;

    if (output[0] != expected[0]) { std::cerr << "Result verification failed at test case 0!" << std::endl; return EXIT_FAILURE; }
    if (output[1] != expected[1]) { std::cerr << "Result verification failed at test case 1!" << std::endl; return EXIT_FAILURE; }
    if (output[2] != expected[2]) { std::cerr << "Result verification failed at test case 2!" << std::endl; return EXIT_FAILURE; }
    if (output[3] != expected[3]) { std::cerr << "Result verification failed at test case 3!" << std::endl; return EXIT_FAILURE; }
    if (output[4] != expected[4]) { std::cerr << "Result verification failed at test case 4!" << std::endl; return EXIT_FAILURE; }
    if (output[5] != expected[5]) { std::cerr << "Result verification failed at test case 5!" << std::endl; return EXIT_FAILURE; }
    if (output[6] != expected[6]) { std::cerr << "Result verification failed at test case 6!" << std::endl; return EXIT_FAILURE; }
    if (output[7] != expected[7]) { std::cerr << "Result verification failed at test case 7!" << std::endl; return EXIT_FAILURE; }
    if (output[8] != expected[8]) { std::cerr << "Result verification failed at test case 8!" << std::endl; return EXIT_FAILURE; }
    if (output[9] != expected[9]) { std::cerr << "Result verification failed at test case 9!" << std::endl; return EXIT_FAILURE; }
    if (output[10] != expected[10]) { std::cerr << "Result verification failed at test case 10!" << std::endl; return EXIT_FAILURE; }
    if (output[11] != expected[11]) { std::cerr << "Result verification failed at test case 11!" << std::endl; return EXIT_FAILURE; }
    if (output[12] != expected[12]) { std::cerr << "Result verification failed at test case 12!" << std::endl; return EXIT_FAILURE; }
    if (output[13] != expected[13]) { std::cerr << "Result verification failed at test case 13!" << std::endl; return EXIT_FAILURE; }
    if (output[14] != expected[14]) { std::cerr << "Result verification failed at test case 14!" << std::endl; return EXIT_FAILURE; }
    if (output[15] != expected[15]) { std::cerr << "Result verification failed at test case 15!" << std::endl; return EXIT_FAILURE; }
    if (output[16] != expected[16]) { std::cerr << "Result verification failed at test case 16!" << std::endl; return EXIT_FAILURE; }
    if (output[17] != expected[17]) { std::cerr << "Result verification failed at test case 17!" << std::endl; return EXIT_FAILURE; }

    std::cout << "Test PASSED" << std::endl;
    std::cout << "Done" << std::endl;

    return 0;
}
