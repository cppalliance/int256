// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// int256 division on the device, over every divisor width.
// Adapted from the Boost.Int128 CUDA test suite.

#include <iostream>
#include <vector>
#include <random>
#include <limits>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <boost/int256.hpp>
#include "random_uint256.hpp"
#include "cuda_managed_ptr.hpp"
#include "stopwatch.hpp"

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>

using test_type = boost::int256::int256;

__global__ void cuda_test(const test_type* in, const test_type* in2, test_type* out, int num_elements)
{
    const int i {static_cast<int>(blockDim.x * blockIdx.x + threadIdx.x)};

    if (i < num_elements)
    {
        out[i] = in[i] / in2[i];
    }
}

int main()
{
    std::mt19937_64 rng {42};

    const int num_elements {50000};
    std::cout << "[Vector operation on " << num_elements << " elements]" << std::endl;

    cuda_managed_ptr<test_type> input_vector(num_elements);
    cuda_managed_ptr<test_type> input_vector2(num_elements);
    cuda_managed_ptr<test_type> output_vector(num_elements);

    for (int i {0}; i < num_elements; ++i)
    {
        input_vector[i] = random_value<test_type>(rng);

        test_type divisor {random_value<test_type>(rng) >> (64 * (i % 4))};
        if (divisor == test_type{0})
        {
            divisor = test_type{1};
        }
        input_vector2[i] = divisor;
    }

    // Force the one defined signed overflow (MIN / -1 wraps to MIN) and its
    // non-overflowing neighbor onto the device path every run, not only when a
    // random draw happens to land on them.
    if (num_elements > 1)
    {
        input_vector[0] = (std::numeric_limits<test_type>::min)();
        input_vector2[0] = test_type{-1};
        input_vector[1] = (std::numeric_limits<test_type>::min)();
        input_vector2[1] = test_type{1};
    }

    const int threads_per_block {256};
    const int blocks_per_grid {(num_elements + threads_per_block - 1) / threads_per_block};
    std::cout << "CUDA kernel launch with " << blocks_per_grid << " blocks of "
              << threads_per_block << " threads" << std::endl;

    watch w;

    cuda_test<<<blocks_per_grid, threads_per_block>>>(input_vector.get(), input_vector2.get(),
                                                      output_vector.get(), num_elements);
    cudaDeviceSynchronize();

    std::cout << "CUDA kernel done in: " << w.elapsed() << "s" << std::endl;

    const cudaError_t err {cudaGetLastError()};
    if (err != cudaSuccess)
    {
        std::cerr << "Failed to launch kernel (error code " << cudaGetErrorString(err) << ")!" << std::endl;
        return EXIT_FAILURE;
    }

    // The same operation on the same inputs, recomputed on the host
    const test_type* in {input_vector.get()};
    const test_type* in2 {input_vector2.get()};
    std::vector<test_type> results;
    results.reserve(static_cast<std::size_t>(num_elements));
    w.reset();
    for (int i {0}; i < num_elements; ++i)
    {
        results.push_back(in[i] / in2[i]);
    }
    const double t {w.elapsed()};

    for (int i {0}; i < num_elements; ++i)
    {
        if (output_vector[i] != results[static_cast<std::size_t>(i)])
        {
            std::cerr << "Result verification failed at element " << i << "!" << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "Test PASSED, host calculation time: " << t << "s" << std::endl;
    std::cout << "Done" << std::endl;

    return 0;
}
