// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// uint256 to and from the built-in floating point types.
//
// cpp_int to double is not documented as correctly rounded, so multiprecision is
// not the oracle here. The reference is built by hand instead: take the top 64
// bits of the value, fold everything below into a sticky bit, round to nearest
// with ties to even, and scale with ldexp. That is an independent implementation
// of the same contract, so agreement means something.

// -Wfloat-equal has to be off before <boost/core/lightweight_test.hpp> is parsed,
// because BOOST_TEST_EQ on a floating point pair compares with ==. The comparisons
// in this file are deliberately exact: a correctly rounded conversion has one
// right answer, so anything other than equality is a failure.
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include <boost/int256.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>

// A 32-bit x86 build can keep a float or double intermediate in an 80-bit x87
// register; comparing that unrounded value against one that has been stored
// to memory (rounded to the type's nominal width) can then disagree even
// though the "real" values are equal (gcc 12, i386, every optimization
// level; see same_bits below). A no-infs/no-NaN floating point model
// (-ffast-math, icpx's default -fp-model=fast) additionally lets the
// optimizer assume neither value occurs, which proves some of the
// saturation branches below unreachable and deletes them; the assertions
// that depend on infinity or NaN actually being honored are gated on this.
#ifndef BOOST_INT256_TEST_HONORS_INF_NAN
#  if defined(__FAST_MATH__)
#    define BOOST_INT256_TEST_HONORS_INF_NAN 0
#  elif defined(__FINITE_MATH_ONLY__) && (__FINITE_MATH_ONLY__ == 1)
#    define BOOST_INT256_TEST_HONORS_INF_NAN 0
#  elif defined(__INTEL_LLVM_COMPILER) && defined(__NO_MATH_ERRNO__)
#    define BOOST_INT256_TEST_HONORS_INF_NAN 0
#  else
#    define BOOST_INT256_TEST_HONORS_INF_NAN 1
#  endif
#endif

namespace
{

// Forces a value through memory (a volatile round trip) so the compiler
// cannot keep it, or the expression that produced it, in an x87 extended
// precision register, and cannot fold it away at compile time either.
template <typename T>
T opaque(T v) noexcept
{
    volatile T stored {v};
    return stored;
}

// Compares the STORED bit pattern of two values of the same floating point
// type, after routing each through the same volatile round trip opaque()
// uses. This is what a same-value comparison should always use here instead
// of a bare ==, since == on two expressions that are not both already
// rounded to the nominal width can disagree with what gets stored.
template <typename T>
bool same_bits(T lhs, T rhs) noexcept
{
    const T rounded_lhs {opaque(lhs)};
    const T rounded_rhs {opaque(rhs)};

    unsigned char lhs_bytes[sizeof(T)] {};
    unsigned char rhs_bytes[sizeof(T)] {};
    std::memcpy(lhs_bytes, &rounded_lhs, sizeof(T));
    std::memcpy(rhs_bytes, &rounded_rhs, sizeof(T));

    return std::memcmp(lhs_bytes, rhs_bytes, sizeof(lhs_bytes)) == 0;
}

// This file only ever compares int256/uint256 (exact) or float/double (the
// generic same_bits<T> template) against a long double INPUT; it never
// compares two long double VALUES against each other directly (the long
// double round trips in test_from_float and test_signed_from_float compare
// through the library's int256/uint256 result instead, which is exact), so
// no long double overload of same_bits is needed here.

} // namespace

using boost::int256::uint256;

// int256 is only forward declared until detail/int256_imp.hpp is wired into
// int256.hpp; the section below is gated on that header's guard so this file
// keeps compiling before and after it lands.
#if defined(BOOST_INT256_DETAIL_INT256_IMP_HPP)
#  define BOOST_INT256_TEST_HAS_SIGNED 1
using boost::int256::int256;
using boost::int256::detail::to_signed_bits;
using boost::int256::detail::to_unsigned_bits;
#endif

