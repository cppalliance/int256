// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Deterministic differential test of uint256 and int256 against
// math::wide_integer::uintwide_t<256U, std::uint32_t> and its signed flavor
// (IsSigned == true): an independent two's complement implementation over
// 32-bit limbs. Operands are moved between the libraries by copying limbs, so
// neither library's arithmetic is involved in building the expected value.
//
// The fuzzers in test/fuzzing throw random words at one operator each. This
// test instead drives an edge corpus and every pair of operand shapes through
// the operators, every shift count, the builtin mixed forms, stream formatting
// and parsing, conversions, and the numeric algorithms.
//
// It needs a checkout of wide-integer, so it is built by
// .github/workflows/wide_integer.yml rather than by the Jamfile. Like the
// library, it has no other dependency:
//
//   git clone https://github.com/ckormanyos/wide-integer $SCRATCH/wide-integer
//   clang++ -std=c++14 -O2 -isystem $SCRATCH/wide-integer -I libs/int256/include libs/int256/test/test_vs_wide_integer.cpp
//   ./a.out --iterations 64 --seed 42

#include <math/wide_integer/uintwide_t.h>

#include <boost/int256.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

using boost::int256::int256;
using boost::int256::uint256;

#if defined(WIDE_INTEGER_NAMESPACE)
using wide_u256 = ::WIDE_INTEGER_NAMESPACE::math::wide_integer::uintwide_t<256U, std::uint32_t>;
using wide_i256 = ::WIDE_INTEGER_NAMESPACE::math::wide_integer::uintwide_t<256U, std::uint32_t, void, true>;
#else
using wide_u256 = ::math::wide_integer::uintwide_t<256U, std::uint32_t>;
using wide_i256 = ::math::wide_integer::uintwide_t<256U, std::uint32_t, void, true>;
#endif

template <typename T>
struct wide_of;

template <>
struct wide_of<uint256>
{
    using type = wide_u256;
};

template <>
struct wide_of<int256>
{
    using type = wide_i256;
};

template <typename T>
using wide_t = typename wide_of<T>::type;

//=====================================
// Moving values between the libraries
//=====================================

// uintwide_t indexes its limbs with its own size type, which is narrower than
// std::size_t on a 64-bit target
template <typename Wide>
using limb_index = typename Wide::representation_type::size_type;

// Copies the four 64-bit words into the eight 32-bit limbs, least significant
// first. Limb 0 is the least significant one on every platform.
template <typename T>
wide_t<T> to_wide(const T& value)
{
    using index = limb_index<wide_t<T>>;

    wide_t<T> result {};
    auto& limbs {result.representation()};

    for (index i {}; i < 4U; ++i)
    {
        limbs[static_cast<index>(2U * i)] = static_cast<std::uint32_t>(value.words[i]);
        limbs[static_cast<index>(2U * i + 1U)] = static_cast<std::uint32_t>(value.words[i] >> 32U);
    }

    return result;
}

template <typename Wide>
std::uint64_t wide_word(const Wide& value, const limb_index<Wide> i)
{
    using index = limb_index<Wide>;

    const auto& limbs {value.crepresentation()};

    return static_cast<std::uint64_t>(limbs[static_cast<index>(2U * i)]) |
           (static_cast<std::uint64_t>(limbs[static_cast<index>(2U * i + 1U)]) << 32U);
}

uint256 bits_from_wide(const wide_u256& value)
{
    return uint256{wide_word(value, 3U), wide_word(value, 2U), wide_word(value, 1U), wide_word(value, 0U)};
}

uint256 bits_from_wide(const wide_i256& value)
{
    return uint256{wide_word(value, 3U), wide_word(value, 2U), wide_word(value, 1U), wide_word(value, 0U)};
}

template <typename T>
T from_bits(const uint256& bits);

template <>
uint256 from_bits<uint256>(const uint256& bits)
{
    return bits;
}

template <>
int256 from_bits<int256>(const uint256& bits)
{
    return boost::int256::detail::to_signed_bits(bits);
}

template <typename T>
T from_wide(const wide_t<T>& value)
{
    return from_bits<T>(bits_from_wide(value));
}

template <typename T>
bool same_bits(const T& got, const wide_t<T>& expected)
{
    for (limb_index<wide_t<T>> i {}; i < 4U; ++i)
    {
        if (got.words[i] != wide_word(expected, i))
        {
            return false;
        }
    }

    return true;
}

