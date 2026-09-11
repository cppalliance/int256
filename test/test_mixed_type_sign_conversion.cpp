// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Arithmetic between uint256 and a signed builtin. The signed operand converts
// to the 256-bit value the usual arithmetic conversions give it, so a negative
// scalar sign fills into all four words and the result follows from that. The
// library reaches the same answer without materializing the widened operand
// (it subtracts the magnitude instead), which is what this test pins.

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

bool report_signed(const char* what, const uint256& lhs, std::int64_t rhs,
                   const mp& got, const mp& expected)
{
    std::cerr << "int256 mixed sign mismatch in " << what << '\n'
              << "  lhs      = " << hex_words(lhs) << '\n'
              << "  rhs      = " << rhs << " (" << hex_words(widen(rhs)) << ")\n"
              << "  got      = " << hex_words(got) << '\n'
              << "  expected = " << hex_words(expected) << std::endl;

    return false;
}

#define CHECK_SIGNED(what, lhs, rhs, got, expected) \
    BOOST_TEST(to_mp(got) == (expected) || report_signed(what, lhs, rhs, to_mp(got), expected))

void check_one(const uint256& x, std::int64_t v)
{
    const mp mx {to_mp(x)};
    const mp mv {widen(v)};
    const uint256 v256 {v};

    {
        const uint256 got {x + v};
        CHECK_SIGNED("operator+(uint256, Signed)", x, v, got, mp{mx + mv});
        BOOST_TEST_EQ(got, x + v256);
    }
    {
        const uint256 got {v + x};
        CHECK_SIGNED("operator+(Signed, uint256)", x, v, got, mp{mv + mx});
        BOOST_TEST_EQ(got, v256 + x);
    }
    {
        const uint256 got {x - v};
        CHECK_SIGNED("operator-(uint256, Signed)", x, v, got, mp{mx - mv});
        BOOST_TEST_EQ(got, x - v256);
    }
    {
        const uint256 got {v - x};
        CHECK_SIGNED("operator-(Signed, uint256)", x, v, got, mp{mv - mx});
        BOOST_TEST_EQ(got, v256 - x);
    }
    {
        const uint256 got {x * v};
        CHECK_SIGNED("operator*(uint256, Signed)", x, v, got, mp{mx * mv});
        BOOST_TEST_EQ(got, x * v256);
    }
    {
        const uint256 got {v * x};
        CHECK_SIGNED("operator*(Signed, uint256)", x, v, got, mp{mv * mx});
        BOOST_TEST_EQ(got, v256 * x);
    }
    if (v != 0)
    {
        const uint256 quotient {x / v};
        CHECK_SIGNED("operator/(uint256, Signed)", x, v, quotient, mp{mx / mv});
        BOOST_TEST_EQ(quotient, x / v256);

        const uint256 remainder {x % v};
        CHECK_SIGNED("operator%(uint256, Signed)", x, v, remainder, mp{mx % mv});
        BOOST_TEST_EQ(remainder, x % v256);
    }
    if (x != 0U)
    {
        const uint256 quotient {v / x};
        CHECK_SIGNED("operator/(Signed, uint256)", x, v, quotient, mp{mv / mx});
        BOOST_TEST_EQ(quotient, v256 / x);

        const uint256 remainder {v % x};
        CHECK_SIGNED("operator%(Signed, uint256)", x, v, remainder, mp{mv % mx});
        BOOST_TEST_EQ(remainder, v256 % x);
    }
    {
        const uint256 got {x & v};
        CHECK_SIGNED("operator&(uint256, Signed)", x, v, got, mp{mx & mv});
    }
    {
        const uint256 got {x | v};
        CHECK_SIGNED("operator|(uint256, Signed)", x, v, got, mp{mx | mv});
    }
    {
        const uint256 got {x ^ v};
        CHECK_SIGNED("operator^(uint256, Signed)", x, v, got, mp{mx ^ mv});
    }
}

// The values whose magnitude cannot be negated inside their own type, plus a
// spread of ordinary ones.
void test_directed()
{
    const std::int64_t scalars[] {
        (std::numeric_limits<std::int64_t>::min)(),
        (std::numeric_limits<std::int64_t>::min)() + 1,
        INT64_C(-1000003), INT64_C(-42), INT64_C(-2), INT64_C(-1),
        INT64_C(0), INT64_C(1), INT64_C(2), INT64_C(42),
        (std::numeric_limits<std::int64_t>::max)() - 1,
        (std::numeric_limits<std::int64_t>::max)()
    };

    for (const auto& x : edge_values())
    {
        for (const auto v : scalars)
        {
            check_one(x, v);

            if (boost::detail::test_errors() != 0)
            {
                return;
            }
        }
    }
}

void test_random()
{
    std::uniform_int_distribution<std::int64_t> dist {
        (std::numeric_limits<std::int64_t>::min)(), (std::numeric_limits<std::int64_t>::max)()};

    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 x {random_uint256(rng, shape::word4)};
        check_one(x, dist(rng));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

} // namespace

int main()
{
    test_directed();
    test_random();

    return boost::report_errors();
}

#endif // BOOST_INT256_ORACLE_UNAVAILABLE
