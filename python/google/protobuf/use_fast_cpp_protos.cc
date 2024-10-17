// go/fastpythonproto
#include <Python.h>

namespace google {
namespace protobuf {
namespace python {

static const char* kModuleName = "_use_fast_cpp_protos";
static const char kModuleDocstring[] =
    "The presence of this module in a build's deps signals to\n"
    "net.google.protobuf.internal.api_implementation that the fast\n"
    "C++ protobuf implementation should be the default.\n";

static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                     kModuleName,
                                     kModuleDocstring,
                                     -1,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr};

extern "C" {


PyMODINIT_FUNC PyInit__use_fast_cpp_protos() {
  PyObject* module = PyModule_Create(&_module);
  if (module == nullptr) {
    return nullptr;
  }

  return module;
}

}  // extern "C"

}  // namespace python
}  // namespace protobuf
}  // namespace google
