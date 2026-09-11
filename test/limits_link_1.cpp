// Copyright 2023 Peter Dimov
// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256.hpp>
#include <cstddef>
#include <limits>

void test_odr_use( int const* );
void test_odr_use( std::size_t const* );

template<typename T> void test()
{
    test_odr_use( &std::numeric_limits<T>::digits10 );
}

void f1()
{
    test<boost::int256::uint256>();
}
