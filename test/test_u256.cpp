// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Drives every operator of uint256 against every built-in integer type, in both
// operand orders, with boost::multiprecision::uint256_t as the reference.
//
// int128 can compare against the builtin unsigned __int128; there is no builtin
// 256-bit type, so the reference is the widened value the usual arithmetic
// conversions produce: a negative signed operand becomes 2^256 minus its
// magnitude, which is why uint256{0} < -1 is true.

#include "oracle.hpp"

#ifdef BOOST_INT256_ORACLE_UNAVAILABLE

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

using boost::int256::uint256;

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

// The conversion operator keeps the low bits of the value and truncates, so the
// expectation depends on how wide the target is.
template <typename T>
T expected_conversion(const uint256& v, std::true_type)
{
    return static_cast<T>(v.words[0]);
}

#if defined(BOOST_INT256_HAS_INT128)

template <typename T>
T expected_conversion(const uint256& v, std::false_type)
{
    using wide = boost::int256::detail::builtin_u128;

    const wide bits {(static_cast<wide>(v.words[1]) << 64) | static_cast<wide>(v.words[0])};

    return static_cast<T>(bits);
}

#endif

template <typename T>
T expected_conversion(const uint256& v)
{
    return expected_conversion<T>(v, is_narrow<T>{});
}

bool report_operand(const char* what, const uint256& value)
{
    std::cerr << "int256 mixed operand mismatch in " << what << '\n'
              << "  operand = " << hex_words(value) << std::endl;

    return false;
}

template <typename T>
bool report_mixed(const char* what, const uint256& lhs, T rhs, const mp& got, const mp& expected)
{
    std::cerr << "int256 mixed operand mismatch in " << what << '\n'
              << "  lhs      = " << hex_words(lhs) << '\n'
              << "  rhs      = " << hex_words(widen(rhs)) << '\n'
              << "  got      = " << hex_words(got) << '\n'
              << "  expected = " << hex_words(expected) << std::endl;

    return false;
}

#define BOOST_INT256_TEST_MIXED(what, lhs, rhs, got, expected) \
    BOOST_TEST(to_mp(got) == (expected) || report_mixed(what, lhs, rhs, to_mp(got), expected))

// A negative operand sign fills every word above the ones its own value
// occupies. Only instantiated for signed T so that an unsigned operand never
// gets compared against zero.
template <typename T>
void check_sign_fill(const uint256& constructed, T value, std::true_type)
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
void check_sign_fill(const uint256&, T, std::false_type)
{
}

template <typename T>
void test_construction()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto value {draw<T>()};
        const mp reference {widen(value)};

        const uint256 constructed {value};
        BOOST_TEST(to_mp(constructed) == reference ||
                   report_mixed("constructor", constructed, value, to_mp(constructed), reference));

        uint256 assigned {};
        assigned = value;
        BOOST_TEST(to_mp(assigned) == reference ||
                   report_mixed("assignment", assigned, value, to_mp(assigned), reference));

        check_sign_fill(constructed, value, is_signed_operand<T>{});
    }
}

template <typename T>
void test_conversion_operator()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 value {random_uint256(rng, shape::word4)};

        const auto converted {static_cast<T>(value)};
        const auto expected {expected_conversion<T>(value)};

        BOOST_TEST(converted == expected || report_operand("conversion operator", value));

        // The bool conversion is the zero test, not a truncation.
        BOOST_TEST_EQ(static_cast<bool>(value), value != 0U);
    }

    BOOST_TEST_EQ(static_cast<bool>(uint256{}), false);
    BOOST_TEST_EQ(static_cast<bool>(uint256{UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)}), true);
}

template <typename T>
void test_comparisons()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto value {draw<T>()};
        const mp reference {widen(value)};

        const uint256 same {value};
        const uint256 other {random_uint256(rng, shape::word4)};

        const mp msame {to_mp(same)};
        const mp mother {to_mp(other)};

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
            return;
        }
    }
}

