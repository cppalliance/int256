// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256.hpp>
#include <iostream>

int main()
{
    // 2^128 is one past the largest value a 128-bit integer can hold
    const boost::int256::uint256 x {boost::int256::uint256{1} << 128U};
    const boost::int256::uint256 y {42};

    std::cout << x << " + " << y << " = " << x + y << std::endl;

    return 0;
}
