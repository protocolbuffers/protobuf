// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
