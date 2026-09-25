// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// A user-defined literal whose value does not fit the target type must be
// rejected at compile time instead of being silently truncated. This is 2^255,
// one past the int256 maximum (a positive literal cannot reach MIN, since
// literals never carry a leading minus sign; the magnitude is parsed as
// uint256, so it overflows before any sign is applied).

#include <boost/int256/literals.hpp>

using namespace boost::int256::literals;

int main()
{
    constexpr auto value = 57896044618658097711785492504343953926634992332820282019728792003956564819968_i256;
    static_cast<void>(value);

    return 0;
}
