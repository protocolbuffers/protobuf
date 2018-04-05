#include <util/gogoproto_generator.h>
#include "google/protobuf/compiler/plugin.h"

int main(int argc, char* argv[]) {
  google::protobuf::compiler::GoGoProtoGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
