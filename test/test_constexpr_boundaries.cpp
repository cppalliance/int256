//  Copyright 2026 Matt Borland
//  Distributed under the Boost Software License, Version 1.0.
//  https://www.boost.org/LICENSE_1_0.txt
//
// Compile-time counterpart of test_boundaries.cpp. Undefined behavior is not
// permitted in a constant expression, so anything undefined here is a
// compilation error rather than a silent difference, and the equalities are
// checked by the compiler as well.

#include <boost/int256.hpp>

#include <cstdint>
#include <limits>

#ifdef _MSC_VER
#  pragma warning(disable : 4307) // integral constant overflow
#  pragma warning(disable : 4146) // unary minus on an unsigned type, result still unsigned
#endif

using boost::int256::uint256;

namespace
{

constexpr uint256 zero {};
constexpr uint256 one {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
constexpr uint256 max {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
constexpr uint256 two_64 {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)};
constexpr uint256 two_128 {UINT64_C(0), UINT64_C(1), UINT64_C(0), UINT64_C(0)};
constexpr uint256 two_192 {UINT64_C(1), UINT64_C(0), UINT64_C(0), UINT64_C(0)};
constexpr uint256 two_255 {UINT64_C(0x8000000000000000), UINT64_C(0), UINT64_C(0), UINT64_C(0)};

constexpr std::int64_t scalar_min {(std::numeric_limits<std::int64_t>::min)()};
constexpr uint256 scalar_min_256 {scalar_min};
constexpr uint256 a {UINT64_C(42), UINT64_C(0), UINT64_C(1234), UINT64_C(7)};
constexpr uint256 b {UINT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(5)};

// numeric_limits agrees with the hand-written maximum.
static_assert((std::numeric_limits<uint256>::max)() == max, "numeric_limits max");
static_assert((std::numeric_limits<uint256>::min)() == zero, "numeric_limits min");
static_assert(std::numeric_limits<uint256>::is_modulo, "wrapping is intended");

// Two's complement rollover.
static_assert(max + one == zero, "MAX + 1 wraps to 0");
static_assert(zero - one == max, "0 - 1 wraps to MAX");
static_assert(max + 1U == zero, "MAX + 1 (scalar) wraps to 0");
static_assert(zero - 1U == max, "0 - 1 (scalar) wraps to MAX");
static_assert(max * 2U == max - one, "MAX * 2 wraps to MAX - 1");
static_assert(-zero == zero, "negating zero is zero");
static_assert(-max == one, "negating MAX gives 1");
static_assert(-one == max, "negating 1 gives MAX");
static_assert(~zero == max, "complement of zero is MAX");
static_assert(~max == zero, "complement of MAX is zero");

// A negative scalar sign fills, so this is well formed and equals the maximum.
static_assert(uint256{-1} == max, "uint256{-1} is the maximum");

// The mixed signed paths must agree with promoting the scalar first, including
// for the most negative value, which cannot be negated inside its own type.
static_assert(a + scalar_min == a + scalar_min_256, "operator+(uint256, Signed)");
static_assert(scalar_min + a == scalar_min_256 + a, "operator+(Signed, uint256)");
static_assert(a - scalar_min == a - scalar_min_256, "operator-(uint256, Signed)");
static_assert(scalar_min - a == scalar_min_256 - a, "operator-(Signed, uint256)");
static_assert(a * scalar_min == a * scalar_min_256, "operator*(uint256, Signed)");
static_assert(scalar_min * a == scalar_min_256 * a, "operator*(Signed, uint256)");
static_assert(a / scalar_min == a / scalar_min_256, "operator/(uint256, Signed)");
static_assert(a % scalar_min == a % scalar_min_256, "operator%(uint256, Signed)");
static_assert(scalar_min / b == scalar_min_256 / b, "operator/(Signed, uint256)");
static_assert(scalar_min % b == scalar_min_256 % b, "operator%(Signed, uint256)");

// A divisor whose low word is zero is not zero.
static_assert(std::uint64_t{5} / two_64 == zero, "operator/(Unsigned, uint256) by 2^64");
static_assert(std::uint64_t{5} % two_64 == b, "operator%(Unsigned, uint256) by 2^64");
static_assert(two_128 / two_64 == two_64, "2^128 / 2^64");
static_assert(two_192 / two_128 == two_64, "2^192 / 2^128");
static_assert(max / two_192 == uint256{UINT64_MAX}, "MAX / 2^192");

// Shifts at both ends of the defined range.
static_assert((two_255 << 1U) == zero, "shifting the top bit out");
static_assert((one << 255U) == two_255, "shifting into the top bit");
static_assert((two_255 >> 255U) == one, "shifting the top bit down");
static_assert((max >> 255U) == one, "MAX >> 255");
static_assert((max << 255U) == two_255, "MAX << 255");
static_assert((max << 0U) == max, "shifting by zero");
static_assert((max >> 0U) == max, "shifting by zero");

// The division identity holds in constant evaluation, which is the Knuth path
// because the reciprocal accelerator is gated to run time.
static_assert(max / (two_64 - 1U) == uint256(UINT64_C(1), UINT64_C(1), UINT64_C(1), UINT64_C(1)),
              "MAX / (2^64 - 1)");
static_assert(max % (two_64 - 1U) == zero, "MAX % (2^64 - 1)");
static_assert(max / (two_128 + 1U) == two_128 - 1U, "MAX / (2^128 + 1)");
static_assert(max / (two_192 - 1U) == two_64, "MAX / (2^192 - 1)");
static_assert(max % (two_192 - 1U) == two_64 - 1U, "MAX % (2^192 - 1)");
static_assert(max / 3U == uint256(UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
                                  UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)),
              "MAX / 3");

// Multiplication cascades.
static_assert(max * max == one, "MAX squared is 1");
static_assert((two_128 - 1U) * (two_128 - 1U) ==
              uint256(UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFE), UINT64_C(0), UINT64_C(1)),
              "(2^128 - 1) squared");
static_assert((two_192 + 1U) * (two_64 + 1U) == uint256(UINT64_C(1), UINT64_C(0), UINT64_C(1), UINT64_C(1)),
              "(2^192 + 1)(2^64 + 1)");
static_assert(two_255 * 2U == zero, "2^255 * 2 wraps to zero");
static_assert((one << 127U) * two_128 == two_255, "2^127 * 2^128");

} // namespace

int main()
{
    return 0;
}
