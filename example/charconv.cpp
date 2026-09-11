// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>
#include <boost/int256/iostream.hpp>
#include <boost/int256/charconv.hpp>
#include <boost/int256/limits.hpp>
#include <boost/charconv.hpp>
#include <cstring>
#include <iostream>
#include <limits>
#include <system_error>

int main()
{
    using boost::int256::uint256;

    // A 256-bit value needs 78 characters in base 10 and 256 in base 2, so size the
    // buffer for the base being used
    char buffer[260];

    // === to_chars: Convert integers to character strings ===
    std::cout << "=== to_chars ===" << std::endl;

    constexpr auto max_value {(std::numeric_limits<uint256>::max)()};
    auto result {boost::charconv::to_chars(buffer, buffer + sizeof(buffer), max_value)};
    *result.ptr = '\0';
    std::cout << "uint256 max (decimal): " << buffer << std::endl;
    std::cout << "  characters written:  " << (result.ptr - buffer) << std::endl;

    // Hexadecimal output (base 16)
    constexpr uint256 hex_value {UINT64_C(0xDEADBEEF), UINT64_C(0xCAFEBABE12345678),
                                 UINT64_C(0x0F1E2D3C4B5A6978), UINT64_C(0x8796A5B4C3D2E1F0)};
    result = boost::charconv::to_chars(buffer, buffer + sizeof(buffer), hex_value, 16);
    *result.ptr = '\0';
    std::cout << "uint256 (hex): 0x" << buffer << std::endl;

    // Octal output (base 8)
    result = boost::charconv::to_chars(buffer, buffer + sizeof(buffer), uint256{511U}, 8);
    *result.ptr = '\0';
    std::cout << "uint256 511 (octal): 0" << buffer << std::endl;

    // Binary output (base 2), which is where the 256-character buffer matters
    result = boost::charconv::to_chars(buffer, buffer + sizeof(buffer), max_value, 2);
    *result.ptr = '\0';
    std::cout << "uint256 max in base 2 is " << (result.ptr - buffer) << " characters" << std::endl;

    // A buffer that is too small is reported rather than truncated
    char small[10];
    const auto too_small {boost::charconv::to_chars(small, small + sizeof(small), max_value)};
    std::cout << "to_chars into 10 chars: " << std::boolalpha << static_cast<bool>(too_small)
              << " (ec == value_too_large: "
              << (too_small.ec == std::errc::value_too_large) << ")" << std::endl;

    // === from_chars: Parse character strings to integers ===
    std::cout << "\n=== from_chars ===" << std::endl;

    const char* decimal_str {"115792089237316195423570985008687907853269984665640564039457584007913129639935"};
    uint256 parsed {};
    auto parse {boost::charconv::from_chars(decimal_str, decimal_str + std::strlen(decimal_str), parsed)};
    std::cout << "Parsed the 78-digit max:" << std::endl;
    std::cout << "  Result: " << parsed << std::endl;
    std::cout << "  Equals max? " << (parsed == max_value) << std::endl;
    std::cout << "  Consumed every character? " << (parse.ptr == decimal_str + std::strlen(decimal_str))
              << std::endl;

    // Parse hexadecimal (base 16)
    const char* hex_str {"DEADBEEFCAFEBABE123456780F1E2D3C4B5A69788796A5B4C3D2E1F0"};
    uint256 parsed_hex {};
    boost::charconv::from_chars(hex_str, hex_str + std::strlen(hex_str), parsed_hex, 16);
    std::cout << "\nParsed hex \"" << hex_str << "\"" << std::endl;
    std::cout << "  Result: " << parsed_hex << std::endl;
    std::cout << "  Round trips: " << (parsed_hex == hex_value) << std::endl;

    // One past the max is rejected rather than wrapped
    std::cout << "\n=== Error handling ===" << std::endl;
    const char* overflow_str {"115792089237316195423570985008687907853269984665640564039457584007913129639936"};
    uint256 unused {};
    const auto overflow {boost::charconv::from_chars(overflow_str,
                                                     overflow_str + std::strlen(overflow_str), unused)};
    std::cout << "max + 1 parses: " << static_cast<bool>(overflow)
              << " (ec == result_out_of_range: " << (overflow.ec == std::errc::result_out_of_range) << ")"
              << std::endl;

    const char* invalid_str {"not a number"};
    const auto invalid {boost::charconv::from_chars(invalid_str,
                                                    invalid_str + std::strlen(invalid_str), unused)};
    std::cout << "\"not a number\" parses: " << static_cast<bool>(invalid)
              << " (ec == invalid_argument: " << (invalid.ec == std::errc::invalid_argument) << ")"
              << std::endl;

    // Parsing stops at the first character that is not a digit of the base
    const char* prefixed {"0x10"};
    uint256 stopped {};
    const auto partial {boost::charconv::from_chars(prefixed, prefixed + std::strlen(prefixed), stopped, 16)};
    std::cout << "\"0x10\" in base 16: value " << stopped << ", stopped at '" << *partial.ptr << "'"
              << std::endl;

    return 0;
}
