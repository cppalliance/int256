# Copyright 2026 Matt Borland
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt
#
# Struct definitions:
#   struct uint256 { std::uint64_t words[4]; };
#   struct int256  { std::uint64_t words[4]; };
#
# words[0] is the least significant word on every platform, including big
# endian ones, so the unsigned value is words[3] * 2^192 + ... + words[0].
# int256 reads that same unsigned value as two's complement: it is negative
# when bit 63 of words[3] is set, in which case the true value is the
# unsigned value minus 2^256.
#
# Usage: source int256_printer_gdb.py

import gdb
import gdb.printing
import re

MASK64 = 0xFFFFFFFFFFFFFFFF


class Uint256Printer:
    """Pretty printer for the uint256 type"""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            words = self.val["words"]
            value = 0
            for i in range(4):
                value |= (int(words[i]) & MASK64) << (64 * i)

            return f"{value:,} (0x{value:064x})"
        except Exception as e:
            return f"<invalid uint256: {e}>"

    def children(self):
        words = self.val["words"]
        for i in range(4):
            yield f"words[{i}]", words[i]

    def display_hint(self):
        return None


class Int256Printer:
    """Pretty printer for the int256 type"""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            words = self.val["words"]
            # Every word is std::uint64_t, so read all four unsigned and fold
            # the two's complement sign in by hand rather than casting.
            value = 0
            for i in range(4):
                value |= (int(words[i]) & MASK64) << (64 * i)

            if value >= (1 << 255):
                value -= (1 << 256)

            return f"{value:,}"
        except Exception as e:
            return f"<invalid int256: {e}>"

    def children(self):
        words = self.val["words"]
        for i in range(4):
            yield f"words[{i}]", words[i]

    def display_hint(self):
        return None


def lookup_int256_type(val):
    """
    Lookup function to detect if a type should use our pretty printers.
    Returns the appropriate printer or None.
    """
    # Get the basic type name, stripping references and const qualifiers
    type_obj = val.type

    # Handle references and pointers
    if type_obj.code == gdb.TYPE_CODE_REF:
        type_obj = type_obj.target()
    if type_obj.code == gdb.TYPE_CODE_PTR:
        return None  # Don't handle pointers directly

    # Strip const/volatile qualifiers
    type_obj = type_obj.unqualified()

    type_name = str(type_obj)

    # Patterns to match the uint256 and int256 types
    uint256_pattern = re.compile(
        r"^(boost::int256::uint256|(\w+::)*uint256|uint256)$"
    )
    int256_pattern = re.compile(
        r"^(boost::int256::int256|(\w+::)*int256|int256)$"
    )

    if uint256_pattern.match(type_name):
        return Uint256Printer(val)
    if int256_pattern.match(type_name):
        return Int256Printer(val)

    return None


def register_int256_printers(objfile=None):
    """Register the int256 pretty printers."""
    if objfile is None:
        objfile = gdb

    objfile.pretty_printers.append(lookup_int256_type)


# Auto-register when the script is sourced
register_int256_printers()
print("uint256 and int256 pretty printers loaded successfully")
