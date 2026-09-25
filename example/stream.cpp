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
    using boost::int256::int256;

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

    std::cout << "\n=== Basic Streaming with int256 ===" << std::endl;

    // int256 streams sign-magnitude in every base: the sign always comes
    // before any base prefix (-0xff, not 0x-ff, which is what Boost.Int128
    // prints before its own fix)
    constexpr int256 negative_value {-255};
    std::cout << "Negative value (dec):          " << negative_value << '\n'
              << "Negative value (hex, showbase): " << std::hex << std::showbase << negative_value << '\n'
              << "Negative value (oct, showbase): " << std::oct << negative_value << std::endl;
    std::cout << std::dec << std::noshowbase;

    // showpos prints '+' for a non-negative int256 in decimal only; uint256
    // never prints a '+', matching the built-in unsigned types
    constexpr int256 positive_value {255};
    std::cout << "\nshowpos with int256 (dec):  " << std::showpos << positive_value << std::endl;
    std::cout << "showpos with int256 (hex):  " << std::hex << positive_value << std::dec << std::endl;
    std::cout << std::noshowpos;
    std::cout << "showpos has no effect on uint256: " << std::showpos << small_value << std::endl;
    std::cout << std::noshowpos;

    std::cout << "\n=== Large Negative Values (Beyond 128 bits) ===" << std::endl;

    constexpr int256 min_value {(std::numeric_limits<int256>::min)()};
    std::cout << "int256 min = " << min_value << std::endl;
    std::cout << "int256 min (hex, showbase) = " << std::hex << std::showbase << min_value
              << std::dec << std::noshowbase << std::endl;

    // MIN / -1 is defined (wraps to MIN), unlike the built-in signed types
    std::cout << "int256 min / -1 = " << (min_value / int256{-1}) << std::endl;
    std::cout << "Equals min again: " << std::boolalpha << (min_value / int256{-1} == min_value) << std::endl;

    std::cout << "\n=== Parsing a Negative Value, Including a Prefix ===" << std::endl;

    // operator>> accepts a leading '-', including before a 0x or 0 prefix
    std::istringstream neg_dec {"-123456789012345678901234567890123456789012345678901234567890"};
    int256 parsed_negative {};
    neg_dec >> parsed_negative;
    std::cout << "Parsed decimal: " << parsed_negative << std::endl;

    std::istringstream neg_hex {"-ff"};
    int256 parsed_negative_hex {};
    neg_hex >> std::hex >> parsed_negative_hex;
    std::cout << "Parsed hex \"-ff\": " << std::dec << parsed_negative_hex << std::endl;

    std::cout << "\n=== Round-trip Conversion with int256 ===" << std::endl;

    constexpr int256 original_signed {INT64_MIN, UINT64_C(0xCAFEBABE12345678),
                                      UINT64_C(0x0F1E2D3C4B5A6978), UINT64_C(0x8796A5B4C3D2E1F0)};
    std::ostringstream oss3;
    oss3 << original_signed;
    const auto original_signed_str {oss3.str()};

    std::istringstream iss3 {original_signed_str};
    int256 round_tripped_signed {};
    iss3 >> round_tripped_signed;

    std::cout << "Original:      " << original_signed << std::endl;
    std::cout << "As string:     \"" << original_signed_str << "\"" << std::endl;
    std::cout << "Round-tripped: " << round_tripped_signed << std::endl;
    std::cout << "Match: " << std::boolalpha << (original_signed == round_tripped_signed) << std::endl;

    return 0;
}
