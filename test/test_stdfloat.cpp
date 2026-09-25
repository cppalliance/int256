// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// C++23 <stdfloat> extended floating point types (std::float16_t, float32_t, float64_t,
// float128_t, bfloat16_t) mixed with uint256/int256: construction, conversion, mixed
// arithmetic, comparisons, compound assignment, and the deleted operators.
//
// This file compiles to a no-op before C++23, or on a standard library that ships
// <stdfloat> but not a given extended type. Never gate any of this on
// __cpp_lib_stdfloat: libstdc++ (as of gcc-16) does not define that macro even though
// <stdfloat> and the types both exist. Use the compiler-predefined __STDCPP_..._T__
// macros instead, one per type.

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
#include <limits>
#include <type_traits>

#ifdef __has_include
#  if (__cplusplus > 202002L || (defined(_MSVC_LANG) && _MSVC_LANG > 202002L)) && __has_include(<stdfloat>)
#    include <stdfloat>
#  endif
#endif

// The rest of this file is only ever meaningful under C++23 (that is what
// <stdfloat> and its types require), and test_mixed_ops below uses <=> (a
// C++20 token) unconditionally in its body. <=> is not even valid syntax
// pre-C++20 (it tokenizes as separate '<', '=', '>'), and that is a hard
// parse error regardless of whether the template is ever instantiated, so
// the whole substantive body has to be gated at the preprocessor level, not
// just per call site in main() as the individual __STDCPP_..._T__ guards do.
#if __cplusplus > 202002L || (defined(_MSVC_LANG) && _MSVC_LANG > 202002L)
#define BOOST_INT256_TEST_STDFLOAT_CPP23 1
#endif

#ifdef BOOST_INT256_TEST_STDFLOAT_CPP23

// See test_float_conversion.cpp (the same rationale applies verbatim): a 32-bit x87
// build can keep an intermediate in an 80-bit register, and -ffast-math lets the
// optimizer assume neither infinity nor NaN occurs.
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

using boost::int256::uint256;
using boost::int256::int256;

