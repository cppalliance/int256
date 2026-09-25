// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Mixed uint256 and floating point expressions. The usual arithmetic conversions
// turn the integer operand into the floating point type before the operation, so
// every one of these must compute bit for bit what the same expression computes
// with the integer replaced by its converted value. The operators the builtins
// do not offer for a floating point operand (modulo, bitwise, shifts) are
// deleted rather than absent, so they are a hard error instead of silently
// truncating through the implicit constructor.

// The comparisons below are exact on purpose, so -Wfloat-equal has to be off
// before <boost/core/lightweight_test.hpp> is parsed: BOOST_TEST_EQ compares
// with ==, and the warning fires inside the macro expansion otherwise.
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
#include <random>
#include <type_traits>

// See test_float_conversion.cpp for the full rationale: a 32-bit x86 build
// can keep a float or double intermediate in an 80-bit x87 register, so an
// inline comparison (an expression compared without first being stored to a
// named object) can disagree with one that has been stored, even though the
// "real" values are equal. Every comparison of a floating point RESULT below
// therefore stores both sides first (opaque()) and compares stored bit
// patterns (same_bits()) rather than using == directly.
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

// long double's object representation can include padding bytes (the 80-bit
// x87 extended format stored in a 12 or 16 byte object on x86) whose content
// is unspecified, so only the meaningful low 10 bytes are compared. This
// overload (an exact match) is preferred over the generic template above
// whenever both arguments are long double, which is what check_agrees and
// check_agrees_i instantiate T = long double actually calls.
inline bool same_bits(long double lhs, long double rhs) noexcept
{
    const long double rounded_lhs {opaque(lhs)};
    const long double rounded_rhs {opaque(rhs)};

    unsigned char lhs_bytes[sizeof(long double)] {};
    unsigned char rhs_bytes[sizeof(long double)] {};
    std::memcpy(lhs_bytes, &rounded_lhs, sizeof(long double));
    std::memcpy(rhs_bytes, &rounded_rhs, sizeof(long double));

    constexpr std::size_t value_bytes {sizeof(long double) >= 10U ? std::size_t{10} : sizeof(long double)};
    return std::memcmp(lhs_bytes, rhs_bytes, value_bytes) == 0;
}

} // namespace

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR
#include <compare>
#endif

using boost::int256::uint256;

// int256 is only forward declared until detail/int256_imp.hpp is wired into
// int256.hpp; the section below is gated on that header's guard so this file
// keeps compiling before and after it lands.
#if defined(BOOST_INT256_DETAIL_INT256_IMP_HPP)
#  define BOOST_INT256_TEST_HAS_SIGNED 1
using boost::int256::int256;
using boost::int256::detail::to_signed_bits;
#endif