// uintwide_t's operator~ is a non-const member that flips the value in place
template <typename Wide>
Wide wide_not(const Wide& value)
{
    auto result {value};
    static_cast<void>(~result);
    return result;
}

bool is_negative(const uint256&) noexcept
{
    return false;
}

bool is_negative(const int256& value) noexcept
{
    return value.signed_high() < 0;
}

template <typename T>
T min_value() noexcept
{
    return (std::numeric_limits<T>::min)();
}

//=====================================
// Checking and reporting
//=====================================

std::size_t checks {};
std::size_t failures {};
constexpr std::size_t max_reports {32U};

std::string describe(const uint256& value)
{
    char buffer[128] {};
    const auto written {std::snprintf(buffer, sizeof(buffer), "{0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX}",
                                      static_cast<unsigned long long>(value.words[0]),
                                      static_cast<unsigned long long>(value.words[1]),
                                      static_cast<unsigned long long>(value.words[2]),
                                      static_cast<unsigned long long>(value.words[3]))};
    static_cast<void>(written);

    return std::string{buffer} + " (" + boost::int256::to_string(value) + ")";
}

std::string describe(const int256& value)
{
    return describe(boost::int256::detail::to_unsigned_bits(value)) + " as int256 " + boost::int256::to_string(value);
}

template <typename Builtin>
std::string describe(const Builtin value)
{
    return std::to_string(value);
}

std::string describe(const std::string& value)
{
    return '"' + value + '"';
}

void report(const char* what, const std::string& lhs, const std::string& rhs,
            const std::string& got, const std::string& expected)
{
    ++failures;

    if (failures <= max_reports)
    {
        std::cerr << "mismatch in " << what << '\n'
                  << "  lhs      = " << lhs << '\n'
                  << "  rhs      = " << rhs << '\n'
                  << "  got      = " << got << '\n'
                  << "  expected = " << expected << std::endl;
    }
}

template <typename T, typename A, typename B>
void check_value(const char* what, const A& a, const B& b, const T& got, const wide_t<T>& expected)
{
    ++checks;

    if (!same_bits(got, expected))
    {
        report(what, describe(a), describe(b), describe(got), describe(from_wide<T>(expected)));
    }
}

template <typename A, typename B, typename R>
void check_equal(const char* what, const A& a, const B& b, const R& got, const R& expected)
{
    ++checks;

    if (!(got == expected))
    {
        report(what, describe(a), describe(b), describe(got), describe(expected));
    }
}

//=====================================
// Operands
//=====================================

enum class shape
{
    word1,
    word2,
    word3,
    word4,
    half_word,
    pow2,
    pow2_minus1,
    pow2_plus1,
    top_bit,
    sparse
};

constexpr shape all_shapes[] {shape::word1, shape::word2, shape::word3, shape::word4, shape::half_word,
                              shape::pow2, shape::pow2_minus1, shape::pow2_plus1, shape::top_bit, shape::sparse};

uint256 pow2(const unsigned k)
{
    return uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)} << k;
}

// Draws a bit pattern of the requested shape. The word counts line up with the
// division tiers, and the power of two families with the normalization edges.
uint256 random_bits(std::mt19937_64& rng, const shape s)
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
        case shape::half_word:
        {
            return uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), rng() >> 32U};
        }
        case shape::pow2:
        {
            return pow2(static_cast<unsigned>(rng() % 256U));
        }
        case shape::pow2_minus1:
        {
            return pow2(static_cast<unsigned>(rng() % 256U)) - 1U;
        }
        case shape::pow2_plus1:
        {
            return pow2(static_cast<unsigned>(rng() % 256U)) + 1U;
        }
        case shape::top_bit:
        {
            const auto w3 {rng()};
            const auto w2 {rng()};
            const auto w1 {rng()};
            const auto w0 {rng()};
            return uint256{w3 | (UINT64_C(1) << 63U), w2, w1, w0};
        }
        case shape::sparse:
        {
            uint256 value {};
            const auto bits {static_cast<unsigned>(rng() % 4U) + 1U};
            for (unsigned i {}; i < bits; ++i)
            {
                value |= pow2(static_cast<unsigned>(rng() % 256U));
            }
            return value;
        }
    }

    return uint256{};
}

