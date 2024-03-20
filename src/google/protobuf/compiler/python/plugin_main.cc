#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/compiler/python/generator.h"

int main(int argc, char *argv[]) {
  ::google::protobuf::compiler::python::Generator generator;
#ifdef GOOGLE_PROTOBUF_RUNTIME_INCLUDE_BASE
  generator.set_opensource_runtime(true);
  generator.set_runtime_include_base(GOOGLE_PROTOBUF_RUNTIME_INCLUDE_BASE);
#endif
  return ::google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
