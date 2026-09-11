// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// from_chars in base 10 on the device.
// Adapted from the Boost.Int128 CUDA test suite.

#include <iostream>
#include <vector>
#include <random>
#include <limits>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <boost/int256.hpp>
#include <boost/int256/charconv.hpp>
#include "random_uint256.hpp"
#include "cuda_managed_ptr.hpp"
#include "stopwatch.hpp"

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>

using test_type = boost::int256::uint256;

// Large enough for base 2 plus slack
constexpr int buf_size {260};

__global__ void cuda_test(const char* in_strings, const int* in_lengths, test_type* out, int num_elements, int base)
{
    const int i {static_cast<int>(blockDim.x * blockIdx.x + threadIdx.x)};

    if (i < num_elements)
    {
        const char* str {in_strings + i * buf_size};
        test_type value {};
        boost::charconv::from_chars(str, str + in_lengths[i], value, base);
        out[i] = value;
    }
}

int main()
{
    std::mt19937_64 rng {42};

    const int num_elements {50000};
    std::cout << "[Vector operation on " << num_elements << " elements]" << std::endl;

    cuda_managed_ptr<char> input_strings(num_elements * buf_size);
    cuda_managed_ptr<int> input_lengths(num_elements);
    cuda_managed_ptr<test_type> output_vector(num_elements);

    const int threads_per_block {256};
    const int blocks_per_grid {(num_elements + threads_per_block - 1) / threads_per_block};

    std::vector<test_type> expected(static_cast<std::size_t>(num_elements));

    for (const int base : {10})
    {
        for (int i {0}; i < num_elements; ++i)
        {
            expected[static_cast<std::size_t>(i)] = random_value<test_type>(rng);
            char* buf {&input_strings[static_cast<std::size_t>(i) * buf_size]};
            const auto res {boost::charconv::to_chars(buf, buf + buf_size,
                                                      expected[static_cast<std::size_t>(i)], base)};
            input_lengths[i] = static_cast<int>(res.ptr - buf);
        }

        std::cout << "CUDA kernel launch with " << blocks_per_grid << " blocks of " << threads_per_block
                  << " threads, base " << base << std::endl;

        watch w;

        cuda_test<<<blocks_per_grid, threads_per_block>>>(input_strings.get(), input_lengths.get(),
                                                          output_vector.get(), num_elements, base);
        cudaDeviceSynchronize();

        std::cout << "CUDA kernel done in: " << w.elapsed() << "s" << std::endl;

        const cudaError_t err {cudaGetLastError()};
        if (err != cudaSuccess)
        {
            std::cerr << "Failed to launch kernel (error code " << cudaGetErrorString(err) << ")!" << std::endl;
            return EXIT_FAILURE;
        }

        // The device parse has to recover the value the host formatted
        w.reset();
        for (int i {0}; i < num_elements; ++i)
        {
            if (output_vector[i] != expected[static_cast<std::size_t>(i)])
            {
                std::cerr << "Result verification failed at element " << i << " base " << base << "!" << std::endl;
                return EXIT_FAILURE;
            }
        }
        const double t {w.elapsed()};

        std::cout << "Test base " << base << " PASSED, host calculation time: " << t << "s" << std::endl;
    }

    std::cout << "Test PASSED" << std::endl;
    std::cout << "Done" << std::endl;

    return 0;
}
