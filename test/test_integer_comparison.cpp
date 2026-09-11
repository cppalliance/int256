// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The C++26 integer comparison functions, extended to uint256. These compare
// mathematical values rather than converted ones, so they answer the opposite
// way from the operators wherever a negative operand is involved:
// uint256{0} < -1 is true, but cmp_less(uint256{0}, -1) is false.

#include "oracle.hpp"

#ifdef BOOST_INT256_ORACLE_UNAVAILABLE

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <boost/int256/utilities.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/algorithm.hpp>

#include <cstring>
#include <limits>
#include <type_traits>

using boost::int256::uint256;
using boost::int256::cmp_equal;
using boost::int256::cmp_not_equal;
using boost::int256::cmp_less;
using boost::int256::cmp_greater;
using boost::int256::cmp_less_equal;
using boost::int256::cmp_greater_equal;
using boost::int256::in_range;

namespace
{

std::mt19937_64 rng {42};

template <typename T>
using is_signed_operand = std::integral_constant<bool, (static_cast<T>(-1) < static_cast<T>(0))>;

template <typename T>
T draw()
{
    const auto bits {rng()};

    T value {};
    std::memcpy(&value, &bits, sizeof(T));

    return value;
}

// The mathematical value of a builtin operand as a signed reference, so the
// expected answers do not go through the usual arithmetic conversions.
template <typename T>
bool reference_less(const uint256& lhs, T rhs, std::true_type)
{
    // A negative right hand side is below every unsigned value.
    if (rhs < 0)
    {
        return false;
    }

    return to_mp(lhs) < widen(static_cast<T>(rhs));
}

template <typename T>
bool reference_less(const uint256& lhs, T rhs, std::false_type)
{
    return to_mp(lhs) < widen(rhs);
}

template <typename T>
bool reference_equal(const uint256& lhs, T rhs, std::true_type)
{
    if (rhs < 0)
    {
        return false;
    }

    return to_mp(lhs) == widen(static_cast<T>(rhs));
}

template <typename T>
bool reference_equal(const uint256& lhs, T rhs, std::false_type)
{
    return to_mp(lhs) == widen(rhs);
}

template <typename T>
void test_against_reference()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto rhs {draw<T>()};
        const uint256 lhs {random_uint256(rng, shape::word4)};

        const bool expect_equal {reference_equal(lhs, rhs, is_signed_operand<T>{})};
        const bool expect_less {reference_less(lhs, rhs, is_signed_operand<T>{})};

        BOOST_TEST_EQ(cmp_equal(lhs, rhs), expect_equal);
        BOOST_TEST_EQ(cmp_not_equal(lhs, rhs), !expect_equal);
        BOOST_TEST_EQ(cmp_less(lhs, rhs), expect_less);
        BOOST_TEST_EQ(cmp_greater_equal(lhs, rhs), !expect_less);
        BOOST_TEST_EQ(cmp_greater(lhs, rhs), !expect_less && !expect_equal);
        BOOST_TEST_EQ(cmp_less_equal(lhs, rhs), expect_less || expect_equal);

        // Reversed operands must be consistent.
        BOOST_TEST_EQ(cmp_equal(rhs, lhs), expect_equal);
        BOOST_TEST_EQ(cmp_greater(rhs, lhs), expect_less);
        BOOST_TEST_EQ(cmp_less(rhs, lhs), !expect_less && !expect_equal);

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  lhs = " << hex_words(lhs) << std::endl;
            return;
        }
    }
}

// A small value of the target type always round trips; a wide one does not.
template <typename T>
void test_in_range()
{
    const uint256 zero {};
    const uint256 one {1U};

    BOOST_TEST(in_range<T>(zero));
    BOOST_TEST_EQ(in_range<T>(one), static_cast<T>(1) == 1);

    const uint256 target_max {static_cast<std::uint64_t>((std::numeric_limits<T>::max)())};
    BOOST_TEST(in_range<T>(target_max));

    // One past the target maximum never fits.
    BOOST_TEST(!in_range<T>(target_max + 1U));

    // Nor does anything with a word above the low one set.
    BOOST_TEST(!in_range<T>(uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0))));
    BOOST_TEST(!in_range<T>(uint256_max()));

    // A negative builtin never fits an unsigned uint256.
    BOOST_TEST_EQ(in_range<uint256>(static_cast<T>(-1)), !(static_cast<T>(-1) < static_cast<T>(0)));
}