// Powers of two around every limb and word boundary, one either side of them,
// and the bit patterns that stress carry and borrow propagation.
std::vector<uint256> edge_bits()
{
    std::vector<uint256> values {uint256{}};

    const unsigned exps[] {0U, 1U, 2U, 31U, 32U, 33U, 63U, 64U, 65U, 95U, 96U, 127U, 128U, 129U,
                           159U, 160U, 191U, 192U, 193U, 223U, 224U, 253U, 254U, 255U};

    for (const auto k : exps)
    {
        values.push_back(pow2(k));
        values.push_back(pow2(k) - 1U);
        values.push_back(pow2(k) + 1U);
    }

    const auto all_ones {~uint256{}};
    values.push_back(all_ones);
    values.push_back(all_ones - 1U);
    values.push_back(uint256{UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)});
    values.push_back(uint256{UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xAAAAAAAAAAAAAAAA)});
    values.push_back(uint256{UINT64_MAX, UINT64_C(0), UINT64_MAX, UINT64_C(0)});
    values.push_back(uint256{UINT64_C(0), UINT64_MAX, UINT64_C(0), UINT64_MAX});
    values.push_back(uint256{UINT64_C(0x00000000FFFFFFFF), UINT64_C(0xFFFFFFFF00000000), UINT64_C(0x00000000FFFFFFFF), UINT64_C(0xFFFFFFFF00000000)});
    values.push_back(uint256{UINT64_C(0x0001020304050607), UINT64_C(0x08090A0B0C0D0E0F), UINT64_C(0x1011121314151617), UINT64_C(0x18191A1B1C1D1E1F)});

    // 10^19, 10^38 and 10^76, the largest power of ten that fits each width
    values.push_back(boost::int256::ipow(uint256{10U}, 19U));
    values.push_back(boost::int256::ipow(uint256{10U}, 38U));
    values.push_back(boost::int256::ipow(uint256{10U}, 76U));

    return values;
}

template <typename T>
std::vector<T> edge_corpus();

template <>
std::vector<uint256> edge_corpus<uint256>()
{
    return edge_bits();
}

// The same bit patterns read as int256 (which already includes MIN, MAX and
// -1), plus the negation of each
template <>
std::vector<int256> edge_corpus<int256>()
{
    std::vector<int256> values {};

    for (const auto& bits : edge_bits())
    {
        const auto value {from_bits<int256>(bits)};
        values.push_back(value);
        values.push_back(-value);
    }

    return values;
}

// The signs an operand of each type can take, and forcing one onto a bit
// pattern. These are overloads, not a test of numeric_limits<T>::is_signed,
// which MSVC reports as a constant condition.
std::size_t sign_count(const uint256&) noexcept
{
    return 1U;
}

std::size_t sign_count(const int256&) noexcept
{
    return 2U;
}

uint256 with_sign(const uint256& bits, const bool, const uint256&)
{
    return bits;
}

uint256 with_sign(const uint256& bits, const bool negative, const int256&)
{
    const auto sign_bit {pow2(255U)};
    return negative ? (bits | sign_bit) : (bits & ~sign_bit);
}

double with_sign(const double value, const bool, const uint256&)
{
    return value;
}

double with_sign(const double value, const bool negative, const int256&)
{
    return negative ? -value : value;
}

// A bit pattern of the given shape, with the sign bit forced when negative
template <typename T>
T random_value(std::mt19937_64& rng, const shape s, const bool negative)
{
    return from_bits<T>(with_sign(random_bits(rng, s), negative, T{}));
}

//=====================================
// The checks
//=====================================

// Division by zero is undefined in both libraries. MIN / -1 is not: both wrap
// to MIN, with a remainder of zero.
template <typename T>
bool can_divide(const T& b)
{
    return b != T{0U};
}

template <typename T>
void check_arithmetic(const T& a, const T& b)
{
    const auto wa {to_wide(a)};
    const auto wb {to_wide(b)};

    check_value("operator+", a, b, T{a + b}, wide_t<T>{wa + wb});
    check_value("operator-", a, b, T{a - b}, wide_t<T>{wa - wb});
    check_value("operator*", a, b, T{a * b}, wide_t<T>{wa * wb});

    auto sum {a};
    sum += b;
    check_value("operator+=", a, b, sum, wide_t<T>{wa + wb});

    auto difference {a};
    difference -= b;
    check_value("operator-=", a, b, difference, wide_t<T>{wa - wb});

    auto product {a};
    product *= b;
    check_value("operator*=", a, b, product, wide_t<T>{wa * wb});

    if (can_divide(b))
    {
        const T quotient {a / b};
        const T remainder {a % b};

        check_value("operator/", a, b, quotient, wide_t<T>{wa / wb});
        check_value("operator%", a, b, remainder, wide_t<T>{wa % wb});

        auto q {a};
        q /= b;
        check_value("operator/=", a, b, q, wide_t<T>{wa / wb});

        auto r {a};
        r %= b;
        check_value("operator%=", a, b, r, wide_t<T>{wa % wb});
    }
}

