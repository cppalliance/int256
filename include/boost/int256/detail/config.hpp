// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT256_DETAIL_CONFIG_HPP
#define BOOST_INT256_DETAIL_CONFIG_HPP

// A handful of detail-namespace entities are exercised directly by the module
// test suite. BOOST_INT256_TEST_EXPORT exports them only when the module is built
// for testing (BOOST_INT256_EXPORT_TESTING), so the normal module API stays limited
// to the public interface. It expands to nothing in ordinary (header) builds.
#if defined(BOOST_INT256_BUILD_MODULE) && defined(BOOST_INT256_EXPORT_TESTING)
#  define BOOST_INT256_TEST_EXPORT export
#else
#  define BOOST_INT256_TEST_EXPORT
#endif

#ifdef BOOST_INT256_BUILD_MODULE
#  define BOOST_INT256_INLINE_CONSTEXPR inline constexpr
#  define BOOST_INT256_EXPORT export
#else
#  define BOOST_INT256_INLINE_CONSTEXPR static constexpr
#  define BOOST_INT256_EXPORT
#endif

// <type_traits> has to be visible before __cpp_lib_is_constant_evaluated is tested:
// on libstdc++ the feature test macro is only defined once a library header is seen
#ifndef BOOST_INT256_BUILD_MODULE

#include <type_traits>
#include <cassert>

#endif

// The SYCL device target (spir64) has no native 128-bit integer, so force the portable
// code path on the device pass. This mirrors a user-supplied BOOST_INT256_NO_BUILTIN_INT128
// and keeps host and device selection consistent even though __x86_64__ stays defined on device.
#if defined(__SYCL_DEVICE_ONLY__) && !defined(BOOST_INT256_NO_BUILTIN_INT128)
#  define BOOST_INT256_NO_BUILTIN_INT128
#endif

// Use the builtin 128-bit integer for 64x64 -> 128 intermediates and for API parity
// (constructors, conversion operators and the mixed operator flavors)
#if (defined(BOOST_HAS_INT128) || (defined(__SIZEOF_INT128__) && !defined(_MSC_VER))) && !defined(__SYCL_DEVICE_ONLY__) && !defined(BOOST_INT256_NO_BUILTIN_INT128)

#define BOOST_INT256_HAS_INT128

#define BOOST_INT256_BUILTIN_CONSTEXPR constexpr

namespace boost {
namespace int256 {
namespace detail {

// A module consumer receives these aliases from the import, so only declare them
// in ordinary builds and in the module interface unit itself; declaring them again
// in a consumer would give a second, distinct type and break overload resolution.
#if !defined(BOOST_INT256_BUILD_MODULE) || defined(BOOST_INT256_INTERFACE_UNIT)

// Avoids pedantic warnings
#ifdef __GNUC__

BOOST_INT256_TEST_EXPORT __extension__ using builtin_i128 = __int128;
BOOST_INT256_TEST_EXPORT __extension__ using builtin_u128 = unsigned __int128;

#else

BOOST_INT256_TEST_EXPORT using builtin_i128 = __int128;
BOOST_INT256_TEST_EXPORT using builtin_u128 = unsigned __int128;

#endif

#endif // declare builtin aliases

} // namespace detail
} // namespace int256
} // namespace boost

#elif defined(_MSVC_LANG) && _MSVC_LANG >= 202002L && __has_include(<__msvc_int128.hpp>) && !defined(__SYCL_DEVICE_ONLY__)

#ifndef BOOST_INT256_BUILD_MODULE
#include <__msvc_int128.hpp>
#endif

#define BOOST_INT256_HAS_MSVC_INT128

#if _MSC_VER >= 1945
#define BOOST_INT256_BUILTIN_CONSTEXPR constexpr
#else
#define BOOST_INT256_BUILTIN_CONSTEXPR inline
#endif

namespace boost {
namespace int256 {
namespace detail {

// See the note above: skip the re-declaration in a module consumer.
#if !defined(BOOST_INT256_BUILD_MODULE) || defined(BOOST_INT256_INTERFACE_UNIT)

BOOST_INT256_TEST_EXPORT using builtin_i128 = std::_Signed128;
BOOST_INT256_TEST_EXPORT using builtin_u128 = std::_Unsigned128;

#endif

} // namespace detail
} // namespace int256
} // namespace boost

