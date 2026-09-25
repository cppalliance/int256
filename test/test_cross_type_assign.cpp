// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The conversion surface, checked with traits rather than by running anything.
// uint256 converts implicitly in both directions with the builtin integers and
// the builtin floating point types, so a uint256 drops into existing code the
// way __int128 does. Anything that should not convert is a compile-fail case in
// compile_tests/ instead.

#include <boost/int256.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#ifdef __has_include
#  if __has_include(<stdfloat>)
#    include <stdfloat>
#  endif
#endif

using boost::int256::uint256;

// int256 is only forward declared until detail/int256_imp.hpp is wired into
// int256.hpp; the section below is gated on that header's guard so this file
// keeps compiling before and after it lands.
#if defined(BOOST_INT256_DETAIL_INT256_IMP_HPP)
#  define BOOST_INT256_TEST_HAS_SIGNED 1
using boost::int256::int256;
#endif

void test_integer_conversion_traits()
{
    // To the builtin integers, matching the builtin 128-bit behavior.
    static_assert(std::is_convertible<uint256, bool>::value, "uint256 -> bool should be implicit");
    static_assert(std::is_convertible<uint256, char>::value, "uint256 -> char should be implicit");
    static_assert(std::is_convertible<uint256, signed char>::value, "uint256 -> signed char");
    static_assert(std::is_convertible<uint256, unsigned char>::value, "uint256 -> unsigned char");
    static_assert(std::is_convertible<uint256, short>::value, "uint256 -> short");
    static_assert(std::is_convertible<uint256, unsigned short>::value, "uint256 -> unsigned short");
    static_assert(std::is_convertible<uint256, int>::value, "uint256 -> int should be implicit");
    static_assert(std::is_convertible<uint256, unsigned int>::value, "uint256 -> unsigned int");
    static_assert(std::is_convertible<uint256, long>::value, "uint256 -> long");
    static_assert(std::is_convertible<uint256, unsigned long>::value, "uint256 -> unsigned long");
    static_assert(std::is_convertible<uint256, long long>::value, "uint256 -> long long");
    static_assert(std::is_convertible<uint256, unsigned long long>::value, "uint256 -> unsigned long long");
    static_assert(std::is_convertible<uint256, std::int64_t>::value, "uint256 -> int64_t");
    static_assert(std::is_convertible<uint256, std::uint64_t>::value, "uint256 -> uint64_t");

    // And from them.
    static_assert(std::is_convertible<bool, uint256>::value, "bool -> uint256 should be implicit");
    static_assert(std::is_convertible<char, uint256>::value, "char -> uint256");
    static_assert(std::is_convertible<int, uint256>::value, "int -> uint256");
    static_assert(std::is_convertible<unsigned, uint256>::value, "unsigned -> uint256");
    static_assert(std::is_convertible<long long, uint256>::value, "long long -> uint256");
    static_assert(std::is_convertible<unsigned long long, uint256>::value, "unsigned long long -> uint256");

    // Assignment in both directions.
    static_assert(std::is_assignable<uint256&, int>::value, "int assignable to uint256");
    static_assert(std::is_assignable<uint256&, unsigned long long>::value, "u64 assignable to uint256");
    static_assert(std::is_assignable<int&, uint256>::value, "uint256 assignable to int");
    static_assert(std::is_assignable<std::uint64_t&, uint256>::value, "uint256 assignable to uint64_t");

    // Copy and move are trivial.
    static_assert(std::is_copy_constructible<uint256>::value, "copy constructible");
    static_assert(std::is_move_constructible<uint256>::value, "move constructible");
    static_assert(std::is_copy_assignable<uint256>::value, "copy assignable");
    static_assert(std::is_move_assignable<uint256>::value, "move assignable");
    static_assert(std::is_trivially_copy_constructible<uint256>::value, "trivially copy constructible");
    static_assert(std::is_trivially_copy_assignable<uint256>::value, "trivially copy assignable");
    static_assert(std::is_nothrow_default_constructible<uint256>::value, "nothrow default constructible");
}