template <typename T>
void check_bitwise(const T& a, const T& b)
{
    const auto wa {to_wide(a)};
    const auto wb {to_wide(b)};

    check_value("operator&", a, b, T{a & b}, wide_t<T>{wa & wb});
    check_value("operator|", a, b, T{a | b}, wide_t<T>{wa | wb});
    check_value("operator^", a, b, T{a ^ b}, wide_t<T>{wa ^ wb});

    auto and_value {a};
    and_value &= b;
    check_value("operator&=", a, b, and_value, wide_t<T>{wa & wb});

    auto or_value {a};
    or_value |= b;
    check_value("operator|=", a, b, or_value, wide_t<T>{wa | wb});

    auto xor_value {a};
    xor_value ^= b;
    check_value("operator^=", a, b, xor_value, wide_t<T>{wa ^ wb});
}

template <typename T>
void check_comparisons(const T& a, const T& b)
{
    const auto wa {to_wide(a)};
    const auto wb {to_wide(b)};

    check_equal("operator==", a, b, a == b, wa == wb);
    check_equal("operator!=", a, b, a != b, wa != wb);
    check_equal("operator<", a, b, a < b, wa < wb);
    check_equal("operator<=", a, b, a <= b, wa <= wb);
    check_equal("operator>", a, b, a > b, wa > wb);
    check_equal("operator>=", a, b, a >= b, wa >= wb);
}

// Every shift form for one count below the width. The count is also passed as
// the 256-bit type itself, which takes a different overload.
template <typename T>
void check_shift(const T& a, const unsigned count)
{
    const auto wa {to_wide(a)};
    const wide_t<T> left {wa << count};
    const wide_t<T> right {wa >> count};

    check_value("operator<< unsigned", a, count, T{a << count}, left);
    check_value("operator>> unsigned", a, count, T{a >> count}, right);
    check_value("operator<< int", a, count, T{a << static_cast<int>(count)}, left);
    check_value("operator>> int", a, count, T{a >> static_cast<int>(count)}, right);
    check_value("operator<< T", a, count, T{a << T{count}}, left);
    check_value("operator>> T", a, count, T{a >> T{count}}, right);

    auto l {a};
    l <<= count;
    check_value("operator<<=", a, count, l, left);

    auto r {a};
    r >>= count;
    check_value("operator>>=", a, count, r, right);
}

template <typename T>
void check_all_shifts(const T& a)
{
    for (unsigned count {}; count < 256U; ++count)
    {
        check_shift(a, count);
    }
}

// One builtin operand type in both positions. The expected value converts the
// builtin through uintwide_t's own constructor, which sign-extends a negative
// value exactly as the usual arithmetic conversions do.
template <typename T, typename Builtin>
void check_builtin(const T& a, const Builtin v)
{
    const auto wa {to_wide(a)};
    const wide_t<T> wv {v};

    check_value("T + builtin", a, v, T{a + v}, wide_t<T>{wa + wv});
    check_value("T - builtin", a, v, T{a - v}, wide_t<T>{wa - wv});
    check_value("T * builtin", a, v, T{a * v}, wide_t<T>{wa * wv});
    check_value("T & builtin", a, v, T{a & v}, wide_t<T>{wa & wv});
    check_value("T | builtin", a, v, T{a | v}, wide_t<T>{wa | wv});
    check_value("T ^ builtin", a, v, T{a ^ v}, wide_t<T>{wa ^ wv});
    check_value("builtin + T", v, a, T{v + a}, wide_t<T>{wv + wa});
    check_value("builtin - T", v, a, T{v - a}, wide_t<T>{wv - wa});
    check_value("builtin * T", v, a, T{v * a}, wide_t<T>{wv * wa});

    auto sum {a};
    sum += v;
    check_value("T += builtin", a, v, sum, wide_t<T>{wa + wv});

    auto difference {a};
    difference -= v;
    check_value("T -= builtin", a, v, difference, wide_t<T>{wa - wv});

    auto product {a};
    product *= v;
    check_value("T *= builtin", a, v, product, wide_t<T>{wa * wv});

    if (v != 0)
    {
        check_value("T / builtin", a, v, T{a / v}, wide_t<T>{wa / wv});
        check_value("T % builtin", a, v, T{a % v}, wide_t<T>{wa % wv});

        auto q {a};
        q /= v;
        check_value("T /= builtin", a, v, q, wide_t<T>{wa / wv});

        auto r {a};
        r %= v;
        check_value("T %= builtin", a, v, r, wide_t<T>{wa % wv});
    }

    if (can_divide(a))
    {
        check_value("builtin / T", v, a, T{v / a}, wide_t<T>{wv / wa});
        check_value("builtin % T", v, a, T{v % a}, wide_t<T>{wv % wa});
    }

    check_equal("T == builtin", a, v, a == v, wa == wv);
    check_equal("T != builtin", a, v, a != v, wa != wv);
    check_equal("T < builtin", a, v, a < v, wa < wv);
    check_equal("T <= builtin", a, v, a <= v, wa <= wv);
    check_equal("T > builtin", a, v, a > v, wa > wv);
    check_equal("T >= builtin", a, v, a >= v, wa >= wv);
    check_equal("builtin < T", v, a, v < a, wv < wa);
    check_equal("builtin > T", v, a, v > a, wv > wa);
}

template <typename T>
void check_builtins(const T& a, const std::uint64_t bits)
{
    check_builtin(a, bits);
    check_builtin(a, static_cast<std::uint32_t>(bits));
    check_builtin(a, static_cast<std::int64_t>(bits));
    check_builtin(a, static_cast<std::int32_t>(bits));
}

// Conversions to the builtin integers keep the low bits, as they do between
// builtin types. The value handed to a builtin constructor round-trips.
template <typename T>
void check_narrowing(const T& a)
{
    const auto wa {to_wide(a)};

    check_equal("to uint64_t", a, 0, static_cast<std::uint64_t>(a), static_cast<std::uint64_t>(wa));
    check_equal("to int64_t", a, 0, static_cast<std::int64_t>(a), static_cast<std::int64_t>(wa));
    check_equal("to uint32_t", a, 0, static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(wa));
    check_equal("to int32_t", a, 0, static_cast<std::int32_t>(a), static_cast<std::int32_t>(wa));

    const auto low {static_cast<std::int64_t>(a.words[0])};
    check_value("from int64_t", low, 0, T{low}, wide_t<T>{low});
    check_value("from uint64_t", a.words[0], 0, T{a.words[0]}, wide_t<T>{a.words[0]});
}

template <typename T>
std::string formatted(const T& value, const std::ios::fmtflags flags, const std::streamsize width = 0)
{
    std::ostringstream out {};
    out.flags(flags);
    out << std::setw(static_cast<int>(width)) << std::setfill('*') << value;
    return out.str();
}

// The standard streams ignore showpos for an unsigned type, and so does uint256
void check_showpos(const uint256&, const wide_u256&)
{
}

void check_showpos(const int256& a, const wide_i256& wa)
{
    check_equal("operator<< dec showpos", a, 0, formatted(a, std::ios::dec | std::ios::showpos),
                formatted(wa, std::ios::dec | std::ios::showpos));
}

// The formats where the two libraries follow the same convention. They differ,
// deliberately, on the rest: int256 prints a negative value in hex or octal as
// sign and magnitude (uintwide_t prints its two's complement bits), follows the
// standard streams in printing zero without a base prefix and ignoring showpos
// for an unsigned type, and honors std::left and std::internal.
template <typename T>
void check_formatting(const T& a)
{
    const auto wa {to_wide(a)};

    const auto decimal {formatted(wa, std::ios::dec)};
    check_equal("operator<< dec", a, 0, formatted(a, std::ios::dec), decimal);
    check_equal("to_string", a, 0, boost::int256::to_string(a), decimal);
    check_equal("operator<< dec setw", a, 0, formatted(a, std::ios::dec, 90), formatted(wa, std::ios::dec, 90));

    check_showpos(a, wa);

    if (!is_negative(a))
    {
        const std::ios::fmtflags bases[] {std::ios::hex, std::ios::oct};

        for (const auto base : bases)
        {
            check_equal("operator<< hex/oct", a, 0, formatted(a, base), formatted(wa, base));
            check_equal("operator<< hex/oct uppercase", a, 0, formatted(a, base | std::ios::uppercase),
                        formatted(wa, base | std::ios::uppercase));
            check_equal("operator<< hex/oct setw", a, 0, formatted(a, base, 90), formatted(wa, base, 90));

            if (a != T{0U})
            {
                check_equal("operator<< hex/oct showbase", a, 0, formatted(a, base | std::ios::showbase),
                            formatted(wa, base | std::ios::showbase));
                check_equal("operator<< hex/oct showbase uppercase", a, 0,
                            formatted(a, base | std::ios::showbase | std::ios::uppercase),
                            formatted(wa, base | std::ios::showbase | std::ios::uppercase));
            }
        }
    }
}

