// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/minitable/names.h"

#include <string>

#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "upb_generator/minitable/names_internal.h"

namespace upb {
namespace generator {

namespace {

std::string ToCIdent(absl::string_view str) {
  return absl::StrReplaceAll(str, {{".", "_"}, {"/", "_"}, {"-", "_"}});
}

std::string MangleName(absl::string_view name) {
  return absl::StrReplaceAll(name, {{"_", "_0"}, {".", "__"}});
}

}  // namespace

std::string MiniTableHeaderFilename(absl::string_view proto_filename) {
  return MiniTableHeaderFilename(proto_filename, false);
}

std::string MiniTableMessageVarName(absl::string_view full_name) {
  return MangleName(full_name) + "_msg_init";
}

std::string MiniTableMessagePtrVarName(absl::string_view full_name) {
  return MiniTableMessageVarName(full_name) + "_ptr";
}

std::string MiniTableEnumVarName(absl::string_view full_name) {
  return MangleName(full_name) + "_enum_init";
}

std::string MiniTableExtensionVarName(absl::string_view full_name) {
  return ToCIdent(full_name) + "_ext";
}

std::string MiniTableFileVarName(absl::string_view proto_filename) {
  return ToCIdent(proto_filename) + "_upb_file_layout";
}

}  // namespace generator
}  // namespace upb
