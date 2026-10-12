# -*- coding: utf-8 -*-
"""Make `gs` share the caller's local $result, as older QSP builds did.

qspAllocateLocalScopeWithArgs() creates a local RESULT variable for *every*
location call.  Older engines created it only when the caller actually wanted a
result, so a `gs`-called location could still fill in the $result of the
function it was called from.  Girl Life depends on that:

    show_all  ->  $func('traits', 'card', ...)   # result wanted
                    card -> gs 'cards', 'shell'   # shell writes $result

With the current engine the shell's HTML lands in a throw-away scope, every
card comes back empty and the trait page renders blank.  Adding a parameter so
the local RESULT is only created when a result is requested restores the old
behaviour.
"""
import io
import os
import sys


def must(cond, msg):
    if not cond:
        raise SystemExit("FAILED: " + msg)


def patch_file(path, pairs):
    text = io.open(path, encoding="utf-8", newline="").read()
    for old, new, why in pairs:
        must(old in text, "%s: pattern not found (%s)" % (os.path.basename(path), why))
        must(text.count(old) == 1, "%s: pattern not unique (%s)" % (os.path.basename(path), why))
        text = text.replace(old, new, 1)
    io.open(path, "w", encoding="utf-8", newline="").write(text)
    print("patched", os.path.basename(path))


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    qsp = os.path.join(root, "qsp")
    must(os.path.isdir(qsp), "no qsp/ directory under " + root)

    patch_file(os.path.join(qsp, "variables.h"), [(
        "QSPVarsScope *qspAllocateLocalScopeWithArgs(QSPVariant *args, int count, QSP_BOOL toMove);",
        "QSPVarsScope *qspAllocateLocalScopeWithArgs(QSPVariant *args, int count, QSP_BOOL toMove, QSP_BOOL toCreateResult);",
        "declaration",
    )])

    patch_file(os.path.join(qsp, "variables.c"), [(
        "QSPVarsScope *qspAllocateLocalScopeWithArgs(QSPVariant *args, int count, QSP_BOOL toMove)",
        "QSPVarsScope *qspAllocateLocalScopeWithArgs(QSPVariant *args, int count, QSP_BOOL toMove, QSP_BOOL toCreateResult)",
        "definition",
    ), (
        "    qspAddVarToScope(scope, QSP_STATIC_STR(QSP_VARRES));",
        "    /* Only the caller that asked for a result gets a private RESULT, so a" + chr(10) +
        "       gs-called location still fills in the enclosing function\'s $result. */" + chr(10) +
        "    if (toCreateResult)" + chr(10) +
        "        qspAddVarToScope(scope, QSP_STATIC_STR(QSP_VARRES));",
        "result variable",
    )])

    patch_file(os.path.join(qsp, "locations.c"), [(
        "    qspAllocateLocalScopeWithArgs(args, argsCount, toMoveArgs);",
        "    qspAllocateLocalScopeWithArgs(args, argsCount, toMoveArgs, res != 0);",
        "call site",
    )])

    patch_file(os.path.join(qsp, "statements.c"), [(
        "    qspAllocateLocalScopeWithArgs(args, count, QSP_TRUE);",
        "    qspAllocateLocalScopeWithArgs(args, count, QSP_TRUE, res != 0);",
        "dynamic code call site",
    )])

    print("done")


if __name__ == "__main__":
    main()
