// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// A shift by a floating point count is deleted, matching the built-in integers.
// This must fail to compile.

#include <boost/int256/int256.hpp>

int main()
{
    const boost::int256::uint256 x {7U};

    return static_cast<int>(x << 2.0);
}
