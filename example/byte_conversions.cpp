// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>
#include <boost/int256/byte_conversions.hpp>
#include <boost/int256/iostream.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>

// Prints the bytes of an array in the order they are stored
template <typename Bytes>
void print_bytes(const char* label, const Bytes& bytes)
{
    std::cout << label;

    for (const auto byte : bytes)
    {
        std::cout << ' ' << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(byte);
    }

    std::cout << std::dec << std::endl;
}

int main()
{
    using boost::int256::uint256;
    using boost::int256::int256;

    // The 32 bytes 01 02 ... 20 read as a big-endian value, four words most
    // significant first
    constexpr uint256 value {UINT64_C(0x0102030405060708), UINT64_C(0x090A0B0C0D0E0F10),
                             UINT64_C(0x1112131415161718), UINT64_C(0x191A1B1C1D1E1F20)};

    std::cout << "=== Byte arrays ===" << std::endl;

    // The byte order of the array is the requested one on every platform
    print_bytes("to_be_bytes:", boost::int256::to_be_bytes(value));
    print_bytes("to_le_bytes:", boost::int256::to_le_bytes(value));

    // Native order is the object representation of the value. On a little-endian host
    // it matches to_le_bytes; on a big-endian host the bytes of each word are big-endian
    // while the words themselves stay least significant first, because the word order
    // of uint256 is fixed on every platform.
    print_bytes("to_ne_bytes:", boost::int256::to_ne_bytes(value));

    std::cout << "\n=== Reading a value back out of bytes ===" << std::endl;

    constexpr std::array<std::uint8_t, sizeof(uint256)> wire
    {{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x2C
    }};

    // The target type is given explicitly, and the byte count has to match
    std::cout << "from_be_bytes: " << boost::int256::from_be_bytes<uint256>(wire) << std::endl;
    std::cout << "from_le_bytes: " << boost::int256::from_le_bytes<uint256>(wire) << std::endl;

    // Everything is constexpr, so a whole round trip can be checked at compile time
    static_assert(boost::int256::from_be_bytes<uint256>(boost::int256::to_be_bytes(value)) == value,
                  "Round trip through big-endian bytes");
    static_assert(boost::int256::from_le_bytes<uint256>(boost::int256::to_le_bytes(value)) == value,
                  "Round trip through little-endian bytes");
    static_assert(boost::int256::from_ne_bytes<uint256>(boost::int256::to_ne_bytes(value)) == value,
                  "Round trip through native bytes");

    std::cout << "\n=== Signed values ===" << std::endl;

    // The two's complement bit pattern is what gets reversed, so a negative
    // int256 needs no special handling: the sign bit is just bit 63 of the
    // last byte written (or the first, for to_be_bytes).
    constexpr int256 negative {-300};

    print_bytes("to_be_bytes(-300):", boost::int256::to_be_bytes(negative));
    std::cout << "from_be_bytes:     "
              << boost::int256::from_be_bytes<int256>(boost::int256::to_be_bytes(negative)) << std::endl;

    static_assert(boost::int256::from_be_bytes<int256>(boost::int256::to_be_bytes(negative)) == negative,
                  "Round trip through big-endian bytes (signed)");

    // uint256 and int256 share the same 32-byte layout, so the same wire bytes
    // convert to either type; only the interpretation of the top bit differs
    constexpr std::array<std::uint8_t, sizeof(int256)> negative_one_wire
    {{
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    }};
    std::cout << "As uint256 (max): " << boost::int256::from_be_bytes<uint256>(negative_one_wire) << std::endl;
    std::cout << "As int256 (-1):   " << boost::int256::from_be_bytes<int256>(negative_one_wire) << std::endl;

    std::cout << "\n=== Whole value conversions ===" << std::endl;

    // to_be and to_le produce a value whose object representation is in the
    // requested order, which is what a memcpy into a packet buffer wants.
    // The value itself is only meaningful again after the matching from_be / from_le.
    const auto big_endian_image {boost::int256::to_be(value)};

    print_bytes("object representation of to_be(value):", boost::int256::to_ne_bytes(big_endian_image));
    std::cout << "from_be recovers: " << boost::int256::from_be(big_endian_image) << std::endl;
    std::cout << "value:            " << value << std::endl;

    // Any byte-like element type can be requested, which is convenient when the
    // surrounding buffer is not made of std::uint8_t
    const auto as_char {boost::int256::to_le_bytes<char>(value)};
    std::cout << "\nfrom_le_bytes over a char buffer: "
              << boost::int256::from_le_bytes<uint256>(as_char.data()) << std::endl;

    // A 256-bit hash is the obvious case: the digest arrives as 32 big-endian bytes
    // and becomes an integer that can be compared, ordered, and reduced
    constexpr std::array<std::uint8_t, 32> digest
    {{
        0xE3, 0xB0, 0xC4, 0x42, 0x98, 0xFC, 0x1C, 0x14,
        0x9A, 0xFB, 0xF4, 0xC8, 0x99, 0x6F, 0xB9, 0x24,
        0x27, 0xAE, 0x41, 0xE4, 0x64, 0x9B, 0x93, 0x4C,
        0xA4, 0x95, 0x99, 0x1B, 0x78, 0x52, 0xB8, 0x55
    }};

    const auto digest_value {boost::int256::from_be_bytes<uint256>(digest)};
    std::cout << "\nSHA-256 of the empty string as an integer:" << std::endl;
    std::cout << "  decimal: " << digest_value << std::endl;
    std::cout << "  hex:     " << std::hex << digest_value << std::dec << std::endl;
    std::cout << "  mod 10^9 + 7: " << digest_value % 1000000007U << std::endl;

    return 0;
}
