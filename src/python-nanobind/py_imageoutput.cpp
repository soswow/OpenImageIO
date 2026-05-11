// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#undef SIZEOF_LONG
#include "py_oiio.h"

#include <OpenImageIO/deepdata.h>

namespace PyOpenImageIO {

static bool
ImageOutput_open_specs(ImageOutput& self, const std::string& name,
                         nb::tuple specs)
{
    const size_t length = specs.size();
    if (length == 0)
        return false;
    std::vector<ImageSpec> Cspecs(length);
    for (size_t i = 0; i < length; ++i) {
        nb::handle s = specs[i];
        if (nb::isinstance<ImageSpec>(s))
            Cspecs[i] = nb::cast<ImageSpec>(s);
        else
            return false;
    }
    return self.open(name, int(length), &Cspecs[0]);
}



static bool
ImageOutput_write_scanline(ImageOutput& self, int y, int z, nb::handle buffer)
{
    Py_buffer view {};
    if (PyObject_GetBuffer(buffer.ptr(), &view,
                             PyBUF_FORMAT | PyBUF_STRIDES)
        != 0) {
        PyErr_Clear();
        return false;
    }
    const ImageSpec& spec(self.spec());
    if (spec.tile_width != 0) {
        self.errorfmt("Cannot write scanlines to a tiled file.");
        PyBuffer_Release(&view);
        return false;
    }
    oiio_bufinfo buf(view, spec.nchannels, spec.width, 1, 1, 1);
    PyBuffer_Release(&view);
    if (!buf.data || buf.error.size()) {
        self.errorfmt("Pixel data array error: {}",
                      buf.error.size() ? buf.error.c_str() : "unspecified");
        return false;
    }
    if (static_cast<int>(buf.size)
        < self.spec().width * self.spec().nchannels) {
        self.errorfmt("write_scanlines was not passed a long enough array");
        return false;
    }
    nb::gil_scoped_release gil;
    return self.write_scanline(y, z, buf.format, buf.data, buf.xstride);
}



static bool
ImageOutput_write_scanlines(ImageOutput& self, int ybegin, int yend, int z,
                            nb::handle buffer)
{
    Py_buffer view {};
    if (PyObject_GetBuffer(buffer.ptr(), &view,
                             PyBUF_FORMAT | PyBUF_STRIDES)
        != 0) {
        PyErr_Clear();
        return false;
    }
    const ImageSpec& spec(self.spec());
    if (spec.tile_width != 0) {
        self.errorfmt("Cannot write scanlines to a filed file.");
        PyBuffer_Release(&view);
        return false;
    }
    oiio_bufinfo buf(view, spec.nchannels, spec.width, yend - ybegin, 1, 2);
    PyBuffer_Release(&view);
    if (!buf.data || buf.error.size()) {
        self.errorfmt("Pixel data array error: {}",
                      buf.error.size() ? buf.error.c_str() : "unspecified");
        return false;
    }
    if (static_cast<int>(buf.size)
        < self.spec().width * self.spec().nchannels * (yend - ybegin)) {
        self.errorfmt("write_scanlines was not passed a long enough array");
        return false;
    }
    nb::gil_scoped_release gil;
    return self.write_scanlines(ybegin, yend, z, buf.format, buf.data,
                                buf.xstride, buf.ystride);
}



static bool
ImageOutput_write_tile(ImageOutput& self, int x, int y, int z,
                       nb::handle buffer)
{
    Py_buffer view {};
    if (PyObject_GetBuffer(buffer.ptr(), &view,
                             PyBUF_FORMAT | PyBUF_STRIDES)
        != 0) {
        PyErr_Clear();
        return false;
    }
    const ImageSpec& spec(self.spec());
    if (spec.tile_width == 0) {
        self.errorfmt("Cannot write tiles to a scanline file.");
        PyBuffer_Release(&view);
        return false;
    }
    oiio_bufinfo buf(view, spec.nchannels, spec.tile_width, spec.tile_height,
                     spec.tile_depth, spec.tile_depth > 1 ? 3 : 2);
    PyBuffer_Release(&view);
    if (!buf.data || buf.error.size()) {
        self.errorfmt("Pixel data array error: {}",
                      buf.error.size() ? buf.error.c_str() : "unspecified");
        return false;
    }
    if (buf.size < self.spec().tile_pixels() * self.spec().nchannels) {
        self.errorfmt("write_tile was not passed a long enough array");
        return false;
    }
    nb::gil_scoped_release gil;
    return self.write_tile(x, y, z, buf.format, buf.data, buf.xstride,
                           buf.ystride, buf.zstride);
}



static bool
ImageOutput_write_tiles(ImageOutput& self, int xbegin, int xend, int ybegin,
                        int yend, int zbegin, int zend, nb::handle buffer)
{
    Py_buffer view {};
    if (PyObject_GetBuffer(buffer.ptr(), &view,
                             PyBUF_FORMAT | PyBUF_STRIDES)
        != 0) {
        PyErr_Clear();
        return false;
    }
    const ImageSpec& spec(self.spec());
    if (spec.tile_width == 0) {
        self.errorfmt("Cannot write tiles to a scanline file.");
        PyBuffer_Release(&view);
        return false;
    }
    oiio_bufinfo buf(view, spec.nchannels, xend - xbegin, yend - ybegin,
                     zend - zbegin, spec.tile_depth > 1 ? 3 : 2);
    PyBuffer_Release(&view);
    if (!buf.data || buf.error.size()) {
        self.errorfmt("Pixel data array error: {}",
                      buf.error.size() ? buf.error.c_str() : "unspecified");
        return false;
    }
    if (static_cast<int>(buf.size) < (xend - xbegin) * (yend - ybegin)
                                         * (zend - zbegin)
                                         * self.spec().nchannels) {
        self.errorfmt("write_tiles was not passed a long enough array");
        return false;
    }
    nb::gil_scoped_release gil;
    return self.write_tiles(xbegin, xend, ybegin, yend, zbegin, zend,
                            buf.format, buf.data, buf.xstride, buf.ystride,
                            buf.zstride);
}



static bool
ImageOutput_write_image(ImageOutput& self, nb::handle buffer)
{
    Py_buffer view {};
    if (PyObject_GetBuffer(buffer.ptr(), &view,
                             PyBUF_FORMAT | PyBUF_STRIDES)
        != 0) {
        PyErr_Clear();
        return false;
    }
    const ImageSpec& spec(self.spec());
    oiio_bufinfo buf(view, spec.nchannels, spec.width, spec.height,
                     spec.depth, spec.depth > 1 ? 3 : 2);
    PyBuffer_Release(&view);
    if (!buf.data || buf.size < spec.image_pixels() * spec.nchannels
        || buf.error.size()) {
        self.errorfmt("Pixel data array error: {}",
                      buf.error.size() ? buf.error.c_str() : "unspecified");
        return false;
    }
    nb::gil_scoped_release gil;
    return self.write_image(buf.format, buf.data, buf.xstride, buf.ystride,
                            buf.zstride);
}



static bool
ImageOutput_write_deep_scanlines(ImageOutput& self, int ybegin, int yend, int z,
                                 const DeepData& deepdata)
{
    nb::gil_scoped_release gil;
    return self.write_deep_scanlines(ybegin, yend, z, deepdata);
}



static bool
ImageOutput_write_deep_tiles(ImageOutput& self, int xbegin, int xend,
                             int ybegin, int yend, int zbegin, int zend,
                             const DeepData& deepdata)
{
    nb::gil_scoped_release gil;
    return self.write_deep_tiles(xbegin, xend, ybegin, yend, zbegin, zend,
                                 deepdata);
}



static bool
ImageOutput_write_deep_image(ImageOutput& self, const DeepData& deepdata)
{
    nb::gil_scoped_release gil;
    return self.write_deep_image(deepdata);
}



void
declare_imageoutput(nb::module_& m)
{
    nb::class_<ImageOutput>(m, "ImageOutput")
        .def_static(
            "create",
            [](const std::string& filename, const std::string& searchpath)
                -> ImageOutput* {
                return ImageOutput::create(filename, nullptr, searchpath)
                    .release();
            },
            nb::rv_policy::take_ownership, "filename"_a,
            "plugin_searchpath"_a = "")
        .def("format_name", &ImageOutput::format_name)
        .def(
            "supports",
            [](const ImageOutput& self, const std::string& feature) {
                return self.supports(feature);
            })
        .def("spec", &ImageOutput::spec)
        .def(
            "open",
            [](ImageOutput& self, const std::string& name,
               const ImageSpec& newspec, const std::string& modestr) {
                ImageOutput::OpenMode mode = ImageOutput::Create;
                if (Strutil::iequals(modestr, "AppendSubimage"))
                    mode = ImageOutput::AppendSubimage;
                else if (Strutil::iequals(modestr, "AppendMIPLevel"))
                    mode = ImageOutput::AppendMIPLevel;
                else if (!Strutil::iequals(modestr, "Create"))
                    throw std::invalid_argument(
                        Strutil::fmt::format("Unknown open mode '{}'", modestr));
                return self.open(name, newspec, mode);
            },
            "filename"_a, "spec"_a, "mode"_a = "Create")
        .def(
            "open",
            [](ImageOutput& self, const std::string& name,
               const std::vector<ImageSpec>& specs) {
                return self.open(name, (int)specs.size(), &specs[0]);
            },
            "filename"_a, "specs"_a)
        .def("open", &ImageOutput_open_specs)
        .def("close", [](ImageOutput& self) { return self.close(); })
        .def("write_image", &ImageOutput_write_image)
        .def("write_scanline", &ImageOutput_write_scanline, "y"_a, "z"_a,
             "pixels"_a)
        .def("write_scanlines", &ImageOutput_write_scanlines, "ybegin"_a,
             "yend"_a, "z"_a, "pixels"_a)
        .def("write_tile", &ImageOutput_write_tile, "x"_a, "y"_a, "z"_a,
             "pixels"_a)
        .def("write_tiles", &ImageOutput_write_tiles, "xbegin"_a, "xend"_a,
             "ybegin"_a, "yend"_a, "zbegin"_a, "zend"_a, "pixels"_a)
        .def("write_deep_scanlines", &ImageOutput_write_deep_scanlines,
             "ybegin"_a, "yend"_a, "z"_a, "deepdata"_a)
        .def("write_deep_tiles", &ImageOutput_write_deep_tiles, "xbegin"_a,
             "xend"_a, "ybegin"_a, "yend"_a, "zbegin"_a, "zend"_a, "deepdata"_a)
        .def("write_deep_image", &ImageOutput_write_deep_image)
        .def("set_thumbnail",
             [](ImageOutput& self, const ImageBuf& thumb) {
                 return self.set_thumbnail(thumb);
             })
        .def("copy_image",
             [](ImageOutput& self, ImageInput& in) {
                 return self.copy_image(&in);
             })
        .def_prop_ro("has_error", &ImageOutput::has_error)
        .def(
            "geterror",
            [](ImageOutput& self, bool clear) {
                return std::string(self.geterror(clear));
            },
            "clear"_a = true);
}

}  // namespace PyOpenImageIO
