
#include <google/protobuf/compiler/plugin.h>

#include "upbc/generator.h"

int main(int argc, char** argv) {
  return google::protobuf::compiler::PluginMain(argc, argv,
                                                upbc::GetGenerator().get());
}
