#!/usr/bin/env python

# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

"""Smoke test for OpenImageIO.ArgParse (nanobind-only binding)."""

from __future__ import annotations

import OpenImageIO as oiio


def main() -> None:
    ap = oiio.ArgParse()
    ap.exit_on_error(False)
    ap.intro("nanobind ArgParse smoke test")
    ap.usage("example [options] files...")
    ap.add_help(False)
    ap.add_argument("-v").help("verbose").store_true()
    ap.add_argument("--count N").help("repeat count").action_store_int()
    ap.arg("filename").hidden().action_append()

    argv = ["example", "-v", "--count", "2", "a.exr", "b.exr"]
    rc = ap.parse_args(argv)
    print("parse_rc", rc)
    print("has_error", ap.has_error)
    print("err", repr(ap.geterror()))
    print("v", ap["v"])
    print("count", ap["count"])
    print("filename", ap["filename"])

    ap2 = oiio.ArgParse()
    ap2.exit_on_error(False)
    ap2.add_help(False)
    ap2.add_argument("--bad-only").help("unknown flag demo").store_true()
    rc2 = ap2.parse_args(["x", "--not-a-real-option"])
    print("bad_rc", rc2)
    e0 = ap2.geterror(False)
    e1 = ap2.geterror(True)
    e2 = ap2.geterror(False)
    print("bad_err_nonempty", len(e0) > 0)
    print("bad_err_cleared_after", e2 == "")

    # Extra Arg / ArgParse surface.
    apx = oiio.ArgParse()
    apx.exit_on_error(False)
    apx.add_help(False)
    apx.description("desc").epilog("epi").prog("myprog").print_defaults(True)
    apx.add_version("9.8.7")
    apx.separator("---sep---")
    apx.arg("-n").help("off").store_false()
    apx.add_argument("--fstore FLOAT").help("float").action_store_float()
    apx.add_argument("--sstore STRING").help("str").action_store_string()
    apx.add_argument("--noop").action_do_nothing()
    apx.add_argument("--ci").help("iconst").store_const(42)
    apx.add_argument("--cf").help("fconst").store_const(1.25)
    apx.add_argument("--cs").help("sconst").store_const("ok")
    apx.add_argument("--mval INT").help("ival").action_store_int()
    chain = apx.add_argument("-q").store_true()
    print("arg_argparse_is_self", chain.argparse() is apx)
    r3 = apx.parse_args(
        [
            "myprog",
            "-n",
            "-q",
            "--fstore",
            "2.5",
            "--sstore",
            "hi",
            "--noop",
            "--ci",
            "--cf",
            "--cs",
            "--mval",
            "3",
        ]
    )
    print("ex_rc", r3)
    print("ex_n", apx["n"])
    print("ex_q", apx["q"])
    print("ex_fstore", apx["fstore"])
    print("ex_sstore", apx["sstore"])
    print("ex_mval", apx["mval"])
    print("ex_ci", apx["ci"])
    print("ex_cf", apx["cf"])
    print("ex_cs", apx["cs"])
    pl = apx.params()
    print("params_len", len(pl))
    print("cmdline_skip", True)
    print("prog_name_skip", True)

    apd = oiio.ArgParse()
    apd.exit_on_error(False)
    apd.add_help(False)
    apd.add_argument("--df DEF").defaultval(9.5).action_store_float()
    r_d = apd.parse_args(["z"])
    print("defl_rc", r_d, "defl_df", apd["df"])

    apy = oiio.ArgParse()
    apy.exit_on_error(False)
    apy.add_help(False)
    apy.add_argument("-k").store_true()
    apy.parse_args(["y", "-k"])
    apy.abort(True)
    print("aborted", apy.aborted)
    apy.abort(False)
    print("aborted_clear", apy.aborted)

    try:
        _ = apy["nope"]
    except Exception as e:
        print("getitem_err", type(e).__name__)

    apz = oiio.ArgParse()
    apz.exit_on_error(False)
    apz.add_help(False)
    apz.add_argument("-a").help("a").always_run()
    r4 = apz.parse_args(["w", "-a"])
    print("always_run_rc", r4)


if __name__ == "__main__":
    main()
