// The opposite side of go/fastpythonproto.
#include <Python.h>

namespace google {
namespace protobuf {
namespace python {

static const char* kModuleName = "use_pure_python";
static const char kModuleDocstring[] =
    "The presence of this module in a build's deps signals to\n"
    "net.google.protobuf.internal.api_implementation that the pure\n"
    "Python protobuf implementation should be used.\n";

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


PyMODINIT_FUNC PyInit_use_pure_python() {
  PyObject* module = PyModule_Create(&_module);
  if (module == nullptr) {
    return nullptr;
  }


  return module;
}
}

}  // namespace python
}  // namespace protobuf
}  // namespace google
