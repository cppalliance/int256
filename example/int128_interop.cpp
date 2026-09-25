// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Including Boost.Int128 before Boost.Int256 is what enables the interoperability:
// Boost.Int256 never includes <boost/int128.hpp> on its own
#include <boost/int128.hpp>
#include <boost/int256.hpp>
#include <iostream>
#include <limits>

int main()
{
    using boost::int128::uint128;
    using boost::int128::int128;
    using boost::int256::uint256;
    using boost::int256::int256;

    std::cout << std::boolalpha;
    std::cout << "=== Construction and Conversion ===" << std::endl;

    // Construction from uint128 and int128 is implicit and exact, and a negative
    // int128 sign fills the upper words exactly as a negative __int128 does
    constexpr uint128 u128_max {(std::numeric_limits<uint128>::max)()};
    const uint256 from_unsigned {u128_max};
    const int256 from_signed {int128{-42}};
    const uint256 sign_filled {int128{-1}};
    std::cout << "uint256{uint128 max}   = " << from_unsigned << std::endl;
    std::cout << "int256{int128{-42}}    = " << from_signed << std::endl;
    std::cout << "uint256{int128{-1}} is uint256 max: "
              << (sign_filled == (std::numeric_limits<uint256>::max)()) << std::endl;

    // The conversion back is implicit too and keeps the low 128 bits. Copy initialization
    // is the portable spelling: see the documentation for static_cast
    const uint256 wide {(uint256{1U} << 128U) + 7U};
    const uint128 low_bits = wide;
    const int128 negative = int256{-5};
    std::cout << "uint128 from 2^128 + 7 = " << low_bits << std::endl;
    std::cout << "int128 from int256{-5} = " << negative << std::endl;

    std::cout << "\n=== Mixed Arithmetic ===" << std::endl;

    // The result has the 256-bit type, so a product that overflows 128 bits is exact here
    std::cout << "uint128 max * uint256{uint128 max} = " << (u128_max * uint256{u128_max}) << std::endl;
    std::cout << "int256{-10} / int128{3}   = " << (int256{-10} / int128{3}) << std::endl;
    std::cout << "int128{-10} % int256{3}   = " << (int128{-10} % int256{3}) << std::endl;

    // The usual arithmetic conversions apply, exactly as with the builtin 128-bit types:
    // against uint256 a negative int128 becomes a huge unsigned value
    std::cout << "uint256{0} < int128{-1}   = " << (uint256{0U} < int128{-1}) << std::endl;
    std::cout << "int256{0} > int128{-1}    = " << (int256{0} > int128{-1}) << std::endl;

    std::cout << "\n=== Compound Assignment and Shifts ===" << std::endl;

    int256 total {100};
    total -= int128{142};
    total *= uint128{3U};
    std::cout << "int256{100} -= 142, *= 3  = " << total << std::endl;

    // With the Boost.Int128 type on the left, the operation happens at 256 bits and only
    // the result is narrowed, so dividing by 2^200 gives 0 rather than dividing by zero
    uint128 counter {10U};
    counter += uint256{5U};
    std::cout << "uint128{10} += uint256{5} = " << counter << std::endl;
    counter /= uint256{1U} << 200U;
    std::cout << "uint128{15} /= 2^200      = " << counter << std::endl;

    // A shift takes its value and its result type from the left operand
    std::cout << "uint128{1} << uint256{100} = " << (uint128{1U} << uint256{100U}) << std::endl;
    std::cout << "int256{-256} >> int128{4}  = " << (int256{-256} >> int128{4}) << std::endl;

    std::cout << "\n=== Numeric Utilities ===" << std::endl;

    std::cout << "saturating_cast<uint128>(2^200)   = "
              << boost::int256::saturating_cast<uint128>(uint256{1U} << 200U) << std::endl;
    std::cout << "saturating_cast<int128>(int256 min) = "
              << boost::int256::saturating_cast<int128>((std::numeric_limits<int256>::min)()) << std::endl;

    // Boost.Int128 has its own ckd_* templates, so qualify these calls to pick this library's
    uint128 sum {};
    const bool overflowed {boost::int256::ckd_add(&sum, u128_max, uint256{1U})};
    std::cout << "ckd_add(uint128 max, 1) overflowed: " << overflowed << ", wrapped sum = " << sum << std::endl;

    std::cout << "cmp_less(int128{-1}, uint256{0})  = " << boost::int256::cmp_less(int128{-1}, uint256{0U}) << std::endl;
    std::cout << "in_range<int128>(uint256{2^127})  = " << boost::int256::in_range<int128>(uint256{1U} << 127U) << std::endl;

    return 0;
}
