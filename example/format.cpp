// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// This example demonstrates std::format support for uint256.
// Including <boost/int256/format.hpp> defines the std::formatter specialization,
// and BOOST_INT256_HAS_FORMAT reports whether C++20 <format> was available.
//
// Apart from the final section, this is the same program as fmt_format.cpp with
// fmt:: replaced by std::, and those lines of output are identical.

#include <boost/int256/int256.hpp>
#include <boost/int256/format.hpp>
#include <boost/int256/limits.hpp>

// tag::exclude[]
#ifdef BOOST_INT256_HAS_FORMAT

// end::exclude[]
#include <format>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>

#ifdef BOOST_INT256_HAS_CONSTEXPR_FORMAT

// With C++26 constexpr std::format the entire call happens at compile time
static_assert(std::format("{:#x}", boost::int256::uint256 {255U}) == "0xff");
static_assert(std::format("{}", boost::int256::uint256 {42U}) == "42");
static_assert(std::format("{:+}", boost::int256::int256 {-42}) == "-42");

#endif

int main()
{
    using boost::int256::uint256;

    std::cout << "=== Basic Formatting ===" << std::endl;

    constexpr uint256 wide_value {UINT64_C(0xDEADBEEF), UINT64_C(0xCAFEBABE12345678),
                                  UINT64_C(0x0F1E2D3C4B5A6978), UINT64_C(0x8796A5B4C3D2E1F0)};

    // Default decimal formatting
    std::cout << std::format("Default (decimal): {}", wide_value) << std::endl;
    std::cout << std::format("Small value: {}", uint256 {42U}) << std::endl;

    std::cout << "\n=== Base Specifiers ===" << std::endl;

    // Different bases: binary, octal, decimal, hex
    constexpr uint256 value {255U};
    std::cout << std::format("Binary:      {:b}", value) << std::endl;
    std::cout << std::format("Octal:       {:o}", value) << std::endl;
    std::cout << std::format("Decimal:     {:d}", value) << std::endl;
    std::cout << std::format("Hexadecimal: {:x}", value) << std::endl;
    std::cout << std::format("Hex (upper): {:X}", value) << std::endl;

    std::cout << "\n=== Alternate Form (Prefixes) ===" << std::endl;

    // Using # for alternate form adds base prefixes
    std::cout << std::format("Binary with prefix:  {:#b}", value) << std::endl;
    std::cout << std::format("Octal with prefix:   {:#o}", value) << std::endl;
    std::cout << std::format("Hex with prefix:     {:#x}", value) << std::endl;
    std::cout << std::format("Hex upper prefix:    {:#X}", value) << std::endl;

    std::cout << "\n=== Sign Options ===" << std::endl;

    // Sign specifiers: + (always show), - (default), space (space for positive)
    std::cout << std::format("Plus sign:  {:+}", value) << std::endl;
    std::cout << std::format("Minus only: {}", value) << std::endl;
    std::cout << std::format("Space sign: {: }", value) << std::endl;

    std::cout << "\n=== Zero Padding ===" << std::endl;

    // Padding with zeros (no alignment specifier)
    std::cout << std::format("8-digit padding:  {:08}", value) << std::endl;
    std::cout << std::format("16-digit padding: {:016}", value) << std::endl;

    std::cout << "\n=== Alignment ===" << std::endl;

    // Left, right, and center alignment with default fill (space)
    std::cout << std::format("Left align:   '{:<10}'", value) << std::endl;
    std::cout << std::format("Right align:  '{:>10}'", value) << std::endl;
    std::cout << std::format("Center align: '{:^10}'", value) << std::endl;

    std::cout << "\n=== Alignment with Fill Characters ===" << std::endl;

    // Custom fill characters
    std::cout << std::format("Left with *:   '{:*<10}'", value) << std::endl;
    std::cout << std::format("Right with 0:  '{:0>10}'", value) << std::endl;
    std::cout << std::format("Center with -: '{:-^10}'", value) << std::endl;

    std::cout << "\n=== Alignment with Hex and Prefix ===" << std::endl;

    // Alignment with base specifiers and prefixes
    std::cout << std::format("Right align hex:  '{:>10x}'", value) << std::endl;
    std::cout << std::format("Left align hex:   '{:<10x}'", value) << std::endl;
    std::cout << std::format("Center with prefix: '{:*^12x}'", value) << std::endl;

    std::cout << "\n=== Large Values ===" << std::endl;

    // Demonstrate with values beyond the 64-bit and 128-bit ranges
    constexpr auto max_value {(std::numeric_limits<uint256>::max)()};

    std::cout << std::format("uint256 max:       {}", max_value) << std::endl;
    std::cout << std::format("uint256 max (hex): {:#x}", max_value) << std::endl;
    std::cout << std::format("2^200:             {}", uint256 {1U} << 200U) << std::endl;

    // A width wider than the number pads it, which is how the 78-digit decimal and the
    // 64-digit hex forms line up in a table
    std::cout << std::format("Padded to 80:  '{:>80}'", uint256 {1U} << 200U) << std::endl;
    std::cout << std::format("Zero-filled:   '{:064x}'", uint256 {1U} << 200U) << std::endl;

    std::cout << "\n=== Combined Format Specifiers ===" << std::endl;

    // Combining multiple specifiers
    std::cout << std::format("Hex with prefix, uppercase, padded: {:#070X}", wide_value) << std::endl;
    std::cout << std::format("Decimal with plus, padded: {:+080}", wide_value) << std::endl;

    std::cout << "\n=== Formatting Into an Existing Buffer ===" << std::endl;

    // formatted_size gives the exact length, and format_to writes through any
    // output iterator, so no intermediate std::string is required
    const auto length {std::formatted_size("{:#x}", max_value)};
    std::string buffer {};
    buffer.reserve(length);
    std::format_to(std::back_inserter(buffer), "{:#x}", max_value);

    std::cout << std::format("formatted_size: {}", length) << std::endl;
    std::cout << std::format("format_to:      {}", buffer) << std::endl;
    std::cout << std::format("Lengths match:  {}", buffer.size() == length) << std::endl;

    std::cout << "\n=== int256 Formatting ===" << std::endl;

    using boost::int256::int256;

    constexpr int256 signed_value {-123456789012345678LL};
    std::cout << std::format("Signed value: {}", signed_value) << std::endl;

    constexpr int256 positive {42};
    constexpr int256 negative {-42};

    // Sign specifiers work the same as for uint256, with the sign int256
    // already carries taking the place of the always-absent uint256 sign
    std::cout << std::format("Plus sign:  {:+} and {:+}", positive, negative) << std::endl;
    std::cout << std::format("Minus only: {} and {}", positive, negative) << std::endl;
    std::cout << std::format("Space sign: {: } and {: }", positive, negative) << std::endl;

    std::cout << "\n=== Sign-Magnitude in Every Base ===" << std::endl;

    // The sign always comes before the base prefix and any zero padding, so
    // '-0xff' and '-00000042', never '0x-ff'
    std::cout << std::format("Negative hex with prefix: {:#x}", int256{-255}) << std::endl;
    std::cout << std::format("Negative, zero-padded:    {:09}", int256{-42}) << std::endl;

    std::cout << "\n=== Alignment with Sign ===" << std::endl;

    std::cout << std::format("Right align +:  '{:>+10}'", positive) << std::endl;
    std::cout << std::format("Left align +:   '{:<+10}'", positive) << std::endl;
    std::cout << std::format("Center align +: '{:^+11}'", positive) << std::endl;
    std::cout << std::format("Right align -:  '{:*>10}'", negative) << std::endl;

    std::cout << "\n=== Large Signed Values ===" << std::endl;

    constexpr auto min_value {(std::numeric_limits<int256>::min)()};
    std::cout << std::format("int256 min:       {}", min_value) << std::endl;
    std::cout << std::format("int256 min (hex): {:#x}", min_value) << std::endl;

    return 0;
}
// tag::exclude[]

#else

#include <iostream>

int main()
{
    std::cout << "This example requires C++20 <format> support." << std::endl;
    return 0;
}

#endif
// end::exclude[]
