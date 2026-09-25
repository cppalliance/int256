// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Drives every operator of int256 against every built-in integer type, in both
// operand orders, with boost::multiprecision::int512_t (via oracle.hpp's smp
// alias) as the reference. Every builtin, including unsigned __int128, fits
// inside int256's 256-bit range, so mixed comparisons are mathematically exact;
// the wrapping operators (+ - * <<) are checked against the value reduced modulo
// 2^256 (oracle.hpp's from_smp_wrapped), and bitwise/shift-left are additionally
// cross-checked in the unsigned bit domain, which uint256 already validates.
//
// Also covers the int128 defect A regression: computing a modulus/dividend
// against MIN must not divide by a zero intermediate (U % MIN, U / MIN, S % MIN).

#include "oracle.hpp"

#if defined(BOOST_INT256_ORACLE_UNAVAILABLE) || !defined(BOOST_INT256_TEST_HAS_SIGNED)

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <boost/mp11/list.hpp>
#include <boost/mp11/algorithm.hpp>

#include <cstring>
#include <type_traits>

using boost::int256::int256;

namespace
{

std::mt19937_64 rng {42};

template <typename T>
using is_narrow = std::integral_constant<bool, sizeof(T) <= sizeof(std::uint64_t)>;

template <typename T>
using is_signed_operand = std::integral_constant<bool, (static_cast<T>(-1) < static_cast<T>(0))>;

template <typename T>
T draw(std::true_type)
{
    const auto bits {rng()};

    T value {};
    std::memcpy(&value, &bits, sizeof(T));

    return value;
}

template <typename T>
T draw(std::false_type)
{
    const std::uint64_t bits[2] {rng(), rng()};

    T value {};
    std::memcpy(&value, bits, sizeof(T));

    return value;
}

// Draws an operand of the builtin type T across its whole range.
template <typename T>
T draw()
{
    return draw<T>(is_narrow<T>{});
}

// The exact mathematical value of a builtin, sign preserved. smp has far more
// headroom than any width up to 128 bits, so unlike uint256's widen() there is no
// wraparound to reason about for construction and comparison.
template <typename T>
smp widen_i(T v, std::true_type)
{
    return smp{v};
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

smp widen_i(boost::int256::detail::builtin_u128 v, std::false_type)
{
    const auto lo {static_cast<std::uint64_t>(v)};
    const auto hi {static_cast<std::uint64_t>(v >> 64)};

    return smp{smp{hi} << 64} | smp{lo};
}

smp widen_i(boost::int256::detail::builtin_i128 v, std::false_type)
{
    using builtin_u128 = boost::int256::detail::builtin_u128;
    using builtin_i128 = boost::int256::detail::builtin_i128;

    if (v >= 0)
    {
        return widen_i(static_cast<builtin_u128>(v), std::false_type{});
    }

    // Magnitude via -(v + 1) + 1 so INT128_MIN does not overflow on the way.
    const auto negated_plus_one {static_cast<builtin_i128>(-(v + 1))};
    const smp magnitude {widen_i(static_cast<builtin_u128>(negated_plus_one), std::false_type{}) + 1};

    return -magnitude;
}

#endif

template <typename T>
smp widen_i(T v)
{
    return widen_i(v, is_narrow<T>{});
}

// The conversion operator keeps the low bits of the value and truncates, so the
// expectation depends on how wide the target is. This is the same truncation
// rule uint256 uses; the sign of the source value does not change which bits
// come out.
template <typename T>
T expected_conversion(const int256& v, std::true_type)
{
    return static_cast<T>(v.words[0]);
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

template <typename T>
T expected_conversion(const int256& v, std::false_type)
{
    // Named builtin_wide, not wide, so it does not hide oracle.hpp's own
    // (unrelated) file-scope wide alias under MSVC /W4 (C4459)
    using builtin_wide = boost::int256::detail::builtin_u128;

    const builtin_wide bits {(static_cast<builtin_wide>(v.words[1]) << 64) | static_cast<builtin_wide>(v.words[0])};

    return static_cast<T>(bits);
}

#endif

template <typename T>
T expected_conversion(const int256& v)
{
    return expected_conversion<T>(v, is_narrow<T>{});
}

bool report_operand(const char* what, const int256& value)
{
    std::cerr << "int256 mixed operand mismatch in " << what << '\n'
              << "  operand = " << hex_words(value) << std::endl;

    return false;
}

template <typename T>
bool report_mixed(const char* what, const int256& lhs, T rhs, const int256& got, const int256& expected)
{
    std::cerr << "int256 mixed operand mismatch in " << what << '\n'
              << "  lhs      = " << hex_words(lhs) << '\n'
              << "  rhs      = " << hex_words(widen_i(rhs)) << '\n'
              << "  got      = " << hex_words(got) << '\n'
              << "  expected = " << hex_words(expected) << std::endl;

    return false;
}

// Compares the library result against the reference wrapped into the int256
// two's complement range, which is what a wrapping operator (+ - * <<) produces.
#define CHECK_MIXED(what, lhs, rhs, got, raw_expected) \
    BOOST_TEST((got) == ::int256_test::from_smp_wrapped(raw_expected) || \
               report_mixed(what, lhs, rhs, got, ::int256_test::from_smp_wrapped(raw_expected)))

template <typename T>
void check_sign_fill(const int256& constructed, T value, std::true_type)
{
    if (value < 0)
    {
        const std::size_t value_words {(sizeof(T) + sizeof(std::uint64_t) - 1U) / sizeof(std::uint64_t)};

        for (std::size_t i {value_words}; i < 4U; ++i)
        {
            BOOST_TEST_EQ(constructed.words[i], UINT64_MAX);
        }
    }
}

template <typename T>
void check_sign_fill(const int256&, T, std::false_type)
{
}

template <typename T>
void test_construction()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto value {draw<T>()};
        const int256 expected {from_smp_wrapped(widen_i(value))};

        const int256 constructed {value};
        BOOST_TEST(constructed == expected || report_mixed("constructor", constructed, value, constructed, expected));

        int256 assigned {};
        assigned = value;
        BOOST_TEST(assigned == expected || report_mixed("assignment", assigned, value, assigned, expected));

        check_sign_fill(constructed, value, is_signed_operand<T>{});
    }
}

template <typename T>
void test_conversion_operator()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const int256 value {random_int256(rng, shape::word4, sign::mixed)};

