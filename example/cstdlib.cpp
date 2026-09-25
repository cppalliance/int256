// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>
#include <boost/int256/cstdlib.hpp>
#include <boost/int256/iostream.hpp>
#include <boost/int256/limits.hpp>
#include <iostream>
#include <limits>

int main()
{
    using boost::int256::uint256;
    using boost::int256::int256;
    using boost::int256::div;

    std::cout << "=== div() Function ===" << std::endl;
    std::cout << "Returns both quotient and remainder from a single division" << std::endl;

    // Unlike a hardware type, a compiler cannot fold a 256-bit division followed by a
    // modulo of the same operands into one operation. The division algorithm produces
    // both halves anyway, so div() hands them back together and costs no more than one.

    std::cout << "\n--- One word divisor ---" << std::endl;

    constexpr uint256 dividend {1000000000000000000ULL};
    constexpr uint256 divisor {7U};

    const auto result {div(dividend, divisor)};
    std::cout << dividend << " / " << divisor << " = " << result.quot
              << " remainder " << result.rem << std::endl;

    // Verify: quot * divisor + rem == dividend
    std::cout << "Verification: " << result.quot << " * " << divisor
              << " + " << result.rem << " = " << (result.quot * divisor + result.rem) << std::endl;

    std::cout << "\n--- Powers of two ---" << std::endl;

    constexpr uint256 large_dividend {uint256{1U} << 200U};
    constexpr uint256 large_divisor {uint256{1U} << 100U};

    const auto large_result {div(large_dividend, large_divisor)};
    std::cout << "2^200 / 2^100 = " << large_result.quot
              << " remainder " << large_result.rem << std::endl;

    std::cout << "\n--- Multi word divisor ---" << std::endl;

    // A divisor with three significant words takes the general path
    constexpr uint256 wide_divisor {UINT64_C(0), UINT64_C(0x1), UINT64_C(0x2), UINT64_C(0x3)};
    constexpr uint256 wide_dividend {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

    const auto wide_result {div(wide_dividend, wide_divisor)};
    std::cout << "max / " << wide_divisor << std::endl;
    std::cout << "  quotient  = " << wide_result.quot << std::endl;
    std::cout << "  remainder = " << wide_result.rem << std::endl;
    std::cout << "  quot * divisor + rem == max: " << std::boolalpha
              << (wide_result.quot * wide_divisor + wide_result.rem == wide_dividend) << std::endl;

    std::cout << "\n--- Edge cases ---" << std::endl;

    const auto small_div {div(uint256{3U}, uint256{10U})};
    std::cout << "3 / 10 = " << small_div.quot << " remainder " << small_div.rem << std::endl;

    const auto exact {div(uint256{100U}, uint256{25U})};
    std::cout << "100 / 25 = " << exact.quot << " remainder " << exact.rem << std::endl;

    const auto by_self {div(wide_divisor, wide_divisor)};
    std::cout << "x / x = " << by_self.quot << " remainder " << by_self.rem << std::endl;

    std::cout << "\n=== div() with int256 (returns i256div_t) ===" << std::endl;

    // Signed division. The division works on the uint256 magnitudes of the
    // operands, so quot's sign is sign(x) != sign(y) and rem takes the sign
    // of the dividend, exactly matching the built-in signed division.
    constexpr int256 signed_dividend {-100};
    constexpr int256 signed_divisor {7};

    const auto sresult {div(signed_dividend, signed_divisor)};
    std::cout << signed_dividend << " / " << signed_divisor << " = " << sresult.quot
              << " remainder " << sresult.rem << std::endl;

    std::cout << "\n--- Sign Combinations ---" << std::endl;

    constexpr int256 pos {17};
    constexpr int256 neg {-17};
    constexpr int256 div_pos {5};
    constexpr int256 div_neg {-5};

    const auto pp {div(pos, div_pos)};
    const auto pn {div(pos, div_neg)};
    const auto np {div(neg, div_pos)};
    const auto nn {div(neg, div_neg)};

    std::cout << " 17 /  5 = " << pp.quot << " remainder " << pp.rem << std::endl;
    std::cout << " 17 / -5 = " << pn.quot << " remainder " << pn.rem << std::endl;
    std::cout << "-17 /  5 = " << np.quot << " remainder " << np.rem << std::endl;
    std::cout << "-17 / -5 = " << nn.quot << " remainder " << nn.rem << std::endl;

    std::cout << "\n--- MIN / -1, the one signed overflow this library defines ---" << std::endl;

    // A built-in int128 division would be undefined here; int256 wraps the
    // quotient back to MIN and the remainder is 0, matching every other
    // two's-complement wrap-around operator in the library.
    constexpr int256 min_value {(std::numeric_limits<int256>::min)()};
    const auto min_by_neg_one {div(min_value, int256{-1})};
    std::cout << "MIN / -1 = " << min_by_neg_one.quot
              << " remainder " << min_by_neg_one.rem << std::endl;
    std::cout << "Quotient equals MIN: " << std::boolalpha << (min_by_neg_one.quot == min_value) << std::endl;

    return 0;
}
