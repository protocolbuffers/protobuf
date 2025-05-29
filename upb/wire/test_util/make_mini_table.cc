// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/test_util/make_mini_table.h"

#include <string>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/log/log.h"
#include "upb/base/status.hpp"
#include "upb/mem/arena.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/combinations.h"

namespace upb {
namespace test {

int FieldModifiers(upb_DecodeFast_Type type,
                   upb_DecodeFast_Cardinality cardinality) {
  int modifiers = 0;
  switch (type) {
    case kUpb_DecodeFast_String:
      modifiers |= kUpb_FieldModifier_ValidateUtf8;
      break;
    default:
      break;
  }
  switch (cardinality) {
    case kUpb_DecodeFast_Repeated:
      modifiers |= kUpb_FieldModifier_IsRepeated;
      break;
    case kUpb_DecodeFast_Packed:
      modifiers |= kUpb_FieldModifier_IsRepeated | kUpb_FieldModifier_IsPacked;
      break;
    default:
      return 0;
  }
  return modifiers;
}

std::pair<const upb_MiniTable*, const upb_MiniTableField*>
MiniTable::MakeSingleFieldTable(int field_number, upb_FieldType type,
                                upb_DecodeFast_Type fast_type,
                                upb_DecodeFast_Cardinality cardinality,
                                upb_Arena* arena) {
  MtDataEncoder encoder;
  encoder.StartMessage(0);
  encoder.PutField(type, 1, FieldModifiers(fast_type, cardinality));
  if (cardinality == kUpb_DecodeFast_Oneof) {
    encoder.StartOneof();
    encoder.PutOneofField(field_number);
  }
  const std::string& data = encoder.data();
  upb::Status status;
  const upb_MiniTable* table =
      upb_MiniTable_Build(data.data(), data.size(), arena, status.ptr());
  ABSL_CHECK(status.ok()) << status.error_message();
  const upb_MiniTableField* field = upb_MiniTable_GetFieldByIndex(table, 0);
  ABSL_CHECK(field != nullptr);
#if UPB_FASTTABLE
  if (field_number < (1 << 11)) {
    ABSL_CHECK_EQ(HasFastTableEntry(table, field),
                  UPB_DECODEFAST_ISENABLED(Field::kFastType, cardinality,
                                           kUpb_DecodeFast_Tag1Byte))
        << Field::kName;
  }
#endif
  return std::make_pair(table, upb_MiniTable_GetFieldByIndex(table, 0));
}

}  // namespace test
}  // namespace upb
