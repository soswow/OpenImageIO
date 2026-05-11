# Suggested PR split: current uncommitted work (`more-nanobind-migration`)

Maps uncommitted work into **reviewable** merges: one small pybind PR, then
**stacked nanobind PRs** (each leaves the extension building and tests honest),
then optional extras.

---

## 1. Inventory (what the branch touches)

### Modified (tracked)

| Area | Paths |
|------|--------|
| Pybind buffer `TypeDesc` | `src/python/py_oiio.cpp` |
| Nanobind core + module glue | `src/python-nanobind/py_oiio.cpp`, `src/python-nanobind/py_oiio.h` |
| Nanobind build | `src/python-nanobind/CMakeLists.txt` |
| Nanobind migration note | `src/python-nanobind/MIGRATION_STATUS.md` |
| CTest / Python paths | `src/cmake/testing.cmake` |
| Libutil tests | `src/libutil/CMakeLists.txt` |
| Shared tests + refs | `testsuite/python-imagebuf/…`, `testsuite/python-imagespec/…`, `testsuite/python-paramlist/…` |

### Untracked (new)

| Area | Paths |
|------|--------|
| C++ unit test (PEP 3118 `=` / format mirror) | `src/libutil/python_array_format_test.cpp` |
| Coverage helper script | `src/build-scripts/report_nanobind_python_coverage.bash` |
| New nanobind translation units | `src/python-nanobind/py_{argparse,deepdata,imagebuf,imagebufalgo,imageinput,imageoutput}.cpp` |
| New testsuites | `testsuite/python-oiio/`, `testsuite/python-argparse-nanobind/` |
| Fixture | `testsuite/python-imageoutput/multipart.exr` |

---

## 2. Build constraint (nanobind) — **does not force one mega-PR**

`PyOpenImageIONanobind` is one link target, but each merge can still list a
**subset** of `nanobind_srcs` and call only the matching `declare_*` functions
from `py_oiio.cpp`. Later PRs **append** sources and declarations.

Cost: you (or git) must **restack** the branch so early PRs do not reference
`.cpp` files that are not in `nanobind_srcs` yet. That is normal stacked-work
flow, not “one impossible review.”

---

## 3. Recommended sequence

### PR 1 — Pybind only: buffer format → `TypeDesc`

**Files:** `src/python/py_oiio.cpp` only.

**Contents:** `typedesc_from_python_array_code` parity (e.g. leading `=`, `q` /
`Q`) as needed.

**Why first:** Small, isolated, easy bisect; no nanobind coupling.

---

### PRs 2–6 — Nanobind in **layers** (example ordering)

Each PR below should: (1) add only the listed new `py_*.cpp` files to
`nanobind_srcs`, (2) wire `declare_*` + any `py_oiio.h` helpers those modules
need, (3) register only the **tests** that exercise what exists,
(4) update `MIGRATION_STATUS.md` for the rows you touch.

Order can shift if `py_imagebuf` needs symbols from I/O—adjust after a quick
dependency skim.

| PR | Focus | Typical contents |
|----|--------|------------------|
| **2** | Image I/O | `py_imageinput.cpp`, `py_imageoutput.cpp`, glue, `testing.cmake` for `python-imageinput` / `python-imageoutput` `.nanobind`, `multipart.exr` if needed |
| **3** | Deep data | `py_deepdata.cpp`, glue, tests if you extend `python-deep.nanobind` |
| **4** | ImageBuf + buffers | `py_imagebuf.cpp`, larger `py_oiio.h` / `py_oiio.cpp` buffer helpers, `testsuite/python-imagebuf` + `ref/out.txt` |
| **5** | ImageBufAlgo | `py_imagebufalgo.cpp`, glue, `python-imagebufalgo.nanobind` in `testing.cmake` |
| **6** | ArgParse | `py_argparse.cpp`, `testsuite/python-argparse-nanobind/`, `testing.cmake` |

**Cross-cutting tests** (`python-imagespec`, `python-paramlist` ref bumps for
module-level `oiio.*` smoke): land with the **nanobind PR that first implements**
those surfaces (often **PR 2** or whenever `py_oiio.cpp` gains those bindings).

---

### PR 7 — Optional extras (or fold into PRs above)

- `src/libutil/python_array_format_test.cpp` + `src/libutil/CMakeLists.txt`
- `testsuite/python-oiio/` + matching `testing.cmake` lines (pybind **and**
  nanobind once the extension runs far enough)
- `src/build-scripts/report_nanobind_python_coverage.bash`

Skip entirely if you do not want the extra surface area.

---

## 4. “Two PR total” — **not** a good default here

`pybind-only` + `everything else including full nanobind` is only useful as an
emergency bisect strategy; the second PR is still **too large for meaningful
review**. Prefer **§3 PRs 2–6** (or merge 2+3 if I/O + DeepData stay small).

---

## 5. What not to mix in without an explicit goal

| Item | Suggestion |
|------|------------|
| Public `OpenImageIO.typedesc_from_python_array_code(...)` | Avoid unless you want a **stable** Python API + stubs. |
| Shared header in `libutil` + bindings | Separate refactor PR. |
| Unrelated `testing.cmake` / plugin toggles | Keep each migration PR focused. |

---

## 6. Checklist

- [ ] `./compile.sh` (or CI-equivalent) for backends you enable.
- [ ] `ctest` for every testsuite you register (`python-*`, `*.nanobind`).
- [ ] Regenerate `ref/out.txt` only for tests you touch.

This file is **developer guidance**; trim or delete before upstreaming if
`docs/dev/` should stay minimal.
