// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#include "py_oiio.h"

#include <memory>

#include <OpenImageIO/deepdata.h>
#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/platform.h>

namespace PyOpenImageIO {

static ImageBuf
ImageBuf_from_buffer(nb::handle buffer_obj)
{
    ImageBuf ib;
    Py_buffer info;
    if (PyObject_GetBuffer(buffer_obj.ptr(), &info,
                           PyBUF_FORMAT | PyBUF_STRIDES)
        != 0) {
        PyErr_Clear();
        return ib;
    }
    TypeDesc format;
    if (info.format && info.format[0])
        format = typedesc_from_python_array_code(info.format);
    if (format == TypeUnknown) {
        PyBuffer_Release(&info);
        return ib;
    }
    if (size_t(info.strides[info.ndim - 1]) != format.size()) {
        ib.errorfmt(
            "ImageBuf-from-numpy-array must have contiguous stride within pixels");
        PyBuffer_Release(&info);
        return ib;
    }

    int width = 1, height = 1, depth = 1, nchans = 1;
    stride_t xstride = AutoStride, ystride = AutoStride, zstride = AutoStride;
    if (info.ndim == 3) {
        width   = static_cast<int>(info.shape[1]);
        height  = static_cast<int>(info.shape[0]);
        nchans  = static_cast<int>(info.shape[2]);
        xstride = info.strides[1];
        ystride = info.strides[0];
    } else if (info.ndim == 2) {
        width   = static_cast<int>(info.shape[1]);
        height  = static_cast<int>(info.shape[0]);
        xstride = info.strides[1];
        ystride = info.strides[0];
    } else if (info.ndim == 4) {
        width   = static_cast<int>(info.shape[2]);
        height  = static_cast<int>(info.shape[1]);
        depth   = static_cast<int>(info.shape[0]);
        nchans  = static_cast<int>(info.shape[3]);
        xstride = info.strides[2];
        ystride = info.strides[1];
        zstride = info.strides[0];
    } else {
        ib.errorfmt(
            "ImageBuf-from-numpy-array must have 2, 3, or 4 dimensions");
        PyBuffer_Release(&info);
        return ib;
    }

    ImageSpec spec(width, height, nchans, format);
    spec.depth      = depth;
    spec.full_depth = depth;
    ib.reset(spec, InitializePixels::No);
    image_span<const std::byte> bufspan(
        reinterpret_cast<std::byte*>(info.buf), nchans, width, height, depth,
        format.size(), xstride, ystride, zstride, format.size());
    ib.set_pixels(get_roi(spec), format, bufspan);
    PyBuffer_Release(&info);
    return ib;
}

static nb::tuple
ImageBuf_getpixel(const ImageBuf& buf, int x, int y, int z = 0,
                  const std::string& wrapname = "black")
{
    ImageBuf::WrapMode wrap = ImageBuf::WrapMode_from_string(wrapname);
    int nchans              = buf.nchannels();
    span<float> pixel       = OIIO_ALLOCA_SPAN(float, nchans);
    buf.getpixel(x, y, z, pixel, wrap);
    return C_to_tuple(cspan<const float>(pixel.data(), pixel.size()));
}

static nb::tuple
ImageBuf_interppixel(const ImageBuf& buf, float x, float y,
                     const std::string& wrapname = "black")
{
    ImageBuf::WrapMode wrap = ImageBuf::WrapMode_from_string(wrapname);
    int nchans              = buf.nchannels();
    span<float> pixel       = OIIO_ALLOCA_SPAN(float, nchans);
    buf.interppixel(x, y, pixel, wrap);
    return C_to_tuple(cspan<const float>(pixel.data(), pixel.size()));
}

static nb::tuple
ImageBuf_interppixel_NDC(const ImageBuf& buf, float x, float y,
                         const std::string& wrapname = "black")
{
    ImageBuf::WrapMode wrap = ImageBuf::WrapMode_from_string(wrapname);
    int nchans              = buf.nchannels();
    span<float> pixel       = OIIO_ALLOCA_SPAN(float, nchans);
    buf.interppixel_NDC(x, y, pixel, wrap);
    return C_to_tuple(cspan<const float>(pixel.data(), pixel.size()));
}

static nb::tuple
ImageBuf_interppixel_bicubic(const ImageBuf& buf, float x, float y,
                             const std::string& wrapname = "black")
{
    ImageBuf::WrapMode wrap = ImageBuf::WrapMode_from_string(wrapname);
    int nchans              = buf.nchannels();
    span<float> pixel       = OIIO_ALLOCA_SPAN(float, nchans);
    buf.interppixel_bicubic(x, y, pixel, wrap);
    return C_to_tuple(cspan<const float>(pixel.data(), pixel.size()));
}

static nb::tuple
ImageBuf_interppixel_bicubic_NDC(const ImageBuf& buf, float x, float y,
                                 const std::string& wrapname = "black")
{
    ImageBuf::WrapMode wrap = ImageBuf::WrapMode_from_string(wrapname);
    int nchans              = buf.nchannels();
    span<float> pixel       = OIIO_ALLOCA_SPAN(float, nchans);
    buf.interppixel_bicubic_NDC(x, y, pixel, wrap);
    return C_to_tuple(cspan<const float>(pixel.data(), pixel.size()));
}

static void
ImageBuf_setpixel(ImageBuf& buf, int x, int y, int z, nb::handle p)
{
    std::vector<float> pixel;
    py_to_stdvector(pixel, p);
    if (pixel.size())
        buf.setpixel(x, y, z, pixel);
}

static void
ImageBuf_setpixel2(ImageBuf& buf, int x, int y, nb::handle p)
{
    ImageBuf_setpixel(buf, x, y, 0, p);
}

static void
ImageBuf_setpixel1(ImageBuf& buf, int i, nb::handle p)
{
    std::vector<float> pixel;
    py_to_stdvector(pixel, p);
    if (pixel.size())
        buf.setpixel(i, pixel);
}

static nb::object
ImageBuf_get_pixels(const ImageBuf& buf, TypeDesc format, ROI roi = ROI::All())
{
    if (!roi.defined())
        roi = buf.roi();
    roi.chend = std::min(roi.chend, buf.nchannels());

    size_t size = static_cast<size_t>(roi.npixels()) * roi.nchannels()
                  * format.size();
    std::unique_ptr<std::byte[]> data(new std::byte[size]);
    if (buf.get_pixels(roi, format, make_span(data.get(), size)))
        return make_numpy_array(format, data.release(),
                                buf.spec().depth > 1 ? 4 : 3, roi.nchannels(),
                                roi.width(), roi.height(), roi.depth());
    else
        return nb::none();
}

static void
ImageBuf_set_deep_value(ImageBuf& buf, int x, int y, int z, int c, int s,
                        float value)
{
    buf.set_deep_value(x, y, z, c, s, value);
}

static void
ImageBuf_set_deep_value_uint(ImageBuf& buf, int x, int y, int z, int c, int s,
                             uint32_t value)
{
    buf.set_deep_value(x, y, z, c, s, value);
}

static bool
ImageBuf_set_pixels_buffer(ImageBuf& self, ROI roi, nb::handle buffer_obj)
{
    Py_buffer view;
    if (PyObject_GetBuffer(buffer_obj.ptr(), &view,
                           PyBUF_FORMAT | PyBUF_STRIDES)
        != 0) {
        PyErr_Clear();
        return false;
    }

    if (!roi.defined())
        roi = self.roi();
    roi.chend = std::min(roi.chend, self.nchannels());
    size_t size = static_cast<size_t>(roi.npixels()) * roi.nchannels();
    if (size == 0) {
        PyBuffer_Release(&view);
        return true;
    }
    oiio_bufinfo buf(view, roi.nchannels(), roi.width(), roi.height(),
                     roi.depth(), self.spec().depth > 1 ? 3 : 2);
    if (!buf.data || buf.error.size()) {
        self.errorfmt("set_pixels error: {}",
                      buf.error.size() ? buf.error.c_str() : "unspecified");
        PyBuffer_Release(&view);
        return false;
    }
    if (!buf.data || buf.size != size) {
        self.errorfmt(
            "ImageBuf.set_pixels: array size ({}) did not match ROI size w={} h={} d={} ch={} (total {})",
            buf.size, roi.width(), roi.height(), roi.depth(), roi.nchannels(),
            size);
        PyBuffer_Release(&view);
        return false;
    }

    nb::gil_scoped_release gil;
    auto bufspan = cspan_from_buffer(buf.data, buf.format, roi.nchannels(),
                                     roi.width(), roi.height(), roi.depth(),
                                     buf.xstride, buf.ystride, buf.zstride);
    bool ok      = self.set_pixels(roi, buf.format, bufspan, nullptr,
                                   buf.xstride, buf.ystride, buf.zstride);
    PyBuffer_Release(&view);
    return ok;
}

static void
ImageBuf_set_write_format(ImageBuf& self, nb::handle py_channelformats)
{
    std::vector<TypeDesc> formats;
    py_to_stdvector(formats, py_channelformats);
    self.set_write_format(formats);
}

static nb::bytes
ImageBuf_repr_png(const ImageBuf& self)
{
    ImageSpec original_spec = self.spec();

    if (original_spec.width < 1 || original_spec.height < 1) {
        return nb::bytes();
    }

    ImageSpec altered_spec = original_spec;
    altered_spec.attribute("oiio:dither", 1);

    std::vector<unsigned char> file_buffer;
    Filesystem::IOVecOutput file_vec(file_buffer);

    std::unique_ptr<ImageOutput> out = ImageOutput::create("temp.png",
                                                           &file_vec);
    if (!out || !out->open("temp.png", altered_spec))
        return nb::bytes();
    self.write(out.get());
    out->close();

    const char* char_ptr = reinterpret_cast<const char*>(file_buffer.data());
    return nb::bytes(char_ptr, file_buffer.size());
}

void
declare_imagebuf(nb::module_& m)
{
    nb::class_<ImageBuf>(m, "ImageBuf")
        .def(nb::init<>())
        .def(nb::init<const std::string&>())
        .def(nb::init<const std::string&, int, int>())
        .def(nb::init<const ImageSpec&>())
        .def("__init__",
             [](ImageBuf* self, const ImageSpec& spec, bool zero) {
                 auto z = zero ? InitializePixels::Yes : InitializePixels::No;
                 new (self) ImageBuf(spec, z);
             },
             "spec"_a, "zero"_a = true)
        .def("__init__",
             [](ImageBuf* self, const std::string& name, int subimage,
                int miplevel, const ImageSpec& config) {
                 new (self) ImageBuf(name, subimage, miplevel, nullptr,
                                     &config);
             },
             "name"_a, "subimage"_a, "miplevel"_a, "config"_a)
        .def("__init__",
             [](ImageBuf* self, nb::handle buffer) {
                 new (self) ImageBuf(ImageBuf_from_buffer(buffer));
             },
             "buffer"_a)
        .def("clear", &ImageBuf::clear)
        .def(
            "reset",
            [](ImageBuf& self, const std::string& name, int subimage,
               int miplevel) { self.reset(name, subimage, miplevel); },
            "name"_a, "subimage"_a = 0, "miplevel"_a = 0)
        .def(
            "reset",
            [](ImageBuf& self, const std::string& name, int subimage,
               int miplevel, const ImageSpec& config) {
                self.reset(name, subimage, miplevel, nullptr, &config);
            },
            "name"_a, "subimage"_a = 0, "miplevel"_a = 0,
            "config"_a = ImageSpec())
        .def(
            "reset",
            [](ImageBuf& self, const ImageSpec& spec, bool zero) {
                auto z = zero ? InitializePixels::Yes : InitializePixels::No;
                self.reset(spec, z);
            },
            "spec"_a, "zero"_a = true)
        .def(
            "reset",
            [](ImageBuf& self, nb::handle buffer) {
                self = ImageBuf_from_buffer(buffer);
            },
            "buffer"_a)
        .def_prop_ro(
            "initialized",
            [](const ImageBuf& self) { return self.initialized(); })
        .def(
            "init_spec",
            [](ImageBuf& self, std::string filename, int subimage,
               int miplevel) {
                nb::gil_scoped_release gil;
                return self.init_spec(filename, subimage, miplevel);
            },
            "filename"_a, "subimage"_a = 0, "miplevel"_a = 0)
        .def(
            "read",
            [](ImageBuf& self, int subimage, int miplevel, int chbegin,
               int chend, bool force, TypeDesc convert) {
                nb::gil_scoped_release gil;
                return self.read(subimage, miplevel, chbegin, chend, force,
                                 convert);
            },
            "subimage"_a, "miplevel"_a, "chbegin"_a, "chend"_a, "force"_a,
            "convert"_a)
        .def(
            "read",
            [](ImageBuf& self, int subimage, int miplevel, bool force,
               TypeDesc convert) {
                nb::gil_scoped_release gil;
                return self.read(subimage, miplevel, force, convert);
            },
            "subimage"_a = 0, "miplevel"_a = 0, "force"_a = false,
            "convert"_a = TypeUnknown)
        .def(
            "write",
            [](ImageBuf& self, const std::string& filename, TypeDesc dtype,
               const std::string& fileformat) {
                nb::gil_scoped_release gil;
                return self.write(filename, dtype, fileformat);
            },
            "filename"_a, "dtype"_a = TypeUnknown, "fileformat"_a = "")
        .def(
            "write",
            [](ImageBuf& self, ImageOutput& out) {
                nb::gil_scoped_release gil;
                return self.write(&out);
            },
            "out"_a)
        .def(
            "make_writable",
            [](ImageBuf& self, bool keep_cache_type) {
                nb::gil_scoped_release gil;
                return self.make_writable(keep_cache_type);
            },
            "keep_cache_type"_a = false)
        .def("set_write_format", &ImageBuf_set_write_format)
        .def("set_write_tiles", &ImageBuf::set_write_tiles, "width"_a = 0,
             "height"_a = 0, "depth"_a = 0)
        .def("spec", &ImageBuf::spec, nb::rv_policy::reference_internal)
        .def("nativespec", &ImageBuf::nativespec,
             nb::rv_policy::reference_internal)
        .def("specmod", &ImageBuf::specmod, nb::rv_policy::reference_internal)
        .def_prop_ro("has_thumbnail",
                     [](const ImageBuf& self) { return self.has_thumbnail(); })
        .def("clear_thumbnail", &ImageBuf::clear_thumbnail)
        .def("set_thumbnail", &ImageBuf::set_thumbnail, "thumb"_a)
        .def("get_thumbnail",
             [](const ImageBuf& self) { return *self.get_thumbnail(); })
        .def_prop_ro("name",
                     [](const ImageBuf& self) {
                         return std::string(self.name());
                     })
        .def_prop_ro("file_format_name",
                     [](const ImageBuf& self) {
                         return std::string(self.file_format_name());
                     })
        .def_prop_ro("subimage", &ImageBuf::subimage)
        .def_prop_ro("nsubimages", &ImageBuf::nsubimages)
        .def_prop_ro("miplevel", &ImageBuf::miplevel)
        .def_prop_ro("nmiplevels", &ImageBuf::nmiplevels)
        .def_prop_ro("nchannels", &ImageBuf::nchannels)
        .def_prop_rw("orientation", &ImageBuf::orientation,
                     &ImageBuf::set_orientation)
        .def_prop_ro("oriented_width", &ImageBuf::oriented_width)
        .def_prop_ro("oriented_height", &ImageBuf::oriented_height)
        .def_prop_ro("oriented_x", &ImageBuf::oriented_x)
        .def_prop_ro("oriented_y", &ImageBuf::oriented_y)
        .def_prop_ro("oriented_full_width", &ImageBuf::oriented_full_width)
        .def_prop_ro("oriented_full_height", &ImageBuf::oriented_full_height)
        .def_prop_ro("oriented_full_x", &ImageBuf::oriented_full_x)
        .def_prop_ro("oriented_full_y", &ImageBuf::oriented_full_y)
        .def_prop_ro("xbegin", &ImageBuf::xbegin)
        .def_prop_ro("xend", &ImageBuf::xend)
        .def_prop_ro("ybegin", &ImageBuf::ybegin)
        .def_prop_ro("yend", &ImageBuf::yend)
        .def_prop_ro("zbegin", &ImageBuf::zbegin)
        .def_prop_ro("zend", &ImageBuf::zend)
        .def_prop_ro("xmin", &ImageBuf::xmin)
        .def_prop_ro("xmax", &ImageBuf::xmax)
        .def_prop_ro("ymin", &ImageBuf::ymin)
        .def_prop_ro("ymax", &ImageBuf::ymax)
        .def_prop_ro("zmin", &ImageBuf::zmin)
        .def_prop_ro("zmax", &ImageBuf::zmax)
        .def_prop_ro("roi", &ImageBuf::roi)
        .def_prop_rw("roi_full", &ImageBuf::roi_full, &ImageBuf::set_roi_full)
        .def("set_origin", &ImageBuf::set_origin, "x"_a, "y"_a, "z"_a = 0)
        .def("set_full", &ImageBuf::set_full)
        .def_prop_ro("pixels_valid", &ImageBuf::pixels_valid)
        .def_prop_ro("pixeltype", &ImageBuf::pixeltype)
        .def_prop_ro("has_error", &ImageBuf::has_error)
        .def(
            "geterror",
            [](const ImageBuf& self, bool clear) {
                return std::string(self.geterror(clear));
            },
            "clear"_a = true)
        .def("pixelindex", &ImageBuf::pixelindex, "x"_a, "y"_a, "z"_a,
             "check_range"_a = false)
        .def(
            "copy",
            [](ImageBuf& self, const ImageBuf& src, TypeDesc format) {
                nb::gil_scoped_release gil;
                return self.copy(src, format);
            },
            "src"_a, "format"_a = TypeUnknown)
        .def(
            "copy",
            [](const ImageBuf& src, TypeDesc format) {
                nb::gil_scoped_release gil;
                return src.copy(format);
            },
            "format"_a = TypeUnknown)
        .def("copy_pixels", &ImageBuf::copy_pixels)
        .def("copy_metadata", &ImageBuf::copy_metadata)
        .def(
            "merge_metadata",
            [](ImageBuf& self, const ImageBuf& src, bool override,
               const std::string& pattern) {
                self.merge_metadata(src, override, pattern);
            },
            "src"_a, "override"_a = false, "pattern"_a = "")
        .def("swap", &ImageBuf::swap)
        .def("getchannel", &ImageBuf::getchannel, "x"_a, "y"_a, "z"_a, "c"_a,
             "wrap"_a = "black")
        .def("getpixel", &ImageBuf_getpixel, "x"_a, "y"_a, "z"_a = 0,
             "wrap"_a = "black")
        .def("interppixel", &ImageBuf_interppixel, "x"_a, "y"_a,
             "wrap"_a = "black")
        .def("interppixel_NDC", &ImageBuf_interppixel_NDC, "x"_a, "y"_a,
             "wrap"_a = "black")
        .def("interppixel_NDC_full", &ImageBuf_interppixel_NDC, "x"_a, "y"_a,
             "wrap"_a = "black")
        .def("interppixel_bicubic", &ImageBuf_interppixel_bicubic, "x"_a, "y"_a,
             "wrap"_a = "black")
        .def("interppixel_bicubic_NDC", &ImageBuf_interppixel_bicubic_NDC,
             "x"_a, "y"_a, "wrap"_a = "black")
        .def("setpixel", &ImageBuf_setpixel, "x"_a, "y"_a, "z"_a, "pixel"_a)
        .def("setpixel", &ImageBuf_setpixel2, "x"_a, "y"_a, "pixel"_a)
        .def("setpixel", &ImageBuf_setpixel1, "i"_a, "pixel"_a)
        .def("get_pixels", &ImageBuf_get_pixels, "format"_a = TypeFloat,
             "roi"_a = ROI::All())
        .def("set_pixels", &ImageBuf_set_pixels_buffer, "roi"_a, "pixels"_a)
        .def_prop_ro("deep", &ImageBuf::deep)
        .def("deep_samples", &ImageBuf::deep_samples, "x"_a, "y"_a, "z"_a = 0)
        .def("set_deep_samples", &ImageBuf::set_deep_samples, "x"_a, "y"_a,
             "z"_a = 0, "nsamples"_a = 1)
        .def("deep_insert_samples", &ImageBuf::deep_insert_samples, "x"_a,
             "y"_a, "z"_a = 0, "samplepos"_a, "nsamples"_a = 1)
        .def("deep_erase_samples", &ImageBuf::deep_erase_samples, "x"_a, "y"_a,
             "z"_a = 0, "samplepos"_a, "nsamples"_a = 1)
        .def("deep_value", &ImageBuf::deep_value, "x"_a, "y"_a, "z"_a,
             "channel"_a, "sample"_a)
        .def("deep_value_uint", &ImageBuf::deep_value_uint, "x"_a, "y"_a, "z"_a,
             "channel"_a, "sample"_a)
        .def("set_deep_value", &ImageBuf_set_deep_value, "x"_a, "y"_a, "z"_a,
             "channel"_a, "sample"_a, "value"_a = 0.0f)
        .def("set_deep_value_uint", &ImageBuf_set_deep_value_uint, "x"_a, "y"_a,
             "z"_a, "channel"_a, "sample"_a, "value"_a = 0)
        .def(
            "deepdata",
            [](const ImageBuf& self) { return *self.deepdata(); },
            nb::rv_policy::copy)
        .def("_repr_png_", &ImageBuf_repr_png);
}

}  // namespace PyOpenImageIO