namespace
{

template <typename T>
T opaque(T v) noexcept
{
    volatile T stored {v};
    return stored;
}

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

// An independent reference for a 4-word magnitude -> T, for any T whose significand
// is at most 64 bits (every currently defined <stdfloat> type except
// std::float128_t). Keeps the top digits(T) bits, folds everything below into a
// guard and a sticky bit, rounds to nearest with ties to even, and scales the exact
// kept integer by an exact power of two: built from nothing the library itself
// uses. The final T() narrows a double that is already exactly the correctly
// rounded value (or exactly the overflow threshold), so that narrowing is not a
// second rounding.
template <typename T>
T reference_to_float(const std::uint64_t (&words)[4])
{
    static_assert(std::numeric_limits<T>::digits <= 64, "use the exact-value-only path for float128_t");

    int top {-1};
    for (int i {3}; i >= 0; --i)
    {
        if (words[i] != 0U) { top = i; break; }
    }

    if (top < 0)
    {
        return T{0};
    }

    int high_bit {top * 64};
    for (int b {63}; b >= 0; --b)
    {
        if ((words[top] >> b) & 1U) { high_bit += b; break; }
    }

    constexpr int digits {std::numeric_limits<T>::digits};

    if (high_bit < digits)
    {
        return static_cast<T>(words[0]);
    }

    if (high_bit + 1 > std::numeric_limits<T>::max_exponent)
    {
        return std::numeric_limits<T>::infinity();
    }

    const int drop {high_bit - digits + 1};

    const auto bit_at = [&](int pos) -> bool
    {
        const auto word {static_cast<unsigned>(pos / 64)};
        const auto bit {static_cast<unsigned>(pos % 64)};
        return word < 4U && ((words[word] >> bit) & 1U) != 0U;
    };

    std::uint64_t kept {0};
    for (int b {0}; b < digits; ++b)
    {
        if (bit_at(drop + b))
        {
            kept |= (UINT64_C(1) << b);
        }
    }

    const bool guard {bit_at(drop - 1)};
    bool sticky {false};
    for (int b {0}; b < drop - 1; ++b)
    {
        if (bit_at(b)) { sticky = true; break; }
    }

    if (guard && (sticky || ((kept & 1U) != 0U)))
    {
        ++kept;
    }

    return static_cast<T>(std::ldexp(static_cast<double>(kept), drop));
}

template <typename T>
void check_to_float(const uint256& v, const char* what)
{
    const T got {opaque(static_cast<T>(v))};
    const T expected {opaque(reference_to_float<T>(v.words))};

    if (!same_bits(got, expected))
    {
        std::fprintf(stderr, "mismatch in %s: got=%.20Lf expected=%.20Lf\n",
                      what, static_cast<long double>(got), static_cast<long double>(expected));
    }

    BOOST_TEST(same_bits(got, expected));
}

template <typename T>
void check_signed_to_float(const int256& v, const char* what)
{
    const T got {opaque(static_cast<T>(v))};

    const bool negative {v.signed_high() < 0};
    const uint256 mag {negative ? static_cast<uint256>(-v) : static_cast<uint256>(v)};
    const T mag_ref {reference_to_float<T>(mag.words)};
    const T expected {opaque(negative ? -mag_ref : mag_ref)};

    if (!same_bits(got, expected))
    {
        std::fprintf(stderr, "mismatch in %s: got=%.20Lf expected=%.20Lf\n",
                      what, static_cast<long double>(got), static_cast<long double>(expected));
    }

    BOOST_TEST(same_bits(got, expected));
}

// SFINAE probe: true if `Lib op= T` is well formed
template <typename Lib, typename T, typename = void>
struct has_mod_assign : std::false_type {};
template <typename Lib, typename T>
struct has_mod_assign<Lib, T, decltype((void)(std::declval<Lib&>() %= std::declval<T>()))> : std::true_type {};

template <typename Lib, typename T, typename = void>
struct has_and_assign : std::false_type {};
template <typename Lib, typename T>
struct has_and_assign<Lib, T, decltype((void)(std::declval<Lib&>() &= std::declval<T>()))> : std::true_type {};

template <typename Lib, typename T, typename = void>
struct has_shl_assign : std::false_type {};
template <typename Lib, typename T>
struct has_shl_assign<Lib, T, decltype((void)(std::declval<Lib&>() <<= std::declval<T>()))> : std::true_type {};

template <typename T>
void test_deleted_ops()
{
    static_assert(!has_mod_assign<uint256, T>::value, "uint256 %= T must stay deleted");
    static_assert(!has_and_assign<uint256, T>::value, "uint256 &= T must stay deleted");
    static_assert(!has_shl_assign<uint256, T>::value, "uint256 <<= T must stay deleted");
    static_assert(!has_mod_assign<int256, T>::value, "int256 %= T must stay deleted");
    static_assert(!has_and_assign<int256, T>::value, "int256 &= T must stay deleted");
    static_assert(!has_shl_assign<int256, T>::value, "int256 <<= T must stay deleted");
}

template <typename T>
void test_construction(const char* name)
{
    // Exact small integer
    BOOST_TEST_EQ(uint256{T{7}}, uint256{7});
    BOOST_TEST_EQ(int256{T{-7}}, int256{-7});

    // Truncation toward zero of a non-integral value
    BOOST_TEST_EQ(uint256{static_cast<T>(12.75)}, uint256{12});
    BOOST_TEST_EQ(int256{static_cast<T>(-12.75)}, int256{-12});

    // Negative into unsigned gives zero
    BOOST_TEST_EQ(uint256{T{-5}}, uint256{0});

    // NaN into either gives zero (gated: -ffast-math can fold this away)
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    const T nan_val {std::numeric_limits<T>::quiet_NaN()};
    BOOST_TEST_EQ(uint256{nan_val}, uint256{0});
    BOOST_TEST_EQ(int256{nan_val}, int256{0});
    #endif

    // Saturation: +infinity into unsigned saturates to max, into signed to the
    // int256 max; -infinity into signed saturates to the int256 min
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    const T inf_val {std::numeric_limits<T>::infinity()};
    BOOST_TEST_EQ(uint256{inf_val}, (std::numeric_limits<uint256>::max)());
    BOOST_TEST_EQ(int256{inf_val}, (std::numeric_limits<int256>::max)());
    BOOST_TEST_EQ(int256{-inf_val}, (std::numeric_limits<int256>::min)());
    #endif

    // The largest finite value of T constructs without UB, whatever its magnitude
    // relative to 256 bits. float16_t's max (65504) fits inside uint256 exactly, so
    // that round trip must be exact; every wider type's max here (float32_t,
    // float64_t, bfloat16_t, float128_t: none exceed max_exponent 16384) also fits
    // inside 256 bits, so this is always an exact round trip for this library
    const T max_finite {(std::numeric_limits<T>::max)()};
    if (std::numeric_limits<T>::max_exponent <= 256)
    {
        const auto as_u256 {uint256{max_finite}};
        const auto back {static_cast<T>(as_u256)};
        BOOST_TEST(same_bits(opaque(back), opaque(max_finite)));
    }
    else
    {
        BOOST_TEST_EQ(uint256{max_finite}, (std::numeric_limits<uint256>::max)());
        BOOST_TEST_EQ(int256{max_finite}, (std::numeric_limits<int256>::max)());
    }

    (void)name;
}

template <typename T>
void test_conversion_narrow(const char* name)
{
    check_to_float<T>(uint256{0, 0, 0, 0}, name);
    check_to_float<T>(uint256{0, 0, 0, 1}, name);
    check_to_float<T>(uint256{0, 0, 0, 100}, name);
    check_to_float<T>(uint256{0, 0, 0, 65504}, name); // float16_t's max finite value
    check_to_float<T>(uint256{0, 0, 0, UINT64_C(1000000)}, name); // beyond float16_t's range
    check_to_float<T>(uint256{0, 0, 0, UINT64_MAX}, name);
    check_to_float<T>(uint256{0, 0, 1, 0}, name);
    check_to_float<T>(uint256{0, 1, 0, 0}, name);
    check_to_float<T>(uint256{1, 0, 0, 0}, name);
    check_to_float<T>(uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}, name);

