// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Interoperability with Boost.Int128's uint128 and int128, which is enabled because
// <boost/int128.hpp> is included before <boost/int256.hpp> below. Every operation must
// give the same answer as first converting the Boost.Int128 operand to the 256-bit type
// by value (sign filling an int128), which is the usual arithmetic conversion a builtin
// 128-bit operand gets. Where the compiler has a builtin 128-bit type, each result is
// also checked against the existing builtin flavor of the same operation.

#if defined(__has_include)
#  if __has_include(<boost/int128.hpp>)
#    define BOOST_INT256_TEST_HAS_INT128_LIBRARY
#  endif
#endif

#ifdef BOOST_INT256_TEST_HAS_INT128_LIBRARY

#include <boost/int128.hpp>
#include <boost/int256.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <type_traits>

using boost::int256::uint256;
using boost::int256::int256;
using boost::int128::uint128;
using boost::int128::int128;

namespace {

constexpr std::size_t N {1024};

std::mt19937_64 rng {42};

// Values with 1, 2, 3 or 4 significant words, so every word boundary is crossed
std::uint64_t draw_word(const unsigned index, const unsigned used)
{
    return index < used ? rng() : UINT64_C(0);
}

template <typename T>
T draw_128()
{
    const auto used {static_cast<unsigned>(rng() % 3U)};
    const auto hi {draw_word(1U, used)};
    const auto lo {draw_word(0U, used)};

    return boost::int256::detail::int128_interop<T>::from_words(hi, lo);
}

template <typename W>
W draw_256()
{
    const auto used {static_cast<unsigned>(rng() % 5U)};
    const uint256 bits {draw_word(3U, used), draw_word(2U, used), draw_word(1U, used), draw_word(0U, used)};

    // Negate half of the values so both signs appear at every width
    return static_cast<W>((rng() & 1U) != 0U ? -bits : bits);
}

// The oracle: the value of a Boost.Int128 operand as a 256-bit value, built from its
// words directly rather than through the converting constructor under test
template <typename W, typename T>
W widen(const T v)
{
    const auto negative {std::is_same<T, int128>::value && (v.high >> 63U) != 0U};
    const auto fill {negative ? UINT64_MAX : UINT64_C(0)};

    return static_cast<W>(uint256{fill, fill, v.high, v.low});
}

// The low 128 bits of a 256-bit value, again without the conversion under test
template <typename T, typename W>
T narrow(const W& w)
{
    return boost::int256::detail::int128_interop<T>::from_words(w.words[1], w.words[0]);
}

template <typename W, typename T>
bool division_is_defined(const W& lhs, const W& rhs)
{
    if (rhs == 0)
    {
        return false;
    }

    // int256 min / -1 overflows, exactly as it does for the builtin signed types
    return !(std::is_same<W, int256>::value && lhs == (std::numeric_limits<W>::min)() && rhs == -1);
}

template <typename W, typename T>
void check_conversions(const W& x, const T v)
{
    // To the 256-bit type: direct, copy and list initialization
    const W direct (v);
    const W copied = v;
    const W listed {v};
    BOOST_TEST(direct == widen<W>(v));
    BOOST_TEST(copied == widen<W>(v));
    BOOST_TEST(listed == widen<W>(v));

    W assigned {};
    assigned = v;
    BOOST_TEST(assigned == widen<W>(v));

    // Back to the Boost.Int128 type keeps the low 128 bits. Copy initialization is the
    // portable spelling; see the documentation for static_cast
    const T back = x;
    BOOST_TEST(back == narrow<T>(x));

    T reassigned {};
    reassigned = x;
    BOOST_TEST(reassigned == narrow<T>(x));

    // A value that fits round trips exactly
    const T round_trip = direct;
    BOOST_TEST(round_trip == v);

    #if !defined(_MSC_VER) && (__cplusplus >= 201703L)
    BOOST_TEST(static_cast<T>(x) == narrow<T>(x));
    #endif
}

template <typename W, typename T>
void check_comparisons(const W& x, const T v)
{
    const W wv {widen<W>(v)};

    BOOST_TEST_EQ(x == v, x == wv);
    BOOST_TEST_EQ(v == x, wv == x);
    BOOST_TEST_EQ(x != v, x != wv);
    BOOST_TEST_EQ(v != x, wv != x);
    BOOST_TEST_EQ(x < v, x < wv);
    BOOST_TEST_EQ(v < x, wv < x);
    BOOST_TEST_EQ(x <= v, x <= wv);
    BOOST_TEST_EQ(v <= x, wv <= x);
    BOOST_TEST_EQ(x > v, x > wv);
    BOOST_TEST_EQ(v > x, wv > x);
    BOOST_TEST_EQ(x >= v, x >= wv);
    BOOST_TEST_EQ(v >= x, wv >= x);

    #ifdef BOOST_INT256_HAS_SPACESHIP_OPERATOR
    BOOST_TEST((x <=> v) == (x <=> wv));
    BOOST_TEST((v <=> x) == (wv <=> x));
    #endif
}

template <typename W, typename T>
void check_arithmetic(const W& x, const T v)
{
    const W wv {widen<W>(v)};

    static_assert(std::is_same<decltype(x + v), W>::value, "The result has the 256-bit type");
    static_assert(std::is_same<decltype(v + x), W>::value, "The result has the 256-bit type");

    BOOST_TEST(x + v == x + wv);
    BOOST_TEST(v + x == wv + x);
    BOOST_TEST(x - v == x - wv);
    BOOST_TEST(v - x == wv - x);
    BOOST_TEST(x * v == x * wv);
    BOOST_TEST(v * x == wv * x);
    BOOST_TEST((x & v) == (x & wv));
    BOOST_TEST((v & x) == (wv & x));
    BOOST_TEST((x | v) == (x | wv));
    BOOST_TEST((v | x) == (wv | x));
    BOOST_TEST((x ^ v) == (x ^ wv));
    BOOST_TEST((v ^ x) == (wv ^ x));

    if (division_is_defined<W, T>(x, wv))
    {
        BOOST_TEST(x / v == x / wv);
        BOOST_TEST(x % v == x % wv);
    }

    if (division_is_defined<W, T>(wv, x))
    {
        BOOST_TEST(v / x == wv / x);
        BOOST_TEST(v % x == wv % x);
    }
}

template <typename W, typename T>
void check_compound(const W& x, const T v)
{
    const W wv {widen<W>(v)};

    // The 256-bit type on the left
    {
        W t {x};
        t += v;
        BOOST_TEST(t == x + wv);
        t = x;
        t -= v;
        BOOST_TEST(t == x - wv);
        t = x;
        t *= v;
        BOOST_TEST(t == x * wv);
        t = x;
        t &= v;
        BOOST_TEST(t == (x & wv));
        t = x;
        t |= v;
        BOOST_TEST(t == (x | wv));
        t = x;
        t ^= v;
        BOOST_TEST(t == (x ^ wv));

        if (division_is_defined<W, T>(x, wv))
        {
            t = x;
            t /= v;
            BOOST_TEST(t == x / wv);
            t = x;
            t %= v;
            BOOST_TEST(t == x % wv);
        }
    }

    // The Boost.Int128 type on the left keeps the low 128 bits of the result
    {
        T t {v};
        t += x;
        BOOST_TEST(t == narrow<T>(wv + x));
        t = v;
        t -= x;
        BOOST_TEST(t == narrow<T>(wv - x));
        t = v;
        t *= x;
        BOOST_TEST(t == narrow<T>(wv * x));
        t = v;
        t &= x;
        BOOST_TEST(t == narrow<T>(wv & x));
        t = v;
        t |= x;
        BOOST_TEST(t == narrow<T>(wv | x));
        t = v;
        t ^= x;
        BOOST_TEST(t == narrow<T>(wv ^ x));

        if (division_is_defined<W, T>(wv, x))
        {
            t = v;
            t /= x;
            BOOST_TEST(t == narrow<T>(wv / x));
            t = v;
            t %= x;
            BOOST_TEST(t == narrow<T>(wv % x));
        }
    }
}

template <typename W, typename T>
void check_shifts(const W& x, const T v)
{
    for (unsigned count {0U}; count < 256U; count += 1U + static_cast<unsigned>(rng() % 7U))
    {
        const T n {count};
        BOOST_TEST((x << n) == (x << count));
        BOOST_TEST((x >> n) == (x >> count));

        W t {x};
        t <<= n;
        BOOST_TEST(t == (x << count));
        t = x;
        t >>= n;
        BOOST_TEST(t == (x >> count));
    }

    // With the Boost.Int128 type on the left the result keeps its type, and a count
    // below 128 matches the low 128 bits of the same shift on the widened value
    static_assert(std::is_same<decltype(v << x), T>::value, "The value's type is kept");
    static_assert(std::is_same<decltype(v >> x), T>::value, "The value's type is kept");

    const W wv {widen<W>(v)};

    for (unsigned count {0U}; count < 128U; count += 1U + static_cast<unsigned>(rng() % 5U))
    {
        const W w {count};
        BOOST_TEST((v << w) == narrow<T>(wv << count));
        BOOST_TEST((v >> w) == narrow<T>(wv >> count));

        T t {v};
        t <<= w;
        BOOST_TEST(t == narrow<T>(wv << count));
        t = v;
        t >>= w;
        BOOST_TEST(t == narrow<T>(wv >> count));
    }
}

// The same operations through the builtin 128-bit flavors must agree, so a program can
// move between __int128 and Boost.Int128 without any result changing
#if defined(BOOST_INT256_HAS_INT128)

template <typename T>
struct builtin_counterpart
{
    using type = boost::int256::detail::builtin_u128;
};

template <>
struct builtin_counterpart<int128>
{
    using type = boost::int256::detail::builtin_i128;
};

template <typename W, typename T>
void check_builtin_parity(const W& x, const T v)
{
    using builtin = typename builtin_counterpart<T>::type;
    const builtin b {static_cast<builtin>(v)};

    BOOST_TEST(W{v} == W{b});
    BOOST_TEST_EQ(x < v, x < b);
    BOOST_TEST_EQ(v < x, b < x);
    BOOST_TEST(x + v == x + b);
    BOOST_TEST(v - x == b - x);
    BOOST_TEST(x * v == x * b);
    BOOST_TEST((v ^ x) == (b ^ x));

    const W wv {v};
    if (division_is_defined<W, T>(x, wv))
    {
        BOOST_TEST(x / v == x / b);
        BOOST_TEST(x % v == x % b);
    }

    const T back = x;
    BOOST_TEST(static_cast<builtin>(back) == static_cast<builtin>(x));

    T sum {};
    builtin builtin_sum {};
    BOOST_TEST_EQ(boost::int256::ckd_add(&sum, x, v), boost::int256::ckd_add(&builtin_sum, x, b));
    BOOST_TEST(static_cast<builtin>(sum) == builtin_sum);

    BOOST_TEST_EQ(boost::int256::cmp_less(v, x), boost::int256::cmp_less(b, x));
    BOOST_TEST_EQ(boost::int256::in_range<T>(x), boost::int256::in_range<builtin>(x));
    BOOST_TEST(static_cast<builtin>(boost::int256::saturating_cast<T>(x)) == boost::int256::saturating_cast<builtin>(x));
}

#else

template <typename W, typename T>
void check_builtin_parity(const W&, const T)
{
}

#endif // builtin 128-bit parity

template <typename W, typename T>
void check_one(const W& x, const T v)
{
    check_conversions(x, v);
    check_comparisons(x, v);
    check_arithmetic(x, v);
    check_compound(x, v);
    check_builtin_parity(x, v);
}

template <typename W, typename T>
void test_random()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto x {draw_256<W>()};
        const auto v {draw_128<T>()};

