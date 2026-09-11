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

#if defined(__STDCPP_FLOAT32_T__) || defined(__STDCPP_FLOAT64_T__) || defined(__STDCPP_BFLOAT16_T__)

// The extended floating point types are not part of the library's float trait,
// so they must not silently pick up the conversion.
void test_stdfloat_types_are_not_library_floats()
{
    #if defined(__STDCPP_FLOAT32_T__)
    static_assert(!std::is_convertible<std::float32_t, uint256>::value,
                  "std::float32_t is not one of the library floating point types");
    #endif
    #if defined(__STDCPP_FLOAT64_T__)
    static_assert(!std::is_convertible<std::float64_t, uint256>::value,
                  "std::float64_t is not one of the library floating point types");
    #endif
    #if defined(__STDCPP_BFLOAT16_T__)
    static_assert(!std::is_convertible<std::bfloat16_t, uint256>::value,
                  "std::bfloat16_t is not one of the library floating point types");
    #endif
}

#else

void test_stdfloat_types_are_not_library_floats()
{
}

#endif

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
    test_stdfloat_types_are_not_library_floats();
    test_builtin_128_conversion_traits();
    test_conversions_run();

    return boost::report_errors();
}
