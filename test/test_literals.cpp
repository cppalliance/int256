// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The user-defined literals and BOOST_INT256_UINT256_C. Both spellings exist in
// a raw and a cooked form, both accept the C++ digit separator, and an out of
// range or malformed literal is a compile error rather than a truncation. The
// rejection cases live in compile_tests/literals_*_fail.cpp; what is checked here
// is that everything legal parses to the right value at compile time.

#ifndef BOOST_INT256_BUILD_MODULE

#include <boost/int256.hpp>

#else

import boost.int256;
// A module import carries no macros, so the BOOST_INT256_* feature and endianness
// macros have to come from the header even in the module build.
#include <boost/int256/detail/config.hpp>
#include <boost/int256/detail/literal_macros.hpp>

#endif

#include <boost/core/lightweight_test.hpp>

#include <cstdint>
#include <stdexcept>

using boost::int256::uint256;
using boost::int256::int256;
using namespace boost::int256::literals;

namespace
{

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
constexpr uint256 max {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
constexpr int256 one_i {INT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
constexpr int256 min_i {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
constexpr int256 max_i {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

// Decimal, at every word boundary.
static_assert(0_u256 == uint256{}, "zero");
static_assert(1_u256 == one, "one");
static_assert(42_u256 == uint256{42U}, "a small value");
static_assert(18446744073709551615_u256 == uint256{UINT64_MAX}, "2^64 - 1");
static_assert(18446744073709551616_u256 == (one << 64U), "2^64");
static_assert(340282366920938463463374607431768211455_u256 == (one << 128U) - 1U, "2^128 - 1");
static_assert(340282366920938463463374607431768211456_u256 == (one << 128U), "2^128");
static_assert(6277101735386680763835789423207666416102355444464034512896_u256 == (one << 192U), "2^192");
static_assert(115792089237316195423570985008687907853269984665640564039457584007913129639935_u256 == max,
              "the maximum");

// The uppercase spelling is the same operator.
static_assert(0_U256 == uint256{}, "zero, uppercase suffix");
static_assert(18446744073709551616_U256 == (one << 64U), "2^64, uppercase suffix");
static_assert(115792089237316195423570985008687907853269984665640564039457584007913129639935_U256 == max,
              "the maximum, uppercase suffix");

// Hexadecimal, octal and binary prefixes.
static_assert(0x0_u256 == uint256{}, "hex zero");
static_assert(0xFF_u256 == uint256{255U}, "hex, uppercase digits");
static_assert(0xff_u256 == uint256{255U}, "hex, lowercase digits");
static_assert(0x10000000000000000_u256 == (one << 64U), "hex 2^64");
static_assert(0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_u256 == max, "hex maximum");
static_assert(0X1F_u256 == uint256{31U}, "an uppercase hex prefix");
static_assert(0777_u256 == uint256{511U}, "octal");
static_assert(0b1010_u256 == uint256{10U}, "binary");
static_assert(0B1010_u256 == uint256{10U}, "an uppercase binary prefix");

// The digit separator is accepted and ignored.
static_assert(1'000'000_u256 == uint256{1000000U}, "decimal separators");
static_assert(0xFF'FF_u256 == uint256{65535U}, "hex separators");
static_assert(115'792'089'237'316'195'423'570'985'008'687'907'853'269'984'665'640'564'039'457'584'007'913'129'639'935_u256 == max,
              "the maximum with separators");

// Leading zeros in a decimal literal are octal to the compiler, so a literal such
// as 0123 is octal; that is the language rule and it is pinned here.
static_assert(0123_u256 == uint256{83U}, "a leading zero means octal");

// BOOST_INT256_UINT256_C accepts the same decimal spellings.
static_assert(BOOST_INT256_UINT256_C(0) == uint256{}, "UINT256_C zero");
static_assert(BOOST_INT256_UINT256_C(42) == uint256{42U}, "UINT256_C small");
static_assert(BOOST_INT256_UINT256_C(18446744073709551616) == (one << 64U), "UINT256_C 2^64");
static_assert(BOOST_INT256_UINT256_C(115792089237316195423570985008687907853269984665640564039457584007913129639935) == max,
              "UINT256_C maximum");

// _i256/_I256, raw form. A raw literal never carries a leading minus sign (see
// the contract note in test_consteval_funcs.cpp: -X_i256 is unary minus
// applied to X_i256, so X itself must be at most MAX). MIN is reached only
// through the string or macro forms below, which pass the '-' to
// parse_literal directly.
static_assert(0_i256 == int256{}, "zero");
static_assert(1_i256 == one_i, "one");
static_assert(42_i256 == int256{42}, "a small value");
static_assert(-1_i256 == int256{-1}, "negated small literal");
static_assert(-42_i256 == int256{-42}, "negated small literal");
static_assert(57896044618658097711785492504343953926634992332820282019728792003956564819967_i256 == max_i,
              "MAX");
static_assert(-57896044618658097711785492504343953926634992332820282019728792003956564819967_i256 ==
              min_i + one_i, "-MAX == MIN + 1");

// The uppercase spelling is the same operator.
static_assert(0_I256 == int256{}, "zero, uppercase suffix");
static_assert(57896044618658097711785492504343953926634992332820282019728792003956564819967_I256 == max_i,
              "MAX, uppercase suffix");

// Hexadecimal, octal and binary prefixes. A prefixed literal's magnitude is
// checked against the same positive range as a decimal one (at most MAX), so
// 0x7FFF...F (MAX itself) is the largest hex literal that does not need the
// string form.
static_assert(0x0_i256 == int256{}, "hex zero");
static_assert(0xFF_i256 == int256{255}, "hex, uppercase digits");
static_assert(0xff_i256 == int256{255}, "hex, lowercase digits");
static_assert(-0xFF_i256 == int256{-255}, "negated hex");
static_assert(0777_i256 == int256{511}, "octal");
static_assert(0b1010_i256 == int256{10}, "binary");
static_assert(0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_i256 == max_i, "hex MAX");

// The digit separator is accepted and ignored.
static_assert(1'000'000_i256 == int256{1000000}, "decimal separators");
static_assert(0xFF'FF_i256 == int256{65535}, "hex separators");

// BOOST_INT256_INT256_C accepts the same raw spellings, plus the leading '-'
// that the raw suffix form cannot (it goes through the string literal
// operator, which does accept one).
static_assert(BOOST_INT256_INT256_C(0) == int256{}, "INT256_C zero");
static_assert(BOOST_INT256_INT256_C(42) == int256{42}, "INT256_C small");
static_assert(BOOST_INT256_INT256_C(-42) == int256{-42}, "INT256_C negative");
static_assert(BOOST_INT256_INT256_C(57896044618658097711785492504343953926634992332820282019728792003956564819967) == max_i,
              "INT256_C MAX");

// MIN, every way the contract allows: the macro over the decimal spelling,
// the macro over the hex spelling, and the string literal operator directly
// over each spelling. None of these is a raw suffixed token.
static_assert(BOOST_INT256_INT256_C(-57896044618658097711785492504343953926634992332820282019728792003956564819968) == min_i,
              "INT256_C MIN via decimal");
static_assert(BOOST_INT256_INT256_C(-0x8000000000000000000000000000000000000000000000000000000000000000) == min_i,
              "INT256_C MIN via hex");
static_assert("-57896044618658097711785492504343953926634992332820282019728792003956564819968"_i256 == min_i,
              "MIN via decimal string literal");
static_assert("-0x8000000000000000000000000000000000000000000000000000000000000000"_i256 == min_i,
              "MIN via hex string literal");

// An out of range literal is a compile error wherever it is a constant expression
// (compile_tests/literals_overflow_fail.cpp pins that). The operators are noexcept, so at
// run time there is nothing to catch; a value that fits still parses through the raw form.
void test_noexcept_contract()
{
    static_assert(noexcept(boost::int256::literals::operator ""_u256("0")), "literal operators are noexcept");
    static_assert(noexcept(boost::int256::literals::operator ""_U256("0")), "literal operators are noexcept");
    static_assert(noexcept(boost::int256::literals::operator ""_u256("0", 1U)), "literal operators are noexcept");
    static_assert(noexcept(boost::int256::literals::operator ""_U256("0", 1U)), "literal operators are noexcept");

    static_assert(noexcept(boost::int256::literals::operator ""_i256("0")), "literal operators are noexcept");
    static_assert(noexcept(boost::int256::literals::operator ""_I256("0")), "literal operators are noexcept");
    static_assert(noexcept(boost::int256::literals::operator ""_i256("0", 1U)), "literal operators are noexcept");
    static_assert(noexcept(boost::int256::literals::operator ""_I256("0", 1U)), "literal operators are noexcept");

    // detail::parse_literal is not part of the module's exported interface,
    // so this direct call is host-only; the public literal operators above
    // (which the module build does export) already cover the noexcept
    // contract there.
    #ifndef BOOST_INT256_BUILD_MODULE
    const char* text {"115792089237316195423570985008687907853269984665640564039457584007913129639935"};
    volatile const char* opaque_text {text};

    const auto value {boost::int256::detail::parse_literal<uint256>(
        const_cast<const char*>(opaque_text),
        const_cast<const char*>(opaque_text) + 78)};

    BOOST_TEST(value == max);

    const char* signed_text {"-57896044618658097711785492504343953926634992332820282019728792003956564819968"};
    volatile const char* opaque_signed_text {signed_text};

    const auto signed_value {boost::int256::detail::parse_literal<int256>(
        const_cast<const char*>(opaque_signed_text),
        const_cast<const char*>(opaque_signed_text) + 78)};

    BOOST_TEST(signed_value == min_i);
    #endif
}

void test_runtime()
{
    // The same literals used as ordinary values.
    BOOST_TEST_EQ(42_u256, uint256{42U});
    BOOST_TEST_EQ(0xFF_u256, uint256{255U});
    BOOST_TEST_EQ(1'000'000_u256, uint256{1000000U});
    BOOST_TEST_EQ(115792089237316195423570985008687907853269984665640564039457584007913129639935_u256, max);

    // Arithmetic on literals.
    BOOST_TEST_EQ(2_u256 * 3_u256, 6_u256);
    BOOST_TEST_EQ(18446744073709551616_u256 - 1_u256, uint256{UINT64_MAX});
    BOOST_TEST_EQ(0x10000000000000000_u256 / 2_u256, uint256{UINT64_C(0x8000000000000000)});

    // The macro in a runtime context.
    BOOST_TEST_EQ(BOOST_INT256_UINT256_C(340282366920938463463374607431768211456), (one << 128U));

    // Every power of two through the decimal literal path would be unwieldy, so
    // the hex form covers the width instead.
    BOOST_TEST_EQ(0x8000000000000000000000000000000000000000000000000000000000000000_u256, one << 255U);
    BOOST_TEST_EQ(0x100000000000000000000000000000000_u256, one << 128U);
    BOOST_TEST_EQ(0x1000000000000000000000000000000000000000000000000_u256, one << 192U);

    // The same, signed.
    BOOST_TEST(42_i256 == int256{42});
    BOOST_TEST(-42_i256 == int256{-42});
    BOOST_TEST(0xFF_i256 == int256{255});
    BOOST_TEST(1'000'000_i256 == int256{1000000});
    BOOST_TEST(57896044618658097711785492504343953926634992332820282019728792003956564819967_i256 == max_i);
    BOOST_TEST(BOOST_INT256_INT256_C(-57896044618658097711785492504343953926634992332820282019728792003956564819968) == min_i);
    BOOST_TEST("-57896044618658097711785492504343953926634992332820282019728792003956564819968"_i256 == min_i);

    // Arithmetic on signed literals.
    BOOST_TEST((2_i256 * -3_i256) == int256{-6});
    BOOST_TEST((-1_i256 - 1_i256) == int256{-2});

    // 0x8000...0 (2^255) is MIN's magnitude, and as a raw _i256 token it is one
    // past MAX (see compile_tests/literals_i256_hex_overflow_fail.cpp), so it
    // has to be built from a value that IS in range before shifting.
    BOOST_TEST((one_i << 255U) == min_i);
    BOOST_TEST(((one_i << 255U) >> 255) == int256{-1});
}

// The literal is a constant expression, so it can size an array.
void test_usable_as_a_constant()
{
    constexpr auto value {42_u256};
    static_assert(value == uint256{42U}, "the literal is constexpr");

    constexpr auto from_macro {BOOST_INT256_UINT256_C(42)};
    static_assert(from_macro == value, "the macro agrees with the literal");

    constexpr auto signed_value {-42_i256};
    static_assert(signed_value == int256{-42}, "the signed literal is constexpr");

    constexpr auto signed_from_macro {BOOST_INT256_INT256_C(-42)};
    static_assert(signed_from_macro == signed_value, "the macro agrees with the signed literal");
}

} // namespace

int main()
{
    test_runtime();
    test_noexcept_contract();
    test_usable_as_a_constant();

    return boost::report_errors();
}
