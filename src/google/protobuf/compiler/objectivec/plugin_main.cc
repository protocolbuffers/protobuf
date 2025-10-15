// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/generator.h"
#include "google/protobuf/compiler/plugin.h"

int main(int argc, char *argv[]) {
  google::protobuf::compiler::objectivec::ObjectiveCGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
