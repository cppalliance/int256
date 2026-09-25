// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// A signed operand meeting uint256 goes through the usual arithmetic
// conversions, so a negative value becomes 2^256 minus its magnitude and every
// comparison follows from that. uint256{0} < -1 is therefore true, exactly as
// 0U < -1 is true for the builtins. That surprise is pinned here so nobody
// "fixes" it into a signed comparison.

#include "oracle.hpp"

#ifdef BOOST_INT256_ORACLE_UNAVAILABLE

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <cstdint>
#include <limits>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

void test_pinned_semantics()
{
    const uint256 zero {};
    const uint256 max {uint256_max()};

    // The builtin behavior, spelled out.
    BOOST_TEST(zero < -1);
    BOOST_TEST(!(zero > -1));
    BOOST_TEST(zero != -1);
    BOOST_TEST(max == -1);
    BOOST_TEST(-1 == max);
    BOOST_TEST(!(max < -1));
    BOOST_TEST(max <= -1);
    BOOST_TEST(max >= -1);

    // A small positive value compares below a negative one.
    const uint256 forty_two {42U};
    BOOST_TEST(forty_two < -42);
    BOOST_TEST(!(forty_two > -42));
    BOOST_TEST(forty_two != -42);
    BOOST_TEST(-42 > forty_two);

    // And the corresponding uint256 built from the same negative scalar.
    BOOST_TEST(uint256{-42} > forty_two);
    BOOST_TEST(uint256{-42} == -42);
}

void test_sign_extension_into_every_word()
{
    const std::int64_t negatives[] {
        INT64_C(-1), INT64_C(-2), INT64_C(-42), INT64_C(-1000003),
        (std::numeric_limits<std::int64_t>::min)(), (std::numeric_limits<std::int64_t>::min)() + 1,
        (std::numeric_limits<std::int64_t>::min)() + 7
    };

    for (const auto v : negatives)
    {
        const uint256 x {v};

        // Words 1 through 3 are the sign fill.
        BOOST_TEST_EQ(x.words[3], UINT64_MAX);
        BOOST_TEST_EQ(x.words[2], UINT64_MAX);
        BOOST_TEST_EQ(x.words[1], UINT64_MAX);
        BOOST_TEST_EQ(x.words[0], static_cast<std::uint64_t>(v));

        // Which is the same value the reference gives.
        BOOST_TEST(to_mp(x) == widen(v) || report_mismatch("sign extension", x, x, widen(v)));
    }

    // A positive signed value leaves the upper words clear.
    const std::int64_t positives[] {INT64_C(0), INT64_C(1), INT64_C(42), INT64_MAX};
    for (const auto v : positives)
    {
        const uint256 x {v};
        BOOST_TEST_EQ(x.words[3], UINT64_C(0));
        BOOST_TEST_EQ(x.words[2], UINT64_C(0));
        BOOST_TEST_EQ(x.words[1], UINT64_C(0));
        BOOST_TEST_EQ(x.words[0], static_cast<std::uint64_t>(v));
    }
}

// The narrow signed types sign fill the same way, so the conversion is not
// silently going through an unsigned intermediate.
void test_narrow_signed_operands()
{
    BOOST_TEST(uint256{static_cast<signed char>(-1)} == uint256_max());
    BOOST_TEST(uint256{static_cast<short>(-1)} == uint256_max());
    BOOST_TEST(uint256{-1} == uint256_max());
    BOOST_TEST(uint256{-1L} == uint256_max());
    BOOST_TEST(uint256{-1LL} == uint256_max());

    BOOST_TEST(uint256{static_cast<signed char>(-128)} ==
               (uint256_max() - uint256{127U}));
    BOOST_TEST(uint256{(std::numeric_limits<short>::min)()} ==
               (uint256_max() - uint256{static_cast<unsigned>((std::numeric_limits<short>::max)())}));
}