// Each library reads back what the other one wrote
template <typename T>
void check_parsing(const T& a)
{
    const auto wa {to_wide(a)};

    const auto decimal {formatted(wa, std::ios::dec)};
    std::istringstream in {decimal};
    T parsed {};
    in >> parsed;
    check_equal("operator>> dec", decimal, 0, !in.fail(), true);
    check_value("operator>> dec", decimal, 0, parsed, wa);

    const auto local_decimal {boost::int256::to_string(a)};
    check_value("uintwide_t from int256 string", local_decimal, 0, a, wide_t<T>{local_decimal.c_str()});

    if (!is_negative(a))
    {
        const auto hex {formatted(wa, std::ios::hex)};
        std::istringstream hex_in {hex};
        T hex_parsed {};
        hex_in >> std::hex >> hex_parsed;
        check_equal("operator>> hex", hex, 0, !hex_in.fail(), true);
        check_value("operator>> hex", hex, 0, hex_parsed, wa);

        const auto local_hex {"0x" + formatted(a, std::ios::hex)};
        check_value("uintwide_t from int256 hex string", local_hex, 0, a, wide_t<T>{local_hex.c_str()});
    }
}

// The helpers that uintwide_t also provides
template <typename T>
void check_single_operand_functions(const T& a)
{
    const auto wa {to_wide(a)};

    check_value("operator~", a, 0, T{~a}, wide_not(wa));
    check_value("unary operator-", a, 0, T{-a}, wide_t<T>{-wa});
    check_value("unary operator+", a, 0, T{+a}, wide_t<T>{+wa});

    auto pre_increment {a};
    auto wide_pre_increment {wa};
    check_value("pre operator++", a, 0, T{++pre_increment}, wide_t<T>{++wide_pre_increment});

    auto pre_decrement {a};
    auto wide_pre_decrement {wa};
    check_value("pre operator--", a, 0, T{--pre_decrement}, wide_t<T>{--wide_pre_decrement});

    auto post_increment {a};
    auto wide_post_increment {wa};
    check_value("post operator++ result", a, 0, T{post_increment++}, wide_t<T>{wide_post_increment++});
    check_value("post operator++ value", a, 0, post_increment, wide_post_increment);

    auto post_decrement {a};
    auto wide_post_decrement {wa};
    check_value("post operator-- result", a, 0, T{post_decrement--}, wide_t<T>{wide_post_decrement--});
    check_value("post operator-- value", a, 0, post_decrement, wide_post_decrement);

    // Every compound operator with itself as the right operand
    auto self_add {a};
    self_add += self_add;
    check_value("x += x", a, a, self_add, wide_t<T>{wa + wa});

    auto self_sub {a};
    self_sub -= self_sub;
    check_value("x -= x", a, a, self_sub, wide_t<T>{wa - wa});

    auto self_mul {a};
    self_mul *= self_mul;
    check_value("x *= x", a, a, self_mul, wide_t<T>{wa * wa});

    auto self_xor {a};
    self_xor ^= self_xor;
    check_value("x ^= x", a, a, self_xor, wide_t<T>{wa ^ wa});

    if (can_divide(a))
    {
        auto self_div {a};
        self_div /= self_div;
        check_value("x /= x", a, a, self_div, wide_t<T>{1});

        auto self_mod {a};
        self_mod %= self_mod;
        check_value("x %= x", a, a, self_mod, wide_t<T>{0});
    }

    if (!is_negative(a))
    {
        check_value("isqrt", a, 0, T{boost::int256::isqrt(a)}, wide_t<T>{sqrt(wa)});
    }

    const unsigned exponents[] {0U, 1U, 2U, 3U, 5U, 17U, 64U, 255U, 256U};
    for (const auto e : exponents)
    {
        check_value("ipow", a, e, T{boost::int256::ipow(a, e)}, wide_t<T>{pow(wa, e)});
    }
}

