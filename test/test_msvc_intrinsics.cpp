// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The MSVC intrinsic paths of the multiply, divide and carry primitives, checked
// against the portable routines that the same headers use everywhere else.
//
// There is no MSVC on the development machine, so this file is normally empty and
// is exercised through Compiler Explorer instead ($SCRATCH/A3/godbolt_msvc.py
// posts an amalgamated source to the four MSVC configurations). It is a real test
// on the Windows CI jobs.

#include <boost/int256.hpp>
#include <boost/core/lightweight_test.hpp>

#if defined(_MSC_VER) && !defined(__GNUC__) && !defined(__clang__) && \
    (defined(_M_AMD64) || defined(_M_X64))

#include <boost/int256/detail/common_div.hpp>
#include <boost/int256/detail/common_mul.hpp>
#include <boost/int256/detail/common_add.hpp>

#include <cstdint>
#include <random>

using boost::int256::uint256;

namespace detail = boost::int256::detail;

namespace
{

std::mt19937_64 rng {42};

// umul takes _umul128 or __umulh on this target. umulh_generic is the portable
// four partial product routine the same header falls back to, so it is the
// reference the intrinsic has to match.
void test_umul()
{
    const std::uint64_t directed[] {
        UINT64_C(0), UINT64_C(1), UINT64_C(2), UINT64_MAX, UINT64_MAX - 1,
        UINT64_C(0x8000000000000000), UINT64_C(0xFFFFFFFF), UINT64_C(0x100000000),
        UINT64_C(0x0123456789ABCDEF)
    };

    for (const auto a : directed)
    {
        for (const auto b : directed)
        {
            std::uint64_t high {};
            const auto low {detail::umul(a, b, high)};

            BOOST_TEST_EQ(low, static_cast<std::uint64_t>(a * b));
            BOOST_TEST_EQ(high, detail::umulh_generic(a, b));
        }
    }

    for (int i {0}; i < 4096; ++i)
    {
        const auto a {rng()};
        const auto b {rng()};

        std::uint64_t high {};
        const auto low {detail::umul(a, b, high)};

        BOOST_TEST_EQ(low, static_cast<std::uint64_t>(a * b));
        BOOST_TEST_EQ(high, detail::umulh_generic(a, b));
    }
}

// udiv_2by1 takes _udiv128 on this target; divlu is the portable two digit
// routine in the same header. The precondition is u1 < d.
void test_udiv_2by1()
{
    const std::uint64_t divisors[] {
        UINT64_C(1), UINT64_C(2), UINT64_C(3), UINT64_C(10), UINT64_MAX,
        UINT64_C(0x8000000000000000), UINT64_C(0x7FFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFF)
    };

    for (const auto d : divisors)
    {
        const std::uint64_t highs[] {UINT64_C(0), d - 1U, d / 2U};

        for (const auto u1 : highs)
        {
            const std::uint64_t lows[] {UINT64_C(0), UINT64_MAX, UINT64_C(0x0123456789ABCDEF)};

            for (const auto u0 : lows)
            {
                std::uint64_t intrinsic_r {};
                const auto intrinsic_q {detail::udiv_2by1(u1, u0, d, intrinsic_r)};

                std::uint64_t portable_r {};
                const auto portable_q {detail::divlu(u1, u0, d, portable_r)};

                BOOST_TEST_EQ(intrinsic_q, portable_q);
                BOOST_TEST_EQ(intrinsic_r, portable_r);
            }
        }
    }

    for (int i {0}; i < 4096; ++i)
    {
        auto d {rng()};
        if (d == 0U)
        {
            d = 1U;
        }

        const auto u1 {rng() % d};
        const auto u0 {rng()};

        std::uint64_t intrinsic_r {};
        const auto intrinsic_q {detail::udiv_2by1(u1, u0, d, intrinsic_r)};

        std::uint64_t portable_r {};
        const auto portable_q {detail::divlu(u1, u0, d, portable_r)};

        BOOST_TEST_EQ(intrinsic_q, portable_q);
        BOOST_TEST_EQ(intrinsic_r, portable_r);
    }
}

// The addition and subtraction chains take _addcarry_u64 and _subborrow_u64 here.
// Constant evaluation always runs the portable chain, so a constexpr result is
// the reference for the runtime one.
void test_carry_chain()
{
    for (int i {0}; i < 4096; ++i)
    {
        uint256 a {};
        uint256 b {};
        for (int w {0}; w < 4; ++w)
        {
            a.words[w] = rng();
            b.words[w] = rng();
        }

        const auto sum {a + b};
        const auto difference {a - b};

        // The identities hold whichever path ran.
        BOOST_TEST_EQ(sum - b, a);
        BOOST_TEST_EQ(difference + b, a);
        BOOST_TEST_EQ(sum - a, b);
    }

    // A carry out of every word boundary.
    const uint256 all_ones {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    BOOST_TEST_EQ(all_ones + uint256{1U}, uint256{});
    BOOST_TEST_EQ(uint256{} - uint256{1U}, all_ones);

    constexpr uint256 low_ones {UINT64_C(0), UINT64_C(0), UINT64_MAX, UINT64_MAX};
    constexpr uint256 carried {low_ones + uint256{1U}};
    static_assert(carried == uint256(UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)),
                  "the portable chain carries into words[2]");
    BOOST_TEST_EQ(low_ones + uint256{1U}, carried);
}

// The whole division dispatcher, whose fast tiers are the intrinsic ones on this
// target, against constant evaluation of the same operands.
void test_division_against_constant_evaluation()
{
    constexpr uint256 dividend {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    constexpr uint256 one_word {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(0xFFFFFFFFFFFFFFFF)};
    constexpr uint256 two_word {UINT64_C(0), UINT64_C(0), UINT64_C(0x8349E2715252E7A5), UINT64_C(0xC2D4404073D1DA3A)};
    constexpr uint256 three_word {UINT64_C(0), UINT64_C(0x8AB084F6F9BD5AE6), UINT64_C(0xF7CA13BABA0A367B), UINT64_C(0xD8259701AEC6EAE9)};

    constexpr uint256 q1 {dividend / one_word};
    constexpr uint256 q2 {dividend / two_word};
    constexpr uint256 q3 {dividend / three_word};
    constexpr uint256 r1 {dividend % one_word};
    constexpr uint256 r2 {dividend % two_word};
    constexpr uint256 r3 {dividend % three_word};

    volatile std::uint64_t keep_alive {dividend.words[0]};
    static_cast<void>(keep_alive);

    const uint256 runtime_dividend {dividend.words[3], dividend.words[2], dividend.words[1], dividend.words[0]};

    BOOST_TEST_EQ(runtime_dividend / one_word, q1);
    BOOST_TEST_EQ(runtime_dividend / two_word, q2);
    BOOST_TEST_EQ(runtime_dividend / three_word, q3);
    BOOST_TEST_EQ(runtime_dividend % one_word, r1);
    BOOST_TEST_EQ(runtime_dividend % two_word, r2);
    BOOST_TEST_EQ(runtime_dividend % three_word, r3);
}

} // namespace

int main()
{
    test_umul();
    test_udiv_2by1();
    test_carry_chain();
    test_division_against_constant_evaluation();

    return boost::report_errors();
}

#else

// Not an MSVC x64 target, so there are no MSVC intrinsics to check here.
int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#endif