void test_random_mixed_comparisons()
{
    std::uniform_int_distribution<std::int64_t> signed_dist {
        (std::numeric_limits<std::int64_t>::min)(), (std::numeric_limits<std::int64_t>::max)()};

    for (std::size_t i {0}; i < N; ++i)
    {
        const auto rhs {signed_dist(rng)};
        const uint256 lhs {random_uint256(rng, shape::word4)};

        const mp mlhs {to_mp(lhs)};
        const mp mrhs {widen(rhs)};

        BOOST_TEST_EQ(lhs == rhs, mlhs == mrhs);
        BOOST_TEST_EQ(lhs != rhs, mlhs != mrhs);
        BOOST_TEST_EQ(lhs <  rhs, mlhs <  mrhs);
        BOOST_TEST_EQ(lhs <= rhs, mlhs <= mrhs);
        BOOST_TEST_EQ(lhs >  rhs, mlhs >  mrhs);
        BOOST_TEST_EQ(lhs >= rhs, mlhs >= mrhs);

        BOOST_TEST_EQ(rhs == lhs, mrhs == mlhs);
        BOOST_TEST_EQ(rhs != lhs, mrhs != mlhs);
        BOOST_TEST_EQ(rhs <  lhs, mrhs <  mlhs);
        BOOST_TEST_EQ(rhs <= lhs, mrhs <= mlhs);
        BOOST_TEST_EQ(rhs >  lhs, mrhs >  mlhs);
        BOOST_TEST_EQ(rhs >= lhs, mrhs >= mlhs);

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  lhs = " << hex_words(lhs) << ", rhs = " << rhs << std::endl;
            return;
        }
    }
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

// int256 meeting uint256 also goes through the usual arithmetic conversions:
// both become uint256, so int256{-1} < uint256{1} is false, exactly like
// int128{-1} < uint128{1}.
void test_int256_uint256_pinned_semantics()
{
    const int256 minus_one {-1};
    const uint256 one_u {1U};

    BOOST_TEST(!(minus_one < one_u));
    BOOST_TEST(minus_one > one_u);
    BOOST_TEST(minus_one != one_u);
    BOOST_TEST(one_u < minus_one);

    const uint256 max_u {uint256_max()};
    BOOST_TEST(minus_one == max_u);
    BOOST_TEST(max_u == minus_one);
}

void test_int256_uint256_random_comparisons()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const int256 lhs {random_int256(rng, shape::word4, sign::mixed)};
        const uint256 rhs {random_uint256(rng, shape::word4)};

        const mp mlhs {to_mp(to_unsigned_bits(lhs))};
        const mp mrhs {to_mp(rhs)};

        BOOST_TEST_EQ(lhs == rhs, mlhs == mrhs);
        BOOST_TEST_EQ(lhs != rhs, mlhs != mrhs);
        BOOST_TEST_EQ(lhs <  rhs, mlhs <  mrhs);
        BOOST_TEST_EQ(lhs <= rhs, mlhs <= mrhs);
        BOOST_TEST_EQ(lhs >  rhs, mlhs >  mrhs);
        BOOST_TEST_EQ(lhs >= rhs, mlhs >= mrhs);

        BOOST_TEST_EQ(rhs == lhs, mrhs == mlhs);
        BOOST_TEST_EQ(rhs != lhs, mrhs != mlhs);
        BOOST_TEST_EQ(rhs <  lhs, mrhs <  mlhs);
        BOOST_TEST_EQ(rhs <= lhs, mrhs <= mlhs);
        BOOST_TEST_EQ(rhs >  lhs, mrhs >  mlhs);
        BOOST_TEST_EQ(rhs >= lhs, mrhs >= mlhs);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_pinned_semantics();
    test_sign_extension_into_every_word();
    test_narrow_signed_operands();
    test_random_mixed_comparisons();

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_int256_uint256_pinned_semantics();
    test_int256_uint256_random_comparisons();
#endif

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE
