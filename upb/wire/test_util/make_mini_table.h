// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_TEST_UTIL_MAKE_MINI_TABLE_H_
#define UPB_MINI_TABLE_TEST_UTIL_MAKE_MINI_TABLE_H_

#include <utility>

#include "upb/base/descriptor_constants.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/combinations.h"

namespace upb {
namespace test {

class MiniTable {
 public:
  template <typename Field>
  static std::pair<const upb_MiniTable*, const upb_MiniTableField*>
  MakeSingleFieldTable(int field_number, upb_DecodeFast_Cardinality cardinality,
                       upb_Arena* arena) {
    return MakeSingleFieldTable(field_number, Field::kFieldType,
                                Field::kFastType, cardinality, arena);
  }

  static bool HasFastTableEntry(const upb_MiniTable* mt,
                                const upb_MiniTableField* field);

 private:
  static std::pair<const upb_MiniTable*, const upb_MiniTableField*>
  MakeSingleFieldTable(int field_number, upb_FieldType type,
                       upb_DecodeFast_Type fast_type,
                       upb_DecodeFast_Cardinality cardinality,
                       upb_Arena* arena);
};

}  // namespace test
}  // namespace upb

#endif  // UPB_MINI_TABLE_TEST_UTIL_MAKE_MINI_TABLE_H_
