// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#include "py_oiio.h"

#include <OpenImageIO/argparse.h>
#include <OpenImageIO/ustring.h>

namespace PyOpenImageIO {

using namespace OIIO;

namespace {

nb::list
cspan_argv_to_list(cspan<const char*> sp)
{
    nb::list lst;
    for (size_t i = 0; i < sp.size(); ++i)
        lst.append(nb::str(sp[i]));
    return lst;
}

}  // namespace

void
declare_argparse(nb::module_& m)
{
    using Arg = ArgParse::Arg;

    nb::class_<Arg>(m, "ArgParseArg")
        .def("help",
             [](Arg& self, const std::string& s) -> Arg& {
                 return self.help(s);
             },
             nb::rv_policy::reference_internal)
        .def("nargs",
             [](Arg& self, int n) -> Arg& { return self.nargs(n); },
             nb::rv_policy::reference_internal)
        .def("metavar",
             [](Arg& self, const std::string& s) -> Arg& {
                 return self.metavar(s);
             },
             nb::rv_policy::reference_internal)
        .def("dest",
             [](Arg& self, const std::string& s) -> Arg& {
                 return self.dest(s);
             },
             nb::rv_policy::reference_internal)
        .def("defaultval",
             [](Arg& self, int v) -> Arg& { return self.defaultval(v); },
             nb::rv_policy::reference_internal)
        .def("defaultval",
             [](Arg& self, float v) -> Arg& { return self.defaultval(v); },
             nb::rv_policy::reference_internal)
        .def("defaultval",
             [](Arg& self, double v) -> Arg& { return self.defaultval(v); },
             nb::rv_policy::reference_internal)
        .def("defaultval",
             [](Arg& self, const std::string& v) -> Arg& {
                 return self.defaultval(v);
             },
             nb::rv_policy::reference_internal)
        .def("hidden", [](Arg& self) -> Arg& { return self.hidden(); },
             nb::rv_policy::reference_internal)
        .def("always_run",
             [](Arg& self) -> Arg& { return self.always_run(); },
             nb::rv_policy::reference_internal)
        .def("store_true",
             [](Arg& self) -> Arg& { return self.store_true(); },
             nb::rv_policy::reference_internal)
        .def("store_false",
             [](Arg& self) -> Arg& { return self.store_false(); },
             nb::rv_policy::reference_internal)
        .def(
            "action",
            [](Arg& self, nb::callable cb) -> Arg& {
                self.action([cb](Arg& arg, cspan<const char*> myargs) {
                    nb::gil_scoped_acquire gil;
                    nb::object py_arg
                        = nb::cast(&arg, nb::rv_policy::reference_internal);
                    cb(py_arg, cspan_argv_to_list(myargs));
                });
                return self;
            },
            nb::rv_policy::reference_internal)
        .def("action_store_int",
             [](Arg& self) -> Arg& {
                 self.action(ArgParse::store<int>());
                 return self;
             },
             nb::rv_policy::reference_internal)
        .def("action_store_float",
             [](Arg& self) -> Arg& {
                 self.action(ArgParse::store<float>());
                 return self;
             },
             nb::rv_policy::reference_internal)
        .def("action_store_string",
             [](Arg& self) -> Arg& {
                 self.action(ArgParse::store<ustring>());
                 return self;
             },
             nb::rv_policy::reference_internal)
        .def("action_append",
             [](Arg& self) -> Arg& {
                 self.action(ArgParse::append<ustring>());
                 return self;
             },
             nb::rv_policy::reference_internal)
        .def("action_do_nothing",
             [](Arg& self) -> Arg& {
                 self.action(ArgParse::do_nothing());
                 return self;
             },
             nb::rv_policy::reference_internal)
        .def("store_const",
             [](Arg& self, int v) -> Arg& {
                 self.action(ArgParse::store_const(v));
                 return self;
             },
             nb::rv_policy::reference_internal)
        .def("store_const",
             [](Arg& self, float v) -> Arg& {
                 self.action(ArgParse::store_const(v));
                 return self;
             },
             nb::rv_policy::reference_internal)
        .def("store_const",
             [](Arg& self, const std::string& v) -> Arg& {
                 self.action(ArgParse::store_const(ustring(v)));
                 return self;
             },
             nb::rv_policy::reference_internal)
        .def_prop_ro(
            "name",
            [](const Arg& self) { return std::string(self.name()); })
        .def_prop_ro(
            "dest",
            [](const Arg& self) { return std::string(self.dest()); })
        .def("argparse",
             [](Arg& self) -> ArgParse& { return self.argparse(); },
             nb::rv_policy::reference_internal);

    nb::class_<ArgParse>(m, "ArgParse")
        .def(nb::init<>())
        .def(
            "intro",
            [](ArgParse& self, const std::string& s) -> ArgParse& {
                return self.intro(s);
            },
            nb::rv_policy::reference_internal)
        .def(
            "usage",
            [](ArgParse& self, const std::string& s) -> ArgParse& {
                return self.usage(s);
            },
            nb::rv_policy::reference_internal)
        .def(
            "description",
            [](ArgParse& self, const std::string& s) -> ArgParse& {
                return self.description(s);
            },
            nb::rv_policy::reference_internal)
        .def(
            "epilog",
            [](ArgParse& self, const std::string& s) -> ArgParse& {
                return self.epilog(s);
            },
            nb::rv_policy::reference_internal)
        .def(
            "prog",
            [](ArgParse& self, const std::string& s) -> ArgParse& {
                return self.prog(s);
            },
            nb::rv_policy::reference_internal)
        .def(
            "print_defaults",
            [](ArgParse& self, bool p) -> ArgParse& {
                return self.print_defaults(p);
            },
            nb::rv_policy::reference_internal)
        .def(
            "add_help",
            [](ArgParse& self, bool h) -> ArgParse& {
                return self.add_help(h);
            },
            nb::rv_policy::reference_internal)
        .def(
            "add_version",
            [](ArgParse& self, const std::string& v) -> ArgParse& {
                return self.add_version(v);
            },
            nb::rv_policy::reference_internal)
        .def(
            "exit_on_error",
            [](ArgParse& self, bool e) -> ArgParse& {
                return self.exit_on_error(e);
            },
            nb::rv_policy::reference_internal)
        .def("abort",
             [](ArgParse& self, bool aborted) { self.abort(aborted); },
             "aborted"_a = true)
        .def_prop_ro("aborted", &ArgParse::aborted)
        .def_prop_rw(
            "running",
            [](const ArgParse& self) { return self.running(); },
            [](ArgParse& self, bool r) { self.running(r); })
        .def_prop_ro("current_arg", &ArgParse::current_arg)
        .def("set_next_arg", &ArgParse::set_next_arg)
        .def(
            "parse_args",
            [](ArgParse& self, const std::vector<std::string>& argv) {
                // Hold storage so c_str() pointers stay valid for the call.
                std::vector<std::string> storage = argv;
                std::vector<const char*> ptrs;
                ptrs.reserve(storage.size());
                for (auto& s : storage)
                    ptrs.push_back(s.c_str());
                const int argc = static_cast<int>(ptrs.size());
                const char** argv_pp = const_cast<const char**>(ptrs.data());
                return self.parse_args(argc, argv_pp);
            },
            "argv"_a)
        .def_prop_ro("has_error", &ArgParse::has_error)
        .def("geterror",
             [](const ArgParse& self, bool clear) {
                 return std::string(self.geterror(clear));
             },
             "clear"_a = true)
        .def_prop_ro("prog_name",
                     [](const ArgParse& self) {
                         return std::string(self.prog_name());
                     })
        .def("print_help", &ArgParse::print_help)
        .def("briefusage", &ArgParse::briefusage)
        .def_prop_ro("command_line",
                     [](const ArgParse& self) {
                         return std::string(self.command_line());
                     })
        .def(
            "add_argument",
            [](ArgParse& self, const char* argname) -> Arg& {
                return self.add_argument(argname);
            },
            nb::rv_policy::reference_internal)
        .def(
            "arg",
            [](ArgParse& self, const char* argname) -> Arg& {
                return self.arg(argname);
            },
            nb::rv_policy::reference_internal)
        .def(
            "separator",
            [](ArgParse& self, const std::string& text) -> Arg& {
                return self.separator(text);
            },
            nb::rv_policy::reference_internal)
        .def(
            "params",
            [](ArgParse& self) -> ParamValueList& { return self.params(); },
            nb::rv_policy::reference_internal)
        .def(
            "__getitem__",
            [](const ArgParse& self, const std::string& key) {
                const ParamValueList& pl = self.cparams();
                auto p                   = pl.find(key);
                if (p == pl.end()) {
                    throw nb::key_error(
                        ("ArgParse: key '" + key + "' does not exist").c_str());
                }
                return make_pyobject(p->data(), p->type(), p->nvalues());
            });
}

}  // namespace PyOpenImageIO
