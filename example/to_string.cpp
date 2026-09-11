// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>
#include <boost/int256/string.hpp>
#include <boost/int256/literals.hpp>
#include <boost/int256/limits.hpp>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

int main()
{
    using boost::int256::uint256;
    using boost::int256::to_string;
    using namespace boost::int256::literals;

    std::cout << "=== to_string with uint256 ===" << std::endl;

    // For values that fit in 64 bits the result matches std::to_string exactly
    constexpr uint256 small {UINT64_C(1234567890)};
    const auto small_str {to_string(small)};
    const auto small_std {std::to_string(std::uint64_t{1234567890})};
    std::cout << "to_string(1234567890):               " << small_str << std::endl;
    std::cout << "std::to_string(uint64_t 1234567890): " << small_std << std::endl;
    std::cout << "Match: " << std::boolalpha << (small_str == small_std) << std::endl;

    constexpr uint256 max64 {UINT64_MAX};
    const auto max64_str {to_string(max64)};
    const auto max64_std {std::to_string(UINT64_MAX)};
    std::cout << "\nto_string(UINT64_MAX):      " << max64_str << std::endl;
    std::cout << "std::to_string(UINT64_MAX): " << max64_std << std::endl;
    std::cout << "Match: " << (max64_str == max64_std) << std::endl;

    std::cout << "\n=== Values beyond the 64-bit range ===" << std::endl;

    // Every word boundary, to show that nothing is truncated
    std::cout << "2^64:  " << to_string(uint256{1U} << 64U) << std::endl;
    std::cout << "2^128: " << to_string(uint256{1U} << 128U) << std::endl;
    std::cout << "2^192: " << to_string(uint256{1U} << 192U) << std::endl;
    std::cout << "2^255: " << to_string(uint256{1U} << 255U) << std::endl;

    const auto max_value {(std::numeric_limits<uint256>::max)()};
    const auto max_str {to_string(max_value)};
    std::cout << "uint256 max: " << max_str << std::endl;
    std::cout << "Digits in the max: " << max_str.size() << std::endl;

    std::cout << "\n=== Round trip through a literal ===" << std::endl;

    const auto from_literal {115792089237316195423570985008687907853269984665640564039457584007913129639935_u256};
    std::cout << "Literal equals the max: " << (from_literal == max_value) << std::endl;
    std::cout << "to_string of it round trips: " << (to_string(from_literal) == max_str) << std::endl;

    std::cout << "\n=== to_wstring ===" << std::endl;

    // to_wstring produces the same digits as a std::wstring
    const auto wide {boost::int256::to_wstring(small)};
    std::cout << "to_wstring(1234567890) has " << wide.size() << " characters" << std::endl;
    std::cout << "Same digits as to_string: "
              << (std::wstring{small_str.begin(), small_str.end()} == wide) << std::endl;

    return 0;
}