        const auto converted {static_cast<T>(value)};
        const auto expected {expected_conversion<T>(value)};

        BOOST_TEST(converted == expected || report_operand("conversion operator", value));

        // The bool conversion is the zero test, not a truncation.
        BOOST_TEST_EQ(static_cast<bool>(value), value != 0);
    }

    BOOST_TEST_EQ(static_cast<bool>(int256{}), false);
    BOOST_TEST_EQ(static_cast<bool>(int256{INT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)}), true);
}

// Every shape (1, 2, 3 and 4 significant words) and every sign, not just the
// full 4-word draw: a comparison bug between a small builtin and a positive
// multi-word int256 whose top word (words[3]) is zero (a 1, 2 or 3 word
// value) would not show up if "other" only ever had all four words populated.
template <typename T>
void test_comparisons()
{
    for (const auto other_shape : all_shapes())
    {
        for (const auto other_sign : {sign::pos, sign::neg, sign::mixed})
        {
            for (std::size_t i {0}; i < N; ++i)
            {
                const auto value {draw<T>()};
                const smp reference {widen_i(value)};

                const int256 same {value};
                const int256 other {random_int256(rng, other_shape, other_sign)};

                const smp msame {to_smp(same)};
                const smp mother {to_smp(other)};

                BOOST_TEST_EQ(same == value, msame == reference);
                BOOST_TEST_EQ(value == same, reference == msame);
                BOOST_TEST_EQ(same != value, msame != reference);
                BOOST_TEST_EQ(value != same, reference != msame);

                BOOST_TEST_EQ(other == value, mother == reference);
                BOOST_TEST_EQ(value == other, reference == mother);
                BOOST_TEST_EQ(other != value, mother != reference);
                BOOST_TEST_EQ(value != other, reference != mother);

                BOOST_TEST_EQ(other <  value, mother <  reference);
                BOOST_TEST_EQ(other <= value, mother <= reference);
                BOOST_TEST_EQ(other >  value, mother >  reference);
                BOOST_TEST_EQ(other >= value, mother >= reference);

                BOOST_TEST_EQ(value <  other, reference <  mother);
                BOOST_TEST_EQ(value <= other, reference <= mother);
                BOOST_TEST_EQ(value >  other, reference >  mother);
                BOOST_TEST_EQ(value >= other, reference >= mother);

                if (boost::detail::test_errors() != 0)
                {
                    std::cerr << "  shape = " << shape_name(other_shape) << std::endl;
                    return;
                }
            }
        }
    }
}

