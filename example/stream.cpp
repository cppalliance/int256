// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>
#include <boost/int256/iostream.hpp>
#include <boost/int256/limits.hpp>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

int main()
{
    using boost::int256::uint256;

    std::cout << "=== Basic Streaming ===" << std::endl;

    // uint256 streams as one would expect from a built-in integer type
    constexpr uint256 small_value {42U};
    std::cout << "Small value: " << small_value << std::endl;

    // The <ios> base manipulators are honored
    constexpr uint256 value {UINT64_C(0), UINT64_C(0), UINT64_C(0x1), UINT64_MAX};
    std::cout << "Value (dec): " << value << '\n'
              << "Value (hex): " << std::hex << value << '\n'
              << "Value (oct): " << std::oct << value << std::endl;

    // Hex can also be uppercased, and a base prefix requested
    std::cout << "Value (upper hex): " << std::hex << std::uppercase << value << '\n'
              << "Value (showbase):  " << std::showbase << value << std::endl;

    // And returned to the default formatting
    std::cout << "Value (dec again): " << std::dec << std::nouppercase << std::noshowbase << value << std::endl;

    std::cout << "\n=== Large Values (Beyond 128 bits) ===" << std::endl;

    // 2^128 is the first value that does not fit in a 128-bit type
    constexpr uint256 two_to_128 {uint256{1U} << 128U};
    std::cout << "2^128 = " << two_to_128 << std::endl;

    constexpr uint256 two_to_200 {uint256{1U} << 200U};
    std::cout << "2^200 = " << two_to_200 << std::endl;

    constexpr auto max_value {(std::numeric_limits<uint256>::max)()};
    std::cout << "uint256 max = " << max_value << std::endl;

    std::cout << "\n=== String Conversion with std::stringstream ===" << std::endl;

    std::ostringstream oss;
    oss << two_to_200;
    const auto str {oss.str()};
    std::cout << "uint256 to string: \"" << str << "\"" << std::endl;

    std::istringstream iss {"123456789012345678901234567890123456789012345678901234567890"};
    uint256 parsed_value {};
    iss >> parsed_value;
    std::cout << "String to uint256: " << parsed_value << std::endl;

    std::cout << "\n=== Round-trip Conversion ===" << std::endl;

    constexpr uint256 original {UINT64_C(0xDEADBEEF), UINT64_C(0xCAFEBABE12345678),
                                UINT64_C(0x0F1E2D3C4B5A6978), UINT64_C(0x8796A5B4C3D2E1F0)};
    std::ostringstream oss2;
    oss2 << original;
    const auto original_str {oss2.str()};

    std::istringstream iss2 {original_str};
    uint256 round_tripped {};
    iss2 >> round_tripped;

    std::cout << "Original:      " << original << std::endl;
    std::cout << "As string:     \"" << original_str << "\"" << std::endl;
    std::cout << "Round-tripped: " << round_tripped << std::endl;
    std::cout << "Match: " << std::boolalpha << (original == round_tripped) << std::endl;

    std::cout << "\n=== Width, Fill and Alignment ===" << std::endl;

    // The width and fill manipulators apply to the whole number
    std::cout << "[" << std::setw(30) << std::setfill('.') << small_value << "]" << std::endl;
    std::cout << "[" << std::left << std::setw(30) << std::setfill('.') << small_value << "]" << std::endl;
    std::cout << std::right << std::setfill(' ');

    return 0;
}