        check_one(x, v);

        if (i % 64U == 0U)
        {
            check_shifts(x, v);
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

template <typename W, typename T>
void test_directed()
{
    const W wide_values[] {W{0}, W{1}, W{-1}, W{2},
                           (std::numeric_limits<W>::max)(), (std::numeric_limits<W>::min)(),
                           W{1} << 127U, W{1} << 128U, (W{1} << 128U) - 1};

    const T narrow_values[] {T{0}, T{1}, T{2}, T{-1},
                             (std::numeric_limits<T>::max)(), (std::numeric_limits<T>::min)(),
                             T{1} << 64U, T{1} << 127U};

    for (const auto& x : wide_values)
    {
        for (const auto& v : narrow_values)
        {
            check_one(x, v);
        }

        check_shifts(x, narrow_values[3]);
    }
}

void test_values()
{
    // The signed type sign fills, the unsigned type zero fills
    BOOST_TEST(uint256{int128{-1}} == (std::numeric_limits<uint256>::max)());
    BOOST_TEST(int256{int128{-1}} == -1);
    BOOST_TEST(int256{(std::numeric_limits<int128>::min)()} == -(int256{1} << 127U));
    BOOST_TEST(uint256{(std::numeric_limits<uint128>::max)()} == (uint256{1} << 128U) - 1U);
    BOOST_TEST(int256{(std::numeric_limits<uint128>::max)()} == (int256{1} << 128U) - 1);

    // The usual arithmetic conversions: a negative int128 compares as a huge uint256
    BOOST_TEST(uint256{0U} < int128{-1});
    BOOST_TEST(int256{0} > int128{-1});
    BOOST_TEST(int256{-1} < uint128{0U});

    // Conversions to the Boost.Int128 types wrap
    const uint128 wrapped = uint256{1U} << 128U;
    BOOST_TEST(wrapped == 0U);
    const int128 negative = uint256{(std::numeric_limits<uint128>::max)()};
    BOOST_TEST(negative == -1);

    // Division is by the full 256-bit value of the divisor
    BOOST_TEST(uint256{100U} / int128{-1} == 0U);
    BOOST_TEST(int256{100} / int128{-3} == -33);
    BOOST_TEST(int256{100} % int128{-3} == 1);
    BOOST_TEST(int128{-100} / int256{3} == -33);

    // A compound assignment onto the Boost.Int128 type divides at full width
    uint128 u {5U};
    u /= (uint256{1U} << 200U);
    BOOST_TEST(u == 0U);

    int128 s {-7};
    s >>= int256{1};
    BOOST_TEST(s == -4);
}

void test_numeric_utilities()
{
    // Qualified on purpose: Boost.Int128 has its own unconstrained ckd_* templates, so an
    // unqualified call with operands from both libraries is ambiguous through ADL
    using boost::int256::saturating_cast;
    using boost::int256::cmp_less;
    using boost::int256::cmp_equal;
    using boost::int256::in_range;

    // saturating_cast clamps into the Boost.Int128 range
    BOOST_TEST(saturating_cast<uint128>((std::numeric_limits<uint256>::max)()) == (std::numeric_limits<uint128>::max)());
    BOOST_TEST(saturating_cast<uint128>(uint256{42U}) == 42U);
    BOOST_TEST(saturating_cast<int128>((std::numeric_limits<uint256>::max)()) == (std::numeric_limits<int128>::max)());
    BOOST_TEST(saturating_cast<int128>((std::numeric_limits<int256>::min)()) == (std::numeric_limits<int128>::min)());
    BOOST_TEST(saturating_cast<uint128>(int256{-5}) == 0U);
    BOOST_TEST(saturating_cast<int128>(int256{-5}) == -5);

    // The checked functions accept Boost.Int128 operands and results
    uint128 u {};
    BOOST_TEST(!boost::int256::ckd_add(&u, uint256{1U}, uint128{2U}));
    BOOST_TEST(u == 3U);
    BOOST_TEST(boost::int256::ckd_add(&u, (std::numeric_limits<uint128>::max)(), uint256{1U}));
    BOOST_TEST(u == 0U);
    BOOST_TEST(boost::int256::ckd_sub(&u, uint128{0U}, int256{1}));
    BOOST_TEST(u == (std::numeric_limits<uint128>::max)());

    int128 s {};
    BOOST_TEST(!boost::int256::ckd_sub(&s, int128{-2}, int256{3}));
    BOOST_TEST(s == -5);
    BOOST_TEST(boost::int256::ckd_mul(&s, int256{1} << 100U, int128{1} << 30U));
    BOOST_TEST(!boost::int256::ckd_mul(&s, -(int256{1} << 63U), int128{1} << 64U));
    BOOST_TEST(s == (std::numeric_limits<int128>::min)());

    uint256 w {};
    BOOST_TEST(!boost::int256::ckd_mul(&w, (std::numeric_limits<uint128>::max)(), (std::numeric_limits<uint128>::max)()));
    BOOST_TEST(w == ((uint256{1U} << 128U) - 1U) * ((uint256{1U} << 128U) - 1U));

    // The comparison functions compare mathematical values regardless of sign
    BOOST_TEST(cmp_less(int128{-1}, uint256{0U}));
    BOOST_TEST(!cmp_less(uint256{0U}, int128{-1}));
    BOOST_TEST(cmp_equal(uint128{7U}, int256{7}));
    BOOST_TEST(!cmp_equal(int128{-1}, (std::numeric_limits<uint256>::max)()));

    BOOST_TEST(in_range<uint128>(uint256{(std::numeric_limits<uint128>::max)()}));
    BOOST_TEST(!in_range<uint128>(uint256{1U} << 128U));
    BOOST_TEST(!in_range<uint128>(int256{-1}));
    BOOST_TEST(in_range<int128>(int256{(std::numeric_limits<int128>::min)()}));
    BOOST_TEST(!in_range<int128>(int256{(std::numeric_limits<int128>::min)()} - 1));
    BOOST_TEST(in_range<uint256>(int128{0}));
    BOOST_TEST(!in_range<uint256>(int128{-1}));
}

void test_traits()
{
    static_assert(boost::int256::detail::is_int128_interop_v<uint128>, "uint128 is recognized");
    static_assert(boost::int256::detail::is_int128_interop_v<int128>, "int128 is recognized");
    static_assert(!boost::int256::detail::is_int128_interop_v<int>, "a builtin is not");
    static_assert(!boost::int256::detail::is_int128_interop_v<uint256>, "the library type is not");

    static_assert(std::is_convertible<uint128, uint256>::value, "uint128 -> uint256");
    static_assert(std::is_convertible<int128, uint256>::value, "int128 -> uint256");
    static_assert(std::is_convertible<uint128, int256>::value, "uint128 -> int256");
    static_assert(std::is_convertible<int128, int256>::value, "int128 -> int256");
    static_assert(std::is_convertible<uint256, uint128>::value, "uint256 -> uint128");
    static_assert(std::is_convertible<uint256, int128>::value, "uint256 -> int128");
    static_assert(std::is_convertible<int256, uint128>::value, "int256 -> uint128");
    static_assert(std::is_convertible<int256, int128>::value, "int256 -> int128");
}

void test_constexpr()
{
    static_assert(uint256{int128{-1}} == (std::numeric_limits<uint256>::max)(), "sign fill");
    static_assert(int256{uint128{5U}} + int128{-7} == -2, "mixed addition");
    static_assert(uint128{3U} * uint256{4U} == 12U, "mixed multiplication");
    static_assert((int256{-8} >> int128{1}) == -4, "arithmetic shift by an int128 count");
    static_assert(int128{-9} / int256{2} == -4, "division truncates toward zero");
}

} // namespace

int main()
{
    test_traits();
    test_constexpr();
    test_values();
    test_numeric_utilities();

    test_directed<uint256, uint128>();
    test_directed<uint256, int128>();
    test_directed<int256, uint128>();
    test_directed<int256, int128>();

    test_random<uint256, uint128>();
    test_random<uint256, int128>();
    test_random<int256, uint128>();
    test_random<int256, int128>();

    return boost::report_errors();
}

#else

int main()
{
    return 0;
}

#endif // BOOST_INT256_TEST_HAS_INT128_LIBRARY
