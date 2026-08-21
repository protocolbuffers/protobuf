// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/reflection/names.h"

#include <string>

#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"

namespace upb {
namespace generator {

namespace {

std::string ToCIdent(absl::string_view str) {
  return absl::StrReplaceAll(str, {{".", "_"}, {"/", "_"}, {"-", "_"}});
}

}  // namespace

std::string ReflectionGetMessageSymbol(absl::string_view full_name) {
  return ToCIdent(full_name) + "_getmsgdef";
}
std::string ReflectionFileSymbol(absl::string_view filename) {
  return ToCIdent(filename) + "_upbdefinit";
}

}  // namespace generator
}  // namespace upb
