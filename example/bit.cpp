// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Individual headers

#include <boost/int256/int256.hpp>
#include <boost/int256/bit.hpp>
#include <boost/int256/iostream.hpp>

// Or you can do a single header

// #include <boost/int256.hpp>

// tag::exclude[]
#if defined(__GNUC__) && __GNUC__ <= 7 && !defined(__clang__)

#include <iostream>

int main()
{
    // The following does not work in a constexpr way for old GCCs
    // Clang is fine
    std::cout << "skipped" << std::endl;
    return 0;
}

#else

#include <iostream>

// end::exclude[]
int main()
{
    using boost::int256::uint256;

    constexpr uint256 x {1U};

    // Every function in <boost/int256/bit.hpp> is constexpr

    // Does the value have only a single bit set?
    static_assert(boost::int256::has_single_bit(x), "Should have one bit");

    // How many zeros from the left
    static_assert(boost::int256::countl_zero(x) == 255, "Should be 255");

    // The bit width of the value
    // 1 + 1 is 10 in binary which is 2 bits wide
    static_assert(boost::int256::bit_width(x + x) == 2, "2 bits wide");

    // The largest power of two not greater than the input value
    static_assert(boost::int256::bit_floor(3U * x) == 2U, "2 < 3");

    // The smallest power of two not smaller than the input value
    static_assert(boost::int256::bit_ceil(5U * x) == 8U, "8 > 5");

    // How many zeros from the right?
    static_assert(boost::int256::countr_zero(2U * x) == 1, "1 zero to the right of 10");

    // How many 1-bits in the value
    static_assert(boost::int256::popcount(7U * x) == 3, "111");

    // The counts span all four words: a bit set in the top word is 192 bits from the bottom
    static_assert(boost::int256::countr_zero(x << 192U) == 192, "Top word");
    static_assert(boost::int256::countl_zero(x << 192U) == 63, "Top word");
    static_assert(boost::int256::bit_width(x << 255U) == 256, "The whole width");

    // Rotation is over the full 256 bits, and the count is taken modulo 256
    static_assert(boost::int256::rotl(x, 256) == x, "A full turn is the identity");
    static_assert(boost::int256::rotl(x, -1) == boost::int256::rotr(x, 1), "A negative count reverses");
    static_assert(boost::int256::rotr(x, 1) == (x << 255U), "One bit right of bit zero is bit 255");

    // Swap the bytes
    // Create a value with a distinct byte pattern, most significant word first
    constexpr uint256 original {UINT64_C(0x0123456789ABCDEF), UINT64_C(0xFEDCBA9876543210),
                                UINT64_C(0x1122334455667788), UINT64_C(0x99AABBCCDDEEFF00)};

    // Expected result after byteswap: every byte of the 32 is reversed, so the words
    // swap places and each one is individually byte reversed
    constexpr uint256 expected {UINT64_C(0x00FFEEDDCCBBAA99), UINT64_C(0x8877665544332211),
                                UINT64_C(0x1032547698BADCFE), UINT64_C(0xEFCDAB8967452301)};

    static_assert(boost::int256::byteswap(original) == expected, "Mismatched byteswap");
    static_assert(boost::int256::byteswap(expected) == original, "Mismatched byteswap");

    // The same functions of course work at run time
    std::cout << "popcount(max)        = " << boost::int256::popcount(~uint256{}) << std::endl;
    std::cout << "countl_zero(1)       = " << boost::int256::countl_zero(x) << std::endl;
    std::cout << "countr_zero(2^192)   = " << boost::int256::countr_zero(x << 192U) << std::endl;
    std::cout << "bit_width(2^255)     = " << boost::int256::bit_width(x << 255U) << std::endl;
    std::cout << "bit_ceil(5)          = " << boost::int256::bit_ceil(5U * x) << std::endl;
    std::cout << "has_single_bit(2^99) = " << std::boolalpha
              << boost::int256::has_single_bit(x << 99U) << std::endl;
    std::cout << "byteswap round trip  = "
              << (boost::int256::byteswap(boost::int256::byteswap(original)) == original) << std::endl;

    return 0;
}
// tag::exclude[]

#endif
// end::exclude[]
