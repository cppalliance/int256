// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// to_be, to_le and from_be on the device.
// Adapted from the Boost.Int128 CUDA test suite.

#include <iostream>
#include <vector>
#include <random>
#include <limits>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <boost/int256.hpp>
#include <boost/int256/byte_conversions.hpp>
#include "random_uint256.hpp"
#include "cuda_managed_ptr.hpp"
#include "stopwatch.hpp"

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>

using test_type = boost::int256::int256;

__global__ void cuda_test(const test_type* in, test_type* out_be, test_type* out_le,
                          test_type* out_round, int num_elements)
{
    const int i {static_cast<int>(blockDim.x * blockIdx.x + threadIdx.x)};

    if (i < num_elements)
    {
        out_be[i] = boost::int256::to_be(in[i]);
        out_le[i] = boost::int256::to_le(in[i]);

        // Both directions on the device, which must recover the input
        out_round[i] = boost::int256::from_be(boost::int256::to_be(in[i]));
    }
}

int main()
{
    std::mt19937_64 rng {42};

    const int num_elements {50000};
    std::cout << "[Vector operation on " << num_elements << " elements]" << std::endl;

    cuda_managed_ptr<test_type> input_vector(num_elements);
    cuda_managed_ptr<test_type> output_be(num_elements);
    cuda_managed_ptr<test_type> output_le(num_elements);
    cuda_managed_ptr<test_type> output_round(num_elements);

    for (int i {0}; i < num_elements; ++i)
    {
        input_vector[i] = random_value<test_type>(rng);
    }

    const int threads_per_block {256};
    const int blocks_per_grid {(num_elements + threads_per_block - 1) / threads_per_block};
    std::cout << "CUDA kernel launch with " << blocks_per_grid << " blocks of "
              << threads_per_block << " threads" << std::endl;

    watch w;

    cuda_test<<<blocks_per_grid, threads_per_block>>>(input_vector.get(), output_be.get(), output_le.get(),
                                                      output_round.get(), num_elements);
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
    std::vector<test_type> be_results;
    std::vector<test_type> le_results;
    be_results.reserve(static_cast<std::size_t>(num_elements));
    le_results.reserve(static_cast<std::size_t>(num_elements));
    w.reset();
    for (int i {0}; i < num_elements; ++i)
    {
        be_results.push_back(boost::int256::to_be(in[i]));
        le_results.push_back(boost::int256::to_le(in[i]));
    }
    const double t {w.elapsed()};

    for (int i {0}; i < num_elements; ++i)
    {
        const std::size_t k {static_cast<std::size_t>(i)};

        if (output_be[i] != be_results[k] || output_le[i] != le_results[k] || output_round[i] != in[i])
        {
            std::cerr << "Result verification failed at element " << i << "!" << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "Test PASSED, host calculation time: " << t << "s" << std::endl;
    std::cout << "Done" << std::endl;

    return 0;
}
