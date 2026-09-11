// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Computes the GCD of 50000 pairs of uint256 values on a CUDA device and checks every
// result against the same computation on the host. Build with
// nvcc -arch=sm_86 --expt-relaxed-constexpr -DBOOST_INT256_ENABLE_CUDA=1 -I include cuda.cu

#include <boost/int256.hpp>
#include <boost/int256/numeric.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <cuda_runtime.h>

using test_type = boost::int256::uint256;

// Calculates the GCD of two values on the device
__global__ void cuda_gcd(const test_type* in1, const test_type* in2, test_type* out, int num_elements)
{
    const int i {static_cast<int>(blockDim.x * blockIdx.x + threadIdx.x)};

    if (i < num_elements)
    {
        out[i] = boost::int256::gcd(in1[i], in2[i]);
    }
}

// Allocates managed space so that an array can be used on both the host and the device
void allocate(test_type** in, int num_elements)
{
    const cudaError_t err {cudaMallocManaged(in, static_cast<std::size_t>(num_elements) * sizeof(test_type))};
    if (err != cudaSuccess)
    {
        throw std::runtime_error(cudaGetErrorString(err));
    }

    cudaDeviceSynchronize();
}

void cleanup(test_type** in1, test_type** in2, test_type** out)
{
    if (*in1 != nullptr)
    {
        cudaFree(*in1);
        *in1 = nullptr;
    }

    if (*in2 != nullptr)
    {
        cudaFree(*in2);
        *in2 = nullptr;
    }

    if (*out != nullptr)
    {
        cudaFree(*out);
        *out = nullptr;
    }

    cudaDeviceReset();
}

int main()
{
    std::mt19937_64 rng {42};

    const int num_elements {50000};
    std::cout << "[Vector operation on " << num_elements << " elements]" << std::endl;

    // Allocate managed space for the inputs and the device outputs, then fill the inputs
    // with random values. One engine draw per 64-bit word.

    test_type* in1 {nullptr};
    test_type* in2 {nullptr};
    test_type* out {nullptr};

    allocate(&in1, num_elements);
    allocate(&in2, num_elements);
    allocate(&out, num_elements);

    for (int i {0}; i < num_elements; ++i)
    {
        in1[i] = test_type{rng(), rng(), rng(), rng()};
        in2[i] = test_type{rng(), rng(), rng(), rng()};
    }

    const int threads_per_block {256};
    const int blocks_per_grid {(num_elements + threads_per_block - 1) / threads_per_block};
    std::cout << "CUDA kernel launch with " << blocks_per_grid << " blocks of "
              << threads_per_block << " threads" << std::endl;

    // Launch the kernel and check for errors

    cuda_gcd<<<blocks_per_grid, threads_per_block>>>(in1, in2, out, num_elements);
    cudaDeviceSynchronize();

    const cudaError_t err {cudaGetLastError()};
    if (err != cudaSuccess)
    {
        std::cerr << "Failed to launch kernel (error code " << cudaGetErrorString(err) << ")!" << std::endl;
        cleanup(&in1, &in2, &out);
        return EXIT_FAILURE;
    }

    // Repeat the same operation on the same inputs on the host

    std::vector<test_type> results;
    results.reserve(static_cast<std::size_t>(num_elements));

    for (int i {0}; i < num_elements; ++i)
    {
        results.emplace_back(boost::int256::gcd(in1[i], in2[i]));
    }

    // The device and the host run the same code, so every element has to agree exactly

    for (int i {0}; i < num_elements; ++i)
    {
        if (out[i] != results[static_cast<std::size_t>(i)])
        {
            std::cerr << "Result verification failed at element: " << i << "!" << std::endl;
            cleanup(&in1, &in2, &out);
            return EXIT_FAILURE;
        }
    }

    cleanup(&in1, &in2, &out);

    std::cout << "All CPU and GPU computed elements match!" << std::endl;

    return 0;
}
