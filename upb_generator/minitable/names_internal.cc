// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/minitable/names_internal.h"

#include <string>

#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "upb_generator/common/names.h"

namespace upb {
namespace generator {

namespace {

std::string ToCIdent(absl::string_view str) {
  return absl::StrReplaceAll(str, {{".", "_"}, {"/", "_"}, {"-", "_"}});
}

}  // namespace

std::string MiniTableHeaderFilename(absl::string_view proto_filename,
                                    bool bootstrap) {
  std::string base;
  if (bootstrap) {
    if (IsDescriptorProto(proto_filename)) {
      base = "upb/reflection/stage1/";
    } else {
      base = "upb_generator/stage1/";
    }
  }
  return base + StripExtension(proto_filename) + ".upb_minitable.h";
}

std::string MiniTableFieldsVarName(absl::string_view msg_full_name) {
  return ToCIdent(msg_full_name) + "__fields";
}

std::string MiniTableSubMessagesVarName(absl::string_view msg_full_name) {
  return ToCIdent(msg_full_name) + "__submsgs";
}

}  // namespace generator
}  // namespace upb
