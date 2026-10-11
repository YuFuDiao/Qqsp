# -*- coding: utf-8 -*-
"""Localize QSP engine error descriptions inside the built qsp.dll.

Each description is a UTF-16LE literal terminated by NUL and consumed through
QString construction, so a shorter Chinese replacement plus a fresh terminator
is safe.  Descriptions the current engine no longer carries are reported and
skipped; this step only fails when qsp.dll itself is missing.
"""
import os
import sys

APOS = chr(39)
ALT = chr(0x2019)

def find_dll(root):
    for dirpath, dirnames, filenames in os.walk(root):
        for fn in filenames:
            if fn.lower() == "qsp.dll":
                return os.path.join(dirpath, fn)
    return None

def patch_one(data, en, zh):
    for variant in (en, en.replace(APOS, ALT)):
        blob = variant.encode("utf-16-le")
        first = data.find(blob)
        if first < 0:
            continue
        if data.find(blob, first + 2) >= 0:
            return "ambiguous"
        new = zh.encode("utf-16-le")
        data[first:first + len(new)] = new
        data[first + len(new):first + len(new) + 2] = b"\x00\x00"
        return "ok"
    return "absent"

def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    here = os.path.dirname(os.path.abspath(__file__))
    pairs = []
    with open(os.path.join(here, "qsp_err_zh.tsv"), encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or "\t" not in line:
                continue
            en, zh = line.split("\t", 1)
            pairs.append((en, zh))
    dll = find_dll(root)
    if not dll:
        raise SystemExit("FAILED: qsp.dll not found under " + root)
    data = bytearray(open(dll, "rb").read())
    print("target:", dll, len(data), "bytes /", len(pairs), "entries")
    ok = 0
    for en, zh in pairs:
        r = patch_one(data, en, zh)
        if r == "ok":
            ok += 1
        else:
            print("  %-9s %s" % (r, en))
    open(dll, "wb").write(bytes(data))
    print("patched", ok, "of", len(pairs))

main()
