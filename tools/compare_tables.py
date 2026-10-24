#!/usr/bin/env python3
"""Byte-for-byte comparison of reconstructed C tables with digger.com.

Each manifest entry names a C declaration in C/, the addressing mode it
uses (DS offsets for data tables, CS offsets for code-segment tables), and the
expected size.  The tool extracts the initializer, converts it to bytes, and
compares it with the binary.

Usage: python3 tools/compare_tables.py [-v]
Exit status is non-zero if any table differs.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "digger.com"
CFILES = ROOT / "C"
DS_BASE = 0x8D70          # file offset of DS:0000
CS_BASE = -0x100          # file offset of CS:0000 (CS loads at 0x100)

# name, source file, declaration, mode, segment, segment offset, size (bytes)
MANIFEST = [
    ("level_data", "levels.c", "level_data", "strings", "DS", 0x4B0E, 8 * 150),
    ("rng_table", "rng.c", "rng_table", "bytes", "DS", 0x0CA7, 256),
    ("cga_crtc_table", "cga.c", "cga_crtc_table", "bytes", "DS", 0x0255, 14),
    ("sprorder", "sprite.c", "sprorder", "words", "DS", 0x0627, 16 * 2),
]


def strip_clike(text):
    """Remove // and /* */ comments, but not inside string literals."""
    out = []
    i = 0
    n = len(text)
    in_str = False
    while i < n:
        c = text[i]
        if in_str:
            out.append(c)
            if c == "\\" and i + 1 < n:
                out.append(text[i + 1])
                i += 2
                continue
            if c == '"':
                in_str = False
            i += 1
            continue
        if c == '"':
            in_str = True
            out.append(c)
            i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            i += 2
            while i + 1 < n and not (text[i] == "*" and text[i + 1] == "/"):
                i += 1
            i += 2
            continue
        out.append(c)
        i += 1
    return "".join(out)


def initializer(src, decl):
    """Return the text of the first {...} initializer after `decl`."""
    m = re.search(r"\b" + re.escape(decl) + r"\b", src)
    if not m:
        raise SystemExit("declaration not found: " + decl)
    i = src.find("{", m.end())
    if i < 0:
        raise SystemExit("initializer not found: " + decl)
    depth = 0
    in_str = False
    j = i
    while j < len(src):
        c = src[j]
        if in_str:
            if c == "\\":
                j += 2
                continue
            if c == '"':
                in_str = False
        elif c == '"':
            in_str = True
        elif c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return src[i + 1:j]
        j += 1
    raise SystemExit("unterminated initializer: " + decl)


def c_string(s):
    """Decode a C string literal body."""
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c != "\\":
            out.append(ord(c))
            i += 1
            continue
        i += 1
        e = s[i] if i < len(s) else ""
        mapping = {"n": 10, "r": 13, "t": 9, "0": 0, "\\": 92, '"': 34, "'": 39}
        if e in mapping:
            out.append(mapping[e])
            i += 1
        elif e == "x":
            h = re.match(r"[0-9a-fA-F]{1,2}", s[i + 1:])
            if not h:
                raise SystemExit("bad \\x escape")
            out.append(int(h.group(0), 16))
            i += 1 + len(h.group(0))
        else:
            out.append(ord(e))
            i += 1
    return bytes(out)


def table_bytes(init, mode):
    if mode == "strings":
        return b"".join(c_string(m) for m in re.findall(r'"((?:[^"\\]|\\.)*)"', init))
    if mode == "bytes":
        vals = [int(v, 0) for v in re.findall(r"0x[0-9a-fA-F]+|\b\d+\b", init)]
        return bytes(v & 0xFF for v in vals)
    if mode == "words":
        vals = [int(v, 0) for v in re.findall(r"0x[0-9a-fA-F]+|\b\d+\b", init)]
        out = bytearray()
        for v in vals:
            out += (v & 0xFFFF).to_bytes(2, "little")
        return bytes(out)
    raise SystemExit("unknown mode: " + mode)


def main():
    verbose = "-v" in sys.argv[1:]
    blob = BIN.read_bytes()
    failures = 0
    total = 0
    for name, fname, decl, mode, seg, seg_off, size in MANIFEST:
        src = (CFILES / fname).read_text()
        data = table_bytes(initializer(strip_clike(src), decl), mode)
        off = (DS_BASE if seg == "DS" else CS_BASE) + seg_off
        total += 1
        if len(data) != size:
            print("SIZE  %-16s %d bytes in C, expected %d" % (name, len(data), size))
            failures += 1
            continue
        want = blob[off:off + size]
        if want == data:
            print("ok    %-16s %4d bytes at %s:0x%04X (%s)" %
                  (name, size, seg, seg_off, fname))
            if verbose:
                print("      ", data[:16].hex())
            continue
        failures += 1
        for i, (a, b) in enumerate(zip(data, want)):
            if a != b:
                print("DIFF  %-16s %s:0x%04X byte %d: C 0x%02X, binary 0x%02X" %
                      (name, seg, seg_off, i, a, b))
                break
        else:
            print("DIFF  %-16s length mismatch" % name)
    print("%d table(s), %d failure(s)" % (total, failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
