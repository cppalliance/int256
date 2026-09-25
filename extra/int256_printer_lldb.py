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
# unsigned value minus 2^256. Every word is read with GetValueAsUnsigned();
# GetValueAsSigned on a std::uint64_t child would print negatives as huge
# positives instead of folding the sign in at the 256-bit level.
#
# Usage: command script import int256_printer_lldb.py

import lldb


def uint256_summary(valobj, internal_dict):
    """
    Custom summary for the uint256 type.
    Displays the decimal value followed by the full 64 digit hex form.
    """
    try:
        val = valobj.GetNonSyntheticValue()
        words = val.GetChildMemberWithName("words")

        value = 0
        for i in range(4):
            value |= words.GetChildAtIndex(i).GetValueAsUnsigned() << (64 * i)

        return f"{value:,} (0x{value:064x})"
    except Exception as e:
        return f"<invalid uint256: {e}>"


def int256_summary(valobj, internal_dict):
    """
    Custom summary for the int256 type.
    Composes the unsigned value from the four words and then subtracts 2^256
    if bit 63 of words[3] (the sign bit) is set.
    """
    try:
        val = valobj.GetNonSyntheticValue()
        words = val.GetChildMemberWithName("words")

        value = 0
        for i in range(4):
            value |= words.GetChildAtIndex(i).GetValueAsUnsigned() << (64 * i)

        if value >= (1 << 255):
            value -= (1 << 256)

        return f"{value:,}"
    except Exception as e:
        return f"<invalid int256: {e}>"


def __lldb_init_module(debugger, internal_dict):
    uint256_pattern = r"^(const )?(boost::int256::uint256|(\w+::)*uint256)( &| \*)?$"
    int256_pattern = r"^(const )?(boost::int256::int256|(\w+::)*int256)( &| \*)?$"

    debugger.HandleCommand(
        f'type summary add -x "{uint256_pattern}" -e -F int256_printer_lldb.uint256_summary'
    )
    debugger.HandleCommand(
        f'type synthetic add -x "{uint256_pattern}" -l int256_printer_lldb.Uint256SyntheticProvider'
    )

    debugger.HandleCommand(
        f'type summary add -x "{int256_pattern}" -e -F int256_printer_lldb.int256_summary'
    )
    debugger.HandleCommand(
        f'type synthetic add -x "{int256_pattern}" -l int256_printer_lldb.Int256SyntheticProvider'
    )

    print("uint256 and int256 pretty printers loaded successfully")


class Uint256SyntheticProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj

    def num_children(self):
        return 4

    def get_child_index(self, name):
        # Accept both the displayed name ("[0]") and the member spelling ("words[0]").
        for i in range(4):
            if name in (f"[{i}]", f"words[{i}]"):
                return i
        return -1

    def get_child_at_index(self, index):
        if 0 <= index < 4:
            words = self.valobj.GetChildMemberWithName("words")
            return words.GetChildAtIndex(index)
        return None

    def update(self):
        pass

    def has_children(self):
        return True


class Int256SyntheticProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj

    def num_children(self):
        return 4

    def get_child_index(self, name):
        for i in range(4):
            if name in (f"[{i}]", f"words[{i}]"):
                return i
        return -1

    def get_child_at_index(self, index):
        if 0 <= index < 4:
            words = self.valobj.GetChildMemberWithName("words")
            return words.GetChildAtIndex(index)
        return None

    def update(self):
        pass

    def has_children(self):
        return True
