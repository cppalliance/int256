// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The builtin 128-bit flavors of every operator, in both operand orders. These
// exist so that uint256 drops into code that already uses __int128, and each one
// forwards to the corresponding uint256 overload after converting. The signed
// builtin sign fills, so a negative one becomes 2^256 minus its magnitude, the
// same as any other signed operand.

#include "oracle.hpp"

#if defined(BOOST_INT256_ORACLE_UNAVAILABLE) || \
    !(defined(BOOST_INT256_HAS_INT128) || defined(BOOST_INT256_HAS_MSVC_INT128))

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <type_traits>

using boost::int256::uint256;
using boost::int256::detail::builtin_i128;
using boost::int256::detail::builtin_u128;

namespace
{

std::mt19937_64 rng {42};

builtin_u128 draw_unsigned()
{
    const auto hi {rng()};
    const auto lo {rng()};

    return (static_cast<builtin_u128>(hi) << 64) | static_cast<builtin_u128>(lo);
}

builtin_i128 draw_signed()
{
    return static_cast<builtin_i128>(draw_unsigned());
}

template <typename Builtin>
bool report_builtin(const char* what, const uint256& lhs, Builtin rhs,
                    const mp& got, const mp& expected)
{
    std::cerr << "int256 builtin 128 parity mismatch in " << what << '\n'
              << "  uint256 operand = " << hex_words(lhs) << '\n'
              << "  builtin operand = " << hex_words(widen(rhs)) << '\n'
              << "  got             = " << hex_words(got) << '\n'
              << "  expected        = " << hex_words(expected) << std::endl;

    return false;
}

#define CHECK_BUILTIN(what, lhs, rhs, got, expected) \
    BOOST_TEST(to_mp(got) == (expected) || report_builtin(what, lhs, rhs, to_mp(got), expected))

// Every binary operator returns uint256 for the four builtin flavors, and the
// comparisons return bool.
void test_return_types()
{
    static_assert(std::is_same<decltype(uint256{} + builtin_i128{}), uint256>::value, "uint256 + i128");
    static_assert(std::is_same<decltype(builtin_i128{} + uint256{}), uint256>::value, "i128 + uint256");
    static_assert(std::is_same<decltype(uint256{} + builtin_u128{}), uint256>::value, "uint256 + u128");
    static_assert(std::is_same<decltype(builtin_u128{} + uint256{}), uint256>::value, "u128 + uint256");

    static_assert(std::is_same<decltype(uint256{} == builtin_u128{}), bool>::value, "uint256 == u128");
    static_assert(std::is_same<decltype(builtin_u128{} < uint256{}), bool>::value, "u128 < uint256");
    static_assert(std::is_same<decltype(uint256{} != builtin_i128{}), bool>::value, "uint256 != i128");

    // A shift with the builtin on the left keeps the builtin's type.
    static_assert(std::is_same<decltype(builtin_u128{} << uint256{}), builtin_u128>::value, "u128 << uint256");
    static_assert(std::is_same<decltype(builtin_i128{} << uint256{}), builtin_i128>::value, "i128 << uint256");
}

template <typename Builtin>
void check_one(const uint256& x, Builtin v)
{
    const mp mx {to_mp(x)};
    const mp mv {widen(v)};

    // Comparisons, both orders.
    BOOST_TEST_EQ(x == v, mx == mv);
    BOOST_TEST_EQ(v == x, mv == mx);
    BOOST_TEST_EQ(x != v, mx != mv);
    BOOST_TEST_EQ(v != x, mv != mx);
    BOOST_TEST_EQ(x <  v, mx <  mv);
    BOOST_TEST_EQ(v <  x, mv <  mx);
    BOOST_TEST_EQ(x <= v, mx <= mv);
    BOOST_TEST_EQ(v <= x, mv <= mx);
    BOOST_TEST_EQ(x >  v, mx >  mv);
    BOOST_TEST_EQ(v >  x, mv >  mx);
    BOOST_TEST_EQ(x >= v, mx >= mv);
    BOOST_TEST_EQ(v >= x, mv >= mx);

    // Arithmetic and bitwise, both orders.
    {
        const uint256 got {x + v};
        CHECK_BUILTIN("operator+(uint256, builtin)", x, v, got, mp{mx + mv});
    }
    {
        const uint256 got {v + x};
        CHECK_BUILTIN("operator+(builtin, uint256)", x, v, got, mp{mv + mx});
    }
    {
        const uint256 got {x - v};
        CHECK_BUILTIN("operator-(uint256, builtin)", x, v, got, mp{mx - mv});
    }
    {
        const uint256 got {v - x};
        CHECK_BUILTIN("operator-(builtin, uint256)", x, v, got, mp{mv - mx});
    }
    {
        const uint256 got {x * v};
        CHECK_BUILTIN("operator*(uint256, builtin)", x, v, got, mp{mx * mv});
    }
    {
        const uint256 got {v * x};
        CHECK_BUILTIN("operator*(builtin, uint256)", x, v, got, mp{mv * mx});
    }
    if (mv != 0)
    {
        const uint256 quotient {x / v};
        CHECK_BUILTIN("operator/(uint256, builtin)", x, v, quotient, mp{mx / mv});

        const uint256 remainder {x % v};
        CHECK_BUILTIN("operator%(uint256, builtin)", x, v, remainder, mp{mx % mv});
    }
    if (mx != 0)
    {
        const uint256 quotient {v / x};
        CHECK_BUILTIN("operator/(builtin, uint256)", x, v, quotient, mp{mv / mx});

        const uint256 remainder {v % x};
        CHECK_BUILTIN("operator%(builtin, uint256)", x, v, remainder, mp{mv % mx});
    }
    {
        const uint256 got {x & v};
        CHECK_BUILTIN("operator&(uint256, builtin)", x, v, got, mp{mx & mv});
    }
    {
        const uint256 got {v & x};
        CHECK_BUILTIN("operator&(builtin, uint256)", x, v, got, mp{mv & mx});
    }
    {
        const uint256 got {x | v};
        CHECK_BUILTIN("operator|(uint256, builtin)", x, v, got, mp{mx | mv});
    }
    {
        const uint256 got {v | x};
        CHECK_BUILTIN("operator|(builtin, uint256)", x, v, got, mp{mv | mx});
    }
    {
        const uint256 got {x ^ v};
        CHECK_BUILTIN("operator^(uint256, builtin)", x, v, got, mp{mx ^ mv});
    }
    {
        const uint256 got {v ^ x};
        CHECK_BUILTIN("operator^(builtin, uint256)", x, v, got, mp{mv ^ mx});
    }

    // Compound assignment onto uint256 with a builtin right hand side.
    {
        uint256 t {x};
        t += v;
        CHECK_BUILTIN("operator+=(builtin)", x, v, t, mp{mx + mv});
    }
    {
        uint256 t {x};
        t *= v;
        CHECK_BUILTIN("operator*=(builtin)", x, v, t, mp{mx * mv});
    }
    if (mv != 0)
    {
        uint256 t {x};
        t /= v;
        CHECK_BUILTIN("operator/=(builtin)", x, v, t, mp{mx / mv});
    }
}

// A shift with the builtin on the left shifts the builtin value by the low word
// of the count and keeps the builtin's type, so plain builtin arithmetic is the
// reference.
void test_builtin_left_shifts()
{
    for (unsigned count {0}; count < 128U; ++count)
    {
        const uint256 rhs {UINT64_C(0), UINT64_C(0), UINT64_C(0), count};

        const builtin_u128 unsigned_value {draw_unsigned()};
        BOOST_TEST((unsigned_value << rhs) == static_cast<builtin_u128>(unsigned_value << count));
        BOOST_TEST((unsigned_value >> rhs) == static_cast<builtin_u128>(unsigned_value >> count));

        // Keep the signed value non-negative so the shift stays well defined.
        const builtin_i128 signed_value {static_cast<builtin_i128>(draw_unsigned() >> 1) >> count};
        BOOST_TEST((signed_value << rhs) == static_cast<builtin_i128>(signed_value << count));
        BOOST_TEST((signed_value >> rhs) == static_cast<builtin_i128>(signed_value >> count));
    }
}

// The builtin constructors and conversion operators move the low 128 bits.
void test_round_trips()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const builtin_u128 value {draw_unsigned()};