void test_float_conversion_traits()
{
    static_assert(std::is_convertible<uint256, float>::value, "uint256 -> float should be implicit");
    static_assert(std::is_convertible<uint256, double>::value, "uint256 -> double should be implicit");
    static_assert(std::is_convertible<float, uint256>::value, "float -> uint256 should be implicit");
    static_assert(std::is_convertible<double, uint256>::value, "double -> uint256 should be implicit");

    static_assert(std::is_assignable<uint256&, double>::value, "double assignable to uint256");
    static_assert(std::is_assignable<double&, uint256>::value, "uint256 assignable to double");

    #ifndef BOOST_INT256_HAS_GPU_SUPPORT
    static_assert(std::is_convertible<uint256, long double>::value, "uint256 -> long double on the host");
    static_assert(std::is_convertible<long double, uint256>::value, "long double -> uint256 on the host");
    #endif

    // A mixed expression yields the floating point type, per the usual
    // arithmetic conversions. test_float_mixed_ops.cpp covers the operators.
    static_assert(std::is_same<decltype(uint256{} + 1.0), double>::value, "uint256 + double -> double");
    static_assert(std::is_same<decltype(1.0F * uint256{}), float>::value, "float * uint256 -> float");
    static_assert(std::is_same<decltype(uint256{} < 1.0), bool>::value, "uint256 < double -> bool");
}

