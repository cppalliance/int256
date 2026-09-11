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
#include <limits>
#include <random>
#include <type_traits>

#ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR
#include <compare>
#endif

using boost::int256::uint256;

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
    const auto converted {static_cast<T>(x)};

    BOOST_TEST_EQ(x + y, converted + y);
    BOOST_TEST_EQ(y + x, y + converted);
    BOOST_TEST_EQ(x - y, converted - y);
    BOOST_TEST_EQ(y - x, y - converted);
    BOOST_TEST_EQ(x * y, converted * y);
    BOOST_TEST_EQ(y * x, y * converted);
    BOOST_TEST_EQ(x / y, converted / y);
    BOOST_TEST_EQ(y / x, y / converted);

    BOOST_TEST_EQ(x == y, converted == y);
    BOOST_TEST_EQ(x != y, converted != y);
    BOOST_TEST_EQ(x < y, converted < y);
    BOOST_TEST_EQ(x <= y, converted <= y);
    BOOST_TEST_EQ(x > y, converted > y);
    BOOST_TEST_EQ(x >= y, converted >= y);

    BOOST_TEST_EQ(y == x, y == converted);
    BOOST_TEST_EQ(y < x, y < converted);
    BOOST_TEST_EQ(y > x, y > converted);

    // The compound form onto the floating point operand.
    {
        T accumulator {y};
        accumulator += x;
        BOOST_TEST_EQ(accumulator, static_cast<T>(y + converted));
    }
    {
        T accumulator {y};
        accumulator -= x;
        BOOST_TEST_EQ(accumulator, static_cast<T>(y - converted));
    }
    {
        T accumulator {y};
        accumulator *= x;
        BOOST_TEST_EQ(accumulator, static_cast<T>(y * converted));
    }
    {
        T accumulator {y};
        accumulator /= x;
        BOOST_TEST_EQ(accumulator, static_cast<T>(y / converted));
    }
}

template <typename T>
void test_agreement()
{
    const T operands[] {
        T{0}, T{1}, T{2}, T{-1}, T{0.5}, T{-0.5}, T{1e10}, T{-1e10},
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
    {
        // Dividing by infinity gives zero.
        uint256 x {42U};
        x /= std::numeric_limits<double>::infinity();
        BOOST_TEST_EQ(x, uint256{});
    }
    {
        // A NaN result becomes zero, since the constructor maps NaN to zero.
        uint256 x {42U};
        x *= std::numeric_limits<double>::quiet_NaN();
        BOOST_TEST_EQ(x, uint256{});
    }
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
    const auto nan {std::numeric_limits<double>::quiet_NaN()};
    const auto inf {std::numeric_limits<double>::infinity()};
    const uint256 x {42U};

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
    BOOST_TEST(std::isnan(x + nan));
    BOOST_TEST(std::isnan(nan - x));
    BOOST_TEST(std::isnan(x * nan));
    BOOST_TEST(std::isnan(x / nan));
    BOOST_TEST(std::isnan(nan / x));

    // Infinity orders above every finite value.
    BOOST_TEST(x < inf);
    BOOST_TEST(x <= inf);
    BOOST_TEST(!(x > inf));
    BOOST_TEST(x > -inf);
    BOOST_TEST(x != inf);
    BOOST_TEST(std::isinf(x + inf));
    BOOST_TEST(std::isinf(x * inf));
    BOOST_TEST(x / inf == 0.0);

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
    BOOST_TEST((x <=> std::numeric_limits<double>::quiet_NaN()) == std::partial_ordering::unordered);
    BOOST_TEST((x <=> std::numeric_limits<double>::infinity()) == std::partial_ordering::less);
    BOOST_TEST((x <=> -std::numeric_limits<double>::infinity()) == std::partial_ordering::greater);

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

    return boost::report_errors();
}