        const uint256 wide {value};
        BOOST_TEST_EQ(wide.words[0], static_cast<std::uint64_t>(value));
        BOOST_TEST_EQ(wide.words[1], static_cast<std::uint64_t>(value >> 64));
        BOOST_TEST_EQ(wide.words[2], UINT64_C(0));
        BOOST_TEST_EQ(wide.words[3], UINT64_C(0));
        BOOST_TEST(static_cast<builtin_u128>(wide) == value);

        const builtin_i128 as_signed {static_cast<builtin_i128>(value)};
        const uint256 from_signed {as_signed};
        BOOST_TEST_EQ(from_signed.words[0], static_cast<std::uint64_t>(value));
        BOOST_TEST_EQ(from_signed.words[1], static_cast<std::uint64_t>(value >> 64));

        // The sign of the builtin decides whether the upper words fill.
        const auto fill {as_signed < 0 ? UINT64_MAX : UINT64_C(0)};
        BOOST_TEST_EQ(from_signed.words[2], fill);
        BOOST_TEST_EQ(from_signed.words[3], fill);
        BOOST_TEST(static_cast<builtin_i128>(from_signed) == as_signed);

        // A value wider than 128 bits truncates on the way out.
        const uint256 wider {random_uint256(rng, shape::word4)};
        const builtin_u128 truncated {static_cast<builtin_u128>(wider)};
        BOOST_TEST(truncated == ((static_cast<builtin_u128>(wider.words[1]) << 64) |
                                 static_cast<builtin_u128>(wider.words[0])));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

void test_random_pairs()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const uint256 x {random_uint256(rng, shape::word4)};

        check_one(x, draw_unsigned());
        check_one(x, draw_signed());

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  uint256 operand = " << hex_words(x) << std::endl;
            return;
        }
    }
}

void test_edge_pairs()
{
    const builtin_u128 unsigned_edges[] {
        static_cast<builtin_u128>(0),
        static_cast<builtin_u128>(1),
        static_cast<builtin_u128>(UINT64_MAX),
        static_cast<builtin_u128>(UINT64_MAX) + 1,
        static_cast<builtin_u128>(1) << 127,
        ~static_cast<builtin_u128>(0)
    };

    const builtin_i128 signed_edges[] {
        static_cast<builtin_i128>(0),
        static_cast<builtin_i128>(1),
        static_cast<builtin_i128>(-1),
        static_cast<builtin_i128>(UINT64_MAX),
        -static_cast<builtin_i128>(UINT64_MAX),
        static_cast<builtin_i128>((static_cast<builtin_u128>(1) << 127) - 1),
        static_cast<builtin_i128>(static_cast<builtin_u128>(1) << 127)
    };

    for (const auto& x : edge_values())
    {
        for (const auto v : unsigned_edges)
        {
            check_one(x, v);
        }

        for (const auto v : signed_edges)
        {
            check_one(x, v);
        }

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  uint256 operand = " << hex_words(x) << std::endl;
            return;
        }
    }
}

} // namespace

int main()
{
    test_return_types();
    test_round_trips();
    test_builtin_left_shifts();
    test_edge_pairs();
    test_random_pairs();

    return boost::report_errors();
}

#endif
