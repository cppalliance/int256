// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Computes the GCD of 50000 pairs of uint256 values on a SYCL device and checks every
// result against the same computation on the host. Build with
// icpx -fsycl -fsycl-device-code-split=per_kernel -DBOOST_INT256_ENABLE_SYCL=1 -I include sycl.cpp

#include <sycl/sycl.hpp>
#include <boost/int256.hpp>
#include <boost/int256/numeric.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

using test_type = boost::int256::uint256;

int main()
{
    std::mt19937_64 rng {42};

    const int num_elements {50000};
    std::cout << "[Vector operation on " << num_elements << " elements]" << std::endl;

    sycl::queue q;
    std::cout << "SYCL device: " << q.get_device().get_info<sycl::info::device::name>() << std::endl;

    // Allocate shared (USM) memory so the arrays are usable on both the host and the device
    test_type* in1 {sycl::malloc_shared<test_type>(num_elements, q)};
    test_type* in2 {sycl::malloc_shared<test_type>(num_elements, q)};
    test_type* out {sycl::malloc_shared<test_type>(num_elements, q)};

    // Fill the inputs with random values, one engine draw per 64-bit word
    for (int i {0}; i < num_elements; ++i)
    {
        in1[i] = test_type{rng(), rng(), rng(), rng()};
        in2[i] = test_type{rng(), rng(), rng(), rng()};
    }

    // Launch the kernel: each work item computes one gcd
    q.submit([&](sycl::handler& h)
    {
        h.parallel_for(sycl::range<1>(num_elements), [=](sycl::id<1> idx)
        {
            const int i {static_cast<int>(idx[0])};
            out[i] = boost::int256::gcd(in1[i], in2[i]);
        });
    }).wait();

    // Repeat the same operation on the same inputs on the host
    std::vector<test_type> results;
    results.reserve(static_cast<std::size_t>(num_elements));
    for (int i {0}; i < num_elements; ++i)
    {
        results.emplace_back(boost::int256::gcd(in1[i], in2[i]));
    }

    // The device and the host run the same code, so every element has to agree exactly
    int ret {EXIT_SUCCESS};
    for (int i {0}; i < num_elements; ++i)
    {
        if (out[i] != results[static_cast<std::size_t>(i)])
        {
            std::cerr << "Result verification failed at element: " << i << "!" << std::endl;
            ret = EXIT_FAILURE;
            break;
        }
    }

    if (ret == EXIT_SUCCESS)
    {
        std::cout << "All CPU and GPU computed elements match!" << std::endl;
    }

    sycl::free(in1, q);
    sycl::free(in2, q);
    sycl::free(out, q);

    return ret;
}
