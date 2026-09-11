// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Shared oracle for the differential tests. boost::multiprecision::uint256_t is
// the reference: it is a fixed 256-bit unchecked type, so it wraps modulo 2^256
// exactly like boost::int256::uint256 does.
//
// This header must be included FIRST by every test that uses it. Multiprecision
// and its dependency closure are not clean under the strict warning set the test
// Jamfile turns on, and b2 sorts <cxxflags> so a -Wno- switch cannot override the
// -W switch that precedes it. The suppressions therefore live here as source
// pragmas, and they are deliberately never popped: multiprecision templates are
// instantiated from the including test's own code, so the region has to cover the
// whole translation unit. -Wfloat-equal in particular must be off before
// <boost/core/lightweight_test.hpp> is seen.

#ifndef BOOST_INT256_TEST_ORACLE_HPP
#define BOOST_INT256_TEST_ORACLE_HPP

// Multiprecision pulls in a hard-coded #pragma GCC warning about thread_local on
// old MinGW that no diagnostic switch can silence, and it needs exceptions. The
// module build does not link Multiprecision at all, so the reference is out of
// reach there too. A test that includes this header keeps its directed cases and
// drops only the parts that need the reference.
#if (defined(__MINGW32__) && defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 9)) || \
    defined(BOOST_INT256_DISABLE_EXCEPTIONS) || defined(BOOST_INT256_BUILD_MODULE)
#  define BOOST_INT256_ORACLE_UNAVAILABLE
#endif

// Propagates up from Boost.Math via Multiprecision.
#ifndef _SILENCE_CXX23_DENORM_DEPRECATION_WARNING
#  define _SILENCE_CXX23_DENORM_DEPRECATION_WARNING
#endif

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-warning-option"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#  pragma clang diagnostic ignored "-Wfloat-equal"
#  pragma clang diagnostic ignored "-Wundef"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wsign-conversion"
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  if defined(__has_warning)
#    if __has_warning("-Wpragma-clang-attribute")
#      pragma clang diagnostic ignored "-Wpragma-clang-attribute"
#    endif
#    if __has_warning("-Wdeprecated-literal-operator")
#      pragma clang diagnostic ignored "-Wdeprecated-literal-operator"
#    endif
#    if __has_warning("-Wfortify-source")
#      pragma clang diagnostic ignored "-Wfortify-source"
#    endif
#  endif
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wcast-qual"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#  pragma GCC diagnostic ignored "-Wundef"
#  pragma GCC diagnostic ignored "-Wuseless-cast"
#  pragma GCC diagnostic ignored "-Wduplicated-branches"
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4127)
#  pragma warning(disable : 4244)
#  pragma warning(disable : 4267)
#  pragma warning(disable : 4305)
#  pragma warning(disable : 4309)
#  pragma warning(disable : 4389)
#endif

#ifndef BOOST_INT256_BUILD_MODULE

#include <boost/int256/int256.hpp>

// The stream inserter is needed for BOOST_TEST_EQ to print a uint256 on failure.
// Without it the implicit conversions to the builtin integers make operator<<
// ambiguous rather than absent.
#include <boost/int256/iostream.hpp>

#else

// The module interface unit is imported by the including test, which carries the
// module preamble, so nothing from the library is included here.

#endif

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE
#  include <boost/multiprecision/cpp_int.hpp>
#endif

#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

// Iteration count for the randomized loops. QEMU runs pass -DBOOST_INT256_TEST_N=256.
#ifndef BOOST_INT256_TEST_N
#  define BOOST_INT256_TEST_N 4096
#endif

