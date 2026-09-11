// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// uint256 to uint64_t truncation and back on the device.
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

using test_type = boost::int256::uint256;

// Converts uint256 to uint64_t (truncation to the low word) and back again, both on device
__global__ void cuda_test(const test_type* in, const std::uint64_t* in_words,
                          std::uint64_t* out_truncated, test_type* out_widened, int num_elements)
{
    const int i {static_cast<int>(blockDim.x * blockIdx.x + threadIdx.x)};

    if (i < num_elements)
    {
        out_truncated[i] = static_cast<std::uint64_t>(in[i]);
        out_widened[i] = test_type{in_words[i]};
    }
}

int main()
{
    std::mt19937_64 rng {42};

    const int num_elements {50000};
    std::cout << "[Vector operation on " << num_elements << " elements]" << std::endl;

    cuda_managed_ptr<test_type> input_vector(num_elements);
    cuda_managed_ptr<std::uint64_t> input_words(num_elements);
    cuda_managed_ptr<std::uint64_t> output_truncated(num_elements);
    cuda_managed_ptr<test_type> output_widened(num_elements);

    for (int i {0}; i < num_elements; ++i)
    {
        input_vector[i] = random_value<test_type>(rng);
        input_words[i] = static_cast<std::uint64_t>(rng());
    }

    const int threads_per_block {256};
    const int blocks_per_grid {(num_elements + threads_per_block - 1) / threads_per_block};
    std::cout << "CUDA kernel launch with " << blocks_per_grid << " blocks of "
              << threads_per_block << " threads" << std::endl;

    watch w;

    cuda_test<<<blocks_per_grid, threads_per_block>>>(input_vector.get(), input_words.get(),
                                                      output_truncated.get(), output_widened.get(), num_elements);
    cudaDeviceSynchronize();

    std::cout << "CUDA kernel done in: " << w.elapsed() << "s" << std::endl;

    const cudaError_t err {cudaGetLastError()};
    if (err != cudaSuccess)
    {
        std::cerr << "Failed to launch kernel (error code " << cudaGetErrorString(err) << ")!" << std::endl;
        return EXIT_FAILURE;
    }

    // The same conversions, recomputed on the host
    const test_type* in {input_vector.get()};
    const std::uint64_t* in_words {input_words.get()};
    w.reset();
    for (int i {0}; i < num_elements; ++i)
    {
        if (output_truncated[i] != static_cast<std::uint64_t>(in[i]) ||
            output_widened[i] != test_type{in_words[i]})
        {
            std::cerr << "Result verification failed at element " << i << "!" << std::endl;
            return EXIT_FAILURE;
        }
    }
    const double t {w.elapsed()};

    std::cout << "Test PASSED, host calculation time: " << t << "s" << std::endl;
    std::cout << "Done" << std::endl;

    return 0;
}