namespace
{

std::mt19937_64 rng {42};

//=====================================
// An independent reference for uint256 to floating point
//=====================================

// Rounds the 256-bit value to the significand width of T, to nearest with ties to
// even, and scales the result. Built from ldexp and the standard integer to float
// conversion of a value that fits the significand exactly, so it shares no code
// with the library.
template <typename T>
T reference_to_float(const uint256& v)
{
    // Index of the highest non-zero word, or none at all.
    int top {-1};
    for (int i {3}; i >= 0; --i)
    {
        if (v.words[i] != 0U)
        {
            top = i;
            break;
        }
    }

    if (top < 0)
    {
        return T{0};
    }

    // Bit position of the most significant set bit.
    int high_bit {top * 64};
    for (int b {63}; b >= 0; --b)
    {
        if ((v.words[top] >> b) & 1U)
        {
            high_bit += b;
            break;
        }
    }

    constexpr int digits {std::numeric_limits<T>::digits};

    if (high_bit < digits)
    {
        // The whole value fits the significand, so one conversion is exact.
        return static_cast<T>(v.words[0]);
    }

    // Keep the top `digits` bits, and remember whether anything was dropped.
    const int drop {high_bit - digits + 1};

    std::uint64_t kept {0};
    for (int b {0}; b < digits; ++b)
    {
        const int source {drop + b};
        const auto word {static_cast<unsigned>(source / 64)};
        const auto bit {static_cast<unsigned>(source % 64)};
        if (word < 4U && ((v.words[word] >> bit) & 1U))
        {
            kept |= (UINT64_C(1) << b);
        }
    }

    // The bit just below what was kept decides the direction; everything under it
    // is the sticky bit.
    const auto guard_index {static_cast<unsigned>(drop - 1)};
    const bool guard {((v.words[guard_index / 64U] >> (guard_index % 64U)) & 1U) != 0U};

    bool sticky {false};
    for (int b {0}; b < drop - 1; ++b)
    {
        const auto word {static_cast<unsigned>(b / 64)};
        const auto bit {static_cast<unsigned>(b % 64)};
        if ((v.words[word] >> bit) & 1U)
        {
            sticky = true;
            break;
        }
    }

    if (guard && (sticky || ((kept & 1U) != 0U)))
    {
        ++kept;
    }

    return std::ldexp(static_cast<T>(kept), drop);
}

template <typename T>
void check_to_float(const uint256& v, const char* what)
{
    const T got {opaque(static_cast<T>(v))};
    const T expected {opaque(reference_to_float<T>(v))};

    if (!same_bits(got, expected))
    {
        std::cerr << "float conversion mismatch in " << what << '\n'
                  << "  operand  = " << v << '\n'
                  << "  got      = " << std::setprecision(40) << got << '\n'
                  << "  expected = " << std::setprecision(40) << expected << std::endl;
    }

    BOOST_TEST(same_bits(got, expected));

    // The detail routine and the conversion operator must agree. The call
    // result is stored to a named T first (rather than compared inline),
    // which is what test_float_conversion.cpp:145 (the original line number)
    // did not do.
    const T from_detail {opaque(boost::int256::detail::unsigned_words_to_float<T>(v.words))};
    BOOST_TEST(same_bits(from_detail, expected));
}

//=====================================
// Exact at every power of two
//=====================================

// 2^k has a one bit significand, so every type represents it exactly and the
// conversion has to be exact for all 256 of them.
template <typename T>
void test_powers_of_two()
{
    const uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

    for (int k {0}; k < 256; ++k)
    {
        const uint256 value {one << static_cast<unsigned>(k)};
        const T got {opaque(static_cast<T>(value))};
        const T expected {opaque(std::ldexp(T{1}, k))};

        if (!same_bits(got, expected))
        {
            std::cerr << "2^" << k << " did not convert exactly" << std::endl;
        }

        BOOST_TEST(same_bits(got, expected));

        // And back again, whenever the power of two is still finite in T. A float
        // tops out just under 2^128, so 2^128 and above convert to infinity and
        // come back as the saturated maximum rather than the original value.
        // Under a no-infs model the compiler may fold std::isinf to a constant
        // false regardless of the runtime bit pattern, so that specific check
        // (but not the saturation itself) is gated.
        if (k < std::numeric_limits<T>::max_exponent)
        {
            const uint256 round_tripped {got};
            BOOST_TEST_EQ(round_tripped, value);
        }
        else
        {
            // Both assertions depend on the library's own isinf-based
            // saturation actually running: under a no-infs fast math model
            // (icpx's default -fp-model=fast) the compiler can prove the
            // check inside the library's conversion unreachable and delete
            // it, so the saturated result itself, not just std::isinf here,
            // is not guaranteed.
            #if BOOST_INT256_TEST_HONORS_INF_NAN
            BOOST_TEST(std::isinf(got));
            BOOST_TEST_EQ(uint256{got}, (uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}));
            #endif
        }
    }