template <typename T>
void test_arithmetic()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto value {draw<T>()};
        const uint256 x {random_uint256(rng, shape::word4)};

        const mp mx {to_mp(x)};
        const mp mv {widen(value)};

        {
            const uint256 got {x + value};
            BOOST_INT256_TEST_MIXED("operator+(uint256, T)", x, value, got, mp{mx + mv});
        }
        {
            const uint256 got {value + x};
            BOOST_INT256_TEST_MIXED("operator+(T, uint256)", x, value, got, mp{mv + mx});
        }
        {
            const uint256 got {x - value};
            BOOST_INT256_TEST_MIXED("operator-(uint256, T)", x, value, got, mp{mx - mv});
        }
        {
            const uint256 got {value - x};
            BOOST_INT256_TEST_MIXED("operator-(T, uint256)", x, value, got, mp{mv - mx});
        }
        {
            const uint256 got {x * value};
            BOOST_INT256_TEST_MIXED("operator*(uint256, T)", x, value, got, mp{mx * mv});
        }
        {
            const uint256 got {value * x};
            BOOST_INT256_TEST_MIXED("operator*(T, uint256)", x, value, got, mp{mv * mx});
        }
        if (mv != 0)
        {
            const uint256 got {x / value};
            BOOST_INT256_TEST_MIXED("operator/(uint256, T)", x, value, got, mp{mx / mv});

            const uint256 rem {x % value};
            BOOST_INT256_TEST_MIXED("operator%(uint256, T)", x, value, rem, mp{mx % mv});
        }
        if (mx != 0)
        {
            const uint256 got {value / x};
            BOOST_INT256_TEST_MIXED("operator/(T, uint256)", x, value, got, mp{mv / mx});

            const uint256 rem {value % x};
            BOOST_INT256_TEST_MIXED("operator%(T, uint256)", x, value, rem, mp{mv % mx});
        }

        // Compound forms take the same paths as the free operators.
        {
            uint256 t {x};
            t += value;
            BOOST_INT256_TEST_MIXED("operator+=(T)", x, value, t, mp{mx + mv});
        }
        {
            uint256 t {x};
            t -= value;
            BOOST_INT256_TEST_MIXED("operator-=(T)", x, value, t, mp{mx - mv});
        }
        {
            uint256 t {x};
            t *= value;
            BOOST_INT256_TEST_MIXED("operator*=(T)", x, value, t, mp{mx * mv});
        }
        if (mv != 0)
        {
            uint256 t {x};
            t /= value;
            BOOST_INT256_TEST_MIXED("operator/=(T)", x, value, t, mp{mx / mv});

            uint256 u {x};
            u %= value;
            BOOST_INT256_TEST_MIXED("operator%=(T)", x, value, u, mp{mx % mv});
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

template <typename T>
void test_bitwise()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto value {draw<T>()};
        const uint256 x {random_uint256(rng, shape::word4)};

        const mp mx {to_mp(x)};
        const mp mv {widen(value)};

        {
            const uint256 got {x & value};
            BOOST_INT256_TEST_MIXED("operator&(uint256, T)", x, value, got, mp{mx & mv});
        }
        {
            const uint256 got {value & x};
            BOOST_INT256_TEST_MIXED("operator&(T, uint256)", x, value, got, mp{mv & mx});
        }
        {
            const uint256 got {x | value};
            BOOST_INT256_TEST_MIXED("operator|(uint256, T)", x, value, got, mp{mx | mv});
        }
        {
            const uint256 got {value | x};
            BOOST_INT256_TEST_MIXED("operator|(T, uint256)", x, value, got, mp{mv | mx});
        }
        {
            const uint256 got {x ^ value};
            BOOST_INT256_TEST_MIXED("operator^(uint256, T)", x, value, got, mp{mx ^ mv});
        }
        {
            const uint256 got {value ^ x};
            BOOST_INT256_TEST_MIXED("operator^(T, uint256)", x, value, got, mp{mv ^ mx});
        }
        {
            uint256 t {x};
            t &= value;
            BOOST_INT256_TEST_MIXED("operator&=(T)", x, value, t, mp{mx & mv});
        }
        {
            uint256 t {x};
            t |= value;
            BOOST_INT256_TEST_MIXED("operator|=(T)", x, value, t, mp{mx | mv});
        }
        {
            uint256 t {x};
            t ^= value;
            BOOST_INT256_TEST_MIXED("operator^=(T)", x, value, t, mp{mx ^ mv});
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// A shift count arrives as every integer type, so the count conversion is
// exercised once per type rather than only for int.
template <typename T>
void test_shifts()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 x {random_uint256(rng, shape::word4)};
        const mp mx {to_mp(x)};

        // The count has to fit T, and shifting by 256 or more is undefined.
        const auto limit {static_cast<unsigned>(sizeof(T) == 1 ? 100U : 256U)};
        const auto count {static_cast<unsigned>(rng() % limit)};
        const auto typed_count {static_cast<T>(count)};

        {
            const uint256 got {x << typed_count};
            BOOST_INT256_TEST_MIXED("operator<<(uint256, T)", x, typed_count, got, mp{mx << count});
        }
        {
            const uint256 got {x >> typed_count};
            BOOST_INT256_TEST_MIXED("operator>>(uint256, T)", x, typed_count, got, mp{mx >> count});
        }
        {
            uint256 t {x};
            t <<= typed_count;
            BOOST_INT256_TEST_MIXED("operator<<=(T)", x, typed_count, t, mp{mx << count});
        }
        {
            uint256 t {x};
            t >>= typed_count;
            BOOST_INT256_TEST_MIXED("operator>>=(T)", x, typed_count, t, mp{mx >> count});
        }

        // A uint256 shift count forwards its low word.
        {
            const uint256 shift_by {UINT64_C(0), UINT64_C(0), UINT64_C(0), count};
            const uint256 got {x << shift_by};
            BOOST_INT256_TEST_MIXED("operator<<(uint256, uint256)", x, typed_count, got, mp{mx << count});

            const uint256 got_right {x >> shift_by};
            BOOST_INT256_TEST_MIXED("operator>>(uint256, uint256)", x, typed_count, got_right, mp{mx >> count});
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
        const uint256 x {random_uint256(rng, shape::word4)};
        const mp mx {to_mp(x)};

        const uint256 plus {+x};
        const uint256 minus {-x};
        const uint256 flipped {~x};

        BOOST_INT256_TEST_UNARY("unary operator+", x, plus, mx);
        BOOST_INT256_TEST_UNARY("unary operator-", x, minus, mp{mp{0} - mx});
        BOOST_INT256_TEST_UNARY("operator~", x, flipped, mp{~mx});

        {
            uint256 t {x};
            const uint256 after {++t};
            BOOST_INT256_TEST_UNARY("operator++ pre", x, after, mp{mx + 1});
        }
        {
            uint256 t {x};
            const uint256 before {t++};
            BOOST_INT256_TEST_UNARY("operator++ post returns old", x, before, mx);
            BOOST_INT256_TEST_UNARY("operator++ post", x, t, mp{mx + 1});
        }
        {
            uint256 t {x};
            const uint256 after {--t};
            BOOST_INT256_TEST_UNARY("operator-- pre", x, after, mp{mx - 1});
        }
        {
            uint256 t {x};
            const uint256 before {t--};
            BOOST_INT256_TEST_UNARY("operator-- post returns old", x, before, mx);
            BOOST_INT256_TEST_UNARY("operator-- post", x, t, mp{mx - 1});
        }
    }

    // The ripple through all four words, and the wrap at both ends.
    uint256 rolling_over {UINT64_C(0), UINT64_MAX, UINT64_MAX, UINT64_MAX};
    ++rolling_over;
    BOOST_TEST(rolling_over == (uint256{UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)} << 192U));

    uint256 at_max {uint256_max()};
    ++at_max;
    BOOST_TEST(at_max == uint256{});

    uint256 at_zero {};
    --at_zero;
    BOOST_TEST(at_zero == uint256_max());

    uint256 borrowing {UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    --borrowing;
    BOOST_TEST(borrowing == uint256(UINT64_C(0), UINT64_MAX, UINT64_MAX, UINT64_MAX));
}

// bool gets its own equality overloads so that MSVC does not warn about forcing
// a value to bool, and the constructor takes it as 0 or 1 rather than sign filling.
void test_bool_operands()
{
    const uint256 zero {};
    const uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    const uint256 big {UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(4)};

    BOOST_TEST(uint256{true} == one);
    BOOST_TEST(uint256{false} == zero);

    BOOST_TEST(one == true);
    BOOST_TEST(true == one);
    BOOST_TEST(zero == false);
    BOOST_TEST(false == zero);
    BOOST_TEST(big != true);
    BOOST_TEST(true != big);
    BOOST_TEST(zero != true);
    BOOST_TEST(true != zero);
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
        #endif
    >;

    boost::mp11::mp_for_each<test_types>(test_caller());

    test_unary_and_increment();
    test_bool_operands();

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE
