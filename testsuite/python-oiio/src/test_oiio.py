#!/usr/bin/env python

# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

# Integration tests for py_oiio.cpp / py_oiio.h buffer paths (pybind + nanobind).
# PEP 3118 format letter coverage (including leading '=') is in the C++ test
# unit_python_array_format (src/libutil/python_array_format_test.cpp), not
# exposed as Python API.

from __future__ import annotations

import OpenImageIO as oiio


def _fmt_td(t: oiio.TypeDesc) -> str:
    return t.c_str()


def test_imagebuf_from_numpy_float32() -> None:
    print("\nTesting ImageBuf(numpy float32)...")
    import numpy as np

    a = np.zeros((2, 2, 1), dtype=np.float32)
    ib = oiio.ImageBuf(a)
    ch = ib.spec().channelformat(0)
    ok = ch == oiio.TypeFloat
    print("  channelformat(0)={} {}".format(_fmt_td(ch), "ok" if ok else "FAIL"))
    if not ok:
        raise RuntimeError("expected float channel")
    print("  Passed ImageBuf(numpy float32)")


def test_imagebuf_from_numpy_float64() -> None:
    print("\nTesting ImageBuf(numpy float64)...")
    import numpy as np

    a = np.zeros((2, 2, 1), dtype=np.float64)
    ib = oiio.ImageBuf(a)
    ch = ib.spec().channelformat(0)
    exp = oiio.TypeDesc(oiio.DOUBLE)
    ok = ch == exp
    print("  channelformat(0)={} {}".format(_fmt_td(ch), "ok" if ok else "FAIL"))
    if not ok:
        raise RuntimeError("expected double channel")
    print("  Passed ImageBuf(numpy float64)")


def test_imagebuf_from_numpy_uint8() -> None:
    print("\nTesting ImageBuf(numpy uint8)...")
    import numpy as np

    a = np.zeros((2, 2, 1), dtype=np.uint8)
    ib = oiio.ImageBuf(a)
    ch = ib.spec().channelformat(0)
    ok = ch == oiio.TypeUInt8
    print("  channelformat(0)={} {}".format(_fmt_td(ch), "ok" if ok else "FAIL"))
    if not ok:
        raise RuntimeError("expected uint8 channel")
    print("  Passed ImageBuf(numpy uint8)")


def test_imagebuf_from_numpy_float16() -> None:
    print("\nTesting ImageBuf(numpy float16)...")
    import numpy as np

    a = np.zeros((2, 2, 1), dtype=np.float16)
    ib = oiio.ImageBuf(a)
    ch = ib.spec().channelformat(0)
    ok = ch == oiio.TypeHalf
    print("  channelformat(0)={} {}".format(_fmt_td(ch), "ok" if ok else "FAIL"))
    if not ok:
        raise RuntimeError("expected half channel")
    print("  Passed ImageBuf(numpy float16)")


######################################################################
# main test starts here

try:
    test_imagebuf_from_numpy_float32()
    test_imagebuf_from_numpy_float64()
    test_imagebuf_from_numpy_uint8()
    test_imagebuf_from_numpy_float16()

    print("\nDone.")
except Exception as detail:
    print("Unknown exception:", detail)
