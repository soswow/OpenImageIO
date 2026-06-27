# Nanobind migration status

Nanobind shares binding sources with pybind11 under `src/python/` (see
`py_backend.h` and `python_dual_backend_srcs` in `CMakeLists.txt`). Configure
with `-DOIIO_PYTHON_BINDINGS_BACKEND=nanobind` for nanobind-only (`PyOpenImageIO`
/ module `OpenImageIO` in site-packages), or `both` to also build
`PyOpenImageIONanobind` (`_OpenImageIO` under `lib/python/nanobind/OpenImageIO`).
Nanobind-only code paths live in binding `.cpp` files and `py_oiio.cpp` behind
`OIIO_PY_BACKEND_NANOBIND`. Shared Python↔C++ conversion helpers live in
`py_oiio.h` for both backends.

## Migrated — full dual-backend sources

All modules below compile for both pybind11 and nanobind from `src/python/`:

| Source file | Python / C++ API |
| --- | --- |
| `py_roi.cpp` | `ROI`, free functions (`union`, `intersection`, `get_roi`, …) |
| `py_typedesc.cpp` | `TypeDesc`, enums, module `Type*` constants |
| `py_imagespec.cpp` | `ImageSpec` |
| `py_paramvalue.cpp` | `ParamValue`, `ParamValueList`, `Interp` |
| `py_deepdata.cpp` | `DeepData` |
| `py_colorconfig.cpp` | `ColorConfig`, module color constants |
| `py_imageinput.cpp` | `ImageInput` |
| `py_imageoutput.cpp` | `ImageOutput` |
| `py_imagebuf.cpp` | `ImageBuf` |
| `py_imagecache.cpp` | `ImageCache` (wrapped) |
| `py_texturesys.cpp` | `Wrap`, `MipMode`, `InterpMode`, `TextureOpt`, `TextureSystem` |
| `py_imagebufalgo.cpp` | `ImageBufAlgo`, `PixelStats`, `CompareResults`, `IBA_*` |
| `py_oiio.cpp` | Module-level attributes and global helpers |

## Known nanobind deltas (vs pybind11)

| Area | Notes |
| --- | --- |
| `half` numpy arrays | Multi-dimensional reads return **float32** numpy (nanobind has no native `half` ndarray). pybind11 returns float16. |
| `ImageBuf` constructors | Lambda/`buffer` inits use nanobind `__init__` placement-new (same Python API). |
| `ParamValue` constructors | Same placement-new pattern as before. |
| Enum `.export_values()` | `py_typedesc.cpp` and `py_imagebufalgo.cpp` set module attrs explicitly on nanobind. |
| `py_typedesc.cpp` | `py::implicitly_convertible` — verify if needed on nanobind (may be pybind-only). |

## Not migrated / packaging

| Item | Notes |
| --- | --- |
| `__init__.py` | Shared env setup; CLI entry-point trampolines still TODO for full wheel layout. |

## Conventions

- Binding macros: `.OIIO_PY_RW`, `.OIIO_PY_PROP_RO`, `.OIIO_PY_PROP_RW`, `.OIIO_PY_RO`, `.OIIO_PY_RO_STATIC` (see `py_backend.h`).
- Declare functions use `py_module&`, not `py::module&`.
- Buffer I/O: `oiio_py_request_buffer()` / `oiio_bufinfo_from_object()` (both backends).
- `#if defined(OIIO_PY_BACKEND_NANOBIND)` only where backends genuinely differ.

Extend **testsuite** coverage when adding behavior; run both pybind and `*.nanobind` ctest variants when `OIIO_PYTHON_BINDINGS_BACKEND=both`.
