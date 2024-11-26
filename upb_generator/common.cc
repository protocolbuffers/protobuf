// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/common.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "upb/mini_table/field.h"
#include "upb/reflection/def.hpp"

// Must be last
#include "upb/port/def.inc"

namespace upb {
namespace generator {

std::string FieldInitializer(upb::FieldDefPtr field,
                             const upb_MiniTableField* field64,
                             const upb_MiniTableField* field32) {
  return absl::Substitute(
      "{$0, $1, $2, $3, $4, $5}", upb_MiniTableField_Number(field64),
      ArchDependentSize(field32->UPB_PRIVATE(offset),
                        field64->UPB_PRIVATE(offset)),
      ArchDependentSize(field32->presence, field64->presence),
      field64->UPB_PRIVATE(submsg_index) == kUpb_NoSub
          ? "kUpb_NoSub"
          : absl::StrCat(field64->UPB_PRIVATE(submsg_index)).c_str(),
      field64->UPB_PRIVATE(descriptortype), GetModeInit(field32, field64));
}

std::string ArchDependentSize(int64_t size32, int64_t size64) {
  if (size32 == size64) return absl::StrCat(size32);
  return absl::Substitute("UPB_SIZE($0, $1)", size32, size64);
}

// Returns the field mode as a string initializer.
//
// We could just emit this as a number (and we may yet go in that direction) but
// for now emitting symbolic constants gives this better readability and
// debuggability.
std::string GetModeInit(const upb_MiniTableField* field32,
                        const upb_MiniTableField* field64) {
  std::string ret;
  uint8_t mode32 = field32->UPB_PRIVATE(mode);
  switch (mode32 & kUpb_FieldMode_Mask) {
    case kUpb_FieldMode_Map:
      ret = "(int)kUpb_FieldMode_Map";
      break;
    case kUpb_FieldMode_Array:
      ret = "(int)kUpb_FieldMode_Array";
      break;
    case kUpb_FieldMode_Scalar:
      ret = "(int)kUpb_FieldMode_Scalar";
      break;
    default:
      break;
  }

  if (mode32 & kUpb_LabelFlags_IsPacked) {
    absl::StrAppend(&ret, " | (int)kUpb_LabelFlags_IsPacked");
  }

  if (mode32 & kUpb_LabelFlags_IsExtension) {
    absl::StrAppend(&ret, " | (int)kUpb_LabelFlags_IsExtension");
  }

  if (mode32 & kUpb_LabelFlags_IsAlternate) {
    absl::StrAppend(&ret, " | (int)kUpb_LabelFlags_IsAlternate");
  }

  absl::StrAppend(&ret, " | ((int)", GetFieldRep(field32, field64),
                  " << kUpb_FieldRep_Shift)");
  return ret;
}

std::string GetFieldRep(const upb_MiniTableField* field32,
                        const upb_MiniTableField* field64) {
  const auto rep32 = UPB_PRIVATE(_upb_MiniTableField_GetRep)(field32);
  const auto rep64 = UPB_PRIVATE(_upb_MiniTableField_GetRep)(field64);

  switch (rep32) {
    case kUpb_FieldRep_1Byte:
      return "kUpb_FieldRep_1Byte";
      break;
    case kUpb_FieldRep_4Byte: {
      if (rep64 == kUpb_FieldRep_4Byte) {
        return "kUpb_FieldRep_4Byte";
      } else {
        assert(rep64 == kUpb_FieldRep_8Byte);
        return "UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte)";
      }
      break;
    }
    case kUpb_FieldRep_StringView:
      return "kUpb_FieldRep_StringView";
      break;
    case kUpb_FieldRep_8Byte:
      return "kUpb_FieldRep_8Byte";
      break;
  }
  UPB_UNREACHABLE();
}

}  // namespace generator
}  // namespace upb