// The directed regression the conductor's review found: a positive int256
// whose significant words stop below words[3] (2^64, a 2-word value; 2^128, a
// 3-word value) must still compare correctly against a small builtin on
// either side.
void test_directed_multiword_comparison_regression()
{
    const int256 two_64 {INT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)};
    const int256 two_128 {INT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)};
    const int256 two_192 {INT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)};

    // Unsigned builtin, both orders.
    BOOST_TEST(5U < two_64);
    BOOST_TEST(5U < two_128);
    BOOST_TEST(5U <= two_64);
    BOOST_TEST(two_64 > 5U);
    BOOST_TEST(!(5U > two_64));
    BOOST_TEST(!(5U >= two_64));
    BOOST_TEST(5U != two_64);
    BOOST_TEST(two_64 != 5U);

    // Signed builtin, both orders.
    BOOST_TEST(5 < two_64);
    BOOST_TEST(5 < two_128);
    BOOST_TEST(5 <= two_64);
    BOOST_TEST(two_64 > 5);
    BOOST_TEST(!(5 > two_64));
    BOOST_TEST(!(5 >= two_64));

    // Every narrow width, 8 to 64 bits, both signedness, both orders, against
    // every 1/2/3/4 word positive shape.
    const int256 shapes[] {two_64, two_128, two_192};
    for (const auto& s : shapes)
    {
        BOOST_TEST(static_cast<std::uint8_t>(5) < s);
        BOOST_TEST(static_cast<std::int8_t>(5) < s);
        BOOST_TEST(static_cast<std::uint16_t>(5) < s);
        BOOST_TEST(static_cast<std::int16_t>(5) < s);
        BOOST_TEST(static_cast<std::uint32_t>(5) < s);
        BOOST_TEST(static_cast<std::int32_t>(5) < s);
        BOOST_TEST(static_cast<std::uint64_t>(5) < s);
        BOOST_TEST(static_cast<std::int64_t>(5) < s);

        BOOST_TEST(s > static_cast<std::uint8_t>(5));
        BOOST_TEST(s > static_cast<std::int8_t>(5));
        BOOST_TEST(s > static_cast<std::uint16_t>(5));
        BOOST_TEST(s > static_cast<std::int16_t>(5));
        BOOST_TEST(s > static_cast<std::uint32_t>(5));
        BOOST_TEST(s > static_cast<std::int32_t>(5));
        BOOST_TEST(s > static_cast<std::uint64_t>(5));
        BOOST_TEST(s > static_cast<std::int64_t>(5));
    }
}

// A modest per-shape-per-sign count: full N would be N * 10 shapes * 3 signs
// per builtin type, which is not needed to reach the one-word branches this
// sweep exists for.
constexpr std::size_t shape_sweep_n {64};

