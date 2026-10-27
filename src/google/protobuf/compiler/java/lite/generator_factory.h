// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_LITE_GENERATOR_FACTORY_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_LITE_GENERATOR_FACTORY_H__

#include "google/protobuf/compiler/java/generator_factory.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

std::unique_ptr<GeneratorFactory> MakeImmutableLiteGeneratorFactory(
    Context* context);

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_LITE_GENERATOR_FACTORY_H__
