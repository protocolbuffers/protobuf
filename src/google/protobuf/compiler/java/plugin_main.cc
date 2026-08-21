#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/compiler/plugin.h"

int main(int argc, char *argv[]) {
  ::google::protobuf::compiler::java::JavaGenerator generator;
#ifdef GOOGLE_PROTOBUF_RUNTIME_INCLUDE_BASE
  generator.set_opensource_runtime(true);
#endif
  return ::google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
