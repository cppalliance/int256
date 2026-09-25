// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Exercises the signed user-defined literal operators inside a SYCL kernel and
// checks the device-produced values against the same literals evaluated on the host.

#include "sycl_test.hpp"
#include <boost/int256/literals.hpp>
#include <iostream>
#include <cstdlib>

using boost::int256::int256;
using namespace boost::int256::literals;

int main()
{
    constexpr int num_tests {18};

    sycl::queue q;
    std::cout << "SYCL device: " << q.get_device().get_info<sycl::info::device::name>() << "\n";

    int256* out {sycl::malloc_shared<int256>(num_tests, q)};

    q.submit([&](sycl::handler& h)
    {
        h.parallel_for(sycl::range<1>(num_tests), [=](sycl::id<1> idx)
        {
            switch (idx[0])
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
        });
    }).wait();

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

    int failures {0};
    for (int i {0}; i < num_tests; ++i)
    {
        if (out[i] != expected[i])
        {
            std::cerr << "Mismatch at literal case " << i << "\n";
            ++failures;
        }
    }

    sycl::free(out, q);

    if (failures == 0)
    {
        std::cout << "Test PASSED\n";
        return EXIT_SUCCESS;
    }

    std::cerr << "Test FAILED with " << failures << " mismatches\n";
    return EXIT_FAILURE;
}
