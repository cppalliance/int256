// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// The Boost.Random integration. random.hpp only supplies trait specializations,
// so what has to be checked is that uniform_int_distribution actually works over
// the full 256-bit range: a distribution that silently narrowed to 64 bits would
// still compile and would never produce a value with an upper word set.

#include <boost/int256.hpp>
#include <boost/int256/random.hpp>

// Boost.Random is not clean under the strict warning set.
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-warning-option"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#  pragma clang diagnostic ignored "-Wsign-conversion"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wundef"
#  pragma clang diagnostic ignored "-Wshadow"
// boost/random/detail/polynomial.hpp has a class with a user-provided copy
// assignment and an implicit copy constructor.
#  pragma clang diagnostic ignored "-Wdeprecated-copy"
#  if defined(__has_warning)
#    if __has_warning("-Wdeprecated-copy-with-user-provided-copy")
#      pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
#    endif
#  endif
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wundef"
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wuseless-cast"
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <type_traits>

using boost::int256::uint256;

#ifdef BOOST_INT256_DETAIL_INT256_IMP_HPP
#  define BOOST_INT256_TEST_HAS_SIGNED 1
using boost::int256::int256;
#endif

namespace
{

constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};

#if defined(BOOST_INT256_TEST_HAS_SIGNED)
constexpr int256 min_i {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
#endif

// The trait specializations Boost.Random dispatches on.
void test_traits()
{
    static_assert(boost::random::traits::is_integral<uint256>::value,
                  "uint256 has to look integral to Boost.Random");
    static_assert(!boost::random::traits::is_signed<uint256>::value,
                  "uint256 is unsigned");
    static_assert(std::is_same<boost::random::traits::make_unsigned<uint256>::type, uint256>::value,
                  "make_unsigned is the identity");
    static_assert(std::is_same<boost::random::traits::make_unsigned_or_unbounded<uint256>::type, uint256>::value,
                  "make_unsigned_or_unbounded is the identity");
}

// Over the whole range, the upper words have to move. A distribution that lost
// the top of the range would produce nothing above 2^64.
void test_full_range()
{
    boost::random::mt19937_64 engine {42};
    boost::random::uniform_int_distribution<uint256> dist {
        (std::numeric_limits<uint256>::min)(), (std::numeric_limits<uint256>::max)()};

    bool saw_word1 {false};
    bool saw_word2 {false};
    bool saw_word3 {false};
    std::set<std::uint64_t> distinct_top_words;

    constexpr int draws {2048};
    for (int i {0}; i < draws; ++i)
    {
        const auto v {dist(engine)};

        if (v.words[1] != 0U) { saw_word1 = true; }
        if (v.words[2] != 0U) { saw_word2 = true; }
        if (v.words[3] != 0U) { saw_word3 = true; }

        distinct_top_words.insert(v.words[3]);
    }

    BOOST_TEST(saw_word1);
    BOOST_TEST(saw_word2);
    BOOST_TEST(saw_word3);

    // Almost every draw should have a distinct top word over the full range.
    BOOST_TEST(distinct_top_words.size() > static_cast<std::size_t>(draws - 10));
}

// A narrow range has to stay inside it, and has to be able to reach both ends.
void test_narrow_range()
{
    boost::random::mt19937_64 engine {42};

    const uint256 low {uint256{1U} << 200U};
    const uint256 high {low + 1000U};

    boost::random::uniform_int_distribution<uint256> dist {low, high};

    bool saw_low {false};
    bool saw_high {false};
    std::set<std::uint64_t> seen;

    for (int i {0}; i < 20000; ++i)
    {
        const auto v {dist(engine)};

        BOOST_TEST(v >= low);
        BOOST_TEST(v <= high);

        // The top word is nonzero, because 2^200 has bit 8 of words[3] set.
        BOOST_TEST(v.words[3] != 0U);

        if (v == low) { saw_low = true; }
        if (v == high) { saw_high = true; }

        seen.insert(static_cast<std::uint64_t>(v - low));

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    BOOST_TEST(saw_low);
    BOOST_TEST(saw_high);

    // All 1001 values in the range should show up in 20000 draws.
    BOOST_TEST_EQ(seen.size(), static_cast<std::size_t>(1001));
}

// A single point range always returns that point.
void test_degenerate_range()
{
    boost::random::mt19937_64 engine {42};

    const uint256 point {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(42)};
    boost::random::uniform_int_distribution<uint256> dist {point, point};

    for (int i {0}; i < 100; ++i)
    {
        BOOST_TEST_EQ(dist(engine), point);
    }

    // And zero to zero.
    boost::random::uniform_int_distribution<uint256> zero_dist {uint256{}, uint256{}};
    for (int i {0}; i < 100; ++i)
    {
        BOOST_TEST_EQ(zero_dist(engine), uint256{});
    }
}

// A range straddling a word boundary, where a carry in the range arithmetic would
// show up as an out of range draw.
void test_word_boundary_ranges()
{
    boost::random::mt19937_64 engine {42};

    for (unsigned k : {63U, 64U, 65U, 127U, 128U, 191U, 192U, 255U})
    {
        const uint256 centre {one << k};
        const uint256 low {centre - 10U};
        const uint256 high {centre + 10U};

        boost::random::uniform_int_distribution<uint256> dist {low, high};

        for (int i {0}; i < 500; ++i)
        {
            const auto v {dist(engine)};
            BOOST_TEST(v >= low);
            BOOST_TEST(v <= high);
        }

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  k = " << k << std::endl;
            return;
        }
    }
}

// The same engine and seed must give the same sequence, which is what makes a
// failing case reproducible.
void test_reproducible()
{
    boost::random::mt19937_64 first {12345};
    boost::random::mt19937_64 second {12345};

    boost::random::uniform_int_distribution<uint256> a {
        (std::numeric_limits<uint256>::min)(), (std::numeric_limits<uint256>::max)()};
    boost::random::uniform_int_distribution<uint256> b {
        (std::numeric_limits<uint256>::min)(), (std::numeric_limits<uint256>::max)()};

    for (int i {0}; i < 256; ++i)
    {
        BOOST_TEST_EQ(a(first), b(second));
    }
}

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

//=====================================
// int256: the same Boost.Random integration, now with is_signed<int256> true
// and make_unsigned<int256> resolving to uint256, so a range can straddle
// zero and the negative half of the range has to come out too
//=====================================

void test_traits_i()
{
    static_assert(boost::random::traits::is_integral<int256>::value,
                  "int256 has to look integral to Boost.Random");
    static_assert(boost::random::traits::is_signed<int256>::value,
                  "int256 is signed");
    static_assert(std::is_same<boost::random::traits::make_unsigned<int256>::type, uint256>::value,
                  "make_unsigned maps int256 to uint256");
    static_assert(std::is_same<boost::random::traits::make_unsigned_or_unbounded<int256>::type, uint256>::value,
                  "make_unsigned_or_unbounded maps int256 to uint256");
}

// Over the whole range, both negative and non-negative values have to come
// out, and the upper words have to move on both sides of zero.
void test_full_range_i()
{
    boost::random::mt19937_64 engine {42};
    boost::random::uniform_int_distribution<int256> dist {
        (std::numeric_limits<int256>::min)(), (std::numeric_limits<int256>::max)()};

    bool saw_negative {false};
    bool saw_non_negative {false};
    bool saw_word1 {false};
    bool saw_word2 {false};
    std::set<std::uint64_t> distinct_top_words;

    constexpr int draws {2048};
    for (int i {0}; i < draws; ++i)
    {
        const auto v {dist(engine)};

        if (v.signed_high() < 0) { saw_negative = true; }
        else { saw_non_negative = true; }

        if (v.words[1] != 0U) { saw_word1 = true; }
        if (v.words[2] != 0U) { saw_word2 = true; }

        distinct_top_words.insert(v.words[3]);
    }

    BOOST_TEST(saw_negative);
    BOOST_TEST(saw_non_negative);
    BOOST_TEST(saw_word1);
    BOOST_TEST(saw_word2);
    BOOST_TEST(distinct_top_words.size() > static_cast<std::size_t>(draws - 10));
}

// A narrow range straddling zero has to stay inside it, has to be able to
// reach both (negative and positive) ends, and has to actually produce
// negative values, not just clamp everything to the non-negative side.
void test_narrow_range_i()
{
    boost::random::mt19937_64 engine {42};

    const int256 low {-500};
    const int256 high {500};

    boost::random::uniform_int_distribution<int256> dist {low, high};

    bool saw_low {false};
    bool saw_high {false};
    bool saw_negative {false};
    bool saw_positive {false};
    std::set<std::int64_t> seen;

    for (int i {0}; i < 20000; ++i)
    {
        const auto v {dist(engine)};

        BOOST_TEST(v >= low);
        BOOST_TEST(v <= high);

        if (v == low) { saw_low = true; }
        if (v == high) { saw_high = true; }
        if (v < int256{}) { saw_negative = true; }
        if (v > int256{}) { saw_positive = true; }

        // v is always within [-500, 500] here, so its magnitude fits the low
        // word alone; words[0] on its own is the two's complement bit
        // pattern, not the magnitude, so a negative value has to go through
        // unary minus first.
        const std::int64_t value {v.signed_high() < 0
                                       ? -static_cast<std::int64_t>((-v).words[0])
                                       : static_cast<std::int64_t>(v.words[0])};
        seen.insert(value);

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }

    BOOST_TEST(saw_low);
    BOOST_TEST(saw_high);
    BOOST_TEST(saw_negative);
    BOOST_TEST(saw_positive);

    // All 1001 values in [-500, 500] should show up in 20000 draws.
    BOOST_TEST_EQ(seen.size(), static_cast<std::size_t>(1001));
}

void test_degenerate_range_i()
{
    boost::random::mt19937_64 engine {42};

    const int256 point {-42};
    boost::random::uniform_int_distribution<int256> dist {point, point};

    for (int i {0}; i < 100; ++i)
    {
        BOOST_TEST_EQ(dist(engine), point);
    }

    // MIN to MIN: the degenerate case sitting right at the extreme where the
    // unsigned range machinery (make_unsigned) has the least room to spare.
    boost::random::uniform_int_distribution<int256> min_dist {min_i, min_i};
    for (int i {0}; i < 100; ++i)
    {
        BOOST_TEST_EQ(min_dist(engine), min_i);
    }
}

// A range straddling a word boundary on the negative side, where a carry in
// the unsigned range arithmetic (min is negative, so make_unsigned's zero
// point sits above it) would show up as an out of range draw.
void test_word_boundary_ranges_i()
{
    boost::random::mt19937_64 engine {42};

    for (unsigned k : {63U, 64U, 65U, 127U, 128U, 191U, 192U, 254U})
    {
        const auto centre {static_cast<int256>(one << k)};
        const int256 low {centre - 10};
        const int256 high {centre + 10};

        boost::random::uniform_int_distribution<int256> pos_dist {low, high};
        boost::random::uniform_int_distribution<int256> neg_dist {-high, -low};

        for (int i {0}; i < 500; ++i)
        {
            const auto v {pos_dist(engine)};
            BOOST_TEST(v >= low);
            BOOST_TEST(v <= high);

            const auto w {neg_dist(engine)};
            BOOST_TEST(w >= -high);
            BOOST_TEST(w <= -low);
        }

        if (boost::detail::test_errors() != 0)
        {
            std::cerr << "  k = " << k << std::endl;
            return;
        }
    }

    // And the extreme case: a range that spans zero from just below MIN's
    // neighbourhood is not reachable (nothing is below MIN), but a range
    // ending exactly at MIN on the low side is.
    {
        const int256 low {min_i};
        const int256 high {min_i + 1000};
        boost::random::uniform_int_distribution<int256> dist {low, high};

        bool saw_min {false};
        for (int i {0}; i < 2000; ++i)
        {
            const auto v {dist(engine)};
            BOOST_TEST(v >= low);
            BOOST_TEST(v <= high);
            if (v == min_i) { saw_min = true; }
        }

        BOOST_TEST(saw_min);
    }
}

void test_reproducible_i()
{
    boost::random::mt19937_64 first {12345};
    boost::random::mt19937_64 second {12345};

    boost::random::uniform_int_distribution<int256> a {
        (std::numeric_limits<int256>::min)(), (std::numeric_limits<int256>::max)()};
    boost::random::uniform_int_distribution<int256> b {
        (std::numeric_limits<int256>::min)(), (std::numeric_limits<int256>::max)()};

    for (int i {0}; i < 256; ++i)
    {
        BOOST_TEST_EQ(a(first), b(second));
    }
}

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    test_traits();
    test_full_range();
    test_narrow_range();
    test_degenerate_range();
    test_word_boundary_ranges();
    test_reproducible();

    #if defined(BOOST_INT256_TEST_HAS_SIGNED)
    test_traits_i();
    test_full_range_i();
    test_narrow_range_i();
    test_degenerate_range_i();
    test_word_boundary_ranges_i();
    test_reproducible_i();
    #endif

    return boost::report_errors();
}