// The C++23 extended floating point types (std::float16_t, float32_t,
// float64_t, float128_t, bfloat16_t) now convert to and from uint256/int256
// exactly like float/double/long double do; test_stdfloat.cpp covers the
// arithmetic and rounding behavior, so this only pins the conversion
// surface (traits) for each type the toolchain actually defines. Never gate
// on __cpp_lib_stdfloat: libstdc++ (as of gcc-16) does not define that macro
// even though <stdfloat> and the types both exist.
void test_stdfloat_conversion_traits()
{
    #if defined(__STDCPP_FLOAT16_T__)
    static_assert(std::is_convertible<std::float16_t, uint256>::value, "float16_t -> uint256");
    static_assert(std::is_convertible<uint256, std::float16_t>::value, "uint256 -> float16_t");
    static_assert(std::is_constructible<uint256, std::float16_t>::value, "uint256 constructible from float16_t");
    BOOST_TEST_EQ(uint256{std::float16_t{7}}, uint256{7U});
    static_assert(std::is_same<decltype(uint256{} + std::float16_t{}), std::float16_t>::value,
                  "an extended float operand keeps its own type as the result type");

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    static_assert(std::is_convertible<std::float16_t, int256>::value, "float16_t -> int256");
    static_assert(std::is_convertible<int256, std::float16_t>::value, "int256 -> float16_t");
    static_assert(std::is_constructible<int256, std::float16_t>::value, "int256 constructible from float16_t");
    BOOST_TEST_EQ(int256{std::float16_t{-7}}, int256{-7});
    static_assert(std::is_same<decltype(int256{} + std::float16_t{}), std::float16_t>::value,
                  "an extended float operand keeps its own type as the result type");
    #endif
    #endif // __STDCPP_FLOAT16_T__

    #if defined(__STDCPP_BFLOAT16_T__)
    static_assert(std::is_convertible<std::bfloat16_t, uint256>::value, "bfloat16_t -> uint256");
    static_assert(std::is_convertible<uint256, std::bfloat16_t>::value, "uint256 -> bfloat16_t");
    BOOST_TEST_EQ(uint256{std::bfloat16_t{5}}, uint256{5U});

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    static_assert(std::is_convertible<std::bfloat16_t, int256>::value, "bfloat16_t -> int256");
    static_assert(std::is_convertible<int256, std::bfloat16_t>::value, "int256 -> bfloat16_t");
    BOOST_TEST_EQ(int256{std::bfloat16_t{-5}}, int256{-5});
    #endif
    #endif // __STDCPP_BFLOAT16_T__

    #if defined(__STDCPP_FLOAT32_T__)
    static_assert(std::is_convertible<std::float32_t, uint256>::value, "float32_t -> uint256");
    static_assert(std::is_convertible<uint256, std::float32_t>::value, "uint256 -> float32_t");
    static_assert(std::is_constructible<uint256, std::float32_t>::value, "uint256 constructible from float32_t");
    BOOST_TEST_EQ(uint256{std::float32_t{7}}, uint256{7U});
    static_assert(std::is_same<decltype(uint256{} + std::float32_t{}), std::float32_t>::value,
                  "an extended float operand keeps its own type as the result type");

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    static_assert(std::is_convertible<std::float32_t, int256>::value, "float32_t -> int256");
    static_assert(std::is_convertible<int256, std::float32_t>::value, "int256 -> float32_t");
    static_assert(std::is_constructible<int256, std::float32_t>::value, "int256 constructible from float32_t");
    BOOST_TEST_EQ(int256{std::float32_t{-7}}, int256{-7});
    static_assert(std::is_same<decltype(int256{} + std::float32_t{}), std::float32_t>::value,
                  "an extended float operand keeps its own type as the result type");
    #endif
    #endif // __STDCPP_FLOAT32_T__

    #if defined(__STDCPP_FLOAT64_T__)
    static_assert(std::is_convertible<std::float64_t, uint256>::value, "float64_t -> uint256");
    static_assert(std::is_convertible<uint256, std::float64_t>::value, "uint256 -> float64_t");
    static_assert(std::is_constructible<uint256, std::float64_t>::value, "uint256 constructible from float64_t");
    BOOST_TEST_EQ(uint256{std::float64_t{7}}, uint256{7U});
    static_assert(std::is_same<decltype(uint256{} + std::float64_t{}), std::float64_t>::value,
                  "an extended float operand keeps its own type as the result type");

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    static_assert(std::is_convertible<std::float64_t, int256>::value, "float64_t -> int256");
    static_assert(std::is_convertible<int256, std::float64_t>::value, "int256 -> float64_t");
    static_assert(std::is_constructible<int256, std::float64_t>::value, "int256 constructible from float64_t");
    BOOST_TEST_EQ(int256{std::float64_t{-7}}, int256{-7});
    static_assert(std::is_same<decltype(int256{} + std::float64_t{}), std::float64_t>::value,
                  "an extended float operand keeps its own type as the result type");
    #endif
    #endif // __STDCPP_FLOAT64_T__

    #if defined(__STDCPP_FLOAT128_T__)
    static_assert(std::is_convertible<std::float128_t, uint256>::value, "float128_t -> uint256");
    static_assert(std::is_convertible<uint256, std::float128_t>::value, "uint256 -> float128_t");
    static_assert(std::is_constructible<uint256, std::float128_t>::value, "uint256 constructible from float128_t");
    BOOST_TEST_EQ(uint256{std::float128_t{7}}, uint256{7U});
    static_assert(std::is_same<decltype(uint256{} + std::float128_t{}), std::float128_t>::value,
                  "an extended float operand keeps its own type as the result type");

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    static_assert(std::is_convertible<std::float128_t, int256>::value, "float128_t -> int256");
    static_assert(std::is_convertible<int256, std::float128_t>::value, "int256 -> float128_t");
    static_assert(std::is_constructible<int256, std::float128_t>::value, "int256 constructible from float128_t");
    BOOST_TEST_EQ(int256{std::float128_t{-7}}, int256{-7});
    static_assert(std::is_same<decltype(int256{} + std::float128_t{}), std::float128_t>::value,
                  "an extended float operand keeps its own type as the result type");
    #endif
    #endif // __STDCPP_FLOAT128_T__
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

void test_builtin_128_conversion_traits()
{
    using boost::int256::detail::builtin_i128;
    using boost::int256::detail::builtin_u128;

    static_assert(std::is_convertible<uint256, builtin_i128>::value, "uint256 -> builtin signed 128");
    static_assert(std::is_convertible<uint256, builtin_u128>::value, "uint256 -> builtin unsigned 128");
    static_assert(std::is_convertible<builtin_i128, uint256>::value, "builtin signed 128 -> uint256");
    static_assert(std::is_convertible<builtin_u128, uint256>::value, "builtin unsigned 128 -> uint256");
    static_assert(std::is_assignable<uint256&, builtin_u128>::value, "builtin unsigned 128 assignable");
    static_assert(std::is_assignable<uint256&, builtin_i128>::value, "builtin signed 128 assignable");
}

#else

void test_builtin_128_conversion_traits()
{
}

#endif

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

void test_signed_integer_conversion_traits()
{
    // To and from the builtin integers, matching uint256's surface.
    static_assert(std::is_convertible<int256, bool>::value, "int256 -> bool should be implicit");
    static_assert(std::is_convertible<int256, int>::value, "int256 -> int should be implicit");
    static_assert(std::is_convertible<int256, unsigned int>::value, "int256 -> unsigned int");
    static_assert(std::is_convertible<int256, long long>::value, "int256 -> long long");
    static_assert(std::is_convertible<int256, unsigned long long>::value, "int256 -> unsigned long long");
    static_assert(std::is_convertible<int256, std::int64_t>::value, "int256 -> int64_t");
    static_assert(std::is_convertible<int256, std::uint64_t>::value, "int256 -> uint64_t");

    static_assert(std::is_convertible<bool, int256>::value, "bool -> int256 should be implicit");
    static_assert(std::is_convertible<int, int256>::value, "int -> int256");
    static_assert(std::is_convertible<unsigned, int256>::value, "unsigned -> int256");
    static_assert(std::is_convertible<long long, int256>::value, "long long -> int256");
    static_assert(std::is_convertible<unsigned long long, int256>::value, "unsigned long long -> int256");

    static_assert(std::is_assignable<int256&, int>::value, "int assignable to int256");
    static_assert(std::is_assignable<int256&, unsigned long long>::value, "u64 assignable to int256");
    static_assert(std::is_assignable<int&, int256>::value, "int256 assignable to int");
    static_assert(std::is_assignable<std::uint64_t&, int256>::value, "int256 assignable to uint64_t");

    static_assert(std::is_copy_constructible<int256>::value, "copy constructible");
    static_assert(std::is_move_constructible<int256>::value, "move constructible");
    static_assert(std::is_copy_assignable<int256>::value, "copy assignable");
    static_assert(std::is_move_assignable<int256>::value, "move assignable");
    static_assert(std::is_trivially_copy_constructible<int256>::value, "trivially copy constructible");
    static_assert(std::is_trivially_copy_assignable<int256>::value, "trivially copy assignable");
    static_assert(std::is_nothrow_default_constructible<int256>::value, "nothrow default constructible");

    // int256 and uint256 convert implicitly both ways (a bit copy), the same
    // relationship uint256 has with the other builtin integers.
    static_assert(std::is_convertible<int256, uint256>::value, "int256 -> uint256 should be implicit");
    static_assert(std::is_convertible<uint256, int256>::value, "uint256 -> int256 should be implicit");
    static_assert(std::is_assignable<int256&, uint256>::value, "uint256 assignable to int256");
    static_assert(std::is_assignable<uint256&, int256>::value, "int256 assignable to uint256");
}

void test_signed_float_conversion_traits()
{
    static_assert(std::is_convertible<int256, float>::value, "int256 -> float should be implicit");
    static_assert(std::is_convertible<int256, double>::value, "int256 -> double should be implicit");
    static_assert(std::is_convertible<float, int256>::value, "float -> int256 should be implicit");
    static_assert(std::is_convertible<double, int256>::value, "double -> int256 should be implicit");

    static_assert(std::is_assignable<int256&, double>::value, "double assignable to int256");
    static_assert(std::is_assignable<double&, int256>::value, "int256 assignable to double");

    #ifndef BOOST_INT256_HAS_GPU_SUPPORT
    static_assert(std::is_convertible<int256, long double>::value, "int256 -> long double on the host");
    static_assert(std::is_convertible<long double, int256>::value, "long double -> int256 on the host");
    #endif

    static_assert(std::is_same<decltype(int256{} + 1.0), double>::value, "int256 + double -> double");
    static_assert(std::is_same<decltype(1.0F * int256{}), float>::value, "float * int256 -> float");
    static_assert(std::is_same<decltype(int256{} < 1.0), bool>::value, "int256 < double -> bool");
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

void test_signed_builtin_128_conversion_traits()
{
    using boost::int256::detail::builtin_i128;
    using boost::int256::detail::builtin_u128;

    static_assert(std::is_convertible<int256, builtin_i128>::value, "int256 -> builtin signed 128");
    static_assert(std::is_convertible<int256, builtin_u128>::value, "int256 -> builtin unsigned 128");
    static_assert(std::is_convertible<builtin_i128, int256>::value, "builtin signed 128 -> int256");
    static_assert(std::is_convertible<builtin_u128, int256>::value, "builtin unsigned 128 -> int256");
    static_assert(std::is_assignable<int256&, builtin_u128>::value, "builtin unsigned 128 assignable");
    static_assert(std::is_assignable<int256&, builtin_i128>::value, "builtin signed 128 assignable");
}

#else

void test_signed_builtin_128_conversion_traits()
{
}

#endif

void test_signed_conversions_run()
{
    const int256 value {INT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(42)};

    const int as_int = value;
    BOOST_TEST_EQ(as_int, 42);

    const std::uint64_t as_u64 = value;
    BOOST_TEST_EQ(as_u64, UINT64_C(42));

    const double as_double = value;
    BOOST_TEST(as_double > 0.0);

    // BOOST_TEST(a == b) rather than BOOST_TEST_EQ: int256 has no operator<<
    // until D2 lands.
    int256 assigned {};
    assigned = 42;
    BOOST_TEST(assigned == int256{42});

    assigned = -42;
    BOOST_TEST(assigned == int256{-42});

    assigned = 42.0;
    BOOST_TEST(assigned == int256{42});

    int256 movable {INT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)};
    const int256 moved {std::move(movable)};
    BOOST_TEST_EQ(moved.words[0], UINT64_C(4));
    BOOST_TEST_EQ(moved.words[3], UINT64_C(1));

    // The int256/uint256 bit copy in both directions.
    const int256 negative {-1};
    const uint256 as_unsigned {negative};
    BOOST_TEST_EQ(as_unsigned, (uint256{UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX}));

    const int256 back {as_unsigned};
    BOOST_TEST(back == negative);
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

// The traits above only say the conversions exist; these check they move the
// right bits.
void test_conversions_run()
{
    const uint256 value {UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(42)};

    const int as_int = value;
    BOOST_TEST_EQ(as_int, 42);

    const std::uint64_t as_u64 = value;
    BOOST_TEST_EQ(as_u64, UINT64_C(42));

    const double as_double = value;
    BOOST_TEST(as_double > 0.0);

    uint256 assigned {};
    assigned = 42;
    BOOST_TEST_EQ(assigned, uint256{42U});

    assigned = 42.0;
    BOOST_TEST_EQ(assigned, uint256{42U});

    uint256 movable {UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)};
    const uint256 moved {std::move(movable)};
    BOOST_TEST_EQ(moved.words[0], UINT64_C(4));
    BOOST_TEST_EQ(moved.words[3], UINT64_C(1));
}

int main()
{
    test_integer_conversion_traits();
    test_float_conversion_traits();
    test_stdfloat_conversion_traits();
    test_builtin_128_conversion_traits();
    test_conversions_run();

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_signed_integer_conversion_traits();
    test_signed_float_conversion_traits();
    test_signed_builtin_128_conversion_traits();
    test_signed_conversions_run();
#endif

    return boost::report_errors();
}