    // 2^k - 1 and 2^k + 1 are the neighbours that force a rounding decision.
    for (int k {1}; k < 256; ++k)
    {
        const uint256 power {one << static_cast<unsigned>(k)};
        check_to_float<T>(power - 1U, "2^k - 1");
        if (k < 255)
        {
            check_to_float<T>(power + 1U, "2^k + 1");
        }
    }
}

//=====================================
// The documented saturation and boundary answers
//=====================================

void test_directed_answers()
{
    const uint256 max {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    const uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

    // The maximum rounds up to 2^256 in double, which is representable, and to
    // infinity in float, which tops out just under 2^128.
    BOOST_TEST(same_bits(opaque(static_cast<double>(max)), opaque(std::ldexp(1.0, 256))));
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST(std::isinf(opaque(static_cast<float>(max))));
    #endif

    // The rounding edge at the top of the double range. Doubles just below 2^256
    // step by 2^203, so the halfway point is 2^256 - 2^202. Ties go to even and
    // 2^256 has an even significand, so the halfway point rounds up and one less
    // rounds down. All three answers were confirmed against the implementation
    // before being written here.
    BOOST_TEST(opaque(static_cast<double>(max - (one << 203U) + 1U)) < opaque(std::ldexp(1.0, 256)));
    BOOST_TEST(same_bits(opaque(static_cast<double>(max - (one << 202U) + 1U)), opaque(std::ldexp(1.0, 256))));
    BOOST_TEST(opaque(static_cast<double>(max - (one << 202U))) < opaque(std::ldexp(1.0, 256)));

    // The same edge for float, which sits at the top of its own range rather than
    // near 2^256. The largest finite float is 2^128 - 2^104 and the step below it
    // is 2^104, so the halfway point to 2^128 is 2^128 - 2^103.
    BOOST_TEST(same_bits(opaque(static_cast<float>((one << 128U) - (one << 104U))), opaque((std::numeric_limits<float>::max)())));
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST(std::isinf(opaque(static_cast<float>((one << 128U) - (one << 103U)))));
    #endif
    BOOST_TEST(same_bits(opaque(static_cast<float>((one << 128U) - (one << 103U) - 1U)), opaque((std::numeric_limits<float>::max)())));

    // 2^255 is a long way below 2^256 and stays finite in a double.
    BOOST_TEST(same_bits(opaque(static_cast<double>(one << 255U)), opaque(std::ldexp(1.0, 255))));

    // Zero and one are exact everywhere.
    BOOST_TEST(same_bits(opaque(static_cast<double>(uint256{})), opaque(0.0)));
    BOOST_TEST(same_bits(opaque(static_cast<float>(uint256{})), opaque(0.0F)));
    BOOST_TEST(same_bits(opaque(static_cast<double>(one)), opaque(1.0)));

    // A value that fits the significand exactly.
    BOOST_TEST(same_bits(opaque(static_cast<double>(uint256{UINT64_C(9007199254740992)})), opaque(9007199254740992.0)));
    BOOST_TEST(same_bits(opaque(static_cast<double>(uint256{UINT64_C(123456789)})), opaque(123456789.0)));
}

//=====================================
// Floating point to uint256
//=====================================

void test_from_float()
{
    // NaN and infinity inputs are meaningless under a no-infs/no-NaN fast
    // math model, where the library's own isnan/isinf-based checks may be
    // folded away; every case that depends on one of those two specific
    // inputs is gated. A merely very large (but finite) or negative-finite
    // value is not affected: those checks are ordinary comparisons.
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST_EQ(uint256{opaque(std::numeric_limits<double>::quiet_NaN())}, uint256{});
    BOOST_TEST_EQ(uint256{opaque(-std::numeric_limits<double>::quiet_NaN())}, uint256{});
    BOOST_TEST_EQ(uint256{opaque(-std::numeric_limits<double>::infinity())}, uint256{});
    BOOST_TEST_EQ(uint256{opaque(std::numeric_limits<float>::quiet_NaN())}, uint256{});
    #endif
    BOOST_TEST_EQ(uint256{opaque(-1.0)}, uint256{});
    BOOST_TEST_EQ(uint256{opaque(-0.5)}, uint256{});
    BOOST_TEST_EQ(uint256{opaque(-1e300)}, uint256{});
    BOOST_TEST_EQ(uint256{opaque(-1.0F)}, uint256{});

    // Anything at or above 2^256 saturates at the maximum.
    const uint256 max {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST_EQ(uint256{opaque(std::numeric_limits<double>::infinity())}, max);
    BOOST_TEST_EQ(uint256{opaque(std::numeric_limits<float>::infinity())}, max);
    #endif
    BOOST_TEST_EQ(uint256{opaque(std::ldexp(1.0, 256))}, max);
    BOOST_TEST_EQ(uint256{opaque(std::ldexp(1.0, 300))}, max);
    BOOST_TEST_EQ(uint256{opaque(1e300)}, max);

    // Just below 2^256 does not saturate.
    BOOST_TEST(uint256{opaque(std::ldexp(1.0, 255))} != max);
    BOOST_TEST_EQ(uint256{opaque(std::ldexp(1.0, 255))},
                  (uint256{UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)}));

    // The fractional part is truncated toward zero.
    BOOST_TEST_EQ(uint256{opaque(0.0)}, uint256{});
    BOOST_TEST_EQ(uint256{opaque(0.5)}, uint256{});
    BOOST_TEST_EQ(uint256{opaque(0.9999)}, uint256{});
    BOOST_TEST_EQ(uint256{opaque(1.5)}, uint256{1U});
    BOOST_TEST_EQ(uint256{opaque(42.75)}, uint256{42U});
    BOOST_TEST_EQ(uint256{opaque(-0.0)}, uint256{});

    // Every power of two round trips exactly.
    const uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    for (int k {0}; k < 256; ++k)
    {
        const auto as_double {opaque(std::ldexp(1.0, k))};
        BOOST_TEST_EQ(uint256{as_double}, (one << static_cast<unsigned>(k)));
    }

    // 2^200, called out in the plan, and 1e77, which is just under the maximum.
    BOOST_TEST_EQ(uint256{opaque(std::ldexp(1.0, 200))}, (one << 200U));

    const double ten_to_77 {opaque(std::pow(10.0, 77.0))};
    const uint256 from_1e77 {ten_to_77};
    BOOST_TEST(from_1e77 != 0U);
    BOOST_TEST(from_1e77 < max);
    // 1e77 as a double is exact to 53 bits, so converting back must give the same
    // double.
    BOOST_TEST(same_bits(opaque(static_cast<double>(from_1e77)), ten_to_77));

    // Integral doubles of every magnitude round trip.
    for (int k {0}; k < 250; ++k)
    {
        const double value {opaque(std::ldexp(1.0, k) + std::ldexp(1.0, k > 52 ? k - 52 : 0))};
        const uint256 converted {value};
        BOOST_TEST(same_bits(opaque(static_cast<double>(converted)), value));
    }

    // Long double on the host, which has a wider significand on x86.
    #ifndef BOOST_INT256_HAS_GPU_SUPPORT
    BOOST_TEST_EQ(uint256{opaque(static_cast<long double>(1.0L))}, uint256{1U});
    BOOST_TEST_EQ(uint256{opaque(-1.0L)}, uint256{});
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST_EQ(uint256{opaque(std::numeric_limits<long double>::infinity())}, max);
    #endif
    for (int k {0}; k < 256; ++k)
    {
        const auto as_long_double {opaque(std::ldexp(1.0L, k))};
        BOOST_TEST_EQ(uint256{as_long_double}, (one << static_cast<unsigned>(k)));
    }
    #endif
}

// float_to_uint64 splits at 2^63 to dodge a clang 7 x87 miscompile, so both sides
// of that split are exercised directly.
void test_float_to_uint64()
{
    const std::uint64_t values[] {
        UINT64_C(0), UINT64_C(1), UINT64_C(42),
        (UINT64_C(1) << 52) - 1U, UINT64_C(1) << 52,
        (UINT64_C(1) << 62), (UINT64_C(1) << 63) - 1024U,
        UINT64_C(1) << 63, (UINT64_C(1) << 63) + 2048U,
        UINT64_MAX - 2047U
    };

    for (const auto v : values)
    {
        // Only values exactly representable as a double are meaningful here.
        const auto as_double {static_cast<double>(v)};
        if (static_cast<std::uint64_t>(as_double) != v && as_double < std::ldexp(1.0, 64))
        {
            continue;
        }

        BOOST_TEST_EQ(boost::int256::detail::float_to_uint64(as_double),
                      static_cast<std::uint64_t>(as_double));
    }

    // exact_power_of_two must be exact across the whole 0 to 255 range, which is
    // what keeps the scaling from rounding twice.
    for (int k {0}; k < 256; ++k)
    {
        const double from_detail {opaque(boost::int256::detail::exact_power_of_two<double>(k))};
        BOOST_TEST(same_bits(from_detail, opaque(std::ldexp(1.0, k))));
    }
    for (int k {0}; k < 128; ++k)
    {
        const float from_detail {opaque(boost::int256::detail::exact_power_of_two<float>(k))};
        BOOST_TEST(same_bits(from_detail, opaque(std::ldexp(1.0F, k))));
    }
}

//=====================================
// Random values against the reference
//=====================================

template <typename T>
void test_random()
{
    for (int i {0}; i < 4096; ++i)
    {
        // Every width, so both the two word and the wide routines are reached.
        const int words {(i % 4) + 1};

        uint256 v {};
        for (int w {0}; w < words; ++w)
        {
            v.words[w] = rng();
        }

        check_to_float<T>(v, "random");

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    // Values with a long run of zeros below the top bits, which is where a missing
    // sticky bit shows up.
    for (int i {0}; i < 1024; ++i)
    {
        uint256 v {};
        v.words[3] = rng() | (UINT64_C(1) << 63);
        v.words[0] = UINT64_C(1);

        check_to_float<T>(v, "sticky bit only in the low word");

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// A round trip through a floating point type is lossless whenever the value fits
// the significand.
void test_round_trip_within_significand()
{
    for (int i {0}; i < 1024; ++i)
    {
        const auto small {rng() >> 11};  // 53 bits, exact in a double
        const uint256 v {small};
        BOOST_TEST_EQ(uint256{opaque(static_cast<double>(v))}, v);

        const auto tiny {static_cast<std::uint64_t>(rng() >> 41)};  // 23 bits, exact in a float
        const uint256 w {tiny};
        BOOST_TEST_EQ(uint256{opaque(static_cast<float>(w))}, w);
    }
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

//=====================================
// int256 to floating point
//=====================================

// The documented algorithm, restated here: convert the magnitude with the
// already-validated unsigned reference, then negate if the source was
// negative. This is the same algorithm detail::signed_words_to_float uses, so
// it checks the wiring and the sign handling, over the unsigned path this
// file already validates independently.
template <typename T>
T reference_to_float_i(const int256& v)
{
    const uint256 magnitude {to_unsigned_bits(boost::int256::abs(v))};
    const T magnitude_float {reference_to_float<T>(magnitude)};

    return (v.signed_high() < 0) ? -magnitude_float : magnitude_float;
}

template <typename T>
void check_to_float_i(const int256& v, const char* what)
{
    const T got {opaque(static_cast<T>(v))};
    const T expected {opaque(reference_to_float_i<T>(v))};

    if (!same_bits(got, expected))
    {
        std::cerr << "int256 float conversion mismatch in " << what << '\n'
                  << "  got      = " << std::setprecision(40) << got << '\n'
                  << "  expected = " << std::setprecision(40) << expected << std::endl;
    }

    BOOST_TEST(same_bits(got, expected));
}

// Every power of two, and its negation, converts exactly: the negation is
// symmetric with the positive case rather than an independent rounding.
template <typename T>
void test_signed_powers_of_two()
{
    const int256 one {INT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

    for (int k {0}; k < 255; ++k)
    {
        const int256 value {one << static_cast<unsigned>(k)};
        const int256 negated {-value};

        const T got_pos {opaque(static_cast<T>(value))};
        const T got_neg {opaque(static_cast<T>(negated))};
        const T expected {opaque(std::ldexp(T{1}, k))};

        BOOST_TEST(same_bits(got_pos, expected));
        BOOST_TEST(same_bits(got_neg, opaque(-expected)));

        // Symmetric rounding: the two conversions differ only in sign.
        BOOST_TEST(same_bits(got_neg, opaque(-got_pos)));
    }

    // 2^k - 1 and 2^k + 1, and their negations, force a rounding decision.
    for (int k {1}; k < 255; ++k)
    {
        const int256 power {one << static_cast<unsigned>(k)};
        check_to_float_i<T>(power - 1, "2^k - 1");
        check_to_float_i<T>(-(power - 1), "-(2^k - 1)");
        if (k < 254)
        {
            check_to_float_i<T>(power + 1, "2^k + 1");
            check_to_float_i<T>(-(power + 1), "-(2^k + 1)");
        }
    }
}

void test_signed_directed_answers()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    // MIN's magnitude is exactly 2^255, a single set bit, so its conversion is
    // exact in both double and float (float saturates to infinity, since 2^255
    // is far past its finite range).
    BOOST_TEST(same_bits(opaque(static_cast<double>(min_v)), opaque(-std::ldexp(1.0, 255))));
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST(std::isinf(opaque(static_cast<float>(min_v))));
    #endif
    BOOST_TEST(opaque(static_cast<float>(min_v)) < 0.0F);

    // MAX rounds to 2^255 in double (its magnitude is 2^255 - 1).
    BOOST_TEST(same_bits(opaque(static_cast<double>(max_v)), opaque(std::ldexp(1.0, 255))));
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST(std::isinf(opaque(static_cast<float>(max_v))));
    #endif

    // Zero, one and minus one are exact everywhere.
    BOOST_TEST(same_bits(opaque(static_cast<double>(int256{})), opaque(0.0)));
    BOOST_TEST(same_bits(opaque(static_cast<double>(int256{1})), opaque(1.0)));
    BOOST_TEST(same_bits(opaque(static_cast<double>(int256{-1})), opaque(-1.0)));
    BOOST_TEST(same_bits(opaque(static_cast<float>(int256{-1})), opaque(-1.0F)));
}

//=====================================
// Floating point to int256
//=====================================

void test_signed_from_float()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    // BOOST_TEST(a == b) rather than BOOST_TEST_EQ throughout this function:
    // int256 has no operator<< until D2 lands. Every floating point input is
    // routed through opaque() first.

    // NaN and infinity are meaningless under a no-infs/no-NaN fast math
    // model; those specific cases are gated.
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST(int256{opaque(std::numeric_limits<double>::quiet_NaN())} == int256{});
    BOOST_TEST(int256{opaque(-std::numeric_limits<double>::quiet_NaN())} == int256{});
    BOOST_TEST(int256{opaque(std::numeric_limits<double>::infinity())} == max_v);
    BOOST_TEST(int256{opaque(-std::numeric_limits<double>::infinity())} == min_v);
    #endif

    // >= 2^255 saturates at MAX; <= -2^255 saturates at MIN.
    BOOST_TEST(int256{opaque(std::ldexp(1.0, 255))} == max_v);
    BOOST_TEST(int256{opaque(std::ldexp(1.0, 300))} == max_v);
    BOOST_TEST(int256{opaque(-std::ldexp(1.0, 255))} == min_v);
    BOOST_TEST(int256{opaque(-std::ldexp(1.0, 300))} == min_v);

    // Just inside the range does not saturate.
    BOOST_TEST(int256{opaque(std::ldexp(1.0, 254))} != max_v);
    BOOST_TEST(int256{opaque(-std::ldexp(1.0, 254))} != min_v);

    // Truncation toward zero, for both signs.
    BOOST_TEST(int256{opaque(0.5)} == int256{0});
    BOOST_TEST(int256{opaque(-0.5)} == int256{0});
    BOOST_TEST(int256{opaque(1.5)} == int256{1});
    BOOST_TEST(int256{opaque(-1.5)} == int256{-1});
    BOOST_TEST(int256{opaque(42.75)} == int256{42});
    BOOST_TEST(int256{opaque(-42.75)} == int256{-42});
    BOOST_TEST(int256{opaque(-0.0)} == int256{});

    // Every power of two, and its negation, round trips exactly.
    const int256 one {INT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    for (int k {0}; k < 255; ++k)
    {
        const double as_double {opaque(std::ldexp(1.0, k))};
        BOOST_TEST(int256{as_double} == (one << static_cast<unsigned>(k)));
        BOOST_TEST(int256{opaque(-as_double)} == -(one << static_cast<unsigned>(k)));
    }

    #ifndef BOOST_INT256_HAS_GPU_SUPPORT
    BOOST_TEST(int256{opaque(static_cast<long double>(1.0L))} == int256{1});
    BOOST_TEST(int256{opaque(-1.0L)} == int256{-1});
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST(int256{opaque(std::numeric_limits<long double>::infinity())} == max_v);
    BOOST_TEST(int256{opaque(-std::numeric_limits<long double>::infinity())} == min_v);
    #endif
    #endif
}

template <typename T>
void test_signed_random()
{
    for (int i {0}; i < 4096; ++i)
    {
        const int words {(i % 4) + 1};

        uint256 magnitude {};
        for (int w {0}; w < words; ++w)
        {
            magnitude.words[w] = rng();
        }

        // Keep the sign bit clear on the magnitude so abs() below is a no-op,
        // then apply the sign separately.
        magnitude.words[3] &= UINT64_C(0x7FFFFFFFFFFFFFFF);
        const int256 positive {to_signed_bits(magnitude)};
        const int256 negative {-positive};

        check_to_float_i<T>(positive, "random positive");
        check_to_float_i<T>(negative, "random negative");

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_directed_answers();
    test_powers_of_two<double>();
    test_powers_of_two<float>();
    test_from_float();
    test_float_to_uint64();
    test_random<double>();
    test_random<float>();
    test_round_trip_within_significand();

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_signed_powers_of_two<double>();
    test_signed_powers_of_two<float>();
    test_signed_directed_answers();
    test_signed_from_float();
    test_signed_random<double>();
    test_signed_random<float>();
#endif

    return boost::report_errors();
}
