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
std::string GetCrateName(Context& ctx, const FileDescriptor& dep);

std::string GetRsFile(Context& ctx, const FileDescriptor& file);
std::string GetThunkCcFile(Context& ctx, const FileDescriptor& file);
std::string GetHeaderFile(Context& ctx, const FileDescriptor& file);

std::string ThunkName(Context& ctx, const FieldDescriptor& field,
                      absl::string_view op);
std::string ThunkName(Context& ctx, const OneofDescriptor& field,
                      absl::string_view op);

std::string ThunkName(Context& ctx, const Descriptor& msg,
                      absl::string_view op);

std::string PrimitiveRsTypeName(const FieldDescriptor& field);

std::string EnumRsName(const EnumDescriptor& desc);

std::string OneofViewEnumRsName(const OneofDescriptor& oneof);
std::string OneofMutEnumRsName(const OneofDescriptor& oneof);
std::string OneofCaseEnumRsName(const OneofDescriptor& oneof);
std::string OneofCaseRsName(const FieldDescriptor& oneof_field);

std::string FieldInfoComment(Context& ctx, const FieldDescriptor& field);

std::string RustModule(Context& ctx, const Descriptor& msg);
std::string RustInternalModuleName(Context& ctx, const FileDescriptor& file);

std::string GetCrateRelativeQualifiedPath(Context& ctx, const Descriptor& msg);

// TODO: Unify these with other case-conversion functions.

// Converts an UpperCamel or lowerCamel string to a snake_case string.
std::string CamelToSnakeCase(absl::string_view input);

// Converts a snake_case string to an UpperCamelCase string.
std::string SnakeToUpperCamelCase(absl::string_view input);

// Converts a SCREAMING_SNAKE_CASE string to an UpperCamelCase string.
std::string ScreamingSnakeToUpperCamelCase(absl::string_view input);

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_NAMING_H__
