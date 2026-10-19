// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/compiler/plugin.h"

int main(int argc, char *argv[]) {
  ::google::protobuf::compiler::java::JavaGenerator generator;
#ifdef GOOGLE_PROTOBUF_RUNTIME_INCLUDE_BASE
  generator.set_opensource_runtime(true);
#endif
  return ::google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
