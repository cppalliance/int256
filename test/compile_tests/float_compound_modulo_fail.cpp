// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// The compound modulo with a floating point operand is deleted as well, so the
// member function cannot be reached through the implicit constructor either.
// This must fail to compile.

#include <boost/int256/int256.hpp>

int main()
{
    boost::int256::uint256 x {7U};
    x %= 2.0;

    return static_cast<int>(x);
}
