// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The 32 byte array conversions on the device.
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
#include <array>
#include <cstring>
#include "random_uint256.hpp"
#include "cuda_managed_ptr.hpp"
#include "stopwatch.hpp"

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>

using test_type = boost::int256::uint256;

// A plain 32 byte block. The device writes through the raw array overloads of to_*_bytes,
// because the std::array returning forms are host only.
struct byte_block
{
    std::uint8_t bytes[sizeof(test_type)];
};

__global__ void cuda_test(const test_type* in, const std::uint8_t* in_bytes,
                          byte_block* out_be, byte_block* out_le, byte_block* out_ne,
                          test_type* from_be, test_type* from_le, test_type* from_ne, int num_elements)
{
    const int i {static_cast<int>(blockDim.x * blockIdx.x + threadIdx.x)};

    if (i < num_elements)
    {
        boost::int256::to_be_bytes(in[i], out_be[i].bytes);
        boost::int256::to_le_bytes(in[i], out_le[i].bytes);
        boost::int256::to_ne_bytes(in[i], out_ne[i].bytes);

        const std::uint8_t* bytes {in_bytes + static_cast<std::size_t>(i) * sizeof(test_type)};
        from_be[i] = boost::int256::from_be_bytes<test_type>(bytes);
        from_le[i] = boost::int256::from_le_bytes<test_type>(bytes);
        from_ne[i] = boost::int256::from_ne_bytes<test_type>(bytes);
    }
}

int main()
{
    std::mt19937_64 rng {42};

    const int num_elements {50000};
    std::cout << "[Vector operation on " << num_elements << " elements]" << std::endl;

    cuda_managed_ptr<test_type> input_vector(num_elements);
    cuda_managed_ptr<std::uint8_t> input_bytes(static_cast<std::size_t>(num_elements) * sizeof(test_type));
    cuda_managed_ptr<byte_block> output_be(num_elements);
    cuda_managed_ptr<byte_block> output_le(num_elements);
    cuda_managed_ptr<byte_block> output_ne(num_elements);
    cuda_managed_ptr<test_type> output_from_be(num_elements);
    cuda_managed_ptr<test_type> output_from_le(num_elements);
    cuda_managed_ptr<test_type> output_from_ne(num_elements);

    for (int i {0}; i < num_elements; ++i)
    {
        input_vector[i] = random_value<test_type>(rng);
    }
    for (std::size_t i {0}; i < static_cast<std::size_t>(num_elements) * sizeof(test_type); ++i)
    {
        input_bytes[i] = static_cast<std::uint8_t>(rng() & 0xFFU);
    }

    const int threads_per_block {256};
    const int blocks_per_grid {(num_elements + threads_per_block - 1) / threads_per_block};
    std::cout << "CUDA kernel launch with " << blocks_per_grid << " blocks of "
              << threads_per_block << " threads" << std::endl;

    watch w;

    cuda_test<<<blocks_per_grid, threads_per_block>>>(input_vector.get(), input_bytes.get(),
                                                      output_be.get(), output_le.get(), output_ne.get(),
                                                      output_from_be.get(), output_from_le.get(), output_from_ne.get(),
                                                      num_elements);
    cudaDeviceSynchronize();

    std::cout << "CUDA kernel done in: " << w.elapsed() << "s" << std::endl;

    const cudaError_t err {cudaGetLastError()};
    if (err != cudaSuccess)
    {
        std::cerr << "Failed to launch kernel (error code " << cudaGetErrorString(err) << ")!" << std::endl;
        return EXIT_FAILURE;
    }

    // The host uses the std::array returning overloads, so this also cross checks the two
    // forms of to_*_bytes against each other
    const test_type* in {input_vector.get()};
    const std::uint8_t* host_bytes {input_bytes.get()};

    w.reset();
    for (int i {0}; i < num_elements; ++i)
    {
        const std::size_t k {static_cast<std::size_t>(i)};

        const auto be {boost::int256::to_be_bytes(in[i])};
        const auto le {boost::int256::to_le_bytes(in[i])};
        const auto ne {boost::int256::to_ne_bytes(in[i])};

        if (std::memcmp(output_be[i].bytes, be.data(), sizeof(test_type)) != 0 ||
            std::memcmp(output_le[i].bytes, le.data(), sizeof(test_type)) != 0 ||
            std::memcmp(output_ne[i].bytes, ne.data(), sizeof(test_type)) != 0)
        {
            std::cerr << "Result verification failed at element " << i << "!" << std::endl;
            return EXIT_FAILURE;
        }

        const std::uint8_t* bytes {host_bytes + k * sizeof(test_type)};

        if (output_from_be[i] != boost::int256::from_be_bytes<test_type>(bytes) ||
            output_from_le[i] != boost::int256::from_le_bytes<test_type>(bytes) ||
            output_from_ne[i] != boost::int256::from_ne_bytes<test_type>(bytes))
        {
            std::cerr << "Result verification failed at element " << i << "!" << std::endl;
            return EXIT_FAILURE;
        }

        // Reading back what the device wrote recovers the value
        if (boost::int256::from_be_bytes<test_type>(output_be[i].bytes) != in[i] ||
            boost::int256::from_le_bytes<test_type>(output_le[i].bytes) != in[i] ||
            boost::int256::from_ne_bytes<test_type>(output_ne[i].bytes) != in[i])
        {
            std::cerr << "Round trip verification failed at element " << i << "!" << std::endl;
            return EXIT_FAILURE;
        }
    }
    const double t {w.elapsed()};

    std::cout << "Test PASSED, host calculation time: " << t << "s" << std::endl;
    std::cout << "Done" << std::endl;

    return 0;
}
