// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// A user-defined literal whose value does not fit the target type must be rejected
// at compile time instead of being silently truncated. This is 2^256, one past the
// uint256 maximum.

#include <boost/int256/literals.hpp>

using namespace boost::int256::literals;

int main()
{
    constexpr auto value = 115792089237316195423570985008687907853269984665640564039457584007913129639936_u256;
    static_cast<void>(value);

    return 0;
}
