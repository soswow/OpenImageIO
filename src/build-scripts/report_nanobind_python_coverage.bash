#!/usr/bin/env bash

# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
#
# Summarize gcov line coverage for PyOpenImageIONanobind (src/python-nanobind/py_*.cpp).
#
# Prerequisites:
#   - Configure with -DCODECOV=ON (see src/cmake/compiler.cmake).
#   - Build PyOpenImageIONanobind.
#   - Run tests that load the nanobind module (e.g. ctest -R '\.nanobind').
#
# Usage:
#   ./src/build-scripts/report_nanobind_python_coverage.bash /path/to/build
#
# Requires llvm-cov (Apple Xcode toolchain has llvm-cov; Homebrew llvm also works).

set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 /path/to/cmake/build" >&2
    exit 2
fi

BUILD_ROOT="$(cd "$1" && pwd)"
OBJDIR="${BUILD_ROOT}/src/python-nanobind/CMakeFiles/PyOpenImageIONanobind.dir"
SRCROOT="$(cd "$(dirname "$0")/../.." && pwd)/src/python-nanobind"

LLVM_COV=""
if command -v llvm-cov >/dev/null 2>&1; then
    LLVM_COV="$(command -v llvm-cov)"
elif command -v xcrun >/dev/null 2>&1; then
    LLVM_COV="xcrun llvm-cov"
else
    echo "llvm-cov not found in PATH" >&2
    exit 1
fi

if [[ ! -d "$OBJDIR" ]]; then
    echo "Object directory not found: $OBJDIR" >&2
    exit 1
fi

echo "# Nanobind py_*.cpp coverage (llvm-cov gcov summary)"
echo "# OBJDIR=$OBJDIR"
echo "# SRCROOT=$SRCROOT"
echo

shopt -s nullglob
for gcno in "${OBJDIR}"/py_*.cpp.gcno; do
    base="$(basename "$gcno" .gcno)"
    echo "--- ${base}"
    ( cd "$OBJDIR" && ${LLVM_COV} gcov -b -s "$SRCROOT" "$gcno" ) 2>&1 \
        | grep -F -A 6 "File '${SRCROOT}/${base}'" | head -8 || \
      ( cd "$OBJDIR" && ${LLVM_COV} gcov -b -s "$SRCROOT" "$gcno" ) 2>&1 \
        | grep -F -A 6 "File '${base}'" | head -8 || true
done

echo
echo "Annotated sources (*.gcov) are written under:"
echo "  $OBJDIR"