template <typename T>
void test_arithmetic_one(const int256& x, T value)
{
        const smp mx {to_smp(x)};
        const smp mv {widen_i(value)};

        {
            const int256 got {x + value};
            CHECK_MIXED("operator+(int256, T)", x, value, got, mx + mv);
        }
        {
            const int256 got {value + x};
            CHECK_MIXED("operator+(T, int256)", x, value, got, mv + mx);
        }
        {
            const int256 got {x - value};
            CHECK_MIXED("operator-(int256, T)", x, value, got, mx - mv);
        }
        {
            const int256 got {value - x};
            CHECK_MIXED("operator-(T, int256)", x, value, got, mv - mx);
        }
        {
            const int256 got {x * value};
            CHECK_MIXED("operator*(int256, T)", x, value, got, mx * mv);
        }
        {
            const int256 got {value * x};
            CHECK_MIXED("operator*(T, int256)", x, value, got, mv * mx);
        }
        if (mv != 0)
        {
            const int256 got {x / value};
            CHECK_MIXED("operator/(int256, T)", x, value, got, mx / mv);

            const int256 rem {x % value};
            CHECK_MIXED("operator%(int256, T)", x, value, rem, mx % mv);
        }
        if (mx != 0)
        {
            const int256 got {value / x};
            CHECK_MIXED("operator/(T, int256)", x, value, got, mv / mx);

            const int256 rem {value % x};
            CHECK_MIXED("operator%(T, int256)", x, value, rem, mv % mx);
        }

        // Compound forms take the same paths as the free operators.
        {
            int256 t {x};
            t += value;
            CHECK_MIXED("operator+=(T)", x, value, t, mx + mv);
        }
        {
            int256 t {x};
            t -= value;
            CHECK_MIXED("operator-=(T)", x, value, t, mx - mv);
        }
        {
            int256 t {x};
            t *= value;
            CHECK_MIXED("operator*=(T)", x, value, t, mx * mv);
        }
        if (mv != 0)
        {
            int256 t {x};
            t /= value;
            CHECK_MIXED("operator/=(T)", x, value, t, mx / mv);

            int256 u {x};
            u %= value;
            CHECK_MIXED("operator%=(T)", x, value, u, mx % mv);
        }
}

// Sweeps x over every shape (1, 2, 3, 4 significant words) and every sign,
// not just a full 4-word draw: T / x, T % x and x / T with a small |x| (the
// one-word branches), and the INT64_MIN magnitude paths, would rarely be
// reached from shape::word4 alone.
template <typename T>
void test_arithmetic()
{
    for (const auto x_shape : all_shapes())
    {
        for (const auto x_sign : {sign::pos, sign::neg, sign::mixed})
        {
            for (std::size_t i {0}; i < shape_sweep_n; ++i)
            {
                const auto value {draw<T>()};
                const int256 x {random_int256(rng, x_shape, x_sign)};

                test_arithmetic_one(x, value);

                if (boost::detail::test_errors() != 0)
                {
                    std::cerr << "  x shape = " << shape_name(x_shape) << std::endl;
                    return;
                }
            }
        }
    }
}

template <typename T>
void test_bitwise_one(const int256& x, T value)
{
        const mp mx_bits {to_mp(to_unsigned_bits(x))};
        const mp mv_bits {widen(value)};

        {
            const int256 got {x & value};
            BOOST_TEST_EQ(to_mp(to_unsigned_bits(got)), mp{mx_bits & mv_bits});
        }
        {
            const int256 got {value & x};
            BOOST_TEST_EQ(to_mp(to_unsigned_bits(got)), mp{mv_bits & mx_bits});
        }
        {
            const int256 got {x | value};
            BOOST_TEST_EQ(to_mp(to_unsigned_bits(got)), mp{mx_bits | mv_bits});
        }
        {
            const int256 got {value | x};
            BOOST_TEST_EQ(to_mp(to_unsigned_bits(got)), mp{mv_bits | mx_bits});
        }
        {
            const int256 got {x ^ value};
            BOOST_TEST_EQ(to_mp(to_unsigned_bits(got)), mp{mx_bits ^ mv_bits});
        }
        {
            const int256 got {value ^ x};
            BOOST_TEST_EQ(to_mp(to_unsigned_bits(got)), mp{mv_bits ^ mx_bits});
        }
        {
            int256 t {x};
            t &= value;
            BOOST_TEST_EQ(to_mp(to_unsigned_bits(t)), mp{mx_bits & mv_bits});
        }
        {
            int256 t {x};
            t |= value;
            BOOST_TEST_EQ(to_mp(to_unsigned_bits(t)), mp{mx_bits | mv_bits});
        }
        {
            int256 t {x};
            t ^= value;
            BOOST_TEST_EQ(to_mp(to_unsigned_bits(t)), mp{mx_bits ^ mv_bits});
        }
}