    // A halfway case for a type with `digits` bits: 2^digits + 2^(digits-1) has its
    // guard bit set and nothing below it, so ties-to-even decides the direction
    constexpr int digits {std::numeric_limits<T>::digits};
    const std::uint64_t halfway_case {(UINT64_C(1) << digits) | (UINT64_C(1) << (digits - 1))};
    check_to_float<T>(uint256{0, 0, 0, halfway_case}, name);

    check_signed_to_float<T>(int256{-1}, name);
    check_signed_to_float<T>(int256{static_cast<std::int64_t>(-65504)}, name);
    check_signed_to_float<T>((std::numeric_limits<int256>::min)(), name);
}

template <typename T>
void test_mixed_ops(const char* name)
{
    const uint256 u {0, 0, 0, 5};
    const int256 i {-5};
    const T half {T{1} / T{2}};

    {
        const auto r {u + half};
        static_assert(std::is_same<decltype(r), const T>::value, "result type must be T");
        BOOST_TEST(same_bits(opaque(r), opaque(T{5} + half)));
    }
    {
        const auto r {half + u};
        static_assert(std::is_same<decltype(r), const T>::value, "result type must be T");
        BOOST_TEST(same_bits(opaque(r), opaque(half + T{5})));
    }
    {
        const auto r {i + half};
        static_assert(std::is_same<decltype(r), const T>::value, "result type must be T");
        BOOST_TEST(same_bits(opaque(r), opaque(T{-5} + half)));
    }

    BOOST_TEST(u < T{10});
    BOOST_TEST(!(T{10} < u));
    BOOST_TEST(i < T{0});

    const auto ord {u <=> T{5}};
    static_assert(std::is_same<decltype(ord), const std::partial_ordering>::value, "must be partial_ordering");
    BOOST_TEST(ord == std::partial_ordering::equivalent);

    // Compound assignment on an integer type stays that integer type: the operation
    // runs in T, and the result truncates back toward zero, exactly like the
    // builtin uint256 += double would
    {
        uint256 x {0, 0, 0, 5};
        x += half; // 5.5 truncates to 5
        BOOST_TEST_EQ(x, uint256{5});
    }
    {
        int256 x {-5};
        x -= half; // -5.5 truncates to -5
        BOOST_TEST_EQ(x, int256{-5});
    }

    test_deleted_ops<T>();
    (void)name;
}