#endif // builtin 128-bit detection

// Determine endianness. The word order of uint256 is fixed on every platform, so this
// is only consumed by the byte conversion functions
#if defined(_WIN32)

#define BOOST_INT256_ENDIAN_BIG_BYTE 0
#define BOOST_INT256_ENDIAN_LITTLE_BYTE 1

#elif defined(__BYTE_ORDER__)

#define BOOST_INT256_ENDIAN_BIG_BYTE (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define BOOST_INT256_ENDIAN_LITTLE_BYTE (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

#else

#error Could not determine endian type. Please file an issue at https://github.com/cppalliance/int256 with your architecture

#endif // Determine endianness

// Is constant evaluated detection
#ifdef __cpp_lib_is_constant_evaluated
#  define BOOST_INT256_HAS_IS_CONSTANT_EVALUATED
#endif

#ifdef __has_builtin
#  if __has_builtin(__builtin_is_constant_evaluated)
#    define BOOST_INT256_HAS_BUILTIN_IS_CONSTANT_EVALUATED
#  endif
#endif

//
// MSVC also supports __builtin_is_constant_evaluated if it's recent enough:
//
#if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 192528326)
#  define BOOST_INT256_HAS_BUILTIN_IS_CONSTANT_EVALUATED
#endif

//
// As does GCC-9:
//
#if defined(__GNUC__) && (__GNUC__ >= 9) && !defined(BOOST_INT256_HAS_BUILTIN_IS_CONSTANT_EVALUATED)
#  define BOOST_INT256_HAS_BUILTIN_IS_CONSTANT_EVALUATED
#endif

#if defined(BOOST_INT256_HAS_IS_CONSTANT_EVALUATED) && !(defined(__CUDACC__) && defined(BOOST_INT256_ENABLE_CUDA))
#  define BOOST_INT256_IS_CONSTANT_EVALUATED(x) std::is_constant_evaluated()
#elif defined(BOOST_INT256_HAS_BUILTIN_IS_CONSTANT_EVALUATED) && !(defined(__CUDACC__) && defined(BOOST_INT256_ENABLE_CUDA))
#  define BOOST_INT256_IS_CONSTANT_EVALUATED(x) __builtin_is_constant_evaluated()
#else
#  define BOOST_INT256_IS_CONSTANT_EVALUATED(x) false
#  define BOOST_INT256_NO_CONSTEVAL_DETECTION
#endif

// https://github.com/llvm/llvm-project/issues/55638
#if defined(__clang__) && __cplusplus > 202002L && __clang_major__ < 17
#  undef BOOST_INT256_IS_CONSTANT_EVALUATED
#  define BOOST_INT256_IS_CONSTANT_EVALUATED(x) false
#  define BOOST_INT256_NO_CONSTEVAL_DETECTION
#endif

#if defined(_MSC_VER)
#  define BOOST_INT256_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#  define BOOST_INT256_FORCE_INLINE __attribute__((always_inline)) inline
#else
#  define BOOST_INT256_FORCE_INLINE inline
#endif

// MinGW defines the MSVC platform macros (_M_AMD64, _M_IX86, _M_ARM64) for source
// compatibility, but it provides the GNU intrinsics rather than the MSVC ones. Every
// guard selecting an MSVC-only intrinsic (_umul128, __umulh, _udiv128, _BitScan*,
// __popcnt*, ...) therefore has to exclude GNU-mode compilers with !defined(__GNUC__).
#ifdef __x86_64__

#ifndef BOOST_INT256_BUILD_MODULE
#  include <x86intrin.h>
#  include <emmintrin.h>
#  ifdef __AVX2__
#    include <immintrin.h>
#  endif
#endif

#elif defined(_M_AMD64) && !defined(__GNUC__)

#ifndef BOOST_INT256_BUILD_MODULE
#  include <intrin.h>
#endif

#elif defined(__i386__)

#ifndef BOOST_INT256_BUILD_MODULE
#  ifdef __SSE2__
#    include <emmintrin.h>
#  endif
#endif

#elif defined(_M_IX86) && !defined(__GNUC__)

#ifndef BOOST_INT256_BUILD_MODULE
#  include <intrin.h>
#endif

#endif // Platform macros

// The add-with-carry and subtract-with-borrow intrinsics take an unsigned long long
// out-param, so every caller has to use unsigned long long locals. They do not exist
// on the CUDA or SYCL device targets.
#if (defined(__x86_64__) || (defined(_M_AMD64) && !defined(__GNUC__))) && !defined(__CUDA_ARCH__) && !defined(__SYCL_DEVICE_ONLY__)

#  ifdef __ADX__
#    define BOOST_INT256_ADD_CARRY _addcarryx_u64
#    define BOOST_INT256_SUB_BORROW _subborrow_u64
#  else
#    define BOOST_INT256_ADD_CARRY _addcarry_u64
#    define BOOST_INT256_SUB_BORROW _subborrow_u64
#  endif

#endif // Carry intrinsics

// Hardware 128-bit by 64-bit unsigned division via the x86-64 DIV instruction.
// Excluded on the CUDA and SYCL device passes (the device target is not x86-64)
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER) && !defined(__CUDA_ARCH__) && !defined(__SYCL_DEVICE_ONLY__)
#  define BOOST_INT256_HAS_X86_64_DIVQ
#endif

// The same operation through the MSVC intrinsic
#if defined(_M_AMD64) && !defined(__GNUC__) && !defined(__clang__) && defined(_MSC_VER) && _MSC_VER >= 1920 && !defined(__CUDA_ARCH__)
#  define BOOST_INT256_HAS_MSVC_UDIV128
#endif

#if defined(BOOST_INT256_HAS_X86_64_DIVQ) || defined(BOOST_INT256_HAS_MSVC_UDIV128)
#  define BOOST_INT256_HAS_FAST_DIV128
#endif

// The builtin is only constexpr from clang-7 or GCC-10
#ifdef __has_builtin
#  if __has_builtin(__builtin_sub_overflow) && ((defined(__clang__) && __clang_major__ >= 7) || (defined(__GNUC__) && __GNUC__ >= 10))
#    define BOOST_INT256_HAS_BUILTIN_SUB_OVERFLOW
#  endif
#  if __has_builtin(__builtin_add_overflow) && ((defined(__clang__) && __clang_major__ >= 7) || (defined(__GNUC__) && __GNUC__ >= 10))
#    define BOOST_INT256_HAS_BUILTIN_ADD_OVERFLOW
#  endif
// The carry chain builtins are never constexpr, so they are runtime only
#  if __has_builtin(__builtin_addcll) && __has_builtin(__builtin_subcll)
#    define BOOST_INT256_HAS_BUILTIN_ADDCLL
#  endif
#endif

#if defined(__cpp_if_constexpr) && __cpp_if_constexpr >= 201606L
#  define BOOST_INT256_HAS_IF_CONSTEXPR
#endif // if constexpr detection

#define BOOST_INT256_ASSERT(x) assert(x)
#define BOOST_INT256_ASSERT_MSG(expr, msg) assert((expr) && (msg))

#ifdef _MSC_VER
#  define BOOST_INT256_ASSUME(expr) __assume(expr)
#elif defined(__clang__)
#  define BOOST_INT256_ASSUME(expr) __builtin_assume(expr)
#elif defined(__GNUC__)
#  if __GNUC__ >= 5 && __GNUC__ < 13
#    define BOOST_INT256_ASSUME(expr) if (expr) {} else { __builtin_unreachable(); }
#  else
#    define BOOST_INT256_ASSUME(expr) __attribute__((assume(expr)))
#  endif
#elif defined(__has_cpp_attribute)
#  if __has_cpp_attribute(assume)
#    define BOOST_INT256_ASSUME(expr) [[assume(expr)]]
#  else
#    define BOOST_INT256_ASSUME(expr) BOOST_INT256_ASSERT(expr)
#  endif
#else
#  define BOOST_INT256_ASSUME(expr) BOOST_INT256_ASSERT(expr)
#endif

#if defined(__has_builtin)
#define BOOST_INT256_HAS_BUILTIN(x) __has_builtin(x)
#else
#define BOOST_INT256_HAS_BUILTIN(x) false
#endif

#if BOOST_INT256_HAS_BUILTIN(__builtin_expect)
#  define BOOST_INT256_LIKELY(x) __builtin_expect(x, 1)
#  define BOOST_INT256_UNLIKELY(x) __builtin_expect(x, 0)
#else
#  define BOOST_INT256_LIKELY(x) x
#  define BOOST_INT256_UNLIKELY(x) x
#endif

#if !defined(__cpp_if_constexpr) || (__cpp_if_constexpr < 201606L)
#  define BOOST_INT256_NO_CXX17_IF_CONSTEXPR
#endif

#ifndef BOOST_INT256_NO_CXX17_IF_CONSTEXPR
#  define BOOST_INT256_IF_CONSTEXPR if constexpr
#else
#  define BOOST_INT256_IF_CONSTEXPR if
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define BOOST_INT256_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#  define BOOST_INT256_UNREACHABLE __assume(0)
#else
#  define BOOST_INT256_UNREACHABLE std::abort()
#endif

// SIMD paths are only used at runtime, so they need constant evaluation detection to
// gate them, and they do not exist on the GPU device targets
#if !defined(BOOST_INT256_NO_SIMD) && !defined(BOOST_INT256_NO_CONSTEVAL_DETECTION) && !defined(__CUDA_ARCH__) && !defined(__SYCL_DEVICE_ONLY__)

#  if defined(__AVX2__) && (defined(__x86_64__) || defined(__i386__) || defined(_M_AMD64) || defined(_M_IX86))
#    define BOOST_INT256_HAS_AVX2
#  endif

#  if defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#    define BOOST_INT256_HAS_SSE2
#  endif

#endif // SIMD detection

// Detect if we can throw or not
// First check if the user said no explicitly
// Then check if it's been disabled elsewhere

#ifdef BOOST_INT256_DISABLE_EXCEPTIONS

#  define BOOST_INT256_THROW_EXCEPTION(expr)

#else

#  ifdef _MSC_VER
#    ifdef _CPPUNWIND
#      define BOOST_INT256_THROW_EXCEPTION(expr) throw expr;
#    else
#      define BOOST_INT256_THROW_EXCEPTION(expr)
#      define BOOST_INT256_DISABLE_EXCEPTIONS
#    endif
#  else
#    ifdef __EXCEPTIONS
#      define BOOST_INT256_THROW_EXCEPTION(expr) throw expr;
#    else
#      define BOOST_INT256_THROW_EXCEPTION(expr)
#      define BOOST_INT256_DISABLE_EXCEPTIONS
#    endif
#  endif

#endif // Exceptions

#if defined(__cpp_impl_three_way_comparison) && __cpp_impl_three_way_comparison >= 201907L && __has_include(<compare>)
#  define BOOST_INT256_HAS_SPACESHIP_OPERATOR
#  ifndef BOOST_INT256_BUILD_MODULE
#    include <compare>
#  endif
#endif

// GPU device support. CUDA is auto-detected via __CUDACC__ (opt-in with
// BOOST_INT256_ENABLE_CUDA). SYCL is fully opt-in via BOOST_INT256_ENABLE_SYCL;
// <sycl/sycl.hpp> must be included before <boost/int256.hpp> so SYCL_EXTERNAL exists.
#if defined(__CUDACC__) && defined(BOOST_INT256_ENABLE_CUDA)
#  define BOOST_INT256_CUDA_ENABLED __host__ __device__
#  define BOOST_INT256_HAS_GPU_SUPPORT
#elif defined(BOOST_INT256_ENABLE_SYCL)
#  define BOOST_INT256_SYCL_ENABLED SYCL_EXTERNAL
#  define BOOST_INT256_HAS_GPU_SUPPORT
#endif

#ifndef BOOST_INT256_CUDA_ENABLED
#  define BOOST_INT256_CUDA_ENABLED
#endif
#ifndef BOOST_INT256_SYCL_ENABLED
#  define BOOST_INT256_SYCL_ENABLED
#endif

// Exactly one sub-macro is ever non-empty; expands to "__host__ __device__" (CUDA),
// "SYCL_EXTERNAL" (SYCL), or nothing (host).
#define BOOST_INT256_HOST_DEVICE BOOST_INT256_CUDA_ENABLED BOOST_INT256_SYCL_ENABLED

#endif // BOOST_INT256_DETAIL_CONFIG_HPP