// Same shape/sign sweep as test_arithmetic, for the same reason.
template <typename T>
void test_bitwise()
{
    for (const auto x_shape : all_shapes())
    {
        for (const auto x_sign : {sign::pos, sign::neg, sign::mixed})
        {
            for (std::size_t i {0}; i < shape_sweep_n; ++i)
            {
                const auto value {draw<T>()};
                const int256 x {random_int256(rng, x_shape, x_sign)};

                test_bitwise_one(x, value);

                if (boost::detail::test_errors() != 0)
                {
                    std::cerr << "  x shape = " << shape_name(x_shape) << std::endl;
                    return;
                }
            }
        }
    }
}

// Directed cases: value in {INT64_MIN, -1, 0, T's max, T's min} against x in
// {MIN, MIN+1, -1, 0, 1, MAX, +-2^63, +-2^64}, covering the small-x-divisor and
// INT64_MIN-magnitude paths by name rather than hoping a random sweep lands on
// them.
// std::numeric_limits<builtin_i128/u128> is not reliable everywhere (only
// defined automatically in GNU mode on older compilers, or GCC 13+), so the
// bound for the 128-bit builtins is computed by hand instead of trusted from
// numeric_limits.
template <typename T>
T directed_max(std::true_type /* narrow */)
{
    return (std::numeric_limits<T>::max)();
}

template <typename T>
T directed_min(std::true_type /* narrow */)
{
    return (std::numeric_limits<T>::min)();
}

#if defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128)

boost::int256::detail::builtin_u128 directed_max_impl(std::true_type)
{
    return ~static_cast<boost::int256::detail::builtin_u128>(0);
}

boost::int256::detail::builtin_i128 directed_max_impl(std::false_type)
{
    return static_cast<boost::int256::detail::builtin_i128>(
        (static_cast<boost::int256::detail::builtin_u128>(1) << 127) - 1);
}

template <typename T>
T directed_max(std::false_type /* wide */)
{
    return static_cast<T>(directed_max_impl(is_signed_operand<T>{}));
}

template <typename T>
T directed_min(std::false_type /* wide */)
{
    return static_cast<T>(is_signed_operand<T>::value ? -directed_max<T>(std::false_type{}) - 1
                                                        : static_cast<T>(0));
}

#endif

template <typename T>
void test_arithmetic_directed()
{
    const T values[] {
        static_cast<T>(std::numeric_limits<std::int64_t>::min()),
        static_cast<T>(-1),
        static_cast<T>(0),
        directed_max<T>(is_narrow<T>{}),
        directed_min<T>(is_narrow<T>{})
    };

    const int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    const int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    const int256 two_63 {INT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1) << 63};
    const int256 two_64 {INT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)};

    const int256 xs[] {
        min_v, min_v + 1, int256{-1}, int256{0}, int256{1}, max_v,
        two_63, -two_63, two_64, -two_64
    };

    for (const auto v : values)
    {
        for (const auto& x : xs)
        {
            test_arithmetic_one(x, v);
            test_bitwise_one(x, v);

            if (boost::detail::test_errors() != 0)
            {
                return;
            }
        }
    }
}