void test_float128_exact()
{
    #ifdef __STDCPP_FLOAT128_T__

    using T = std::float128_t;

    BOOST_TEST_EQ(uint256{T{7}}, uint256{7});
    BOOST_TEST_EQ(int256{T{-7}}, int256{-7});
    BOOST_TEST_EQ(uint256{T{-5}}, uint256{0});

    // 113 digits covers a 256-bit magnitude exactly only when no more than 113 of
    // its bits are significant; only the top two words are nonzero here (128 bits
    // of magnitude, 15 low bits zeroed), so the conversion is exact (never
    // rounded), and a plain equality check is valid
    const uint256 big {UINT64_C(0), UINT64_C(0), UINT64_C(0x0102030405060708), UINT64_C(0x1112131415160000)};
    const T as_f128 {static_cast<T>(big)};
    const auto back {static_cast<uint256>(as_f128)};
    BOOST_TEST_EQ(back, big);

    constexpr auto mn {(std::numeric_limits<int256>::min)()};
    const T mn_f128 {static_cast<T>(mn)};
    BOOST_TEST_EQ(static_cast<int256>(mn_f128), mn);

    const auto r {uint256{5} + T{0.5}};
    static_assert(std::is_same<decltype(r), const T>::value, "result type must be float128_t");
    BOOST_TEST(r == T{5.5});

    test_deleted_ops<T>();

    #endif // __STDCPP_FLOAT128_T__
}

// Existing code must not gain new ambiguity now that the extended types satisfy
// detail::is_floating_point_v: plain double/float usage, integer conversions, and
// std::sqrt/std::abs-style calls on the builtin types are unaffected.
void test_no_new_ambiguity()
{
    const uint256 x {0, 0, 0, 4};
    BOOST_TEST(x == 4.0);
    BOOST_TEST(!(x == 1.5));
    const auto as_int {static_cast<int>(x)};
    BOOST_TEST_EQ(as_int, 4);
    const auto as_double {static_cast<double>(x)};
    BOOST_TEST_EQ(as_double, 4.0);
    using std::sqrt;
    BOOST_TEST_EQ(sqrt(as_double), 2.0);
}

} // namespace

int main()
{
    test_no_new_ambiguity();

    #ifdef __STDCPP_FLOAT16_T__
    test_construction<std::float16_t>("float16_t");
    test_conversion_narrow<std::float16_t>("float16_t");
    test_mixed_ops<std::float16_t>("float16_t");
    #endif

    #ifdef __STDCPP_BFLOAT16_T__
    test_construction<std::bfloat16_t>("bfloat16_t");
    test_conversion_narrow<std::bfloat16_t>("bfloat16_t");
    test_mixed_ops<std::bfloat16_t>("bfloat16_t");
    #endif

    #ifdef __STDCPP_FLOAT32_T__
    test_construction<std::float32_t>("float32_t");
    test_conversion_narrow<std::float32_t>("float32_t");
    test_mixed_ops<std::float32_t>("float32_t");
    #endif

    #ifdef __STDCPP_FLOAT64_T__
    test_construction<std::float64_t>("float64_t");
    test_conversion_narrow<std::float64_t>("float64_t");
    test_mixed_ops<std::float64_t>("float64_t");
    #endif

    test_float128_exact();

    return boost::report_errors();
}

#else // BOOST_INT256_TEST_STDFLOAT_CPP23

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#endif // BOOST_INT256_TEST_STDFLOAT_CPP23

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
