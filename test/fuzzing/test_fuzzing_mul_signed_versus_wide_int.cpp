///////////////////////////////////////////////////////////////////////////////
//  Copyright Matt Borland 2026.
//  Copyright Christopher Kormanyos 2024 - 2026.
//  Distributed under the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt
//  or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Differential fuzzing of int256 multiplication against
// math::wide_integer::uintwide_t<256U, std::uint32_t, void, true>, the signed
// flavor of the independent 256-bit, 32-bit-limb implementation that the
// unsigned fuzzers check against. The two's complement bits of a wrapped result
// do not depend on the signedness, but int256 has its own operator
// implementations, so they get their own harness rather than riding on the
// uint256 fuzzers. Every operand pair is defined, including MIN and -1.
//
// Build and run with:
//
//   git clone https://github.com/ckormanyos/wide-integer $SCRATCH/wide-integer
//   clang++ -std=c++20 -g -O2 -fsanitize=fuzzer \
//     -I $SCRATCH/wide-integer -I libs/int256/include \
//     libs/int256/test/fuzzing/test_fuzzing_mul_signed_versus_wide_int.cpp -o fuzz_mul_signed
//   ./fuzz_mul_signed -max_total_time=1200 -max_len=64

#include <math/wide_integer/uintwide_t.h>

#include <boost/int256.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>

extern "C"
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

namespace fuzzing
{
  template <typename CntrlIntType,
            typename LocalIntType>
  auto eval_op(const CntrlIntType& a_cntrl,
               const CntrlIntType& b_cntrl,
               const LocalIntType& a_local,
               const LocalIntType& b_local) -> bool;
}

template <typename CntrlIntType,
          typename LocalIntType>
auto fuzzing::eval_op(const CntrlIntType& a_cntrl,
                      const CntrlIntType& b_cntrl,
                      const LocalIntType& a_local,
                      const LocalIntType& b_local) -> bool
{
  using cntrl_int_type = CntrlIntType;
  using local_int_type = LocalIntType;

  static_assert
  (
       (std::numeric_limits<cntrl_int_type>::digits == std::numeric_limits<local_int_type>::digits)
    && (std::numeric_limits<local_int_type>::digits == int { INT32_C(255) }),
    "Error: the control and local types must both have 255 magnitude bits (256 total, one sign bit)"
  );

  const local_int_type result_local { local_int_type(a_local) *= b_local };
  const cntrl_int_type result_cntrl { cntrl_int_type(a_cntrl) *= b_cntrl };

  // Compare the bit pattern one 64-bit word at a time, so a mismatch in any
  // single word is caught without depending on either type's formatting.
  bool result_is_ok { true };

  for (unsigned i { }; i < 4U; ++i)
  {
    const auto shift { unsigned { UINT8_C(64) } * i };

    const std::uint64_t word_local { static_cast<std::uint64_t>(result_local >> shift) };
    const std::uint64_t word_cntrl { static_cast<std::uint64_t>(result_cntrl >> shift) };

    result_is_ok = ((word_local == word_cntrl) && result_is_ok);
  }

  if(!result_is_ok)
  {
    std::cout << "Error: lhs: " << a_local
              << ", rhs: " << b_local
              << ", result obtained: " << result_local
              << ", result expected: " << result_cntrl << std::endl;
  }

  return result_is_ok;
}

// The fuzzing entry point.
extern "C"
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  constexpr std::size_t max_size { UINT8_C(64) };
  constexpr std::size_t min_size { UINT8_C(33) };

  bool result_is_ok { true };

  if(((size >= min_size) && (size <= max_size)) && (data != nullptr))
  {
    using local_data_array_type = std::array<std::uint8_t, max_size>;

    local_data_array_type tmp_data { };

    tmp_data.fill(UINT8_C(0));

    static_cast<void>(std::copy(data, data + size, tmp_data.begin()));

    // Eight 64-bit words: a is words 0 through 3, b is words 4 through 7.
    // std::memcpy rather than a reinterpret_cast, so this stays defined.
    std::uint64_t words[8] { };
    std::memcpy(words, tmp_data.data(), sizeof(words));

    using local_uint_type = ::boost::int256::uint256;
    using local_int_type = ::boost::int256::int256;

    #if defined(WIDE_INTEGER_NAMESPACE)
    using cntrl_uint_type = ::WIDE_INTEGER_NAMESPACE::math::wide_integer::uintwide_t<256U, std::uint32_t>;
    using cntrl_int_type = ::WIDE_INTEGER_NAMESPACE::math::wide_integer::uintwide_t<256U, std::uint32_t, void, true>;
    #else
    using cntrl_uint_type = ::math::wide_integer::uintwide_t<256U, std::uint32_t>;
    using cntrl_int_type = ::math::wide_integer::uintwide_t<256U, std::uint32_t, void, true>;
    #endif

    // The bit pattern is assembled once, as an unsigned value, then bit-copied
    // into both the local and the control signed types. This keeps the same
    // random 256-bit corpus that the unsigned fuzzers already exercise, and it
    // sidesteps any implementation-defined narrowing from std::uint64_t to
    // std::int64_t for the top word.
    local_uint_type a_bits { words[3], words[2], words[1], words[0] };
    local_uint_type b_bits { words[7], words[6], words[5], words[4] };

    local_int_type a_local { ::boost::int256::detail::to_signed_bits(a_bits) };
    local_int_type b_local { ::boost::int256::detail::to_signed_bits(b_bits) };

    cntrl_uint_type a_cntrl_bits { };
    cntrl_uint_type b_cntrl_bits { };

    // Most significant word first, so the shift is applied before each word is
    // merged in.
    for (unsigned i { }; i < 4U; ++i)
    {
      a_cntrl_bits <<= unsigned { UINT8_C(64) };
      a_cntrl_bits |= cntrl_uint_type(words[3U - i]);

      b_cntrl_bits <<= unsigned { UINT8_C(64) };
      b_cntrl_bits |= cntrl_uint_type(words[7U - i]);
    }

    cntrl_int_type a_cntrl { };
    cntrl_int_type b_cntrl { };
    std::memcpy(&a_cntrl, &a_cntrl_bits, sizeof(a_cntrl));
    std::memcpy(&b_cntrl, &b_cntrl_bits, sizeof(b_cntrl));

    const bool result_op_is_ok { fuzzing::eval_op(a_cntrl, b_cntrl, a_local, b_local) };

    if(!result_op_is_ok)
    {
      assert(result_op_is_ok);
    }

    result_is_ok = (result_op_is_ok && result_is_ok);
  }

  return (result_is_ok ? 0 : -1);
}
