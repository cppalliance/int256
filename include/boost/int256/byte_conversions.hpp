// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_BYTE_CONVERSIONS_HPP
#define BOOST_INT256_BYTE_CONVERSIONS_HPP

#include <boost/int256/int256.hpp>
#include <boost/int256/bit.hpp>
#include <boost/int256/detail/config.hpp>

#ifndef BOOST_INT256_BUILD_MODULE

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#endif

namespace boost {
namespace int256 {

namespace detail {

// detail::valid_overload / is_valid_overload_v (the library types that the byte
// conversion functions accept as a target) now live in detail/conversions.hpp, the
// single definition shared with every other header that needs to accept both uint256
// and int256

// Reverses the word order, leaving the bytes inside each word alone
BOOST_INT256_HOST_DEVICE constexpr uint256 reverse_words(const uint256& value) noexcept
{
    return uint256{value.words[0], value.words[1], value.words[2], value.words[3]};
}

// Reverses the bytes of each word in place, leaving the word order alone. A big-endian
// host needs this because its object representation is word ascending with big-endian
// bytes inside each word.
BOOST_INT256_HOST_DEVICE constexpr uint256 byteswap_each_word(const uint256& value) noexcept
{
    return reverse_words(byteswap(value));
}

} // namespace detail

//=====================================
// Whole value byte order conversions
//=====================================

// Returns an object whose object representation is the big-endian encoding of value.
// The word order of uint256 is fixed, so on a big-endian host that is a word reversal
// rather than the identity.
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 to_be(const uint256& value) noexcept
{
    #if BOOST_INT256_ENDIAN_BIG_BYTE

    return detail::reverse_words(value);

    #else

    return byteswap(value);

    #endif
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 from_be(const uint256& value) noexcept
{
    // Self-inverse
    return to_be(value);
}

// Returns an object whose object representation is the little-endian encoding of value
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 to_le(const uint256& value) noexcept
{
    #if BOOST_INT256_ENDIAN_LITTLE_BYTE

    return value;

    #else

    return detail::byteswap_each_word(value);

    #endif
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr uint256 from_le(const uint256& value) noexcept
{
    // Self-inverse
    return to_le(value);
}

// The signed twins: byte order has no notion of sign, so each of these is the
// corresponding uint256 overload run on the bit image and converted back
BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 to_be(const int256& value) noexcept
{
    return detail::to_signed_bits(to_be(detail::to_unsigned_bits(value)));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 from_be(const int256& value) noexcept
{
    // Self-inverse
    return to_be(value);
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 to_le(const int256& value) noexcept
{
    return detail::to_signed_bits(to_le(detail::to_unsigned_bits(value)));
}

BOOST_INT256_EXPORT BOOST_INT256_HOST_DEVICE constexpr int256 from_le(const int256& value) noexcept
{
    // Self-inverse
    return to_le(value);
}

namespace detail {

// The byte-like element types that the byte array functions operate on.
// std::byte only exists from C++17, and the library is usable from C++14.
template <typename T>
struct byte_like
{
    static constexpr bool value = std::is_same<T, char>::value ||
                                  std::is_same<T, signed char>::value ||
                                  std::is_same<T, unsigned char>::value
                                  #if defined(__cpp_lib_byte) && __cpp_lib_byte >= 201603L
                                  || std::is_same<T, std::byte>::value
                                  #endif
                                  ;
};

template <typename T>
BOOST_INT256_INLINE_CONSTEXPR bool is_byte_like_v = byte_like<T>::value;

// Reads the byte sitting at bit offset shift of a 64-bit word
template <typename ByteType>
BOOST_INT256_HOST_DEVICE constexpr ByteType extract_byte(const std::uint64_t word, const unsigned shift) noexcept
{
    return static_cast<ByteType>(static_cast<unsigned char>((word >> shift) & UINT64_C(0xFF)));
}

// Places a byte at bit offset shift of a 64-bit word
template <typename ByteType>
BOOST_INT256_HOST_DEVICE constexpr std::uint64_t insert_byte(const ByteType value, const unsigned shift) noexcept
{
    return static_cast<std::uint64_t>(static_cast<unsigned char>(value)) << shift;
}

// Rebuilds a 64-bit word from eight most significant byte first elements
template <typename Bytes>
BOOST_INT256_HOST_DEVICE constexpr std::uint64_t word_from_be(const Bytes& bytes, const std::size_t offset) noexcept
{
    return insert_byte(bytes[offset], 56U)     | insert_byte(bytes[offset + 1], 48U) |
           insert_byte(bytes[offset + 2], 40U) | insert_byte(bytes[offset + 3], 32U) |
           insert_byte(bytes[offset + 4], 24U) | insert_byte(bytes[offset + 5], 16U) |
           insert_byte(bytes[offset + 6], 8U)  | insert_byte(bytes[offset + 7], 0U);
}

// Rebuilds a 64-bit word from eight least significant byte first elements
template <typename Bytes>
BOOST_INT256_HOST_DEVICE constexpr std::uint64_t word_from_le(const Bytes& bytes, const std::size_t offset) noexcept
{
    return insert_byte(bytes[offset], 0U)      | insert_byte(bytes[offset + 1], 8U)  |
           insert_byte(bytes[offset + 2], 16U) | insert_byte(bytes[offset + 3], 24U) |
           insert_byte(bytes[offset + 4], 32U) | insert_byte(bytes[offset + 5], 40U) |
           insert_byte(bytes[offset + 6], 48U) | insert_byte(bytes[offset + 7], 56U);
}

// The byte arrays are built from shifts rather than from the object representation, so
// the be and le forms are exact on either endianness. Bytes is anything that can be
// indexed with the subscript operator: a std::array or a pointer.

// Writes the 32 bytes of a most significant byte first encoding
template <typename ByteType, typename Bytes>
BOOST_INT256_HOST_DEVICE constexpr void write_be_bytes(const uint256& value, Bytes& bytes) noexcept
{
    for (std::size_t word {}; word < 4U; ++word)
    {
        const auto w {value.words[3U - word]};
        for (std::size_t byte {}; byte < 8U; ++byte)
        {
            bytes[word * 8U + byte] = extract_byte<ByteType>(w, static_cast<unsigned>(56U - byte * 8U));
        }
    }
}

// Writes the 32 bytes of a least significant byte first encoding
template <typename ByteType, typename Bytes>
BOOST_INT256_HOST_DEVICE constexpr void write_le_bytes(const uint256& value, Bytes& bytes) noexcept
{
    for (std::size_t word {}; word < 4U; ++word)
    {
        const auto w {value.words[word]};
        for (std::size_t byte {}; byte < 8U; ++byte)
        {
            bytes[word * 8U + byte] = extract_byte<ByteType>(w, static_cast<unsigned>(byte * 8U));
        }
    }
}

// Writes the object representation: ascending word order, and the host byte order
// inside each word
template <typename ByteType, typename Bytes>
BOOST_INT256_HOST_DEVICE constexpr void write_ne_bytes(const uint256& value, Bytes& bytes) noexcept
{
    for (std::size_t word {}; word < 4U; ++word)
    {
        const auto w {value.words[word]};
        for (std::size_t byte {}; byte < 8U; ++byte)
        {
            #if BOOST_INT256_ENDIAN_LITTLE_BYTE
            bytes[word * 8U + byte] = extract_byte<ByteType>(w, static_cast<unsigned>(byte * 8U));
            #else
            bytes[word * 8U + byte] = extract_byte<ByteType>(w, static_cast<unsigned>(56U - byte * 8U));
            #endif
        }
    }
}

// std::array::operator[] is not constexpr before C++17, so the array forms are built
// as a braced list of 32 extract_byte calls instead of a loop. These three are host only:
// std::array is not usable from a device pass, and marking them HOST_DEVICE would draw
// nvcc #20011-D / #20013-D. Device code uses the raw array overloads instead.
template <typename ByteType>
constexpr std::array<ByteType, sizeof(uint256)> to_be_bytes_impl(const uint256& v) noexcept
{
    return {{extract_byte<ByteType>(v.words[3], 56U), extract_byte<ByteType>(v.words[3], 48U),
             extract_byte<ByteType>(v.words[3], 40U), extract_byte<ByteType>(v.words[3], 32U),
             extract_byte<ByteType>(v.words[3], 24U), extract_byte<ByteType>(v.words[3], 16U),
             extract_byte<ByteType>(v.words[3], 8U),  extract_byte<ByteType>(v.words[3], 0U),
             extract_byte<ByteType>(v.words[2], 56U), extract_byte<ByteType>(v.words[2], 48U),
             extract_byte<ByteType>(v.words[2], 40U), extract_byte<ByteType>(v.words[2], 32U),
             extract_byte<ByteType>(v.words[2], 24U), extract_byte<ByteType>(v.words[2], 16U),
             extract_byte<ByteType>(v.words[2], 8U),  extract_byte<ByteType>(v.words[2], 0U),
             extract_byte<ByteType>(v.words[1], 56U), extract_byte<ByteType>(v.words[1], 48U),
             extract_byte<ByteType>(v.words[1], 40U), extract_byte<ByteType>(v.words[1], 32U),
             extract_byte<ByteType>(v.words[1], 24U), extract_byte<ByteType>(v.words[1], 16U),
             extract_byte<ByteType>(v.words[1], 8U),  extract_byte<ByteType>(v.words[1], 0U),
             extract_byte<ByteType>(v.words[0], 56U), extract_byte<ByteType>(v.words[0], 48U),
             extract_byte<ByteType>(v.words[0], 40U), extract_byte<ByteType>(v.words[0], 32U),
             extract_byte<ByteType>(v.words[0], 24U), extract_byte<ByteType>(v.words[0], 16U),
             extract_byte<ByteType>(v.words[0], 8U),  extract_byte<ByteType>(v.words[0], 0U)}};
}

template <typename ByteType>
constexpr std::array<ByteType, sizeof(uint256)> to_le_bytes_impl(const uint256& v) noexcept
{
    return {{extract_byte<ByteType>(v.words[0], 0U),  extract_byte<ByteType>(v.words[0], 8U),
             extract_byte<ByteType>(v.words[0], 16U), extract_byte<ByteType>(v.words[0], 24U),
             extract_byte<ByteType>(v.words[0], 32U), extract_byte<ByteType>(v.words[0], 40U),
             extract_byte<ByteType>(v.words[0], 48U), extract_byte<ByteType>(v.words[0], 56U),
             extract_byte<ByteType>(v.words[1], 0U),  extract_byte<ByteType>(v.words[1], 8U),
             extract_byte<ByteType>(v.words[1], 16U), extract_byte<ByteType>(v.words[1], 24U),
             extract_byte<ByteType>(v.words[1], 32U), extract_byte<ByteType>(v.words[1], 40U),
             extract_byte<ByteType>(v.words[1], 48U), extract_byte<ByteType>(v.words[1], 56U),
             extract_byte<ByteType>(v.words[2], 0U),  extract_byte<ByteType>(v.words[2], 8U),
             extract_byte<ByteType>(v.words[2], 16U), extract_byte<ByteType>(v.words[2], 24U),
             extract_byte<ByteType>(v.words[2], 32U), extract_byte<ByteType>(v.words[2], 40U),
             extract_byte<ByteType>(v.words[2], 48U), extract_byte<ByteType>(v.words[2], 56U),
             extract_byte<ByteType>(v.words[3], 0U),  extract_byte<ByteType>(v.words[3], 8U),
             extract_byte<ByteType>(v.words[3], 16U), extract_byte<ByteType>(v.words[3], 24U),
             extract_byte<ByteType>(v.words[3], 32U), extract_byte<ByteType>(v.words[3], 40U),
             extract_byte<ByteType>(v.words[3], 48U), extract_byte<ByteType>(v.words[3], 56U)}};
}

// The native form is the object representation: ascending word order with the host byte
// order inside each word, which is mixed-endian on a big-endian host
template <typename ByteType>
constexpr std::array<ByteType, sizeof(uint256)> to_ne_bytes_impl(const uint256& v) noexcept
{
    #if BOOST_INT256_ENDIAN_LITTLE_BYTE

    return to_le_bytes_impl<ByteType>(v);

    #else

    return {{extract_byte<ByteType>(v.words[0], 56U), extract_byte<ByteType>(v.words[0], 48U),
             extract_byte<ByteType>(v.words[0], 40U), extract_byte<ByteType>(v.words[0], 32U),
             extract_byte<ByteType>(v.words[0], 24U), extract_byte<ByteType>(v.words[0], 16U),
             extract_byte<ByteType>(v.words[0], 8U),  extract_byte<ByteType>(v.words[0], 0U),
             extract_byte<ByteType>(v.words[1], 56U), extract_byte<ByteType>(v.words[1], 48U),
             extract_byte<ByteType>(v.words[1], 40U), extract_byte<ByteType>(v.words[1], 32U),
             extract_byte<ByteType>(v.words[1], 24U), extract_byte<ByteType>(v.words[1], 16U),
             extract_byte<ByteType>(v.words[1], 8U),  extract_byte<ByteType>(v.words[1], 0U),
             extract_byte<ByteType>(v.words[2], 56U), extract_byte<ByteType>(v.words[2], 48U),
             extract_byte<ByteType>(v.words[2], 40U), extract_byte<ByteType>(v.words[2], 32U),
             extract_byte<ByteType>(v.words[2], 24U), extract_byte<ByteType>(v.words[2], 16U),
             extract_byte<ByteType>(v.words[2], 8U),  extract_byte<ByteType>(v.words[2], 0U),
             extract_byte<ByteType>(v.words[3], 56U), extract_byte<ByteType>(v.words[3], 48U),
             extract_byte<ByteType>(v.words[3], 40U), extract_byte<ByteType>(v.words[3], 32U),
             extract_byte<ByteType>(v.words[3], 24U), extract_byte<ByteType>(v.words[3], 16U),
             extract_byte<ByteType>(v.words[3], 8U),  extract_byte<ByteType>(v.words[3], 0U)}};

    #endif
}

// T is either uint256 or int256 (is_valid_overload_v gates the public entry points on
// this). The words are always assembled into a uint256 first: its 4-word constructor
// takes every word as std::uint64_t, so nothing narrows. static_cast to T is then a no-op
// for T = uint256, or the bit-copying int256(const uint256&) constructor for T = int256.
// A generic T{word, word, word, word} here would narrow std::uint64_t into the signed
// top word of int256's own 4-word constructor
template <typename T, typename Bytes>
BOOST_INT256_HOST_DEVICE constexpr T from_be_bytes_impl(const Bytes& bytes) noexcept
{
    const uint256 assembled {word_from_be(bytes, 0U), word_from_be(bytes, 8U),
                             word_from_be(bytes, 16U), word_from_be(bytes, 24U)};
    return static_cast<T>(assembled);
}

template <typename T, typename Bytes>
BOOST_INT256_HOST_DEVICE constexpr T from_le_bytes_impl(const Bytes& bytes) noexcept
{
    const uint256 assembled {word_from_le(bytes, 24U), word_from_le(bytes, 16U),
                             word_from_le(bytes, 8U), word_from_le(bytes, 0U)};
    return static_cast<T>(assembled);
}

template <typename T, typename Bytes>
BOOST_INT256_HOST_DEVICE constexpr T from_ne_bytes_impl(const Bytes& bytes) noexcept
{
    #if BOOST_INT256_ENDIAN_LITTLE_BYTE

    return from_le_bytes_impl<T>(bytes);

    #else

    const uint256 assembled {word_from_be(bytes, 24U), word_from_be(bytes, 16U),
                             word_from_be(bytes, 8U), word_from_be(bytes, 0U)};
    return static_cast<T>(assembled);

    #endif
}

} // namespace detail

//=====================================
// Byte array conversions
//=====================================

// The std::array returning forms are host only: std::array is not usable from a device
// pass. Device code uses the raw array overloads below.
BOOST_INT256_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
constexpr std::array<ByteType, sizeof(uint256)> to_be_bytes(const uint256& value) noexcept
{
    return detail::to_be_bytes_impl<ByteType>(value);
}

BOOST_INT256_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
constexpr std::array<ByteType, sizeof(uint256)> to_le_bytes(const uint256& value) noexcept
{
    return detail::to_le_bytes_impl<ByteType>(value);
}

BOOST_INT256_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
constexpr std::array<ByteType, sizeof(uint256)> to_ne_bytes(const uint256& value) noexcept
{
    return detail::to_ne_bytes_impl<ByteType>(value);
}

// Signed twins: byte order and byte extraction have no notion of sign, so each of these
// runs the corresponding uint256 overload on the bit image
BOOST_INT256_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
constexpr std::array<ByteType, sizeof(int256)> to_be_bytes(const int256& value) noexcept
{
    return detail::to_be_bytes_impl<ByteType>(detail::to_unsigned_bits(value));
}

BOOST_INT256_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
constexpr std::array<ByteType, sizeof(int256)> to_le_bytes(const int256& value) noexcept
{
    return detail::to_le_bytes_impl<ByteType>(detail::to_unsigned_bits(value));
}

BOOST_INT256_EXPORT template <typename ByteType = std::uint8_t, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
constexpr std::array<ByteType, sizeof(int256)> to_ne_bytes(const int256& value) noexcept
{
    return detail::to_ne_bytes_impl<ByteType>(detail::to_unsigned_bits(value));
}

// Raw array forms, usable from a device pass
BOOST_INT256_EXPORT template <typename ByteType, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr void to_be_bytes(const uint256& value, ByteType (&out)[sizeof(uint256)]) noexcept
{
    detail::write_be_bytes<ByteType>(value, out);
}

BOOST_INT256_EXPORT template <typename ByteType, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr void to_le_bytes(const uint256& value, ByteType (&out)[sizeof(uint256)]) noexcept
{
    detail::write_le_bytes<ByteType>(value, out);
}

BOOST_INT256_EXPORT template <typename ByteType, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr void to_ne_bytes(const uint256& value, ByteType (&out)[sizeof(uint256)]) noexcept
{
    detail::write_ne_bytes<ByteType>(value, out);
}

BOOST_INT256_EXPORT template <typename ByteType, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr void to_be_bytes(const int256& value, ByteType (&out)[sizeof(int256)]) noexcept
{
    detail::write_be_bytes<ByteType>(detail::to_unsigned_bits(value), out);
}

BOOST_INT256_EXPORT template <typename ByteType, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr void to_le_bytes(const int256& value, ByteType (&out)[sizeof(int256)]) noexcept
{
    detail::write_le_bytes<ByteType>(detail::to_unsigned_bits(value), out);
}

BOOST_INT256_EXPORT template <typename ByteType, std::enable_if_t<detail::is_byte_like_v<ByteType>, bool> = true>
BOOST_INT256_HOST_DEVICE constexpr void to_ne_bytes(const int256& value, ByteType (&out)[sizeof(int256)]) noexcept
{
    detail::write_ne_bytes<ByteType>(detail::to_unsigned_bits(value), out);
}

BOOST_INT256_EXPORT template <typename T, typename ByteType, std::size_t N>
constexpr T from_be_bytes(const std::array<ByteType, N>& bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int256::uint256 or boost::int256::int256");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");
    static_assert(N == sizeof(T), "The number of bytes provided, and the target type number of bytes do not match");

    return detail::from_be_bytes_impl<T>(bytes);
}

// Reads sizeof(T) bytes starting at bytes
BOOST_INT256_EXPORT template <typename T, typename ByteType>
BOOST_INT256_HOST_DEVICE constexpr T from_be_bytes(const ByteType* bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int256::uint256 or boost::int256::int256");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");

    return detail::from_be_bytes_impl<T>(bytes);
}

BOOST_INT256_EXPORT template <typename T, typename ByteType, std::size_t N>
constexpr T from_le_bytes(const std::array<ByteType, N>& bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int256::uint256 or boost::int256::int256");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");
    static_assert(N == sizeof(T), "The number of bytes provided, and the target type number of bytes do not match");

    return detail::from_le_bytes_impl<T>(bytes);
}

// Reads sizeof(T) bytes starting at bytes
BOOST_INT256_EXPORT template <typename T, typename ByteType>
BOOST_INT256_HOST_DEVICE constexpr T from_le_bytes(const ByteType* bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int256::uint256 or boost::int256::int256");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");

    return detail::from_le_bytes_impl<T>(bytes);
}

BOOST_INT256_EXPORT template <typename T, typename ByteType, std::size_t N>
constexpr T from_ne_bytes(const std::array<ByteType, N>& bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int256::uint256 or boost::int256::int256");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");
    static_assert(N == sizeof(T), "The number of bytes provided, and the target type number of bytes do not match");

    return detail::from_ne_bytes_impl<T>(bytes);
}

// Reads sizeof(T) bytes starting at bytes
BOOST_INT256_EXPORT template <typename T, typename ByteType>
BOOST_INT256_HOST_DEVICE constexpr T from_ne_bytes(const ByteType* bytes) noexcept
{
    static_assert(detail::is_valid_overload_v<T>,
                  "The target type must be boost::int256::uint256 or boost::int256::int256");
    static_assert(detail::is_byte_like_v<ByteType>,
                  "The source bytes must be char, signed char, unsigned char, or std::byte");

    return detail::from_ne_bytes_impl<T>(bytes);
}

} // namespace int256
} // namespace boost

#endif // BOOST_INT256_BYTE_CONVERSIONS_HPP
