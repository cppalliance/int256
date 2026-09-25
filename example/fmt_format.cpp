// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// This example demonstrates {fmt} library integration with uint256.
// Requires {fmt} to be installed: https://github.com/fmtlib/fmt
//
// For C++20 <format> support use <boost/int256/format.hpp> instead. See
// format.cpp, which is this same program with fmt:: replaced by std::.

// tag::exclude[]
#define FMT_HEADER_ONLY
// end::exclude[]
#include <boost/int256/int256.hpp>
#include <boost/int256/fmt_format.hpp>
#include <boost/int256/limits.hpp>

// tag::exclude[]
#ifdef BOOST_INT256_HAS_FMT_FORMAT

// end::exclude[]
#include <fmt/format.h>
#include <iostream>
#include <limits>

int main()
{
    using boost::int256::uint256;

    std::cout << "=== Basic Formatting ===" << std::endl;

    constexpr uint256 wide_value {UINT64_C(0xDEADBEEF), UINT64_C(0xCAFEBABE12345678),
                                  UINT64_C(0x0F1E2D3C4B5A6978), UINT64_C(0x8796A5B4C3D2E1F0)};

    // Default decimal formatting
    std::cout << fmt::format("Default (decimal): {}", wide_value) << std::endl;
    std::cout << fmt::format("Small value: {}", uint256 {42U}) << std::endl;

    std::cout << "\n=== Base Specifiers ===" << std::endl;

    // Different bases: binary, octal, decimal, hex
    constexpr uint256 value {255U};
    std::cout << fmt::format("Binary:      {:b}", value) << std::endl;
    std::cout << fmt::format("Octal:       {:o}", value) << std::endl;
    std::cout << fmt::format("Decimal:     {:d}", value) << std::endl;
    std::cout << fmt::format("Hexadecimal: {:x}", value) << std::endl;
    std::cout << fmt::format("Hex (upper): {:X}", value) << std::endl;

    std::cout << "\n=== Alternate Form (Prefixes) ===" << std::endl;

    // Using # for alternate form adds base prefixes
    std::cout << fmt::format("Binary with prefix:  {:#b}", value) << std::endl;
    std::cout << fmt::format("Octal with prefix:   {:#o}", value) << std::endl;
    std::cout << fmt::format("Hex with prefix:     {:#x}", value) << std::endl;
    std::cout << fmt::format("Hex upper prefix:    {:#X}", value) << std::endl;

    std::cout << "\n=== Sign Options ===" << std::endl;

    // Sign specifiers: + (always show), - (default), space (space for positive)
    std::cout << fmt::format("Plus sign:  {:+}", value) << std::endl;
    std::cout << fmt::format("Minus only: {}", value) << std::endl;
    std::cout << fmt::format("Space sign: {: }", value) << std::endl;

    std::cout << "\n=== Zero Padding ===" << std::endl;

    // Padding with zeros (no alignment specifier)
    std::cout << fmt::format("8-digit padding:  {:08}", value) << std::endl;
    std::cout << fmt::format("16-digit padding: {:016}", value) << std::endl;

    std::cout << "\n=== Alignment ===" << std::endl;

    // Left, right, and center alignment with default fill (space)
    std::cout << fmt::format("Left align:   '{:<10}'", value) << std::endl;
    std::cout << fmt::format("Right align:  '{:>10}'", value) << std::endl;
    std::cout << fmt::format("Center align: '{:^10}'", value) << std::endl;

    std::cout << "\n=== Alignment with Fill Characters ===" << std::endl;

    // Custom fill characters
    std::cout << fmt::format("Left with *:   '{:*<10}'", value) << std::endl;
    std::cout << fmt::format("Right with 0:  '{:0>10}'", value) << std::endl;
    std::cout << fmt::format("Center with -: '{:-^10}'", value) << std::endl;

    std::cout << "\n=== Alignment with Hex and Prefix ===" << std::endl;

    // Alignment with base specifiers and prefixes
    std::cout << fmt::format("Right align hex:  '{:>10x}'", value) << std::endl;
    std::cout << fmt::format("Left align hex:   '{:<10x}'", value) << std::endl;
    std::cout << fmt::format("Center with prefix: '{:*^12x}'", value) << std::endl;

    std::cout << "\n=== Large Values ===" << std::endl;

    // Demonstrate with values beyond the 64-bit and 128-bit ranges
    constexpr auto max_value {(std::numeric_limits<uint256>::max)()};

    std::cout << fmt::format("uint256 max:       {}", max_value) << std::endl;
    std::cout << fmt::format("uint256 max (hex): {:#x}", max_value) << std::endl;
    std::cout << fmt::format("2^200:             {}", uint256 {1U} << 200U) << std::endl;

    // A width wider than the number pads it, which is how the 78-digit decimal and the
    // 64-digit hex forms line up in a table
    std::cout << fmt::format("Padded to 80:  '{:>80}'", uint256 {1U} << 200U) << std::endl;
    std::cout << fmt::format("Zero-filled:   '{:064x}'", uint256 {1U} << 200U) << std::endl;

    std::cout << "\n=== Combined Format Specifiers ===" << std::endl;

    // Combining multiple specifiers
    std::cout << fmt::format("Hex with prefix, uppercase, padded: {:#070X}", wide_value) << std::endl;
    std::cout << fmt::format("Decimal with plus, padded: {:+080}", wide_value) << std::endl;

    std::cout << "\n=== int256 Formatting ===" << std::endl;

    using boost::int256::int256;

    constexpr int256 signed_value {-123456789012345678LL};
    std::cout << fmt::format("Signed value: {}", signed_value) << std::endl;

    constexpr int256 positive {42};
    constexpr int256 negative {-42};

    std::cout << fmt::format("Plus sign:  {:+} and {:+}", positive, negative) << std::endl;
    std::cout << fmt::format("Minus only: {} and {}", positive, negative) << std::endl;
    std::cout << fmt::format("Space sign: {: } and {: }", positive, negative) << std::endl;

    std::cout << "\n=== Sign-Magnitude in Every Base ===" << std::endl;

    // The sign always comes before the base prefix and any zero padding, so
    // '-0xff' and '-00000042', never '0x-ff'
    std::cout << fmt::format("Negative hex with prefix: {:#x}", int256{-255}) << std::endl;
    std::cout << fmt::format("Negative, zero-padded:    {:09}", int256{-42}) << std::endl;

    std::cout << "\n=== Alignment with Sign ===" << std::endl;

    std::cout << fmt::format("Right align +:  '{:>+10}'", positive) << std::endl;
    std::cout << fmt::format("Left align +:   '{:<+10}'", positive) << std::endl;
    std::cout << fmt::format("Center align +: '{:^+11}'", positive) << std::endl;
    std::cout << fmt::format("Right align -:  '{:*>10}'", negative) << std::endl;

    std::cout << "\n=== Large Signed Values ===" << std::endl;

    constexpr auto min_value {(std::numeric_limits<int256>::min)()};
    std::cout << fmt::format("int256 min:       {}", min_value) << std::endl;
    std::cout << fmt::format("int256 min (hex): {:#x}", min_value) << std::endl;

    return 0;
}
// tag::exclude[]

#else

#include <iostream>

int main()
{
    std::cout << "This example requires the {fmt} library to be installed." << std::endl;
    std::cout << "Install from: https://github.com/fmtlib/fmt" << std::endl;
    return 0;
}

#endif
// end::exclude[]
