// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_GENERATOR_MINITABLE_NAMES_H__
#define GOOGLE_UPB_UPB_GENERATOR_MINITABLE_NAMES_H__

#include <string>

#include "absl/strings/string_view.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace generator {

// Maps: foo/bar/baz.proto -> foo/bar/baz.upb_minitable.h
UPBC_API std::string MiniTableHeaderFilename(absl::string_view proto_filename);

// These are the publicly visible symbols defined in foo.upb_minitable.h.
//   extern const upb_MiniTable <Message>;             // One for each message.
//   extern const upb_MiniTableEnum <Enum>;            // One for each enum.
//   extern const upb_MiniTableExtension <Extension>;  // One for each ext.
//   extern const upb_MiniTableFile <File>;            // One for each file.
//
//   extern const upb_MiniTable* <MessagePtr>;

UPBC_API std::string MiniTableMessageVarName(absl::string_view full_name);
UPBC_API std::string MiniTableEnumVarName(absl::string_view full_name);
UPBC_API std::string MiniTableExtensionVarName(absl::string_view full_name);
UPBC_API std::string MiniTableFileVarName(absl::string_view proto_filename);

// This is a weak reference to the MiniTable for the given message. A parent
// message may use this reference to allow the MiniTable to be tree shaken if
// it's not used directly.
//
// This symbol name will alias one of the following symbols:
//   - MiniTableMessageVarName(msg_full_name), if it was linked in.
//   - An empty, placeholder MiniTable, if it was not linked in.
UPBC_API std::string WeakMiniTableMessageVarName(
    absl::string_view msg_full_name);

}  // namespace generator
}  // namespace upb

#endif  // GOOGLE_UPB_UPB_GENERATOR_MINITABLE_NAMES_H__
