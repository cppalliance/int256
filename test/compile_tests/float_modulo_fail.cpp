// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// The built-in integers offer no modulo with a floating point operand, so neither
// does uint256. The overload is deleted rather than absent, which is what stops
// the implicit floating point constructor from silently truncating the operand.
// This must fail to compile.

#include <boost/int256/int256.hpp>

int main()
{
    const boost::int256::uint256 x {7U};

    return static_cast<int>(x % 2.0);
}
