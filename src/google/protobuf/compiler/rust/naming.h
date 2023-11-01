// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_NAMING_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_NAMING_H__

#include <string>

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
std::string GetCrateName(Context<FileDescriptor> dep);

std::string GetRsFile(Context<FileDescriptor> file);
std::string GetThunkCcFile(Context<FileDescriptor> file);
std::string GetHeaderFile(Context<FileDescriptor> file);

std::string Thunk(Context<FieldDescriptor> field, absl::string_view op);
std::string Thunk(Context<OneofDescriptor> field, absl::string_view op);

std::string Thunk(Context<Descriptor> msg, absl::string_view op);

std::string PrimitiveRsTypeName(const FieldDescriptor& desc);

std::string FieldInfoComment(Context<FieldDescriptor> field);

std::string RustModule(Context<Descriptor> msg);
std::string RustInternalModuleName(Context<FileDescriptor> file);

std::string GetCrateRelativeQualifiedPath(Context<Descriptor> msg);

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_NAMING_H__