// Left shift wraps in the unsigned domain, so it is checked the same way as
// arithmetic: reduce the exact mathematical product modulo 2^256. Right shift
// is arithmetic (sign extending), which oracle.hpp's
// reference_arithmetic_shift_right computes safely without mp's ambiguous >> on
// negative operands.
template <typename T>
void test_shifts()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const int256 x {random_int256(rng, shape::word4, sign::mixed)};
        const smp mx {to_smp(x)};

        const auto limit {static_cast<unsigned>(sizeof(T) == 1 ? 100U : 256U)};
        const auto count {static_cast<unsigned>(rng() % limit)};
        const auto typed_count {static_cast<T>(count)};

        {
            const int256 got {x << typed_count};
            CHECK_MIXED("operator<<(int256, T)", x, typed_count, got, mx * (smp{1} << count));
        }
        {
            const int256 got {x >> typed_count};
            const int256 expected {reference_arithmetic_shift_right(x, count)};
            BOOST_TEST(got == expected || report_mixed("operator>>(int256, T)", x, typed_count, got, expected));
        }
        {
            int256 t {x};
            t <<= typed_count;
            CHECK_MIXED("operator<<=(T)", x, typed_count, t, mx * (smp{1} << count));
        }
        {
            int256 t {x};
            t >>= typed_count;
            const int256 expected {reference_arithmetic_shift_right(x, count)};
            BOOST_TEST(t == expected || report_mixed("operator>>=(T)", x, typed_count, t, expected));
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_unary_and_increment()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const int256 x {random_int256(rng, shape::word4, sign::mixed)};
        const smp mx {to_smp(x)};

        const int256 plus {+x};
        const int256 minus {-x};

        BOOST_TEST(plus == x);
        {
            const int256 expected {from_smp_wrapped(-mx)};
            BOOST_TEST(minus == expected || report_mixed("unary operator-", x, 0, minus, expected));
        }
        {
            const mp mx_bits {to_mp(to_unsigned_bits(x))};
            const int256 flipped {~x};
            BOOST_TEST_EQ(to_mp(to_unsigned_bits(flipped)), mp{~mx_bits});
        }

        {
            int256 t {x};
            const int256 after {++t};
            const int256 expected {from_smp_wrapped(mx + 1)};
            BOOST_TEST(after == expected || report_mixed("operator++ pre", x, 0, after, expected));
        }
        {
            int256 t {x};
            const int256 before {t++};
            BOOST_TEST(before == x);
            const int256 expected {from_smp_wrapped(mx + 1)};
            BOOST_TEST(t == expected || report_mixed("operator++ post", x, 0, t, expected));
        }
        {
            int256 t {x};
            const int256 after {--t};
            const int256 expected {from_smp_wrapped(mx - 1)};
            BOOST_TEST(after == expected || report_mixed("operator-- pre", x, 0, after, expected));
        }
        {
            int256 t {x};
            const int256 before {t--};
            BOOST_TEST(before == x);
            const int256 expected {from_smp_wrapped(mx - 1)};
            BOOST_TEST(t == expected || report_mixed("operator-- post", x, 0, t, expected));
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    // MIN is its own negation and its own absolute value: the magnitude 2^255
    // is not representable, so both wrap back to MIN, exactly like the builtins.
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    constexpr int256 max_v {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    // Boost.Int256's iostream support (D2) has not landed yet, so int256 has
    // no operator<<; BOOST_TEST(a == b) is used instead of BOOST_TEST_EQ for
    // int256 operands until then (BOOST_TEST_EQ would try to stream them on a
    // failure, which is currently ambiguous since int256 converts implicitly
    // to several builtin types).
    BOOST_TEST(-min_v == min_v);
    BOOST_TEST(boost::int256::abs(min_v) == min_v);
    BOOST_TEST(boost::int256::abs(min_v + 1) == max_v);
    BOOST_TEST(boost::int256::abs(max_v) == max_v);
    BOOST_TEST(boost::int256::abs(int256{0}) == int256{0});
    BOOST_TEST(boost::int256::abs(int256{-1}) == int256{1});

    // MIN / -1 == MIN and MIN % -1 == 0: the mathematical quotient 2^255 is not
    // representable, so it wraps, and it does so exactly back to MIN.
    BOOST_TEST(min_v / int256{-1} == min_v);
    BOOST_TEST(min_v % int256{-1} == int256{0});

    // Wraparound at the boundaries.
    BOOST_TEST(max_v + int256{1} == min_v);
    BOOST_TEST(min_v - int256{1} == max_v);
}

// bool gets its own equality overloads so that MSVC does not warn about forcing
// a value to bool, and the constructor takes it as 0 or 1 rather than sign
// extending.
void test_bool_operands()
{
    const int256 zero {};
    const int256 one {INT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    const int256 big {INT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)};

    BOOST_TEST(int256{true} == one);
    BOOST_TEST(int256{false} == zero);

    BOOST_TEST(one == true);
    BOOST_TEST(true == one);
    BOOST_TEST(zero == false);
    BOOST_TEST(false == zero);
    BOOST_TEST(big != true);
    BOOST_TEST(true != big);
    BOOST_TEST(zero != true);
    BOOST_TEST(true != zero);
}

// int128 defect A: int128's UnsignedInteger % INT128_MIN divided by zero
// internally, because abs(MIN) is still MIN (its magnitude wraps to itself),
// and a careless implementation used that wrapped abs() as an unsigned divisor
// word directly. This must not happen here: U % MIN, U / MIN and S % MIN are
// all exact. This has to exercise the actual mixed builtin-vs-int256
// overloads directly (T % min_v, not static_cast<int256>(T) % min_v, which
// never touches them at all), so it is templated per builtin type and called
// from test_caller for every T, exactly like the other mixed operand tests.
template <typename IntType, std::enable_if_t<boost::int256::detail::is_signed_integer_v<IntType>, bool> = true>
void test_min_divisor()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};

    const IntType values[] {
        static_cast<IntType>(0), static_cast<IntType>(1), static_cast<IntType>(-1),
        static_cast<IntType>(5), static_cast<IntType>(-5),
        directed_max<IntType>(is_narrow<IntType>{}), directed_min<IntType>(is_narrow<IntType>{})
    };

    // Magnitude smaller than |MIN|, whatever the sign: mathematically exact
    // quotient 0, remainder the dividend itself.
    for (const auto v : values)
    {
        BOOST_TEST(v % min_v == v);
        BOOST_TEST(v / min_v == 0);
    }
}

