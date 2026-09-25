// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// to_chars in every base from 2 to 36 on the device.
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
#include <cstring>
#include "random_uint256.hpp"
#include "cuda_managed_ptr.hpp"
#include "stopwatch.hpp"

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>

using test_type = boost::int256::int256;

// Large enough for base 2 plus slack
constexpr int buf_size {260};

__global__ void cuda_test(const test_type* in, char* out_strings, int* out_lengths, int num_elements, int base)
{
    const int i {static_cast<int>(blockDim.x * blockIdx.x + threadIdx.x)};

    if (i < num_elements)
    {
        char* buf {out_strings + i * buf_size};
        const auto res {boost::charconv::to_chars(buf, buf + buf_size, in[i], base)};
        out_lengths[i] = static_cast<int>(res.ptr - buf);
    }
}

int main()
{
    std::mt19937_64 rng {42};

    const int num_elements {50000};
    std::cout << "[Vector operation on " << num_elements << " elements]" << std::endl;

    cuda_managed_ptr<test_type> input_vector(num_elements);
    cuda_managed_ptr<char> output_strings(num_elements * buf_size);
    cuda_managed_ptr<int> output_lengths(num_elements);

    const int threads_per_block {256};
    const int blocks_per_grid {(num_elements + threads_per_block - 1) / threads_per_block};

    for (int base {2}; base <= 36; ++base)
    {
        for (int i {0}; i < num_elements; ++i)
        {
            input_vector[i] = random_value<test_type>(rng);
        }

        std::cout << "CUDA kernel launch with " << blocks_per_grid << " blocks of " << threads_per_block
                  << " threads, base " << base << std::endl;

        watch w;

        cuda_test<<<blocks_per_grid, threads_per_block>>>(input_vector.get(), output_strings.get(),
                                                          output_lengths.get(), num_elements, base);
        cudaDeviceSynchronize();

        std::cout << "CUDA kernel done in: " << w.elapsed() << "s" << std::endl;

        const cudaError_t err {cudaGetLastError()};
        if (err != cudaSuccess)
        {
            std::cerr << "Failed to launch kernel (error code " << cudaGetErrorString(err) << ")!" << std::endl;
            return EXIT_FAILURE;
        }

        // The same formatting, recomputed on the host
        w.reset();
        for (int i {0}; i < num_elements; ++i)
        {
            char host_buf[buf_size];
            const auto host_res {boost::charconv::to_chars(host_buf, host_buf + buf_size, input_vector[i], base)};
            const int host_len {static_cast<int>(host_res.ptr - host_buf)};
            const char* device_buf {&output_strings[static_cast<std::size_t>(i) * buf_size]};

            if (output_lengths[i] != host_len ||
                std::memcmp(host_buf, device_buf, static_cast<std::size_t>(host_len)) != 0)
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
