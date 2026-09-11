// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Adapted from Boost.Int128 (Copyright 2022 Peter Dimov, Copyright 2025 Matt Borland)

#ifndef BOOST_INT256_DETAIL_LITERAL_MACROS_HPP
#define BOOST_INT256_DETAIL_LITERAL_MACROS_HPP

// Convenience macros for the user-defined literals. This header intentionally has
// no includes and declares nothing, so module consumers can pull it in on its own
// to obtain the macros (macros are never part of a module's exported interface).

#define BOOST_INT256_STRINGIFY(x) #x
#define BOOST_INT256_UINT256_C(x) boost::int256::literals::operator""_u256(BOOST_INT256_STRINGIFY(x))

#endif // BOOST_INT256_DETAIL_LITERAL_MACROS_HPP
