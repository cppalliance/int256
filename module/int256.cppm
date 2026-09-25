// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

module;

// The platform intrinsic headers and the MSVC built-in 128-bit header are not
// part of the standard library module, so they are always brought in here.
#if __has_include(<__msvc_int128.hpp>) && _MSVC_LANG >= 202002L

#include <__msvc_int128.hpp>

#endif

#ifdef __x86_64__
#  include <x86intrin.h>
#  include <emmintrin.h>
#  ifdef __AVX2__
#    include <immintrin.h>
#  endif
#elif defined(_M_AMD64) && !defined(__GNUC__)
#  include <intrin.h>
#elif defined(__i386__)
#  include <emmintrin.h>
#elif defined(_M_IX86) && !defined(__GNUC__)
#  include <intrin.h>
#endif

// import std exports declarations but not macros. The headers below provide the
// feature-test macros and the object-like macros the library uses (assert, errno,
// UINT64_C, ...) so they are included regardless of whether import std is used.
#include <version>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdint>

// When the standard library module is available these are provided by import std,
// otherwise they are supplied here in the global module fragment. This is exactly
// the set of standard headers that include/ includes inside a
// #ifndef BOOST_INT256_BUILD_MODULE guard, minus the five above that are needed
// for their macros either way. Regenerate it after adding a guarded include with
//   grep -rn -B1 '#include <' include | grep -A1 BUILD_MODULE
// <stdfloat> below is the one exception: it is itself conditionally included in
// every header that uses it, so the grep recipe above will not surface it (or
// reproduce its condition) on its own. Add and maintain that block by hand.
#ifndef BOOST_INT256_USE_STD_MODULE

#include <array>
#include <compare>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <format>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

// Unlike its neighbours above, this one is conditional: <stdfloat> only exists from
// C++23, and only when the standard library ships it.
#if (__cplusplus > 202002L || (defined(_MSVC_LANG) && _MSVC_LANG > 202002L)) && defined(__has_include) && __has_include(<stdfloat>)
#include <stdfloat>
#endif

#endif

#define BOOST_INT256_BUILD_MODULE

// Marks this translation unit as the module interface unit (as opposed to a
// consumer that imports the module). Some headers key off this to declare
// entities here that consumers instead receive through the import.
#define BOOST_INT256_INTERFACE_UNIT

export module boost.int256;

#ifdef BOOST_INT256_USE_STD_MODULE
import std;
#endif

// Wrap the exported declarations in extern "C++" so they attach to the global
// module (classic mangling), keeping the module and header forms ABI compatible.
extern "C++" {

export namespace boost::int256 {

struct uint256;
struct int256;

} // namespace boost::int256

export namespace std {

template <>
class numeric_limits<boost::int256::uint256>;

template <>
class numeric_limits<boost::int256::int256>;

} // namespace std

#ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable : 5244 )
#elif defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Winclude-angled-in-module-purview"
#endif

#include <boost/int256.hpp>

#ifdef _MSC_VER
#  pragma warning( pop )
#elif defined(__clang__)
#  pragma clang diagnostic pop
#endif

} // extern "C++"
