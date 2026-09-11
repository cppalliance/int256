// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>
#include <boost/int256/literals.hpp>
#include <boost/int256/iostream.hpp>
#include <boost/int256/limits.hpp>
#include <iostream>
#include <limits>
#include <sstream>

int main()
{
    using boost::int256::uint256;

    std::cout << "=== uint256 Construction ===" << std::endl;

    // 1) From a builtin integer type
    constexpr uint256 from_builtin {42U};
    std::cout << "From builtin (42U): " << from_builtin << std::endl;

    // 2) From the four 64-bit words, most significant first
    constexpr uint256 from_parts {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)};  // 2^64
    std::cout << "From words (0, 0, 1, 0) = 2^64: " << from_parts << std::endl;

    constexpr uint256 max_value {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    std::cout << "From words (max, max, max, max): " << max_value << std::endl;
    std::cout << "  Equals numeric_limits max? " << std::boolalpha
              << (max_value == (std::numeric_limits<uint256>::max)()) << std::endl;

    // 3) From user-defined literals.
    // The library provides only string-form UDLs
    // For small values like this a string is still parsed rather than direct construction
    // Using the constructors for values that fit in (unsigned) long long should be preferred for performance
    using namespace boost::int256::literals;
    const auto small_literal {12345_U256};
    std::cout << "From literal 12345_U256: " << small_literal << std::endl;

    const auto big_literal {"115792089237316195423570985008687907853269984665640564039457584007913129639935"_u256};
    std::cout << "From string literal (max): " << big_literal << std::endl;

    // 4) From macro (like UINT64_C but for 256-bit), good for values that exceed unsigned long long
    const auto from_macro {BOOST_INT256_UINT256_C(6277101735386680763835789423207666416102355444464034512896)};
    std::cout << "From BOOST_INT256_UINT256_C(2^192): " << from_macro << std::endl;

    // 5) From input stream
    std::stringstream ss;
    ss.str("123456789012345678901234567890123456789012345678901234567890");
    uint256 from_stream;
    ss >> from_stream;
    std::cout << "From stringstream: " << from_stream << std::endl;

    std::cout << "\n=== Default and Copy Construction ===" << std::endl;

    // Default construction (zero-initialized)
    constexpr uint256 default_constructed {};
    std::cout << "Default constructed: " << default_constructed << std::endl;

    // Copy construction
    const uint256 copied {from_macro};
    std::cout << "Copy constructed: " << copied << std::endl;

    std::cout << "\n=== Floating-Point Construction ===" << std::endl;

    // Floating-point construction truncates toward zero, matching the behavior of
    // a static_cast from a floating-point type to a built-in integer.
    constexpr uint256 from_double {12345.9};
    std::cout << "uint256 from 12345.9 (truncated): " << from_double << std::endl;

    // Values that exceed the 64-bit range are routed through the full 256-bit decomposition.
    const double two_to_the_200 {1.6069380442589903e60};  // 2^200
    const uint256 large_from_double {two_to_the_200};
    std::cout << "uint256 from 2^200: " << large_from_double << std::endl;

    std::cout << "\n=== Floating-Point Edge Cases ===" << std::endl;

    // NaN yields zero, which mirrors libgcc's __fixunsdfti.
    const double nan_value {std::numeric_limits<double>::quiet_NaN()};
    const uint256 unsigned_from_nan {nan_value};
    std::cout << "uint256 from NaN: " << unsigned_from_nan << std::endl;

    // Negative values are clamped to zero.
    const uint256 unsigned_from_negative {-1.0};
    std::cout << "uint256 from -1.0 (clamped to zero): " << unsigned_from_negative << std::endl;

    // Positive overflow saturates: anything >= 2^256 (including +infinity) becomes the max.
    const double infinity {std::numeric_limits<double>::infinity()};
    const uint256 saturated {infinity};
    std::cout << "uint256 from +infinity (saturates): " << saturated << std::endl;

    const double huge {1e80};  // Well beyond 2^256 (about 1.16e77)
    const uint256 saturated_from_huge {huge};
    std::cout << "uint256 from 1e80 (saturates): " << saturated_from_huge << std::endl;

    return 0;
}
