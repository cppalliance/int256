// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The compound assignment operators that write back into a built-in integer:
// Integer& operator@=(Integer&, const uint256&). Each is specified as
// lhs = static_cast<Integer>(lhs op rhs), so the wide operation happens first
// and the result is then truncated to the narrow left hand side. The free
// operators themselves are checked against multiprecision in test_u256.cpp;
// what this file pins is the return type, the truncation, and the fact that a
// shift takes its value from the left operand and only its count from the right.

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
#include <limits>
#include <type_traits>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

template <typename T>
T draw()
{
    const auto bits {rng()};

    T value {};
    std::memcpy(&value, &bits, sizeof(T));

    return value;
}

bool report_compound(const char* what, const uint256& rhs)
{
    std::cerr << "int256 compound assignment mismatch in " << what << '\n'
              << "  rhs = " << hex_words(rhs) << std::endl;

    return false;
}

// Every one of the ten operators returns a reference to the left operand, so a
// chain like (i += u) += u compiles and both steps take effect.
template <typename T>
void test_return_types()
{
    static_assert(std::is_same<decltype(std::declval<T&>() += std::declval<const uint256&>()), T&>::value, "+=");
    static_assert(std::is_same<decltype(std::declval<T&>() -= std::declval<const uint256&>()), T&>::value, "-=");
    static_assert(std::is_same<decltype(std::declval<T&>() *= std::declval<const uint256&>()), T&>::value, "*=");
    static_assert(std::is_same<decltype(std::declval<T&>() /= std::declval<const uint256&>()), T&>::value, "/=");
    static_assert(std::is_same<decltype(std::declval<T&>() %= std::declval<const uint256&>()), T&>::value, "%=");
    static_assert(std::is_same<decltype(std::declval<T&>() &= std::declval<const uint256&>()), T&>::value, "&=");
    static_assert(std::is_same<decltype(std::declval<T&>() |= std::declval<const uint256&>()), T&>::value, "|=");
    static_assert(std::is_same<decltype(std::declval<T&>() ^= std::declval<const uint256&>()), T&>::value, "^=");
    static_assert(std::is_same<decltype(std::declval<T&>() <<= std::declval<const uint256&>()), T&>::value, "<<=");
    static_assert(std::is_same<decltype(std::declval<T&>() >>= std::declval<const uint256&>()), T&>::value, ">>=");
}

template <typename T>
void test_arithmetic_and_bitwise()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto original {draw<T>()};
        const uint256 rhs {random_uint256(rng, shape::word4)};

        {
            T lhs {original};
            lhs += rhs;
            BOOST_TEST(lhs == static_cast<T>(original + rhs) || report_compound("operator+=", rhs));
        }
        {
            T lhs {original};
            lhs -= rhs;
            BOOST_TEST(lhs == static_cast<T>(original - rhs) || report_compound("operator-=", rhs));
        }
        {
            T lhs {original};
            lhs *= rhs;
            BOOST_TEST(lhs == static_cast<T>(original * rhs) || report_compound("operator*=", rhs));
        }
        {
            T lhs {original};
            lhs &= rhs;
            BOOST_TEST(lhs == static_cast<T>(original & rhs) || report_compound("operator&=", rhs));
        }
        {
            T lhs {original};
            lhs |= rhs;
            BOOST_TEST(lhs == static_cast<T>(original | rhs) || report_compound("operator|=", rhs));
        }
        {
            T lhs {original};
            lhs ^= rhs;
            BOOST_TEST(lhs == static_cast<T>(original ^ rhs) || report_compound("operator^=", rhs));
        }
        if (rhs != 0U)
        {
            T lhs {original};
            lhs /= rhs;
            BOOST_TEST(lhs == static_cast<T>(original / rhs) || report_compound("operator/=", rhs));

            T other {original};
            other %= rhs;
            BOOST_TEST(other == static_cast<T>(original % rhs) || report_compound("operator%=", rhs));
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// The shift count is the right operand's low word. The value being shifted is the
// built-in left operand, promoted, so the count has to stay inside its width.
template <typename T>
void test_shifts()
{
    using promoted = typename boost::int256::detail::promoted<T>::type;
    constexpr auto promoted_bits {static_cast<unsigned>(sizeof(promoted) * 8U)};

    for (unsigned count {0}; count < promoted_bits; ++count)
    {
        const auto original {draw<T>()};
        const uint256 rhs {UINT64_C(0), UINT64_C(0), UINT64_C(0), count};

        {
            T lhs {original};
            lhs <<= rhs;
            BOOST_TEST(lhs == static_cast<T>(original << rhs) || report_compound("operator<<=", rhs));
        }
        {
            T lhs {original};
            lhs >>= rhs;
            BOOST_TEST(lhs == static_cast<T>(original >> rhs) || report_compound("operator>>=", rhs));
        }

        // The shifted value comes from the left operand alone, so the upper words
        // of the count are ignored.
        const uint256 noisy_count {UINT64_MAX, UINT64_MAX, UINT64_MAX, count};
        {
            T lhs {original};
            lhs <<= noisy_count;
            T expected {original};
            expected <<= rhs;
            BOOST_TEST(lhs == expected || report_compound("operator<<= ignores the upper words", noisy_count));
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// The truncation is a plain narrowing of the wide result, so a value that does
// not fit keeps only the low bits.
void test_directed_truncation()
{
    {
        // 2^64 truncates to zero in a 64-bit target.
        std::uint64_t lhs {0};
        lhs += uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0));
        BOOST_TEST_EQ(lhs, UINT64_C(0));
    }
    {
        // The maximum truncates to all ones in every unsigned target.
        std::uint64_t lhs {0};
        lhs |= uint256_max();
        BOOST_TEST_EQ(lhs, UINT64_MAX);

        unsigned short narrow {0};
        narrow |= uint256_max();
        BOOST_TEST_EQ(narrow, (std::numeric_limits<unsigned short>::max)());
    }
    {
        // Adding one to the maximum wraps to zero before the truncation.
        std::uint64_t lhs {1};
        lhs += uint256_max();
        BOOST_TEST_EQ(lhs, UINT64_C(0));
    }
    {
        // A count of 64 shifts a 64-bit value out entirely.
        std::uint32_t small {1};
        small <<= uint256{4U};
        BOOST_TEST_EQ(small, 16U);

        std::uint64_t wide {UINT64_C(1) << 63};
        wide >>= uint256{63U};
        BOOST_TEST_EQ(wide, UINT64_C(1));
    }
    {
        // Division by a value wider than the dividend gives zero.
        std::uint64_t lhs {UINT64_MAX};
        lhs /= uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0));
        BOOST_TEST_EQ(lhs, UINT64_C(0));

        std::uint64_t other {UINT64_MAX};
        other %= uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0));
        BOOST_TEST_EQ(other, UINT64_MAX);
    }
    {
        // The result is written back, so a chain sees the updated value.
        std::uint64_t lhs {1};
        (lhs += uint256{2U}) += uint256{3U};
        BOOST_TEST_EQ(lhs, UINT64_C(6));
    }
}

struct test_caller
{
    template <typename T>
    void operator()(T) const
    {
        test_return_types<T>();
        test_arithmetic_and_bitwise<T>();
        test_shifts<T>();
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

    test_directed_truncation();

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE
