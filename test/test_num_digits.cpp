// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// charconv's num_digits, which sizes the output buffer for to_chars. It estimates
// the decimal length from the most significant bit position and then corrects
// against a table of the 78 powers of ten, so the values that matter are exactly
// 10^k and 10^k - 1 for every k: those are where the estimate is off by one and
// the correction has to fire.

#include "oracle.hpp"

// The charconv integration is opt-in and needs Boost.Charconv to be present.
#if __has_include(<boost/charconv.hpp>)
#  include <boost/int256/charconv.hpp>
#  define BOOST_INT256_TEST_HAS_CHARCONV
#endif

#if defined(BOOST_INT256_ORACLE_UNAVAILABLE) || !defined(BOOST_INT256_TEST_HAS_CHARCONV)

int main()
{
    BOOST_TEST(true);

    return boost::report_errors();
}

#else

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using boost::int256::uint256;

namespace
{

std::mt19937_64 rng {42};

int digits_of(const uint256& v)
{
    return boost::charconv::detail::num_digits(v);
}

// The powers of ten up to 10^77, built by repeated multiplication so the table in
// the header is never the reference for itself.
const std::vector<uint256>& powers_of_ten()
{
    static const std::vector<uint256> table {[]
    {
        std::vector<uint256> values;
        uint256 power {1U};
        for (int k {0}; k <= 77; ++k)
        {
            values.push_back(power);
            power = power * 10U;
        }
        return values;
    }()};

    return table;
}

// The decimal length according to the reference, which renders the value.
int reference_digits(const uint256& v)
{
    return static_cast<int>(mp_str(to_mp(v), 10, false).size());
}

void test_powers_of_ten()
{
    const auto& powers {powers_of_ten()};

    for (int k {0}; k <= 77; ++k)
    {
        const auto& power {powers[static_cast<std::size_t>(k)]};

        // 10^k has k + 1 digits.
        if (digits_of(power) != k + 1)
        {
            std::cerr << "num_digits(10^" << k << ") = " << digits_of(power)
                      << ", expected " << k + 1 << " (value " << hex_words(power) << ")" << std::endl;
        }
        BOOST_TEST_EQ(digits_of(power), k + 1);
        BOOST_TEST_EQ(digits_of(power), reference_digits(power));

        // 10^k - 1 has k digits, except at k == 0 where it is zero and still one.
        if (k > 0)
        {
            const uint256 below {power - 1U};
            if (digits_of(below) != k)
            {
                std::cerr << "num_digits(10^" << k << " - 1) = " << digits_of(below)
                          << ", expected " << k << " (value " << hex_words(below) << ")" << std::endl;
            }
            BOOST_TEST_EQ(digits_of(below), k);
            BOOST_TEST_EQ(digits_of(below), reference_digits(below));
        }

        // And 10^k + 1 still has k + 1.
        const uint256 above {power + 1U};
        BOOST_TEST_EQ(digits_of(above), k + 1);
        BOOST_TEST_EQ(digits_of(above), reference_digits(above));
    }
}

void test_directed()
{
    BOOST_TEST_EQ(digits_of(uint256{}), 1);
    BOOST_TEST_EQ(digits_of(uint256{1U}), 1);
    BOOST_TEST_EQ(digits_of(uint256{9U}), 1);
    BOOST_TEST_EQ(digits_of(uint256{10U}), 2);
    BOOST_TEST_EQ(digits_of(uint256{99U}), 2);
    BOOST_TEST_EQ(digits_of(uint256{100U}), 3);

    // The 64-bit boundary, where the implementation switches from the one word
    // routine to the estimate.
    BOOST_TEST_EQ(digits_of(uint256{UINT64_MAX}), 20);
    BOOST_TEST_EQ(digits_of(uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0))), 20);
    BOOST_TEST_EQ(digits_of(uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(1))), 20);

    // The maximum has 78 digits, which is one more than digits10.
    BOOST_TEST_EQ(digits_of(uint256_max()), 78);
    BOOST_TEST_EQ(digits_of(uint256_max() - 1U), 78);

    // Every power of two, which is where the log10 estimate is worst.
    const uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
    for (unsigned k {0}; k < 256U; ++k)
    {
        const uint256 value {one << k};
        BOOST_TEST_EQ(digits_of(value), reference_digits(value));
    }
}

void test_random()
{
    for (std::size_t i {0}; i < N; ++i)
    {
        const auto v {random_uint256(rng, shape::word4)};
        BOOST_TEST_EQ(digits_of(v), reference_digits(v));

        // Values of every width, so the one word shortcut is reached too.
        for (const auto s : all_shapes())
        {
            const auto w {random_uint256(rng, s)};
            if (digits_of(w) != reference_digits(w))
            {
                std::cerr << "num_digits mismatch for shape " << shape_name(s)
                          << ": got " << digits_of(w) << ", expected " << reference_digits(w)
                          << ", value " << hex_words(w) << std::endl;
            }
            BOOST_TEST_EQ(digits_of(w), reference_digits(w));
        }

        if (boost::detail::test_errors() != 0)
        {
            return;
        }
    }
}

// num_digits sizes the buffer, so to_chars into exactly that many characters has
// to succeed and one fewer has to fail.
void test_sizes_the_buffer()
{
    const uint256 values[] {
        uint256{}, uint256{1U}, uint256{UINT64_MAX}, uint256_max(),
        uint256(UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0))
    };

    for (const auto& v : values)
    {
        const auto needed {static_cast<std::size_t>(digits_of(v))};

        std::vector<char> exact(needed);
        const auto fits {boost::charconv::to_chars(exact.data(), exact.data() + exact.size(), v)};
        BOOST_TEST(fits);
        BOOST_TEST_EQ(static_cast<std::size_t>(fits.ptr - exact.data()), needed);

        if (needed > 1U)
        {
            std::vector<char> tight(needed - 1U);
            const auto overflows {boost::charconv::to_chars(tight.data(), tight.data() + tight.size(), v)};
            BOOST_TEST(!overflows);
        }
    }
}

} // namespace

int main()
{
    test_directed();
    test_powers_of_ten();
    test_sizes_the_buffer();
    test_random();

    return boost::report_errors();
}

#endif
