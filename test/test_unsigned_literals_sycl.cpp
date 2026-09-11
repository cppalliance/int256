// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Exercises the user-defined literal operators inside a SYCL kernel and checks the
// device-produced values against the same literals evaluated on the host.

#include "sycl_test.hpp"
#include <boost/int256/literals.hpp>
#include <iostream>
#include <cstdlib>

using boost::int256::uint256;
using namespace boost::int256::literals;

int main()
{
    constexpr int num_tests {24};

    sycl::queue q;
    std::cout << "SYCL device: " << q.get_device().get_info<sycl::info::device::name>() << "\n";

    uint256* out {sycl::malloc_shared<uint256>(num_tests, q)};

    q.submit([&](sycl::handler& h)
    {
        h.parallel_for(sycl::range<1>(num_tests), [=](sycl::id<1> idx)
        {
            switch (idx[0])
            {
                // Raw literal operator, with values too large for unsigned long long
                case 0: out[0] = 18446744073709551616_u256; break;
                case 1: out[1] = 340282366920938463463374607431768211456_u256; break;
                case 2: out[2] = 6277101735386680763835789423207666416102355444464034512896_u256; break;
                case 3: out[3] = 115792089237316195423570985008687907853269984665640564039457584007913129639935_u256; break;
                case 4: out[4] = 18446744073709551616_U256; break;
                case 5: out[5] = 340282366920938463463374607431768211456_U256; break;
                case 6: out[6] = 6277101735386680763835789423207666416102355444464034512896_U256; break;
                case 7: out[7] = 115792089237316195423570985008687907853269984665640564039457584007913129639935_U256; break;

                // Cooked string literal operator
                case 8: out[8] = "0"_u256; break;
                case 9: out[9] = "1"_u256; break;
                case 10: out[10] = "340282366920938463463374607431768211455"_u256; break;
                case 11: out[11] = "115792089237316195423570985008687907853269984665640564039457584007913129639935"_u256; break;
                case 12: out[12] = "0"_U256; break;
                case 13: out[13] = "1"_U256; break;
                case 14: out[14] = "340282366920938463463374607431768211455"_U256; break;
                case 15: out[15] = "115792089237316195423570985008687907853269984665640564039457584007913129639935"_U256; break;

                // Raw literal operator, with small values
                case 16: out[16] = 0_u256; break;
                case 17: out[17] = 1_u256; break;
                case 18: out[18] = 42_u256; break;
                case 19: out[19] = 18446744073709551615_u256; break;
                case 20: out[20] = 0_U256; break;
                case 21: out[21] = 1_U256; break;
                case 22: out[22] = 42_U256; break;
                case 23: out[23] = 18446744073709551615_U256; break;

                default: break;
            }
        });
    }).wait();

    uint256 expected[num_tests];
    // Raw literal operator, with values too large for unsigned long long
    expected[0] = 18446744073709551616_u256;
    expected[1] = 340282366920938463463374607431768211456_u256;
    expected[2] = 6277101735386680763835789423207666416102355444464034512896_u256;
    expected[3] = 115792089237316195423570985008687907853269984665640564039457584007913129639935_u256;
    expected[4] = 18446744073709551616_U256;
    expected[5] = 340282366920938463463374607431768211456_U256;
    expected[6] = 6277101735386680763835789423207666416102355444464034512896_U256;
    expected[7] = 115792089237316195423570985008687907853269984665640564039457584007913129639935_U256;

    // Cooked string literal operator
    expected[8] = "0"_u256;
    expected[9] = "1"_u256;
    expected[10] = "340282366920938463463374607431768211455"_u256;
    expected[11] = "115792089237316195423570985008687907853269984665640564039457584007913129639935"_u256;
    expected[12] = "0"_U256;
    expected[13] = "1"_U256;
    expected[14] = "340282366920938463463374607431768211455"_U256;
    expected[15] = "115792089237316195423570985008687907853269984665640564039457584007913129639935"_U256;

    // Raw literal operator, with small values
    expected[16] = 0_u256;
    expected[17] = 1_u256;
    expected[18] = 42_u256;
    expected[19] = 18446744073709551615_u256;
    expected[20] = 0_U256;
    expected[21] = 1_U256;
    expected[22] = 42_U256;
    expected[23] = 18446744073709551615_U256;

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