// On MSVC, detail::builtin_u128 (std::_Unsigned128) is in the test_types list
// below, but is_unsigned_integer_v only recognizes it under
// BOOST_INT256_HAS_INT128 (the GCC/clang __int128 path), so it needs to be
// named explicitly here. The builtin_u128 % and / overloads against int256
// exist under BOOST_INT256_HAS_MSVC_INT128 too (traits.hpp's dedicated
// 128-bit concept checks is_same directly, not this trait), so this is not a
// functionality gap, only a missing trait entry.
template <typename IntType, std::enable_if_t<boost::int256::detail::is_unsigned_integer_v<IntType>
#ifdef BOOST_INT256_HAS_MSVC_INT128
    || std::is_same<IntType, boost::int256::detail::builtin_u128>::value
#endif
    , bool> = true>
void test_min_divisor()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};

    const IntType values[] {
        static_cast<IntType>(0), static_cast<IntType>(1), static_cast<IntType>(5),
        static_cast<IntType>(-1), // wraps to the type's maximum
        directed_max<IntType>(is_narrow<IntType>{})
    };

    for (const auto v : values)
    {
        BOOST_TEST(v % min_v == v);
        BOOST_TEST(v / min_v == 0);
    }
}

void test_min_against_itself()
{
    constexpr int256 min_v {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};

    // MIN against itself: quotient 1, remainder 0.
    BOOST_TEST(min_v / min_v == int256{1});
    BOOST_TEST(min_v % min_v == int256{0});
}

struct test_caller
{
    template <typename T>
    void operator()(T) const
    {
        test_construction<T>();
        test_conversion_operator<T>();
        test_comparisons<T>();
        test_arithmetic<T>();
        test_bitwise<T>();
        test_shifts<T>();
        test_arithmetic_directed<T>();
        test_min_divisor<T>();
    }
};

} // namespace

int main()
{
    using test_types = boost::mp11::mp_list<
        char,
        signed char,
        unsigned char,
        char16_t,
        char32_t,
        wchar_t,
        short,
        unsigned short,
        int,
        unsigned int,
        long,
        unsigned long,
        long long,
        unsigned long long
        #if defined(BOOST_INT256_HAS_INT128)
        , boost::int256::detail::builtin_i128
        , boost::int256::detail::builtin_u128
        #elif defined(BOOST_INT256_HAS_MSVC_INT128)
        // std::_Signed128 is not compatible with Boost.Random, matching
        // Boost.Int128's own test_types list on MSVC.
        , boost::int256::detail::builtin_u128
        #endif
    >;

    boost::mp11::mp_for_each<test_types>(test_caller());

    test_unary_and_increment();
    test_bool_operands();
    test_min_against_itself();
    test_directed_multiword_comparison_regression();

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE || !BOOST_INT256_TEST_HAS_SIGNED
