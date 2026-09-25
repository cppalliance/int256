// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256.hpp>
#include <cstdint>
#include <iostream>

int main()
{
    // Arithmetic and comparison between uint256 and the built-in integer types
    // follow the C++ usual arithmetic conversions, identical to what the built-in
    // unsigned __int128 does with a narrower operand.

    using boost::int256::uint256;

    std::cout << "=== Arithmetic with built-in integers ===" << std::endl;

    constexpr uint256 value {3U};
    std::cout << "value = " << value << std::endl;

    std::cout << "value + 1  = " << (value + 1) << std::endl;
    std::cout << "value - 1  = " << (value - 1) << std::endl;
    std::cout << "value * 2  = " << (value * 2) << std::endl;
    std::cout << "value / 3  = " << (value / 3) << std::endl;
    std::cout << "value % 3  = " << (value % 3) << std::endl;
    std::cout << "10 - value = " << (10 - value) << std::endl;
    std::cout << "10 / value = " << (10 / value) << std::endl;

    std::cout << "\n=== A signed operand is converted, not compared as signed ===" << std::endl;

    // A negative operand becomes the unsigned value it represents modulo 2^256, so
    // subtracting a negative adds, and a comparison against a negative is false. This
    // is exactly what a built-in unsigned type does with a negative operand.
    std::cout << std::boolalpha;
    std::cout << "value - (-1)   = " << (value - -1) << std::endl;
    std::cout << "value > -1     = " << (value > -1) << " (the -1 becomes 2^256 - 1)" << std::endl;
    std::cout << "uint256{0} - 1 = " << (uint256{0U} - 1) << std::endl;
    std::cout << "(use cmp_greater from <boost/int256/utilities.hpp> for the mathematical answer)"
              << std::endl;

    std::cout << "\n=== The result type is always the wider one ===" << std::endl;

    // A 256-bit operand on either side means the operation happens in 256 bits, so
    // a product that would overflow the narrow type does not overflow here
    const std::uint64_t narrow {UINT64_MAX};
    std::cout << "UINT64_MAX * uint256{UINT64_MAX} = " << (narrow * uint256{narrow}) << std::endl;
    std::cout << "UINT64_MAX * UINT64_MAX in 64 bits = " << (narrow * narrow) << " (wrapped)" << std::endl;

    std::cout << "\n=== Compound Assignment onto a Built-in Integer ===" << std::endl;

    // A built-in integer accepts a uint256 right operand for every compound assignment.
    // The operation happens in the common type and the result is converted back, so the
    // left operand keeps its own type exactly as it would with a built-in __int128
    unsigned flags {0};
    flags |= uint256{1U};
    std::cout << "unsigned{0} |= uint256{1} -> " << flags << std::endl;

    int counter {12};
    counter += uint256{5U};
    std::cout << "int{12} += uint256{5} -> " << counter << std::endl;

    // A right operand wider than the left operand is not truncated before the operation:
    // the sum is formed in 256 bits and only the result is narrowed
    std::uint32_t word {0};
    word += (uint256{1U} << 64U) + uint256{7U};
    std::cout << "uint32_t{0} += 2^64 + 7 -> " << word << std::endl;

    // A shift takes only its count from the 256-bit operand, so the value shifted and the
    // type of the result are those of the left operand
    std::uint64_t bits {1};
    bits <<= uint256{40U};
    std::cout << "uint64_t{1} <<= uint256{40} -> " << bits << std::endl;

    std::cout << "\n=== Conversions ===" << std::endl;

    // The conversion to a narrower integer keeps the low bits, like a static_cast
    // between built-in integers
    const uint256 wide {(uint256{1U} << 128U) + 12345U};
    std::cout << "static_cast<std::uint64_t>(2^128 + 12345) = "
              << static_cast<std::uint64_t>(wide) << std::endl;

    // operator bool is explicit, so a uint256 works in a condition but not in arithmetic
    std::cout << "static_cast<bool>(uint256{0}) = " << static_cast<bool>(uint256{0U}) << std::endl;
    std::cout << "static_cast<bool>(wide)       = " << static_cast<bool>(wide) << std::endl;

    std::cout << "\n=== Mixed Type Arithmetic with int256 ===" << std::endl;

    using boost::int256::int256;

    constexpr int256 signed_value {-3};
    std::cout << "signed_value = " << signed_value << std::endl;

    // Every built-in integer fits inside int256's range, so the operation is
    // always mathematically exact: there is no mixed sign trap here the way
    // there is between uint256 and a negative builtin above.
    std::cout << "signed_value + 1U = " << (signed_value + 1U) << std::endl;
    std::cout << "signed_value - 4U = " << (signed_value - 4U) << std::endl;
    std::cout << "signed_value * 2  = " << (signed_value * 2) << std::endl;
    std::cout << "signed_value / 4U = " << (signed_value / 4U) << std::endl;

    std::cout << "\n=== int256 and uint256 Follow the Same Rule as uint256 and a Builtin ===" << std::endl;

    // int256 and uint256 use the usual arithmetic conversions with each other
    // too: both become uint256, so the same "converted, not compared as
    // signed" trap from above applies between the two library types as well.
    std::cout << "int256{-1} < uint256{1}   = " << (int256{-1} < uint256{1U})
              << " (the -1 becomes uint256 max)" << std::endl;
    std::cout << "int256{-1} + uint256{1}   = " << (int256{-1} + uint256{1U})
              << " (result type is uint256)" << std::endl;

    std::cout << "\n=== Compound Assignment Between int256 and uint256 ===" << std::endl;

    // int256's ten operator@=(int256&, const uint256&) overloads are the one
    // place this library computes a compound assignment in a domain other
    // than *this's own type: dividing or shifting by a uint256 happens
    // entirely in the uint256 domain, and only the final result converts
    // back, matching what long long /= unsigned long long does at 64 bits.
    // (Boost.Int128's int128 /= uint128 has no such overload and divides
    // signed instead, which this library does not repeat.)
    int256 dividend {-10};
    dividend /= uint256{3U};
    std::cout << "int256{-10} /= uint256{3} -> " << dividend
              << " ((uint256)-10 / 3, converted back)" << std::endl;

    return 0;
}
