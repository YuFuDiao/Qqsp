# -*- coding: utf-8 -*-
"""Localize the freshly built qsp.dll: Chinese error descriptions + a compatible
reported engine version.

Error descriptions are UTF-16LE literals terminated by NUL and consumed through
QString, so shorter Chinese plus a fresh terminator is safe.  The version the
engine reports is what games compare against: Girl Life refuses to start above
5.9.2, so we report 5.9.2 (what the vendor-recommended players use).
"""
import os
import sys

APOS = chr(39)
ALT = chr(0x2019)
REPORTED_VERSION = "5.9.2"  # Girl Life gate: refuses to start above 5.9.2
# NOTE: 5.9.4 is the engine's QSP_GAMEMIN_VER constant, never touch it.
KNOWN_VERSIONS = ["5.9.6", "5.9.7", "5.9.8", "5.10.0", "5.9.5"]


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


def patch_version(data):
    new = REPORTED_VERSION.encode("utf-16-le")
    for candidate in KNOWN_VERSIONS:
        blob = candidate.encode("utf-16-le")
        if len(blob) != len(new):
            continue
        if data.count(blob) == 1:
            i = data.find(blob)
            data[i:i + len(new)] = new
            print("  version: reports %s (was %s)" % (REPORTED_VERSION, candidate))
            return True
    print("  version: no unique known version literal found")
    return False


def load_pairs(here):
    pairs = []
    with open(os.path.join(here, "qsp_err_zh.tsv"), encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or "\t" not in line:
                continue
            en, zh = line.split("\t", 1)
            pairs.append((en, zh))
    return pairs


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    here = os.path.dirname(os.path.abspath(__file__))
    pairs = load_pairs(here)
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
    patch_version(data)
    open(dll, "wb").write(bytes(data))
    print("patched", ok, "of", len(pairs))


if __name__ == "__main__":
    main()
