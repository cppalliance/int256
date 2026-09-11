// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>
#include <boost/int256/iostream.hpp>
#include <iostream>

int main()
{
    // uint256 supports every arithmetic operation one would expect, against another
    // uint256 and against any built-in integer type.
    // See `mixed_type_arithmetic.cpp` for operations with the built-in types.

    using boost::int256::uint256;

    std::cout << "=== The 128 by 128 bit product ===" << std::endl;

    // The exact product of two 128-bit values needs 256 bits, which is the case this
    // library exists for: a 128-bit type wraps here, and a 64-bit type is not even close.
    const uint256 a {UINT64_C(0), UINT64_C(0), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFF)};
    const uint256 b {a};

    std::cout << "a = 2^128 - 1 = " << a << std::endl;
    std::cout << "a * a         = " << (a * a) << std::endl;
    std::cout << "  (that is 2^256 - 2^129 + 1, and it needs all four words)" << std::endl;

    std::cout << "\n=== Addition, subtraction, multiplication ===" << std::endl;

    const uint256 x {1000000000000ULL};  // 1 trillion
    const uint256 y {999999999999ULL};   // Just under 1 trillion

    std::cout << "x = " << x << std::endl;
    std::cout << "y = " << y << std::endl;
    std::cout << "x + y = " << (x + y) << std::endl;
    std::cout << "x - y = " << (x - y) << std::endl;
    std::cout << "x * y = " << (x * y) << std::endl;

    std::cout << "\n=== Division and modulo ===" << std::endl;

    // Division returns the truncated quotient, exactly as it does for a built-in
    // unsigned type. See cstdlib.cpp for computing both halves in one division.
    std::cout << "x / 7 = " << (x / 7U) << std::endl;
    std::cout << "x % 7 = " << (x % 7U) << std::endl;
    std::cout << "(a * a) / a = " << ((a * a) / a) << std::endl;
    std::cout << "(a * a) % a = " << ((a * a) % a) << std::endl;

    std::cout << "\n=== Comparisons ===" << std::endl;

    std::cout << "x > y:  " << std::boolalpha << (x > y) << std::endl;
    std::cout << "x == y: " << (x == y) << std::endl;
    std::cout << "x != y: " << (x != y) << std::endl;
    std::cout << "b == a: " << (b == a) << std::endl;

    std::cout << "\n=== Compound assignment operators ===" << std::endl;

    uint256 z {100U};
    std::cout << "z = " << z << std::endl;

    z += 50U;
    std::cout << "z += 50: " << z << std::endl;

    z -= 25U;
    std::cout << "z -= 25: " << z << std::endl;

    z *= 2U;
    std::cout << "z *= 2:  " << z << std::endl;

    z /= 5U;
    std::cout << "z /= 5:  " << z << std::endl;

    z %= 7U;
    std::cout << "z %= 7:  " << z << std::endl;

    std::cout << "\n=== Increment and decrement ===" << std::endl;

    uint256 counter {10U};
    std::cout << "counter = " << counter << std::endl;
    std::cout << "++counter = " << ++counter << std::endl;
    std::cout << "counter++ = " << counter++ << std::endl;
    std::cout << "counter = " << counter << std::endl;
    std::cout << "--counter = " << --counter << std::endl;

    std::cout << "\n=== Unary operators ===" << std::endl;

    // Negation is modulo 2^256, so it is the two's complement of the value.
    const uint256 one {1U};
    std::cout << "-one = " << -one << std::endl;
    std::cout << "~one = " << ~one << std::endl;

    // abs is provided for parity with the signed type and returns its argument.
    std::cout << "abs(x) = " << boost::int256::abs(x) << std::endl;

    return 0;
}
