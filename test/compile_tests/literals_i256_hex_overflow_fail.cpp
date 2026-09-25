// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// A prefixed (hex) literal never carries a leading minus sign either, so its
// magnitude is checked against the same positive range as a decimal literal:
// at most MAX (2^255 - 1). This is 2^255, one past it, spelled in hex with
// bit 255 set. (Conductor finding F4: this range check was missing from
// detail/mini_from_chars.hpp's prefixed parse_literal<int256>, verified fixed
// as of this file compiling correctly failing.)

#include <boost/int256/literals.hpp>

using namespace boost::int256::literals;

int main()
{
    constexpr auto value = 0x8000000000000000000000000000000000000000000000000000000000000000_i256;
    static_cast<void>(value);

    return 0;
}
