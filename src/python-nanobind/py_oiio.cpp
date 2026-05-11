// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#include "py_oiio.h"

#include <OpenImageIO/half.h>

namespace {

using namespace OIIO;

nb::object
oiio_getattribute_typed(const std::string& name, TypeDesc type = TypeUnknown)
{
    if (type == TypeUnknown)
        return nb::none();
    char* data = OIIO_ALLOCA(char, type.size());
    if (!OIIO::getattribute(name, type, data))
        return nb::none();
    return PyOpenImageIO::make_pyobject(data, type);
}


struct oiio_global_attrib_wrapper {
    bool attribute(string_view name, TypeDesc type, const void* data)
    {
        return OIIO::attribute(name, type, data);
    }
    bool attribute(string_view name, int val)
    {
        return OIIO::attribute(name, val);
    }
    bool attribute(string_view name, float val)
    {
        return OIIO::attribute(name, val);
    }
    bool attribute(string_view name, const std::string& val)
    {
        return OIIO::attribute(name, val);
    }
};

}  // namespace


namespace PyOpenImageIO {

TypeDesc
typedesc_from_python_array_code(string_view code)
{
    // NumPy / buffer formats often use a leading '=' (standard-size prefix).
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


nb::object
make_pyobject(const void* data, TypeDesc type, int nvalues,
              nb::handle defaultvalue)
{
    if (!data || !nvalues)
        return nb::borrow(defaultvalue);
    if (type.basetype == TypeDesc::INT32)
        return C_to_val_or_tuple(static_cast<const int*>(data), type, nvalues);
    if (type.basetype == TypeDesc::FLOAT)
        return C_to_val_or_tuple(static_cast<const float*>(data), type,
                                 nvalues);
    if (type.basetype == TypeDesc::STRING)
        return C_to_val_or_tuple(static_cast<const char* const*>(data), type,
                                 nvalues);
    if (type.basetype == TypeDesc::UINT32)
        return C_to_val_or_tuple(static_cast<const unsigned int*>(data), type,
                                 nvalues);
    if (type.basetype == TypeDesc::INT16)
        return C_to_val_or_tuple(static_cast<const short*>(data), type,
                                 nvalues);
    if (type.basetype == TypeDesc::UINT16)
        return C_to_val_or_tuple(static_cast<const unsigned short*>(data), type,
                                 nvalues);
    if (type.basetype == TypeDesc::INT64)
        return C_to_val_or_tuple(static_cast<const int64_t*>(data), type,
                                 nvalues);
    if (type.basetype == TypeDesc::UINT64)
        return C_to_val_or_tuple(static_cast<const uint64_t*>(data), type,
                                 nvalues);
    if (type.basetype == TypeDesc::DOUBLE)
        return C_to_val_or_tuple(static_cast<const double*>(data), type,
                                 nvalues);
    if (type.basetype == TypeDesc::HALF)
        return C_to_val_or_tuple(static_cast<const half*>(data), type, nvalues);
    if (type.basetype == TypeDesc::UINT8 && type.arraylen > 0) {
        int n = type.arraylen * nvalues;
        if (n <= 0)
            return nb::borrow(defaultvalue);
        auto* copy = new uint8_t[n];
        std::memcpy(copy, data, static_cast<size_t>(n));
        return make_numpy_array(copy, static_cast<size_t>(n));
    }
    if (type.basetype == TypeDesc::UINT8) {
        return C_to_val_or_tuple(static_cast<const unsigned char*>(data), type,
                                 nvalues);
    }
    return nb::borrow(defaultvalue);
}

namespace {

// NumPy float16 matches Imath::half layout; nanobind ndarray<> cannot use half as
// Scalar, so we alias storage as uint16_t with an explicit float16 dtype.
static constexpr nanobind::dlpack::dtype dtype_float16{
    (uint8_t)nanobind::dlpack::dtype_code::Float, 16, 1};

template<typename T>
nb::object
make_numpy_array_nd(T* mem, int dims, size_t chans, size_t width, size_t height,
                    size_t depth)
{
    using ndarray_t = nb::ndarray<nb::numpy, T>;
    const size_t size = chans * width * height * depth;
    T* data           = mem ? mem : new T[size];
    nb::capsule owner(data, [](void* p) noexcept {
        delete[] reinterpret_cast<T*>(p);
    });

    if (dims == 4) {
        ndarray_t arr(
            data,
            { depth, height, width, chans },
            owner,
            { static_cast<int64_t>(height * width * chans),
              static_cast<int64_t>(width * chans),
              static_cast<int64_t>(chans), 1 });
        return nb::cast(std::move(arr), nb::rv_policy::move);
    }
    if (dims == 3 && depth == 1) {
        ndarray_t arr(data,
                      { height, width, chans },
                      owner,
                      { static_cast<int64_t>(width * chans),
                        static_cast<int64_t>(chans), 1 });
        return nb::cast(std::move(arr), nb::rv_policy::move);
    }
    if (dims == 2 && depth == 1 && height == 1) {
        ndarray_t arr(data,
                      { width, chans },
                      owner,
                      { static_cast<int64_t>(chans), 1 });
        return nb::cast(std::move(arr), nb::rv_policy::move);
    }
    ndarray_t arr(data, { size }, owner, { 1 });
    return nb::cast(std::move(arr), nb::rv_policy::move);
}


inline nb::object
make_numpy_array_half_nd(void* mem, int dims, size_t chans, size_t width,
                         size_t height, size_t depth)
{
    auto* data_u16 = reinterpret_cast<uint16_t*>(mem);
    using ndarray_t = nb::ndarray<nb::numpy, uint16_t>;
    nb::capsule owner(mem, [](void* p) noexcept {
        delete[] reinterpret_cast<char*>(p);
    });

    if (dims == 4) {
        ndarray_t arr(
            data_u16,
            { depth, height, width, chans },
            owner,
            { static_cast<int64_t>(height * width * chans),
              static_cast<int64_t>(width * chans),
              static_cast<int64_t>(chans), 1 },
            dtype_float16);
        return nb::cast(std::move(arr), nb::rv_policy::move);
    }
    if (dims == 3 && depth == 1) {
        ndarray_t arr(data_u16,
                      { height, width, chans },
                      owner,
                      { static_cast<int64_t>(width * chans),
                        static_cast<int64_t>(chans), 1 },
                      dtype_float16);
        return nb::cast(std::move(arr), nb::rv_policy::move);
    }
    if (dims == 2 && depth == 1 && height == 1) {
        ndarray_t arr(data_u16,
                      { width, chans },
                      owner,
                      { static_cast<int64_t>(chans), 1 },
                      dtype_float16);
        return nb::cast(std::move(arr), nb::rv_policy::move);
    }
    const size_t size = chans * width * height * depth;
    ndarray_t arr(data_u16, { size }, owner, { 1 }, dtype_float16);
    return nb::cast(std::move(arr), nb::rv_policy::move);
}

}  // namespace

oiio_bufinfo::oiio_bufinfo(const Py_buffer& pybuf)
{
    if (pybuf.format && pybuf.format[0])
        format = typedesc_from_python_array_code(pybuf.format);
    if (format != TypeUnknown) {
        data    = pybuf.buf;
        xstride = format.size();
        size    = 1;
        for (Py_ssize_t i = pybuf.ndim - 1; i >= 0; --i) {
            if (pybuf.strides == nullptr
                || pybuf.strides[i] != Py_ssize_t(size * xstride)) {
                format = TypeUnknown;
                size   = 0;
                break;
            }
            size *= static_cast<size_t>(pybuf.shape[i]);
        }
    }
}



oiio_bufinfo::oiio_bufinfo(const Py_buffer& pybuf, int nchans, int width,
                           int height, int depth, int pixeldims)
{
    if (pybuf.format && pybuf.format[0])
        format = typedesc_from_python_array_code(pybuf.format);
    int64_t expected_elems = int64_t(width) * int64_t(height)
                             * int64_t(depth * nchans);
    int64_t buf_elems      = 0;
    if (pybuf.itemsize > 0 && pybuf.len >= 0
        && pybuf.len % pybuf.itemsize == 0)
        buf_elems = int64_t(pybuf.len / pybuf.itemsize);

    if (size_t(pybuf.itemsize) != format.size()
        || buf_elems != expected_elems) {
        format = TypeUnknown;
        error  = Strutil::fmt::format(
            "buffer is wrong size (expected {}x{}x{}x{}, got total {})", depth,
            height, width, nchans, buf_elems);
        return;
    }
    size = static_cast<size_t>(buf_elems);
    if (pixeldims == 3) {
        if (pybuf.ndim == 4 && pybuf.shape[0] == depth
            && pybuf.shape[1] == height && pybuf.shape[2] == width
            && pybuf.shape[3] == nchans) {
            xstride = pybuf.strides[2];
            ystride = pybuf.strides[1];
            zstride = pybuf.strides[0];
        } else if (pybuf.ndim == 3 && pybuf.shape[0] == depth
                   && pybuf.shape[1] == height
                   && pybuf.shape[2] == int64_t(width) * int64_t(nchans)) {
            xstride = pybuf.strides[2];
            ystride = pybuf.strides[1];
            zstride = pybuf.strides[0];
        } else {
            format = TypeUnknown;
            error  = "Bad dimensions of pixel data";
        }
    } else if (pixeldims == 2) {
        if (pybuf.ndim == 3 && pybuf.shape[0] == height
            && pybuf.shape[1] == width && pybuf.shape[2] == nchans) {
            xstride = pybuf.strides[1];
            ystride = pybuf.strides[0];
        } else if (pybuf.ndim == 2) {
            if (pybuf.shape[0] == int64_t(width) * int64_t(height)
                && pybuf.shape[1] == nchans)
                xstride = pybuf.strides[0];
            else if (pybuf.shape[0] == height
                     && pybuf.shape[1] == int64_t(width) * int64_t(nchans)) {
                ystride = pybuf.strides[1];
                xstride = pybuf.strides[0] * nchans;
            } else {
                format = TypeUnknown;
                error  = Strutil::fmt::format(
                    "Can't figure out array shape (pixeldims={}, pydim={})",
                    pixeldims, pybuf.ndim);
            }
        } else if (pybuf.ndim == 1
                   && pybuf.shape[0]
                          == int64_t(width) * int64_t(height)
                                 * int64_t(nchans)) {
            // Single flat buffer — rely on autostride in set_pixels path.
        } else {
            format = TypeUnknown;
            error  = Strutil::fmt::format(
                "Python array shape is incompatible but expecting h={}, w={}, ch={}",
                height, width, nchans);
        }
    } else if (pixeldims == 1) {
        if (pybuf.ndim == 2 && pybuf.shape[0] == width
            && pybuf.shape[1] == nchans) {
            xstride = pybuf.strides[0];
        } else if (pybuf.ndim == 1
                   && pybuf.shape[0] == int64_t(width) * int64_t(nchans)) {
            xstride = pybuf.strides[0] * nchans;
        } else {
            format = TypeUnknown;
            error  = Strutil::fmt::format(
                "Can't figure out array shape (pixeldims={}, pydim={})",
                pixeldims, pybuf.ndim);
        }
    } else {
        error = Strutil::fmt::format(
            "Can't figure out array shape (pixeldims={}, pydim={})", pixeldims,
            pybuf.ndim);
    }

    if (nchans > 1 && format.size() && pybuf.strides != nullptr
        && pybuf.ndim > 0
        && size_t(pybuf.strides[pybuf.ndim - 1]) != format.size()) {
        format = TypeUnknown;
        error  = "Can't handle numpy array with noncontiguous channels";
    }
    if (format != TypeUnknown)
        data = pybuf.buf;
}



nb::object
make_numpy_array(TypeDesc format, void* data, int dims, size_t chans,
                 size_t width, size_t height, size_t depth)
{
    // Match pybind: native half arrays surface as NumPy float16 (same bit layout).
    if (format == TypeDesc::HALF)
        return make_numpy_array_half_nd(data, dims, chans, width, height, depth);
    if (format == TypeDesc::FLOAT)
        return make_numpy_array_nd(reinterpret_cast<float*>(data), dims, chans,
                                   width, height, depth);
    if (format == TypeDesc::UINT8)
        return make_numpy_array_nd(reinterpret_cast<unsigned char*>(data), dims,
                                   chans, width, height, depth);
    if (format == TypeDesc::UINT16)
        return make_numpy_array_nd(reinterpret_cast<unsigned short*>(data), dims,
                                   chans, width, height, depth);
    if (format == TypeDesc::INT8)
        return make_numpy_array_nd(reinterpret_cast<char*>(data), dims, chans,
                                   width, height, depth);
    if (format == TypeDesc::INT16)
        return make_numpy_array_nd(reinterpret_cast<short*>(data), dims, chans,
                                   width, height, depth);
    if (format == TypeDesc::UINT)
        return make_numpy_array_nd(reinterpret_cast<unsigned int*>(data), dims,
                                   chans, width, height, depth);
    if (format == TypeDesc::INT)
        return make_numpy_array_nd(reinterpret_cast<int*>(data), dims, chans,
                                   width, height, depth);
    delete[] reinterpret_cast<char*>(data);
    return nb::none();
}

}  // namespace PyOpenImageIO

NB_MODULE(_OpenImageIO, m)
{
    m.doc() = "OpenImageIO nanobind bindings.";

    m.def("geterror", &OIIO::geterror, "clear"_a = true);
    m.def(
        "is_imageio_format_name",
        [](const std::string& name) {
            return OIIO::is_imageio_format_name(name);
        },
        "name"_a);

    PyOpenImageIO::declare_typedesc(m);
    PyOpenImageIO::declare_paramvalue(m);
    PyOpenImageIO::declare_roi(m);
    PyOpenImageIO::declare_imagespec(m);
    PyOpenImageIO::declare_deepdata(m);
    PyOpenImageIO::declare_imageinput(m);
    PyOpenImageIO::declare_imageoutput(m);
    PyOpenImageIO::declare_imagebuf(m);
    PyOpenImageIO::declare_imagebufalgo(m);
    PyOpenImageIO::declare_argparse(m);

    m.def("attribute", [](const std::string& name, nb::handle obj) {
        oiio_global_attrib_wrapper wrapper;
        PyOpenImageIO::attribute_onearg(wrapper, name, obj);
    });
    m.def("attribute",
          [](const std::string& name, TypeDesc type, nb::handle obj) {
              oiio_global_attrib_wrapper wrapper;
              PyOpenImageIO::attribute_typed(wrapper, name, type, obj);
          });
    m.def(
        "get_int_attribute",
        [](const std::string& name, int def) {
            return OIIO::get_int_attribute(name, def);
        },
        "name"_a, "defaultval"_a = 0);
    m.def(
        "get_float_attribute",
        [](const std::string& name, float def) {
            return OIIO::get_float_attribute(name, def);
        },
        "name"_a, "defaultval"_a = 0.0f);
    m.def(
        "get_string_attribute",
        [](const std::string& name, const std::string& def) {
            return std::string(OIIO::get_string_attribute(name, def));
        },
        "name"_a, "defaultval"_a = "");
    m.def("getattribute", &oiio_getattribute_typed, "name"_a,
          "type"_a = TypeUnknown);
    m.attr("__version__") = OIIO_VERSION_STRING;
}
