// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

// Regression for PEP 3118 format mapping used by Python bindings
// (typedesc_from_python_array_code in src/python/py_oiio.cpp and
// src/python-nanobind/py_oiio.cpp). Not linked against the bindings
// (no Python.h here); this file must be kept in sync with those two copies.

#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/unittest.h>

using namespace OIIO;

namespace {

TypeDesc
typedesc_from_python_array_code(string_view code)
{
    if (code.size() >= 2 && code[0] == '=') {
        code.remove_prefix(1);
    }

    TypeDesc t(code);
    if (!t.is_unknown()) {
        return t;
    }

    if (code == "b" || code == "c") {
        return TypeDesc::INT8;
    }
    if (code == "B") {
        return TypeDesc::UINT8;
    }
    if (code == "h") {
        return TypeDesc::INT16;
    }
    if (code == "H") {
        return TypeDesc::UINT16;
    }
    if (code == "i") {
        return TypeDesc::INT;
    }
    if (code == "I") {
        return TypeDesc::UINT;
    }
    if (code == "l") {
        return TypeDesc::INT64;
    }
    if (code == "L") {
        return TypeDesc::UINT64;
    }
    if (code == "q") {
        return TypeDesc::INT64;
    }
    if (code == "Q") {
        return TypeDesc::UINT64;
    }
    if (code == "f") {
        return TypeDesc::FLOAT;
    }
    if (code == "d") {
        return TypeDesc::DOUBLE;
    }
    if (code == "float16" || code == "e") {
        return TypeDesc::HALF;
    }
    return TypeDesc::UNKNOWN;
}

}  // namespace


int
main(int /*argc*/, char* /*argv*/[])
{
    OIIO_CHECK_EQUAL(typedesc_from_python_array_code("f"), TypeFloat);
    OIIO_CHECK_EQUAL(typedesc_from_python_array_code("=f"), TypeFloat);
    OIIO_CHECK_EQUAL(typedesc_from_python_array_code("=d"),
                     TypeDesc(TypeDesc::DOUBLE));
    OIIO_CHECK_EQUAL(typedesc_from_python_array_code("=B"), TypeUInt8);

    return unit_test_failures;
}