namespace int256_test {

using boost::int256::uint256;

constexpr std::size_t N {static_cast<std::size_t>(BOOST_INT256_TEST_N)};

// The operand families the differential tests draw from. Every pair of shapes is
// exercised so that word-count and normalization edges are hit deliberately
// rather than by luck.
enum class shape
{
    word1,
    word2,
    word3,
    word4,
    pow2,
    pow2_minus1,
    pow2_plus1,
    all_ones,
    top_bit,
    sparse
};

inline const char* shape_name(shape s)
{
    switch (s)
    {
        case shape::word1:       return "word1";
        case shape::word2:       return "word2";
        case shape::word3:       return "word3";
        case shape::word4:       return "word4";
        case shape::pow2:        return "pow2";
        case shape::pow2_minus1: return "pow2_minus1";
        case shape::pow2_plus1:  return "pow2_plus1";
        case shape::all_ones:    return "all_ones";
        case shape::top_bit:     return "top_bit";
        case shape::sparse:      return "sparse";
    }

    return "unknown";
}

inline const std::vector<shape>& all_shapes()
{
    static const std::vector<shape> shapes {
        shape::word1, shape::word2, shape::word3, shape::word4,
        shape::pow2, shape::pow2_minus1, shape::pow2_plus1,
        shape::all_ones, shape::top_bit, shape::sparse
    };

    return shapes;
}

inline uint256 uint256_max()
{
    return uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
}

inline uint256 one()
{
    return uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
}

// Prints a value as a little-significant-word-first hex list so a failing case can
// be pasted straight back into a directed test.
inline std::string hex_words(const uint256& v)
{
    char buffer[96] {};
    const int written {std::snprintf(buffer, sizeof(buffer), "{0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX}",
                                     static_cast<unsigned long long>(v.words[0]),
                                     static_cast<unsigned long long>(v.words[1]),
                                     static_cast<unsigned long long>(v.words[2]),
                                     static_cast<unsigned long long>(v.words[3]))};
    static_cast<void>(written);

    return std::string{buffer};
}

// Draws a value of the requested shape from the engine.
inline uint256 random_uint256(std::mt19937_64& rng, shape s)
{
    switch (s)
    {
        case shape::word1:
        {
            return uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), rng()};
        }
        case shape::word2:
        {
            const auto w1 {rng()};
            const auto w0 {rng()};
            return uint256{UINT64_C(0), UINT64_C(0), w1, w0};
        }
        case shape::word3:
        {
            const auto w2 {rng()};
            const auto w1 {rng()};
            const auto w0 {rng()};
            return uint256{UINT64_C(0), w2, w1, w0};
        }
        case shape::word4:
        {
            const auto w3 {rng()};
            const auto w2 {rng()};
            const auto w1 {rng()};
            const auto w0 {rng()};
            return uint256{w3, w2, w1, w0};
        }
        case shape::pow2:
        {
            const auto k {static_cast<unsigned>(rng() % 256U)};
            return one() << k;
        }
        case shape::pow2_minus1:
        {
            const auto k {static_cast<unsigned>(rng() % 256U)};
            return (one() << k) - 1U;
        }
        case shape::pow2_plus1:
        {
            const auto k {static_cast<unsigned>(rng() % 256U)};
            return (one() << k) + 1U;
        }
        case shape::all_ones:
        {
            return uint256_max();
        }
        case shape::top_bit:
        {
            const auto w3 {rng()};
            const auto w2 {rng()};
            const auto w1 {rng()};
            const auto w0 {rng()};
            return uint256{w3 | (UINT64_C(1) << 63), w2, w1, w0};
        }
        case shape::sparse:
        {
            const auto bits {static_cast<unsigned>(rng() % 4U) + 1U};
            uint256 v {};
            for (unsigned i {0}; i < bits; ++i)
            {
                v = v | (one() << static_cast<unsigned>(rng() % 256U));
            }
            return v;
        }
    }

    return uint256{};
}

inline uint256 random_uint256(std::mt19937_64& rng)
{
    return random_uint256(rng, shape::word4);
}