void check_bit_functions(const uint256& a)
{
    if (a == 0U)
    {
        return;
    }

    const auto wa {to_wide(a)};
    const auto high {static_cast<int>(msb(wa))};
    const auto low {static_cast<int>(lsb(wa))};

    check_equal("countl_zero", a, 0, boost::int256::countl_zero(a), 255 - high);
    check_equal("bit_width", a, 0, boost::int256::bit_width(a), high + 1);
    check_equal("countr_zero", a, 0, boost::int256::countr_zero(a), low);
}

void check_bit_functions(const int256&)
{
}

void check_abs(const uint256&)
{
}

// abs(MIN) wraps back to MIN in both libraries
void check_abs(const int256& a)
{
    check_value("abs", a, 0, int256{boost::int256::abs(a)}, wide_i256{abs(to_wide(a))});
}

template <typename T>
void check_one(const T& a)
{
    check_single_operand_functions(a);
    check_narrowing(a);
    check_formatting(a);
    check_parsing(a);
    check_bit_functions(a);
    check_abs(a);
}

// int256 always returns a non-negative gcd and lcm, the way std::gcd and
// std::lcm do, where uintwide_t's carry a sign. Compared on magnitudes, with
// MIN left out since its magnitude is not representable.
void check_gcd_lcm(const uint256& a, const uint256& b)
{
    const auto wa {to_wide(a)};
    const auto wb {to_wide(b)};

    check_value("gcd", a, b, uint256{boost::int256::gcd(a, b)}, wide_u256{gcd(wa, wb)});
    check_value("lcm", a, b, uint256{boost::int256::lcm(a, b)}, wide_u256{lcm(wa, wb)});
}

void check_gcd_lcm(const int256& a, const int256& b)
{
    if (a == min_value<int256>() || b == min_value<int256>())
    {
        return;
    }

    const auto wa {abs(to_wide(a))};
    const auto wb {abs(to_wide(b))};

    check_value("gcd", a, b, int256{boost::int256::gcd(a, b)}, wide_i256{gcd(wa, wb)});
    check_value("lcm", a, b, int256{boost::int256::lcm(a, b)}, wide_i256{lcm(wa, wb)});
}

// int256's signed powm reduces a negative base into [0, m) first, so only the
// non-negative operands are shared semantics
template <typename T>
void check_powm(const T& b, const T& e, const T& m)
{
    if (m == T{0U} || is_negative(b) || is_negative(e) || is_negative(m))
    {
        return;
    }

    check_value("powm", b, m, T{boost::int256::powm(b, e, m)}, wide_t<T>{powm(to_wide(b), to_wide(e), to_wide(m))});
}

// int256 and uint256 operands combine as uint256, the int256 operand being
// reinterpreted, exactly like a builtin signed and unsigned pair of one width
void check_cross_type(const int256& a, const uint256& b)
{
    const auto wa {to_wide(boost::int256::detail::to_unsigned_bits(a))};
    const auto wb {to_wide(b)};

    check_value("int256 + uint256", a, b, uint256{a + b}, wide_u256{wa + wb});
    check_value("int256 - uint256", a, b, uint256{a - b}, wide_u256{wa - wb});
    check_value("int256 * uint256", a, b, uint256{a * b}, wide_u256{wa * wb});
    check_value("uint256 - int256", b, a, uint256{b - a}, wide_u256{wb - wa});
    check_value("int256 & uint256", a, b, uint256{a & b}, wide_u256{wa & wb});
    check_value("int256 | uint256", a, b, uint256{a | b}, wide_u256{wa | wb});
    check_value("int256 ^ uint256", a, b, uint256{a ^ b}, wide_u256{wa ^ wb});

    if (b != 0U)
    {
        check_value("int256 / uint256", a, b, uint256{a / b}, wide_u256{wa / wb});
        check_value("int256 % uint256", a, b, uint256{a % b}, wide_u256{wa % wb});
    }

    if (a != 0)
    {
        check_value("uint256 / int256", b, a, uint256{b / a}, wide_u256{wb / wa});
        check_value("uint256 % int256", b, a, uint256{b % a}, wide_u256{wb % wa});
    }

    check_equal("int256 == uint256", a, b, a == b, wa == wb);
    check_equal("int256 < uint256", a, b, a < b, wa < wb);
    check_equal("uint256 < int256", b, a, b < a, wb < wa);
    check_equal("int256 >= uint256", a, b, a >= b, wa >= wb);
}

