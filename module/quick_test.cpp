// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Import the STL if we can
#if defined(__cpp_lib_modules) && __cpp_lib_modules >= 202207L
import std;
#else
#include <iostream>
#endif

import boost.int256;

int main()
{
    using namespace boost::int256;

    uint256 a {uint256{1} << 128U};
    a += 42U;
    std::cout << a << std::endl;

    int256 b {int256{-1, 0, 0, 0} >> 1};
    b -= 42;
    std::cout << b << std::endl;

    return 0;
}
