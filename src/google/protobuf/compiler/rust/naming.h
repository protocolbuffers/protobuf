// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_NAMING_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_NAMING_H__

#include <array>
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
std::string RawMapThunk(Context& ctx, const Descriptor& msg,
                        absl::string_view key_t, absl::string_view op);
std::string RawMapThunk(Context& ctx, const EnumDescriptor& desc,
                        absl::string_view key_t, absl::string_view op);

// Returns the local constant that defines the vtable for mutating `field`.
std::string VTableName(const FieldDescriptor& field);

// Returns an absolute path to the Proxied Rust type of the given field.
// The absolute path is guaranteed to work in the crate that defines the field.
// It may be crate-relative, or directly reference the owning crate of the type.
std::string RsTypePath(Context& ctx, const FieldDescriptor& field);

std::string EnumRsName(const EnumDescriptor& desc);
std::string EnumValueRsName(const EnumValueDescriptor& value);

std::string OneofViewEnumRsName(const OneofDescriptor& oneof);
std::string OneofCaseEnumRsName(const OneofDescriptor& oneof);
std::string OneofCaseRsName(const FieldDescriptor& oneof_field);

std::string FieldInfoComment(Context& ctx, const FieldDescriptor& field);

// Returns how to 'spell' the provided name in Rust, which is the provided name
// verbatim unless it is a Rust keyword that isn't a legal symbol name.
std::string RsSafeName(absl::string_view name);

// Constructs a string of the Rust modules which will contain the message.
//
// Example: Given a message 'NestedMessage' which is defined in package 'x.y'
// which is inside 'ParentMessage', the message will be placed in the
// x::y::ParentMessage_ Rust module, so this function will return the string
// "x::y::ParentMessage_::".
//
// If the message has no package and no containing messages then this returns
// empty string.
std::string RustModuleForContainingType(Context& ctx,
                                        const Descriptor* containing_type);
std::string RustModule(Context& ctx, const Descriptor& msg);
std::string RustModule(Context& ctx, const EnumDescriptor& enum_);
std::string RustModule(Context& ctx, const OneofDescriptor& oneof);
std::string RustInternalModuleName(Context& ctx, const FileDescriptor& file);

std::string GetCrateRelativeQualifiedPath(Context& ctx, const Descriptor& msg);
std::string GetCrateRelativeQualifiedPath(Context& ctx,
                                          const EnumDescriptor& enum_);
std::string GetCrateRelativeQualifiedPath(Context& ctx,
                                          const OneofDescriptor& oneof);

template <typename Desc>
std::string GetUnderscoreDelimitedFullName(Context& ctx, const Desc& desc);

std::string UnderscoreDelimitFullName(Context& ctx,
                                      absl::string_view full_name);

// TODO: Unify these with other case-conversion functions.

// Converts an UpperCamel or lowerCamel string to a snake_case string.
std::string CamelToSnakeCase(absl::string_view input);

// Converts a snake_case string to an UpperCamelCase string.
std::string SnakeToUpperCamelCase(absl::string_view input);

// Converts a SCREAMING_SNAKE_CASE string to an UpperCamelCase string.
std::string ScreamingSnakeToUpperCamelCase(absl::string_view input);

// Given a fixed prefix, this will repeatedly strip provided
// string_views if they start with the prefix, the prefix in UpperCamel, or
// the prefix in snake_case.
class MultiCasePrefixStripper final {
 public:
  explicit MultiCasePrefixStripper(absl::string_view prefix);

  // Strip a prefix from the name in UpperCamel or snake_case, if present.
  // If there is an underscore after the prefix, that will also be stripped.
  // The stripping is case-insensitive.
  absl::string_view StripPrefix(absl::string_view name) const;

 private:
  std::array<std::string, 3> prefixes_;
};

// More efficient overload if a stripper is already constructed.
std::string EnumValueRsName(const MultiCasePrefixStripper& stripper,
                            absl::string_view value_name);

// Describes the names and conversions for a supported map key type.
struct MapKeyType {
  // Identifier used in thunk name.
  absl::string_view thunk_ident;

  // Rust key typename (K in Map<K, V>, so e.g. `[u8]` for bytes).
  // This field may have an unexpanded `$pb$` variable.
  absl::string_view rs_key_t;

  // Rust key typename used by thunks for FFI (e.g. `PtrAndLen` for bytes).
  // This field may have an unexpanded `$pbi$` variable.
  absl::string_view rs_ffi_key_t;

  // Rust expression converting `key: rs_key_t` into an `rs_ffi_key_t`.
  absl::string_view rs_to_ffi_key_expr;

  // Rust expression converting `ffi_key: rs_ffi_key_t` into an `rs_key_t`.
  // This field may have an unexpanded `$pb$` variable.
  absl::string_view rs_from_ffi_key_expr;

  // C++ key typename (K in Map<K, V>, so e.g. `std::string` for bytes).
  absl::string_view cc_key_t;

  // C++ key typename used by thunks for FFI (e.g. `PtrAndLen` for bytes).
  absl::string_view cc_ffi_key_t;

  // C++ expression converting `cc_ffi_key_t key` into a `cc_key_t`.
  absl::string_view cc_from_ffi_key_expr;

  // C++ expression converting `cc_key_t cpp_key` into a `cc_ffi_key_t`.
  absl::string_view cc_to_ffi_key_expr;
};

extern const MapKeyType kMapKeyTypes[6];

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_NAMING_H__
