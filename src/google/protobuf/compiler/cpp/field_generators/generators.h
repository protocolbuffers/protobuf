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

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_GENERATORS_GENERATORS_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_GENERATORS_GENERATORS_H__

#include <memory>

#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/helpers.h"

// The functions in this file construct FieldGeneratorBase objects for
// generating different "codegen types" of fields. The logic for selecting the
// correct choice of generator lives in compiler/cpp/field.cc; this is merely
// the API that file uses for constructing generators.
//
// Functions are of the form `Make<card><kind>Generator()`, where <card> is
// `Singular`, `Repeated`, or `Oneof`, and <kind> is the field type, plus
// `MakeMapGenerator()`, since map fields are always repeated message fields.
//
// The returned pointers are never null.

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
std::unique_ptr<FieldGeneratorBase> MakeSinguarPrimitiveGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc);

std::unique_ptr<FieldGeneratorBase> MakeRepeatedPrimitiveGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc);

std::unique_ptr<FieldGeneratorBase> MakeSinguarEnumGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc);

std::unique_ptr<FieldGeneratorBase> MakeRepeatedEnumGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc);

std::unique_ptr<FieldGeneratorBase> MakeSinguarStringGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc);

std::unique_ptr<FieldGeneratorBase> MakeRepeatedStringGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc);

std::unique_ptr<FieldGeneratorBase> MakeSinguarMessageGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc);

std::unique_ptr<FieldGeneratorBase> MakeRepeatedMessageGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc);

std::unique_ptr<FieldGeneratorBase> MakeOneofMessageGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc);

std::unique_ptr<FieldGeneratorBase> MakeMapGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc);
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_GENERATORS_GENERATORS_H__