template <typename T>
void check_pair(const T& a, const T& b)
{
    check_arithmetic(a, b);
    check_bitwise(a, b);
    check_comparisons(a, b);
    check_shift(a, static_cast<unsigned>(b.words[0] % 256U));
    check_builtins(a, b.words[0]);
}

void check_pair_extras(const uint256&, const uint256&)
{
}

void check_pair_extras(const int256& a, const int256& b)
{
    check_cross_type(a, boost::int256::detail::to_unsigned_bits(b));
}

// Values that a double holds exactly, so that both conversion directions have
// a single right answer
template <typename T>
void check_floating_point(std::mt19937_64& rng, const std::size_t count)
{
    for (std::size_t i {}; i < count; ++i)
    {
        const auto mantissa {rng() >> 11U};
        const auto exponent {static_cast<int>(rng() % 200U)};
        const auto negative {(rng() & 1U) != 0U};
        const auto value {with_sign(std::ldexp(static_cast<double>(mantissa), exponent), negative, T{})};

        const T local {value};
        const wide_t<T> wide {value};

        check_value("construct from double", value, 0, local, wide);
        check_equal("convert to double", local, 0, static_cast<double>(local), static_cast<double>(wide));
    }
}

//=====================================
// Driving it
//=====================================

template <typename T>
void run_edge_corpus()
{
    const auto corpus {edge_corpus<T>()};

    for (const auto& a : corpus)
    {
        check_one(a);
        check_all_shifts(a);

        for (const auto& b : corpus)
        {
            check_pair(a, b);
            check_pair_extras(a, b);
            check_gcd_lcm(a, b);
        }
    }
}

template <typename T>
void run_random(std::mt19937_64& rng, const std::size_t iterations)
{
    const bool signs[] {false, true};
    const auto signs_used {sign_count(T{})};

    for (const auto sa : all_shapes)
    {
        for (const auto sb : all_shapes)
        {
            for (std::size_t na {}; na < signs_used; ++na)
            {
                for (std::size_t nb {}; nb < signs_used; ++nb)
                {
                    for (std::size_t i {}; i < iterations; ++i)
                    {
                        const auto a {random_value<T>(rng, sa, signs[na])};
                        const auto b {random_value<T>(rng, sb, signs[nb])};

                        check_one(a);
                        check_pair(a, b);
                        check_pair_extras(a, b);
                        check_gcd_lcm(a, b);

                        // The heavier checks run on a sample
                        if (i % 16U == 0U)
                        {
                            check_all_shifts(a);

                            const auto m {random_value<T>(rng, sb, false)};
                            check_powm(a, b, m);
                        }
                    }
                }
            }
        }
    }
}

template <typename T>
void run_suite(const char* name, const std::size_t iterations, const std::uint64_t seed)
{
    const auto checks_before {checks};
    const auto failures_before {failures};

    std::mt19937_64 rng {seed};

    run_edge_corpus<T>();
    run_random<T>(rng, iterations);
    check_floating_point<T>(rng, iterations * 64U);

    std::cout << name << ": " << (checks - checks_before) << " checks, "
              << (failures - failures_before) << " failures" << std::endl;
}

} // namespace

int main(int argc, char* argv[])
{
    std::size_t iterations {64U};
    std::uint64_t seed {42U};

    for (int i {1}; i + 1 < argc; i += 2)
    {
        if (std::strcmp(argv[i], "--iterations") == 0)
        {
            iterations = static_cast<std::size_t>(std::strtoull(argv[i + 1], nullptr, 10));
        }
        else if (std::strcmp(argv[i], "--seed") == 0)
        {
            seed = static_cast<std::uint64_t>(std::strtoull(argv[i + 1], nullptr, 10));
        }
    }

    std::cout << "uint256 and int256 versus uintwide_t<256>, " << iterations
              << " iterations per shape pair, seed " << seed << std::endl;

    run_suite<uint256>("uint256", iterations, seed);
    run_suite<int256>("int256", iterations, seed);

    if (failures > max_reports)
    {
        std::cerr << (failures - max_reports) << " further mismatches not shown" << std::endl;
    }

    return failures == 0U ? 0 : 1;
}