// The Appendix B.7 corpus. Every value here was recomputed against
// multiprecision before being written down.
inline const std::vector<uint256>& edge_values()
{
    static const std::vector<uint256> values {
        // small values
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(2)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(3)},

        // 2^63, 2^64 - 1, 2^64, 2^64 + 1
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0x8000000000000000)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_MAX},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(1)},

        // 2^127, 2^128 - 1, 2^128, 2^128 + 1
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_MAX, UINT64_MAX},
        uint256{UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(1)},

        // 2^191, 2^192 - 1, 2^192, 2^192 + 1
        uint256{UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_MAX, UINT64_MAX, UINT64_MAX},
        uint256{UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
        uint256{UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(1)},

        // 2^255, 2^255 + 1, 2^256 - 1, 2^256 - 2
        uint256{UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)},
        uint256{UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(1)},
        uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX},
        uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX - 1},

        // bit patterns
        uint256{UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)},
        uint256{UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xAAAAAAAAAAAAAAAA)},
        uint256{UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0x5555555555555555), UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0x5555555555555555)},
        uint256{UINT64_MAX, UINT64_C(0), UINT64_MAX, UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_MAX, UINT64_C(0), UINT64_MAX},

        // the 0x00 .. 0x1F byte pattern in big-endian byte order
        uint256{UINT64_C(0x0001020304050607), UINT64_C(0x08090A0B0C0D0E0F), UINT64_C(0x1011121314151617), UINT64_C(0x18191A1B1C1D1E1F)},

        // one word wide, top word MSB set or == 1
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0x8000000000000000)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)},

        // two words wide, top word MSB set or == 1
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_MAX},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_MAX},

        // three words wide, top word MSB set or == 1
        uint256{UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_MAX, UINT64_MAX},
        uint256{UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)},
        uint256{UINT64_C(0), UINT64_C(1), UINT64_MAX, UINT64_MAX},

        // four words wide, top word MSB set or == 1
        uint256{UINT64_C(0x8000000000000000), UINT64_MAX, UINT64_MAX, UINT64_MAX},
        uint256{UINT64_C(1), UINT64_MAX, UINT64_MAX, UINT64_MAX},

        // 10^20 and 10^37, the short-dividend regression operands
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(5), UINT64_C(7766279631452241920)},
        uint256{UINT64_C(0), UINT64_C(0), UINT64_C(542101086242752217), UINT64_C(68739955140067328)}
    };

    return values;
}

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

using mp = boost::multiprecision::uint256_t;

inline mp to_mp(const uint256& v)
{
    mp r {v.words[3]};
    r <<= 64;
    r |= mp{v.words[2]};
    r <<= 64;
    r |= mp{v.words[1]};
    r <<= 64;
    r |= mp{v.words[0]};

    return r;
}

inline uint256 from_mp(const mp& v)
{
    const mp mask {(std::numeric_limits<std::uint64_t>::max)()};

    mp t {v};
    uint256 r {};
    for (std::size_t i {0}; i < 4U; ++i)
    {
        r.words[i] = static_cast<std::uint64_t>(mp{t & mask});
        t >>= 64;
    }

    return r;
}

// Renders the reference value in an arbitrary base so the string conversions can
// be compared without trusting the library's own digit emission.
inline std::string mp_str(mp v, int base, bool uppercase)
{
    if (base < 2 || base > 36)
    {
        return std::string{};
    }

    if (v == 0)
    {
        return std::string{"0"};
    }

    const char* const digits {uppercase ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        : "0123456789abcdefghijklmnopqrstuvwxyz"};

    const mp radix {base};
    std::string reversed;
    while (v != 0)
    {
        const auto digit {static_cast<unsigned>(mp{v % radix})};
        reversed.push_back(digits[digit]);
        v /= radix;
    }

    std::string result;
    result.reserve(reversed.size());
    for (std::size_t i {reversed.size()}; i > 0U; --i)
    {
        result.push_back(reversed[i - 1U]);
    }

    return result;
}

inline std::string hex_words(const mp& v)
{
    return hex_words(from_mp(v));
}

// A 512-bit reference for the places where a 256-bit one would wrap: the product
// of two operands, the sum of two near-maximal ones, a squaring inside a modular
// exponentiation. Doing those in mp gives a silently wrong expectation.
using wide = boost::multiprecision::uint512_t;

inline wide to_wide(const uint256& v)
{
    wide r {v.words[3]};
    r <<= 64;
    r |= wide{v.words[2]};
    r <<= 64;
    r |= wide{v.words[1]};
    r <<= 64;
    r |= wide{v.words[0]};

    return r;
}

// The low 256 bits of a wide value, which is what a wrapping operation produces.
inline uint256 from_wide_truncated(const wide& v)
{
    const wide mask {(std::numeric_limits<std::uint64_t>::max)()};

    wide t {v};
    uint256 r {};
    for (std::size_t i {0}; i < 4U; ++i)
    {
        r.words[i] = static_cast<std::uint64_t>(wide{t & mask});
        t >>= 64;
    }

    return r;
}

inline bool fits_256(const wide& v)
{
    return v < (wide{1} << 256);
}

// Reads the low bits of a non-negative builtin operand into the reference type.
// The wide overload is only instantiated for the builtin 128-bit types, so the
// narrow types never see a shift by 64.
template <typename T>
mp mp_bits(T v, std::true_type)
{
    return mp{static_cast<std::uint64_t>(v)};
}