// The answers the mathematical comparison must give where it differs from the
// operator, spelled out so a regression is unambiguous.
void test_pinned_answers()
{
    const uint256 zero {};
    const uint256 one {1U};
    const uint256 max {uint256_max()};

    // The operator converts, so these are true.
    BOOST_TEST(zero < -1);
    BOOST_TEST(max == -1);

    // The comparison functions do not convert, so these are false.
    BOOST_TEST(!cmp_less(zero, -1));
    BOOST_TEST(cmp_greater(zero, -1));
    BOOST_TEST(!cmp_equal(max, -1));
    BOOST_TEST(cmp_not_equal(max, -1));
    BOOST_TEST(!cmp_less(max, -1));
    BOOST_TEST(cmp_greater(max, -1));

    BOOST_TEST(cmp_equal(one, 1));
    BOOST_TEST(cmp_equal(one, 1U));
    BOOST_TEST(cmp_equal(1, one));
    BOOST_TEST(cmp_less(zero, 1));
    BOOST_TEST(cmp_less_equal(zero, 0));
    BOOST_TEST(cmp_greater_equal(zero, 0));

    // Every negative value is below every uint256, including zero.
    BOOST_TEST(cmp_less(INT64_MIN, zero));
    BOOST_TEST(cmp_greater(zero, INT64_MIN));
    BOOST_TEST(cmp_less(-1, zero));

    // And the maximum is above every builtin.
    BOOST_TEST(cmp_greater(max, (std::numeric_limits<std::uint64_t>::max)()));
    BOOST_TEST(cmp_greater(max, (std::numeric_limits<std::int64_t>::max)()));
    BOOST_TEST(cmp_less((std::numeric_limits<std::uint64_t>::max)(), max));

    // These are constant expressions.
    static_assert(!cmp_less(uint256{}, -1), "cmp_less(0, -1) is false");
    static_assert(cmp_greater(uint256{}, -1), "cmp_greater(0, -1) is true");
    static_assert(cmp_equal(uint256{1U}, 1), "cmp_equal(1, 1)");
    static_assert(in_range<int>(uint256{1U}), "1 fits an int");
    static_assert(!in_range<int>(uint256(UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0))),
                  "2^192 does not fit an int");
}

// The comparison functions reject the character types, bool and std::byte, per
// the standard note. compile_tests/utilities_comparison_fail.cpp checks that the
// rejection is a hard error; here it only has to be visible to SFINAE.
template <typename T, typename U>
struct is_comparable
{
private:

    template <typename A, typename B>
    static auto probe(int) -> decltype(cmp_equal(std::declval<A>(), std::declval<B>()), std::true_type{});

    template <typename, typename>
    static std::false_type probe(...);

public:

    static constexpr bool value {decltype(probe<T, U>(0))::value};
};

void test_constraints()
{
    static_assert(is_comparable<uint256, int>::value, "int is comparable");
    static_assert(is_comparable<uint256, unsigned>::value, "unsigned is comparable");
    static_assert(is_comparable<uint256, long long>::value, "long long is comparable");
    static_assert(is_comparable<uint256, uint256>::value, "uint256 is comparable with itself");
    static_assert(is_comparable<int, uint256>::value, "the reversed order works too");

    static_assert(!is_comparable<uint256, char>::value, "char is rejected");
    static_assert(!is_comparable<uint256, bool>::value, "bool is rejected");
    static_assert(!is_comparable<uint256, char16_t>::value, "char16_t is rejected");
    static_assert(!is_comparable<uint256, char32_t>::value, "char32_t is rejected");
    static_assert(!is_comparable<uint256, wchar_t>::value, "wchar_t is rejected");

    // At least one operand has to be a wide type; the plain builtins are the
    // standard's own function templates, not these.
    static_assert(!is_comparable<int, long>::value, "two builtins are not this overload set");
}

struct test_caller
{
    template <typename T>
    void operator()(T) const
    {
        test_against_reference<T>();
        test_in_range<T>();
    }
};

} // namespace

int main()
{
    using test_types = boost::mp11::mp_list<
        signed char,
        unsigned char,
        short,
        unsigned short,
        int,
        unsigned int,
        long,
        unsigned long,
        long long,
        unsigned long long
    >;

    boost::mp11::mp_for_each<test_types>(test_caller());

    test_pinned_answers();
    test_constraints();

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE
