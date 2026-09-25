// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Individual headers

#include <boost/int256/utilities.hpp>
#include <boost/int256/iostream.hpp>
#include <boost/int256/limits.hpp>

// Or you can do a single header

// #include <boost/int256.hpp>

#include <cstdint>
#include <iostream>
#include <limits>

int main()
{
    using boost::int256::uint256;
    using boost::int256::int256;
    using boost::int256::cmp_equal;
    using boost::int256::cmp_less;
    using boost::int256::in_range;

    std::cout << std::boolalpha;

    constexpr auto max_value {(std::numeric_limits<uint256>::max)()};

    // The built-in relational operators follow the usual arithmetic conversions, so
    // comparing a uint256 with a negative value converts the negative one to a huge
    // unsigned value and gives the mathematically wrong answer. That is what the
    // built-in unsigned types do too, and this library matches them deliberately.
    // The cmp_* family compares the true mathematical values instead.
    std::cout << "=== Signedness-safe comparison ===" << std::endl;
    std::cout << "max == -1 (operator):        " << (max_value == -1) << std::endl;
    std::cout << "cmp_equal(max, -1):          " << cmp_equal(max_value, -1) << std::endl;
    std::cout << "uint256{0} > -1 (operator):  " << (uint256{0U} > -1) << std::endl;
    std::cout << "cmp_less(-1, uint256{0}):    " << cmp_less(-1, uint256{0U}) << std::endl;

    // Either operand may be a built-in integer of any width and signedness.
    std::cout << "\n=== Mixed with builtin integers ===" << std::endl;
    std::cout << "cmp_less(-5, uint256{0}):    " << cmp_less(-5, uint256{0U}) << std::endl;
    std::cout << "cmp_equal(uint256{42}, 42):  " << cmp_equal(uint256{42U}, 42) << std::endl;
    std::cout << "cmp_less(uint256{42}, 43U):  " << cmp_less(uint256{42U}, 43U) << std::endl;

    // in_range<R>(v) reports whether v is representable in the target type R.
    // R and the type of v may each be a built-in integer or uint256.
    std::cout << "\n=== in_range ===" << std::endl;
    std::cout << "in_range<std::uint8_t>(uint256{200}):  " << in_range<std::uint8_t>(uint256{200U}) << std::endl;
    std::cout << "in_range<std::int8_t>(uint256{200}):   " << in_range<std::int8_t>(uint256{200U}) << std::endl;
    std::cout << "in_range<std::uint64_t>(max):          " << in_range<std::uint64_t>(max_value) << std::endl;
    std::cout << "in_range<std::uint64_t>(UINT64_MAX):   "
              << in_range<std::uint64_t>(uint256{UINT64_MAX}) << std::endl;
    std::cout << "in_range<uint256>(-1):                 " << in_range<uint256>(-1) << std::endl;
    std::cout << "in_range<uint256>(UINT64_MAX):         " << in_range<uint256>(UINT64_MAX) << std::endl;

    // A truncating conversion is easy to guard with in_range first
    std::cout << "\n=== Guarding a narrowing conversion ===" << std::endl;
    const uint256 candidates[] {uint256{7U}, uint256{UINT64_MAX}, max_value};

    for (const auto& candidate : candidates)
    {
        if (in_range<std::uint64_t>(candidate))
        {
            std::cout << candidate << " fits in a std::uint64_t" << std::endl;
        }
        else
        {
            std::cout << candidate << " does not fit in a std::uint64_t" << std::endl;
        }
    }

    std::cout << "\n=== int256 against a builtin integer is always exact ===" << std::endl;

    // Every built-in integer, including unsigned __int128, fits inside the
    // range of int256, so there is no mixed sign trap the way there is
    // against uint256 below: the operator and cmp_less always agree.
    constexpr int256 signed_min {(std::numeric_limits<int256>::min)()};
    std::cout << "signed_min < -1 (operator):  " << (signed_min < -1) << std::endl;
    std::cout << "cmp_less(signed_min, -1):    " << cmp_less(signed_min, -1) << std::endl;

    std::cout << "\n=== int256 against uint256 has the same trap as unsigned ===" << std::endl;

    // int256 and uint256 follow the usual arithmetic conversions: both become
    // uint256, so a negative int256 becomes huge before the comparison, just
    // like a negative builtin does against a uint256 above.
    std::cout << "int256{-1} < uint256{1} (operator): " << (int256{-1} < uint256{1U}) << std::endl;
    std::cout << "cmp_less(int256{-1}, uint256{1}):   " << cmp_less(int256{-1}, uint256{1U}) << std::endl;

    std::cout << "\n=== in_range with int256 ===" << std::endl;

    // in_range<int256> from a uint256 tests against int256's positive range
    std::cout << "in_range<int256>(uint256 max):         " << in_range<int256>(max_value) << std::endl;
    std::cout << "in_range<int256>(uint256{200}):        " << in_range<int256>(uint256{200U}) << std::endl;

    // in_range<uint256> from a negative int256 is always false
    std::cout << "in_range<uint256>(int256{-1}):         " << in_range<uint256>(int256{-1}) << std::endl;
    std::cout << "in_range<std::int8_t>(int256{200}):    " << in_range<std::int8_t>(int256{200}) << std::endl;
    std::cout << "in_range<std::uint8_t>(int256{-1}):    " << in_range<std::uint8_t>(int256{-1}) << std::endl;

    return 0;
}
