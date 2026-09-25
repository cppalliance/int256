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

// int256 is only forward declared until detail/int256_imp.hpp is wired into
// int256.hpp; the section below is gated on that header's guard so this file
// keeps compiling before and after it lands.
#if defined(BOOST_INT256_DETAIL_INT256_IMP_HPP)
#  define BOOST_INT256_TEST_HAS_SIGNED 1
using boost::int256::int256;
#endif

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

#if defined(BOOST_INT256_TEST_HAS_SIGNED)

constexpr int256 i_zero {};
constexpr int256 i_one {INT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(1)};
constexpr int256 i_minus_one {-1};
constexpr int256 i_min {INT64_MIN, UINT64_C(0), UINT64_C(0), UINT64_C(0)};
constexpr int256 i_max {INT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};

static_assert((std::numeric_limits<int256>::max)() == i_max, "numeric_limits max");
static_assert((std::numeric_limits<int256>::min)() == i_min, "numeric_limits min");
static_assert(std::numeric_limits<int256>::is_modulo, "wrapping is intended");
static_assert(std::numeric_limits<int256>::is_signed, "int256 is signed");

// MIN's magnitude is not representable, so negation and abs wrap it back to
// itself, exactly like the built-in signed integers.
static_assert(-i_min == i_min, "-MIN wraps to MIN");
static_assert(boost::int256::abs(i_min) == i_min, "abs(MIN) wraps to MIN");
static_assert(boost::int256::abs(i_min + i_one) == i_max, "abs(MIN + 1) == MAX");
static_assert(boost::int256::abs(i_max) == i_max, "abs(MAX) == MAX");
static_assert(boost::int256::abs(i_minus_one) == i_one, "abs(-1) == 1");

// MIN / -1 wraps to MIN, and MIN % -1 is exactly zero.
static_assert(i_min / i_minus_one == i_min, "MIN / -1 wraps to MIN");
static_assert(i_min % i_minus_one == i_zero, "MIN % -1 == 0");

// Wraparound at the signed range boundary.
static_assert(i_max + i_one == i_min, "MAX + 1 wraps to MIN");
static_assert(i_min - i_one == i_max, "MIN - 1 wraps to MAX");

// Arithmetic right shift sign extends; left shift is logical (unsigned
// domain), regardless of the operand's sign.
static_assert((i_minus_one >> 255U) == i_minus_one, "-1 >> 255 == -1");
static_assert((i_min >> 1U) == int256(static_cast<std::int64_t>(UINT64_C(0xC000000000000000)), UINT64_C(0), UINT64_C(0), UINT64_C(0)),
              "MIN >> 1 sign extends");
static_assert((i_min << 1U) == i_zero, "MIN << 1 shifts the sign bit out");

// A negative scalar sign fills, matching the builtin signed integers exactly
// (no widening surprise the way uint256{-1} has).
static_assert(int256{-1} == i_minus_one, "int256{-1} == -1");

// The signed mixed paths must agree with promoting the scalar first,
// including for the most negative int64_t, which cannot be negated inside its
// own type.
constexpr std::int64_t i_scalar_min {(std::numeric_limits<std::int64_t>::min)()};
constexpr int256 i_scalar_min_256 {i_scalar_min};
constexpr int256 ia {INT64_C(42), UINT64_C(0), UINT64_C(1234), UINT64_C(7)};
constexpr int256 ib {INT64_C(0), UINT64_C(0), UINT64_C(0), UINT64_C(5)};

static_assert(ia + i_scalar_min == ia + i_scalar_min_256, "operator+(int256, Signed)");
static_assert(i_scalar_min + ia == i_scalar_min_256 + ia, "operator+(Signed, int256)");
static_assert(ia - i_scalar_min == ia - i_scalar_min_256, "operator-(int256, Signed)");
static_assert(i_scalar_min - ia == i_scalar_min_256 - ia, "operator-(Signed, int256)");
static_assert(ia * i_scalar_min == ia * i_scalar_min_256, "operator*(int256, Signed)");
static_assert(i_scalar_min * ia == i_scalar_min_256 * ia, "operator*(Signed, int256)");
static_assert(ia / i_scalar_min == ia / i_scalar_min_256, "operator/(int256, Signed)");
static_assert(ia % i_scalar_min == ia % i_scalar_min_256, "operator%(int256, Signed)");
static_assert(i_scalar_min / ib == i_scalar_min_256 / ib, "operator/(Signed, int256)");
static_assert(i_scalar_min % ib == i_scalar_min_256 % ib, "operator%(Signed, int256)");

// The division identity holds in constant evaluation.
static_assert(i_min / int256{-7} != i_zero, "constant-evaluated division runs");
static_assert(int256{-7} / int256{2} == int256{-3}, "-7 / 2 == -3 (truncating toward zero)");
static_assert(int256{-7} % int256{2} == int256{-1}, "-7 % 2 == -1 (remainder takes the dividend's sign)");
static_assert(int256{7} / int256{-2} == int256{-3}, "7 / -2 == -3");
static_assert(int256{7} % int256{-2} == int256{1}, "7 % -2 == 1");

#endif // BOOST_INT256_TEST_HAS_SIGNED

} // namespace

int main()
{
    return 0;
}
