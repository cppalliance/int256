// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256.hpp>
#include <boost/int256/random.hpp> // Not included in the convenience header, but needed for boost.random interop

// tag::exclude[]
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wstring-conversion"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

// end::exclude[]
#include <boost/random/uniform_int_distribution.hpp>
#include <array>
#include <iostream>
#include <limits>
#include <random>

int main()
{
    using boost::int256::uint256;

    std::cout << "=== Random uint256 values ===" << std::endl;

    // Setup our rng and distribution. <boost/int256/random.hpp> provides the traits
    // that Boost.Random needs to treat uint256 as an integer type.
    std::mt19937_64 rng {42};
    boost::random::uniform_int_distribution<uint256> dist {0, (std::numeric_limits<uint256>::max)()};

    std::cout << "Three draws over the whole range:" << std::endl;
    for (int i {0}; i < 3; ++i)
    {
        std::cout << "  " << dist(rng) << std::endl;
    }

    // A distribution over a narrow range at the top of the type works the same way
    std::cout << "\nThree draws from [2^200, 2^200 + 1000]:" << std::endl;
    const uint256 low {uint256{1U} << 200U};
    boost::random::uniform_int_distribution<uint256> narrow {low, low + 1000U};
    for (int i {0}; i < 3; ++i)
    {
        const auto draw {narrow(rng)};
        std::cout << "  " << draw << " (offset " << (draw - low) << ")" << std::endl;
    }

    std::cout << "\n=== A random data set ===" << std::endl;

    // Create a data set of random uint256 values using the dist and rng from above
    std::array<uint256, 10000> data_set {};
    for (auto& value : data_set)
    {
        value = dist(rng);
    }

    // Nothing here needs a wider accumulator: the mean of the top words is enough to
    // show the draws are spread over the whole range. Summing the values themselves
    // would wrap, so the average is taken over the most significant word.
    long double top_word_total {0.0L};
    uint256 minimum {(std::numeric_limits<uint256>::max)()};
    uint256 maximum {0U};

    for (const auto& value : data_set)
    {
        top_word_total += static_cast<long double>(value.words[3]);
        minimum = value < minimum ? value : minimum;
        maximum = value > maximum ? value : maximum;
    }

    const auto expected_mean {static_cast<long double>(UINT64_MAX) / 2.0L};
    const auto actual_mean {top_word_total / static_cast<long double>(data_set.size())};

    std::cout << "Samples:               " << data_set.size() << std::endl;
    std::cout << "Mean of the top word:  " << actual_mean << std::endl;
    std::cout << "Expected (UINT64_MAX/2): " << expected_mean << std::endl;
    std::cout << "Within 2 percent:      " << std::boolalpha
              << (actual_mean > expected_mean * 0.98L && actual_mean < expected_mean * 1.02L) << std::endl;
    std::cout << "Smallest draw has a nonzero top word: " << (minimum.words[3] != 0U) << std::endl;
    std::cout << "Largest draw is above 2^255:          " << (maximum > (uint256{1U} << 255U)) << std::endl;

    return 0;
}