template <typename T>
mp mp_bits(T v, std::false_type)
{
    const auto lo {static_cast<std::uint64_t>(v)};
    const auto hi {static_cast<std::uint64_t>(v >> 64)};

    return mp{mp{hi} << 64} | mp{lo};
}

template <typename T>
mp mp_bits(T v)
{
    return mp_bits(v, std::integral_constant<bool, sizeof(T) <= sizeof(std::uint64_t)>{});
}

template <typename T>
mp widen_impl(T v, std::false_type)
{
    return mp_bits(v);
}

// A negative builtin operand converts to the 256-bit two's complement value, so
// the reference is 2^256 minus the magnitude. The magnitude is formed as
// -(v + 1) + 1 so that the most negative value does not overflow on the way.
template <typename T>
mp widen_impl(T v, std::true_type)
{
    if (v >= 0)
    {
        return mp_bits(v);
    }

    const auto negated_plus_one {static_cast<T>(-static_cast<T>(v + 1))};
    const mp magnitude {mp_bits(negated_plus_one) + 1};

    return mp{mp{0} - magnitude};
}

// The 256-bit value a builtin operand takes on under the usual conversions.
// Signedness is decided without std::make_unsigned, which is unspecified for
// __int128 outside GNU mode.
template <typename T>
mp widen(T v)
{
    return widen_impl(v, std::integral_constant<bool, (static_cast<T>(-1) < static_cast<T>(0))>{});
}

// Failure reporter for the differential loops. Always returns false so it can sit
// on the right-hand side of a short-circuiting BOOST_TEST expression.
inline bool report_mismatch(const char* what, const uint256& lhs, const uint256& rhs,
                            const uint256& got, const mp& expected)
{
    std::cerr << "int256 differential mismatch in " << what << '\n'
              << "  lhs      = " << hex_words(lhs) << '\n'
              << "  rhs      = " << hex_words(rhs) << '\n'
              << "  got      = " << hex_words(got) << '\n'
              << "  expected = " << hex_words(expected) << std::endl;

    return false;
}

inline bool report_mismatch(const char* what, const uint256& lhs,
                            const uint256& got, const mp& expected)
{
    std::cerr << "int256 differential mismatch in " << what << '\n'
              << "  operand  = " << hex_words(lhs) << '\n'
              << "  got      = " << hex_words(got) << '\n'
              << "  expected = " << hex_words(expected) << std::endl;

    return false;
}

inline bool report_string_mismatch(const char* what, const uint256& value,
                                   const std::string& got, const std::string& expected)
{
    std::cerr << "int256 differential mismatch in " << what << '\n'
              << "  operand  = " << hex_words(value) << '\n'
              << "  got      = \"" << got << "\"\n"
              << "  expected = \"" << expected << '"' << std::endl;

    return false;
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE

} // namespace int256_test

// Compares a library result against the reference and prints both operands as hex
// word lists when they differ.
#define BOOST_INT256_TEST_BINARY(what, lhs, rhs, got, expected) \
    BOOST_TEST(::int256_test::to_mp(got) == (expected) || ::int256_test::report_mismatch(what, lhs, rhs, got, expected))

#define BOOST_INT256_TEST_UNARY(what, operand, got, expected) \
    BOOST_TEST(::int256_test::to_mp(got) == (expected) || ::int256_test::report_mismatch(what, operand, got, expected))

#define BOOST_INT256_TEST_STRING(what, value, got, expected) \
    BOOST_TEST((got) == (expected) || ::int256_test::report_string_mismatch(what, value, got, expected))

using int256_test::N;
using int256_test::shape;
using int256_test::shape_name;
using int256_test::all_shapes;
using int256_test::edge_values;
using int256_test::hex_words;
using int256_test::random_uint256;
using int256_test::uint256_max;

#ifndef BOOST_INT256_ORACLE_UNAVAILABLE

using int256_test::mp;
using int256_test::to_mp;
using int256_test::from_mp;
using int256_test::mp_str;
using int256_test::widen;
using int256_test::wide;
using int256_test::to_wide;
using int256_test::from_wide_truncated;
using int256_test::fits_256;
using int256_test::report_mismatch;
using int256_test::report_string_mismatch;

#endif

#endif // BOOST_INT256_TEST_ORACLE_HPP
