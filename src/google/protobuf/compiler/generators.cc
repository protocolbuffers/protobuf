// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/generators.h"

#include <memory>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/cpp/generator.h"
#include "google/protobuf/compiler/csharp/csharp_generator.h"
#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/compiler/objectivec/objectivec_generator.h"
#include "google/protobuf/compiler/php/php_generator.h"
#include "google/protobuf/compiler/python/generator.h"
#include "google/protobuf/compiler/ruby/ruby_generator.h"

namespace google {
namespace protobuf {
namespace compiler {

std::unique_ptr<CodeGenerator> CreateGenerator(GeneratorLanguage lang) {
  switch(lang) {
    case GeneratorLanguage::kCpp:
      return std::make_unique<cpp::CppGenerator>();
    case GeneratorLanguage::kCsharp:
      return std::make_unique<csharp::Generator>();
    case GeneratorLanguage::kJava:
      return std::make_unique<java::JavaGenerator>();
    case GeneratorLanguage::kObjectivec:
      return std::make_unique<objectivec::ObjectiveCGenerator>();
    case GeneratorLanguage::kPhp:
      return std::make_unique<php::Generator>();
    case GeneratorLanguage::kPython:
      return std::make_unique<python::Generator>();
    case GeneratorLanguage::kRuby:
      return std::make_unique<ruby::Generator>();
  }
  return nullptr;
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