namespace
{

std::mt19937_64 rng {42};

//=====================================
// Result types
//=====================================

// A mixed expression yields the floating point type, and the comparisons yield
// bool, exactly as they would for a built-in integer operand.
void test_result_types()
{
    static_assert(std::is_same<decltype(uint256{} + 1.0), double>::value, "uint256 + double");
    static_assert(std::is_same<decltype(1.0 + uint256{}), double>::value, "double + uint256");
    static_assert(std::is_same<decltype(uint256{} - 1.0), double>::value, "uint256 - double");
    static_assert(std::is_same<decltype(1.0 - uint256{}), double>::value, "double - uint256");
    static_assert(std::is_same<decltype(uint256{} * 1.0), double>::value, "uint256 * double");
    static_assert(std::is_same<decltype(1.0 * uint256{}), double>::value, "double * uint256");
    static_assert(std::is_same<decltype(uint256{} / 1.0), double>::value, "uint256 / double");
    static_assert(std::is_same<decltype(1.0 / uint256{}), double>::value, "double / uint256");

    static_assert(std::is_same<decltype(uint256{} + 1.0F), float>::value, "uint256 + float");
    static_assert(std::is_same<decltype(1.0F * uint256{}), float>::value, "float * uint256");

    static_assert(std::is_same<decltype(uint256{} == 1.0), bool>::value, "uint256 == double");
    static_assert(std::is_same<decltype(uint256{} != 1.0), bool>::value, "uint256 != double");
    static_assert(std::is_same<decltype(uint256{} < 1.0), bool>::value, "uint256 < double");
    static_assert(std::is_same<decltype(uint256{} <= 1.0), bool>::value, "uint256 <= double");
    static_assert(std::is_same<decltype(uint256{} > 1.0), bool>::value, "uint256 > double");
    static_assert(std::is_same<decltype(uint256{} >= 1.0), bool>::value, "uint256 >= double");
    static_assert(std::is_same<decltype(1.0 < uint256{}), bool>::value, "double < uint256");

    // The compound form on a floating point left hand side keeps that type.
    static_assert(std::is_same<decltype(std::declval<double&>() += uint256{}), double&>::value, "double += uint256");
    static_assert(std::is_same<decltype(std::declval<float&>() *= uint256{}), float&>::value, "float *= uint256");

    // And on a uint256 left hand side it stays a uint256.
    static_assert(std::is_same<decltype(std::declval<uint256&>() += 1.0), uint256&>::value, "uint256 += double");

    #ifndef BOOST_INT256_HAS_GPU_SUPPORT
    static_assert(std::is_same<decltype(uint256{} + 1.0L), long double>::value, "uint256 + long double");
    #endif
}

//=====================================
// Bit for bit agreement with the converted operand
//=====================================

// Every mixed expression must equal the same expression with the uint256 replaced
// by static_cast<T>(uint256), which is the whole contract.
template <typename T>
void check_agrees(const uint256& x, T y)
{
    const T converted {opaque(static_cast<T>(x))};

    BOOST_TEST(same_bits(opaque(x + y), opaque(converted + y)));
    BOOST_TEST(same_bits(opaque(y + x), opaque(y + converted)));
    BOOST_TEST(same_bits(opaque(x - y), opaque(converted - y)));
    BOOST_TEST(same_bits(opaque(y - x), opaque(y - converted)));
    BOOST_TEST(same_bits(opaque(x * y), opaque(converted * y)));
    BOOST_TEST(same_bits(opaque(y * x), opaque(y * converted)));
    BOOST_TEST(same_bits(opaque(x / y), opaque(converted / y)));
    BOOST_TEST(same_bits(opaque(y / x), opaque(y / converted)));

    BOOST_TEST_EQ(opaque(x == y), opaque(converted == y));
    BOOST_TEST_EQ(opaque(x != y), opaque(converted != y));
    BOOST_TEST_EQ(opaque(x < y), opaque(converted < y));
    BOOST_TEST_EQ(opaque(x <= y), opaque(converted <= y));
    BOOST_TEST_EQ(opaque(x > y), opaque(converted > y));
    BOOST_TEST_EQ(opaque(x >= y), opaque(converted >= y));

    BOOST_TEST_EQ(opaque(y == x), opaque(y == converted));
    BOOST_TEST_EQ(opaque(y < x), opaque(y < converted));
    BOOST_TEST_EQ(opaque(y > x), opaque(y > converted));

    // The compound form onto the floating point operand.
    {
        T accumulator {y};
        accumulator += x;
        BOOST_TEST(same_bits(opaque(accumulator), opaque(static_cast<T>(y + converted))));
    }
    {
        T accumulator {y};
        accumulator -= x;
        BOOST_TEST(same_bits(opaque(accumulator), opaque(static_cast<T>(y - converted))));
    }
    {
        T accumulator {y};
        accumulator *= x;
        BOOST_TEST(same_bits(opaque(accumulator), opaque(static_cast<T>(y * converted))));
    }
    {
        T accumulator {y};
        accumulator /= x;
        BOOST_TEST(same_bits(opaque(accumulator), opaque(static_cast<T>(y / converted))));
    }
}

template <typename T>
void test_agreement()
{
    const T operands[] {
        T{0}, T{1}, T{2}, T{-1}, static_cast<T>(0.5), static_cast<T>(-0.5), static_cast<T>(1e10), static_cast<T>(-1e10),
        static_cast<T>(3.14159), static_cast<T>(1) / T{3}
    };

    for (const auto& x : {uint256{1U}, uint256{42U}, uint256{UINT64_MAX},
                          uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)),
                          uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX)})
    {
        for (const auto y : operands)
        {
            if (y != T{0})
            {
                check_agrees(x, y);
            }
        }
    }

    for (int i {0}; i < 512; ++i)
    {
        uint256 x {};
        const int words {(i % 4) + 1};
        for (int w {0}; w < words; ++w)
        {
            x.words[w] = rng();
        }

        if (x == 0U)
        {
            continue;
        }

        const auto y {static_cast<T>(static_cast<double>(rng()) / 65536.0)};
        if (y != T{0})
        {
            check_agrees(x, y);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

//=====================================
// The compound form on a uint256 left hand side
//=====================================

// x @= f is specified as x = static_cast<uint256>(static_cast<Float>(x) @ f), so
// the whole operation happens in floating point and the result is then truncated
// back, saturating at the ends of the range.
void test_uint256_compound()
{
    {
        uint256 x {100U};
        x += 2.5;
        BOOST_TEST_EQ(x, uint256{102U});
    }
    {
        uint256 x {100U};
        x -= 2.5;
        BOOST_TEST_EQ(x, uint256{97U});
    }
    {
        uint256 x {100U};
        x *= 2.5;
        BOOST_TEST_EQ(x, uint256{250U});
    }
    {
        uint256 x {100U};
        x /= 2.5;
        BOOST_TEST_EQ(x, uint256{40U});
    }
    {
        // A negative result becomes zero, not a wrapped value.
        uint256 x {1U};
        x -= 2.0;
        BOOST_TEST_EQ(x, uint256{});
    }
    {
        // And an overflowing result saturates.
        uint256 x {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
        x *= 2.0;
        BOOST_TEST_EQ(x, (uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}));
    }
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    {
        // Dividing by infinity gives zero.
        uint256 x {42U};
        x /= opaque(std::numeric_limits<double>::infinity());
        BOOST_TEST_EQ(x, uint256{});
    }
    {
        // A NaN result becomes zero, since the constructor maps NaN to zero.
        uint256 x {42U};
        x *= opaque(std::numeric_limits<double>::quiet_NaN());
        BOOST_TEST_EQ(x, uint256{});
    }
    #endif
    {
        uint256 x {1U};
        x += 1.0F;
        BOOST_TEST_EQ(x, uint256{2U});
    }
}

//=====================================
// NaN, infinity and negative operands
//=====================================

void test_special_values()
{
    const uint256 x {42U};

    // NaN and infinity are meaningless under a no-infs/no-NaN fast math
    // model; this whole block depends on one of the two, so it is gated.
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    const double nan {opaque(std::numeric_limits<double>::quiet_NaN())};
    const double inf {opaque(std::numeric_limits<double>::infinity())};

    // Every comparison against a NaN is false, except the inequality.
    BOOST_TEST(!(x == nan));
    BOOST_TEST(x != nan);
    BOOST_TEST(!(x < nan));
    BOOST_TEST(!(x <= nan));
    BOOST_TEST(!(x > nan));
    BOOST_TEST(!(x >= nan));
    BOOST_TEST(!(nan == x));
    BOOST_TEST(nan != x);
    BOOST_TEST(!(nan < x));

    // An arithmetic result involving a NaN is a NaN.
    BOOST_TEST(std::isnan(opaque(x + nan)));
    BOOST_TEST(std::isnan(opaque(nan - x)));
    BOOST_TEST(std::isnan(opaque(x * nan)));
    BOOST_TEST(std::isnan(opaque(x / nan)));
    BOOST_TEST(std::isnan(opaque(nan / x)));

    // Infinity orders above every finite value.
    BOOST_TEST(x < inf);
    BOOST_TEST(x <= inf);
    BOOST_TEST(!(x > inf));
    BOOST_TEST(x > -inf);
    BOOST_TEST(x != inf);
    BOOST_TEST(std::isinf(opaque(x + inf)));
    BOOST_TEST(std::isinf(opaque(x * inf)));
    BOOST_TEST(same_bits(opaque(x / inf), opaque(0.0)));
    #endif // BOOST_INT256_TEST_HONORS_INF_NAN

    // A negative operand compares below every uint256, including zero, because the
    // comparison happens in floating point rather than through a conversion to
    // uint256. This is the opposite of the integer operand behavior, where -1
    // widens to the maximum, so it is pinned here.
    BOOST_TEST(uint256{} > -1.0);
    BOOST_TEST(!(uint256{} < -1.0));
    BOOST_TEST(uint256{} != -1.0);
    BOOST_TEST(uint256(UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX) > -1.0);

    // Which is worth contrasting directly with the integer operand.
    BOOST_TEST(uint256{} < -1);
    BOOST_TEST(uint256{} > -1.0);
}

//=====================================
// The deleted operators
//=====================================

// A floating point operand is ill formed for the modulo, bitwise and shift
// operators. Detecting that with SFINAE proves the overload is deleted rather
// than merely absent, which is what stops the implicit constructor from taking
// over and truncating.
template <typename T, typename U>
struct has_modulo
{
private:

    template <typename A, typename B>
    static auto probe(int) -> decltype(std::declval<A>() % std::declval<B>(), std::true_type{});

    template <typename, typename>
    static std::false_type probe(...);

public:

    static constexpr bool value {decltype(probe<T, U>(0))::value};
};

template <typename T, typename U>
struct has_bitwise_and
{
private:

    template <typename A, typename B>
    static auto probe(int) -> decltype(std::declval<A>() & std::declval<B>(), std::true_type{});

    template <typename, typename>
    static std::false_type probe(...);

public:

    static constexpr bool value {decltype(probe<T, U>(0))::value};
};

template <typename T, typename U>
struct has_left_shift
{
private:

    template <typename A, typename B>
    static auto probe(int) -> decltype(std::declval<A>() << std::declval<B>(), std::true_type{});

    template <typename, typename>
    static std::false_type probe(...);

public:

    static constexpr bool value {decltype(probe<T, U>(0))::value};
};

template <typename T, typename U>
struct has_modulo_assign
{
private:

    template <typename A, typename B>
    static auto probe(int) -> decltype(std::declval<A&>() %= std::declval<B>(), std::true_type{});

    template <typename, typename>
    static std::false_type probe(...);

public:

    static constexpr bool value {decltype(probe<T, U>(0))::value};
};

void test_deleted_operators()
{
    // The integer forms are fine.
    static_assert(has_modulo<uint256, int>::value, "uint256 % int is well formed");
    static_assert(has_bitwise_and<uint256, int>::value, "uint256 & int is well formed");
    static_assert(has_left_shift<uint256, int>::value, "uint256 << int is well formed");
    static_assert(has_modulo_assign<uint256, int>::value, "uint256 %= int is well formed");

    // The floating point forms are not, in either operand order.
    static_assert(!has_modulo<uint256, double>::value, "uint256 % double is ill formed");
    static_assert(!has_modulo<double, uint256>::value, "double % uint256 is ill formed");
    static_assert(!has_modulo<uint256, float>::value, "uint256 % float is ill formed");

    static_assert(!has_bitwise_and<uint256, double>::value, "uint256 & double is ill formed");
    static_assert(!has_bitwise_and<double, uint256>::value, "double & uint256 is ill formed");

    static_assert(!has_left_shift<uint256, double>::value, "uint256 << double is ill formed");
    static_assert(!has_left_shift<double, uint256>::value, "double << uint256 is ill formed");

    static_assert(!has_modulo_assign<uint256, double>::value, "uint256 %= double is ill formed");
    static_assert(!has_modulo_assign<uint256, float>::value, "uint256 %= float is ill formed");

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    static_assert(has_modulo<int256, int>::value, "int256 % int is well formed");
    static_assert(has_bitwise_and<int256, int>::value, "int256 & int is well formed");
    static_assert(has_left_shift<int256, int>::value, "int256 << int is well formed");
    static_assert(has_modulo_assign<int256, int>::value, "int256 %= int is well formed");

    static_assert(!has_modulo<int256, double>::value, "int256 % double is ill formed");
    static_assert(!has_modulo<double, int256>::value, "double % int256 is ill formed");
    static_assert(!has_bitwise_and<int256, double>::value, "int256 & double is ill formed");
    static_assert(!has_left_shift<int256, double>::value, "int256 << double is ill formed");
    static_assert(!has_modulo_assign<int256, double>::value, "int256 %= double is ill formed");
    #endif
}

//=====================================
// The three way comparison
//=====================================

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR

// A floating point operand can be unordered, so the result is a partial ordering
// rather than the strong ordering the integer operands give.
void test_spaceship()
{
    const uint256 x {42U};

    static_assert(std::is_same<decltype(x <=> 1.0), std::partial_ordering>::value,
                  "a floating point operand gives a partial ordering");
    static_assert(std::is_same<decltype(x <=> 1U), std::strong_ordering>::value,
                  "an integer operand gives a strong ordering");

    BOOST_TEST((x <=> 41.0) == std::partial_ordering::greater);
    BOOST_TEST((x <=> 42.0) == std::partial_ordering::equivalent);
    BOOST_TEST((x <=> 43.0) == std::partial_ordering::less);
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST((x <=> opaque(std::numeric_limits<double>::quiet_NaN())) == std::partial_ordering::unordered);
    BOOST_TEST((x <=> opaque(std::numeric_limits<double>::infinity())) == std::partial_ordering::less);
    BOOST_TEST((x <=> opaque(-std::numeric_limits<double>::infinity())) == std::partial_ordering::greater);
    #endif

    // A negative operand is below, unlike the integer case.
    BOOST_TEST((uint256{} <=> -1.0) == std::partial_ordering::greater);

    BOOST_TEST((42.0 <=> x) == std::partial_ordering::equivalent);
    BOOST_TEST((41.0 <=> x) == std::partial_ordering::less);
}

#else

void test_spaceship()
{
}

#endif

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

void test_signed_result_types()
{
    static_assert(std::is_same<decltype(int256{} + 1.0), double>::value, "int256 + double");
    static_assert(std::is_same<decltype(1.0 + int256{}), double>::value, "double + int256");
    static_assert(std::is_same<decltype(int256{} - 1.0), double>::value, "int256 - double");
    static_assert(std::is_same<decltype(int256{} * 1.0), double>::value, "int256 * double");
    static_assert(std::is_same<decltype(int256{} / 1.0), double>::value, "int256 / double");

    static_assert(std::is_same<decltype(int256{} + 1.0F), float>::value, "int256 + float");

    static_assert(std::is_same<decltype(int256{} == 1.0), bool>::value, "int256 == double");
    static_assert(std::is_same<decltype(int256{} < 1.0), bool>::value, "int256 < double");
    static_assert(std::is_same<decltype(1.0 < int256{}), bool>::value, "double < int256");

    static_assert(std::is_same<decltype(std::declval<double&>() += int256{}), double&>::value, "double += int256");
    static_assert(std::is_same<decltype(std::declval<int256&>() += 1.0), int256&>::value, "int256 += double");

    #ifndef BOOST_INT256_HAS_GPU_SUPPORT
    static_assert(std::is_same<decltype(int256{} + 1.0L), long double>::value, "int256 + long double");
    #endif
}

// The int256 twin of check_agrees: every mixed expression must equal the same
// expression with int256 replaced by static_cast<T>(int256), including when x
// is negative.
template <typename T>
void check_agrees_i(const int256& x, T y)
{
    const T converted {opaque(static_cast<T>(x))};

    BOOST_TEST(same_bits(opaque(x + y), opaque(converted + y)));
    BOOST_TEST(same_bits(opaque(y + x), opaque(y + converted)));
    BOOST_TEST(same_bits(opaque(x - y), opaque(converted - y)));
    BOOST_TEST(same_bits(opaque(y - x), opaque(y - converted)));
    BOOST_TEST(same_bits(opaque(x * y), opaque(converted * y)));
    BOOST_TEST(same_bits(opaque(y * x), opaque(y * converted)));
    BOOST_TEST(same_bits(opaque(x / y), opaque(converted / y)));
    BOOST_TEST(same_bits(opaque(y / x), opaque(y / converted)));

    BOOST_TEST_EQ(opaque(x == y), opaque(converted == y));
    BOOST_TEST_EQ(opaque(x != y), opaque(converted != y));
    BOOST_TEST_EQ(opaque(x < y), opaque(converted < y));
    BOOST_TEST_EQ(opaque(x <= y), opaque(converted <= y));
    BOOST_TEST_EQ(opaque(x > y), opaque(converted > y));
    BOOST_TEST_EQ(opaque(x >= y), opaque(converted >= y));

    {
        T accumulator {y};
        accumulator += x;
        BOOST_TEST(same_bits(opaque(accumulator), opaque(static_cast<T>(y + converted))));
    }
    {
        T accumulator {y};
        accumulator *= x;
        BOOST_TEST(same_bits(opaque(accumulator), opaque(static_cast<T>(y * converted))));
    }
}

template <typename T>
void test_signed_agreement()
{
    const T operands[] {
        T{0}, T{1}, T{2}, T{-1}, static_cast<T>(0.5), static_cast<T>(-0.5), static_cast<T>(1e10), static_cast<T>(-1e10),
        static_cast<T>(3.14159), static_cast<T>(1) / T{3}
    };

    const int256 values[] {
        int256{1}, int256{42}, int256{-1}, int256{-42},
        int256(INT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)),
        int256(INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)),
        int256(INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX)
    };

    for (const auto& x : values)
    {
        for (const auto y : operands)
        {
            if (y != T{0})
            {
                check_agrees_i(x, y);
            }
        }
    }

    for (int i {0}; i < 512; ++i)
    {
        uint256 bits {};
        const int words {(i % 4) + 1};
        for (int w {0}; w < words; ++w)
        {
            bits.words[w] = rng();
        }

        const int256 x {to_signed_bits(bits)};
        if (x == 0)
        {
            continue;
        }

        const auto y {static_cast<T>(static_cast<double>(rng()) / 65536.0 - 32768.0)};
        if (y != T{0})
        {
            check_agrees_i(x, y);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// x @= f truncates toward zero and saturates at the ends of int256's range,
// rather than clamping at zero the way uint256 does.
// BOOST_TEST(a == b) rather than BOOST_TEST_EQ throughout: int256 has no
// operator<< until D2 lands.
void test_int256_compound()
{
    {
        int256 x {100};
        x += 2.5;
        BOOST_TEST(x == int256{102});
    }
    {
        int256 x {-100};
        x -= 2.5;
        BOOST_TEST(x == int256{-102});
    }
    {
        int256 x {-100};
        x *= 2.5;
        BOOST_TEST(x == int256{-250});
    }
    {
        int256 x {-100};
        x /= 2.5;
        BOOST_TEST(x == int256{-40});
    }
    {
        // A negative result is a normal negative int256, not clamped to zero.
        int256 x {1};
        x -= 2.0;
        BOOST_TEST(x == int256{-1});
    }
    {
        // An overflowing result saturates at MAX or MIN.
        int256 x {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
        x *= 2.0;
        BOOST_TEST(x == (int256{INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}));

        int256 y {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
        y *= 2.0;
        BOOST_TEST(y == (int256{INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)}));
    }
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    {
        int256 x {42};
        x /= opaque(std::numeric_limits<double>::infinity());
        BOOST_TEST(x == int256{});
    }
    {
        int256 x {42};
        x *= opaque(std::numeric_limits<double>::quiet_NaN());
        BOOST_TEST(x == int256{});
    }
    #endif
}

void test_signed_special_values()
{
    const int256 x {42};
    const int256 neg {-42};

    #if BOOST_INT256_TEST_HONORS_INF_NAN
    const double nan {opaque(std::numeric_limits<double>::quiet_NaN())};
    const double inf {opaque(std::numeric_limits<double>::infinity())};

    BOOST_TEST(!(x == nan));
    BOOST_TEST(x != nan);
    BOOST_TEST(!(x < nan));
    BOOST_TEST(!(nan == x));

    BOOST_TEST(std::isnan(opaque(x + nan)));
    BOOST_TEST(std::isnan(opaque(neg * nan)));

    BOOST_TEST(x < inf);
    BOOST_TEST(neg > -inf);
    BOOST_TEST(neg < inf);
    BOOST_TEST(std::isinf(opaque(x + inf)));
    BOOST_TEST(std::isinf(opaque(neg * inf)));
    #endif // BOOST_INT256_TEST_HONORS_INF_NAN

    // Unlike uint256, a negative int256 compares below a negative double the
    // same way it would as a builtin signed integer: the sign is preserved on
    // both sides, so there is no widening surprise here.
    BOOST_TEST(neg < -1.0);
    BOOST_TEST(neg > -100.0);
    BOOST_TEST(x > -1.0);
    BOOST_TEST(int256{} > -1.0);
    BOOST_TEST(int256{} < 1.0);
}

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR

void test_signed_spaceship()
{
    const int256 x {42};
    const int256 neg {-42};

    static_assert(std::is_same<decltype(x <=> 1.0), std::partial_ordering>::value,
                  "a floating point operand gives a partial ordering");

    BOOST_TEST((x <=> 41.0) == std::partial_ordering::greater);
    BOOST_TEST((x <=> 42.0) == std::partial_ordering::equivalent);
    #if BOOST_INT256_TEST_HONORS_INF_NAN
    BOOST_TEST((x <=> opaque(std::numeric_limits<double>::quiet_NaN())) == std::partial_ordering::unordered);
    #endif

    // Sign preserved, unlike uint256's <=> against a negative double.
    BOOST_TEST((neg <=> -41.0) == std::partial_ordering::less);
    BOOST_TEST((neg <=> 0.0) == std::partial_ordering::less);
    BOOST_TEST((int256{} <=> -1.0) == std::partial_ordering::greater);
}

#else

void test_signed_spaceship()
{
}

#endif

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_result_types();
    test_deleted_operators();
    test_agreement<double>();
    test_agreement<float>();
    #ifndef BOOST_INT256_HAS_GPU_SUPPORT
    test_agreement<long double>();
    #endif
    test_uint256_compound();
    test_special_values();
    test_spaceship();

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_signed_result_types();
    test_signed_agreement<double>();
    test_signed_agreement<float>();
    #ifndef BOOST_INT256_HAS_GPU_SUPPORT
    test_signed_agreement<long double>();
    #endif
    test_int256_compound();
    test_signed_special_values();
    test_signed_spaceship();
#endif

    return boost::report_errors();
}
