// Copyright 2023 Peter Dimov
// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256.hpp>
#include <cstddef>
#include <limits>

void test_odr_use( int const* );
void test_odr_use( std::size_t const* );

template<typename T> void test()                        // LCOV_EXCL_LINE
{
    test_odr_use( &std::numeric_limits<T>::digits10 );  // LCOV_EXCL_LINE
}

void f2()
{
    test<boost::int256::uint256>();

// int256 is only forward declared until detail/int256_imp.hpp is wired into
// int256.hpp; gated on that header's guard so this file keeps compiling before
// and after it lands.
#if defined(BOOST_INT256_DETAIL_INT256_IMP_HPP)
    test<boost::int256::int256>();
#endif
}
