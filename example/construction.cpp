// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>
#include <boost/int256/literals.hpp>
#include <boost/int256/iostream.hpp>
#include <boost/int256/limits.hpp>
#include <boost/int256/climits.hpp>
#include <iostream>
#include <limits>
#include <sstream>

int main()
{
    using boost::int256::uint256;

    std::cout << "=== uint256 Construction ===" << std::endl;

    // 1) From a builtin integer type
    constexpr uint256 from_builtin {42U};
    std::cout << "From builtin (42U): " << from_builtin << std::endl;

    // 2) From the four 64-bit words, most significant first
    constexpr uint256 from_parts {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)};  // 2^64
    std::cout << "From words (0, 0, 1, 0) = 2^64: " << from_parts << std::endl;

    constexpr uint256 max_value {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
    std::cout << "From words (max, max, max, max): " << max_value << std::endl;
    std::cout << "  Equals numeric_limits max? " << std::boolalpha
              << (max_value == (std::numeric_limits<uint256>::max)()) << std::endl;

    // 3) From user-defined literals.
    // The library provides only string-form UDLs
    // For small values like this a string is still parsed rather than direct construction
    // Using the constructors for values that fit in (unsigned) long long should be preferred for performance
    using namespace boost::int256::literals;
    const auto small_literal {12345_U256};
    std::cout << "From literal 12345_U256: " << small_literal << std::endl;

    const auto big_literal {"115792089237316195423570985008687907853269984665640564039457584007913129639935"_u256};
    std::cout << "From string literal (max): " << big_literal << std::endl;

    // 4) From macro (like UINT64_C but for 256-bit), good for values that exceed unsigned long long
    const auto from_macro {BOOST_INT256_UINT256_C(6277101735386680763835789423207666416102355444464034512896)};
    std::cout << "From BOOST_INT256_UINT256_C(2^192): " << from_macro << std::endl;

    // 5) From input stream
    std::stringstream ss;
    ss.str("123456789012345678901234567890123456789012345678901234567890");
    uint256 from_stream;
    ss >> from_stream;
    std::cout << "From stringstream: " << from_stream << std::endl;

    std::cout << "\n=== int256 Construction ===" << std::endl;

    using boost::int256::int256;

    // Signed from a builtin integer type
    constexpr int256 signed_builtin {-42};
    std::cout << "From builtin (-42): " << signed_builtin << std::endl;

    // Signed from the four words, most significant first. The top word is
    // signed, so int256{-1, 0, 0, 0} works; read the sign back with signed_high().
    constexpr int256 min_value {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
    std::cout << "From words (INT64_MIN, 0, 0, 0): " << min_value << std::endl;
    std::cout << "  Equals numeric_limits min? " << std::boolalpha
              << (min_value == (std::numeric_limits<int256>::min)()) << std::endl;
    std::cout << "  signed_high(): " << min_value.signed_high() << std::endl;

    // Signed literals. Values that fit in unsigned long long can be written
    // directly; the leading minus is parsed as a unary operator on the
    // literal result (lowercase and uppercase suffixes both work):
    const auto negative_literal {-12345_i256};
    std::cout << "From literal -12345_i256: " << negative_literal << std::endl;

    const auto positive_literal {12345_I256};
    std::cout << "From literal 12345_I256: " << positive_literal << std::endl;

    // For magnitudes beyond unsigned long long, use a string literal or the macro
    const auto large_signed_string {"-99999999999999999999"_i256};
    std::cout << "From string literal: " << large_signed_string << std::endl;

    const auto from_signed_macro {BOOST_INT256_INT256_C(-99999999999999999999)};
    std::cout << "From BOOST_INT256_INT256_C(-99999999999999999999): " << from_signed_macro << std::endl;

    // A raw (unquoted) literal never sees a leading '-': -X_i256 is unary
    // minus applied to X_i256, so X itself must be at most MAX in every
    // base. The minimum value's magnitude, 2^255, is one more than MAX, so
    // -57896...968_i256 and -0x8000...0_i256 (unquoted) are BOTH ill-formed,
    // the same way -9223372036854775808LL is for a built-in long long.
    // A quoted string literal embeds the sign in the digit sequence instead,
    // so it (and the macro, which stringifies its whole argument including
    // the sign) can reach MIN directly:
    const auto min_from_hex {"-0x8000000000000000000000000000000000000000000000000000000000000000"_i256};
    std::cout << "From hex string literal (-0x8, 63 zeros): " << min_from_hex << std::endl;
    std::cout << "  Equals numeric_limits min? " << std::boolalpha
              << (min_from_hex == (std::numeric_limits<int256>::min)()) << std::endl;

    // BOOST_INT256_INT256_MIN and BOOST_INT256_INT256_MAX are the simplest
    // way to spell the extremes and never risk this trap
    std::cout << "BOOST_INT256_INT256_MIN: " << BOOST_INT256_INT256_MIN << std::endl;
    std::cout << "BOOST_INT256_INT256_MAX: " << BOOST_INT256_INT256_MAX << std::endl;
    std::cout << "  MIN equals numeric_limits min? "
              << (BOOST_INT256_INT256_MIN == (std::numeric_limits<int256>::min)()) << std::endl;

    std::cout << "\n=== Default and Copy Construction ===" << std::endl;

    // Default construction (zero-initialized)
    constexpr uint256 default_constructed {};
    constexpr int256 signed_default_constructed {};
    std::cout << "Default constructed (uint256): " << default_constructed << std::endl;
    std::cout << "Default constructed (int256): " << signed_default_constructed << std::endl;

    // Copy construction
    const uint256 copied {from_macro};
    const int256 signed_copied {from_signed_macro};
    std::cout << "Copy constructed (uint256): " << copied << std::endl;
    std::cout << "Copy constructed (int256): " << signed_copied << std::endl;

    std::cout << "\n=== Floating-Point Construction ===" << std::endl;

    // Floating-point construction truncates toward zero, matching the behavior of
    // a static_cast from a floating-point type to a built-in integer.
    constexpr uint256 from_double {12345.9};
    std::cout << "uint256 from 12345.9 (truncated): " << from_double << std::endl;

    constexpr int256 from_negative_double {-12345.9};
    std::cout << "int256 from -12345.9 (truncated toward zero): " << from_negative_double << std::endl;

    // Values that exceed the 64-bit range are routed through the full 256-bit decomposition.
    const double two_to_the_200 {1.6069380442589903e60};  // 2^200
    const uint256 large_from_double {two_to_the_200};
    std::cout << "uint256 from 2^200: " << large_from_double << std::endl;

    std::cout << "\n=== Floating-Point Edge Cases ===" << std::endl;

    // NaN yields zero for both types (mirrors libgcc's __fix(uns)Xfti).
    const double nan_value {std::numeric_limits<double>::quiet_NaN()};
    const uint256 unsigned_from_nan {nan_value};
    const int256 signed_from_nan {nan_value};
    std::cout << "uint256 from NaN: " << unsigned_from_nan << std::endl;
    std::cout << "int256 from NaN: " << signed_from_nan << std::endl;

    // Negative values are clamped to zero when constructing uint256.
    const uint256 unsigned_from_negative {-1.0};
    std::cout << "uint256 from -1.0 (clamped to zero): " << unsigned_from_negative << std::endl;

    // Positive overflow saturates: anything >= 2^256 (including +infinity) becomes the max.
    const double infinity {std::numeric_limits<double>::infinity()};
    const uint256 saturated {infinity};
    std::cout << "uint256 from +infinity (saturates): " << saturated << std::endl;

    const double huge {1e80};  // Well beyond 2^256 (about 1.16e77)
    const uint256 saturated_from_huge {huge};
    std::cout << "uint256 from 1e80 (saturates): " << saturated_from_huge << std::endl;

    // For int256, values >= 2^255 saturate to INT256_MAX and values <= -2^255
    // saturate to INT256_MIN. 1e80 is well beyond 2^255 (~5.79e76) in magnitude.
    const int256 saturated_positive {huge};
    const int256 saturated_negative {-huge};
    std::cout << "int256 from 1e80 (saturates to max): " << saturated_positive << std::endl;
    std::cout << "int256 from -1e80 (saturates to min): " << saturated_negative << std::endl;

    return 0;
}
