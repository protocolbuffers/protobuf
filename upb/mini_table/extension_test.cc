// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/extension.h"

#include <stdint.h>

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb_minitable.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/status.h"
#include "upb/mem/arena.hpp"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/test/test.upb_minitable.h"

namespace {

const upb_MiniTable* Proto2Mt() {
  return &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;
}

// Proto2 `TestAllTypesProto2` field numbers.
constexpr uint32_t kOptionalNestedMessage = 18;
constexpr uint32_t kOptionalNestedEnum = 21;
constexpr uint32_t kOptionalForeignEnum = 22;

TEST(MiniTableExtensionTest, GeneratedMessageExtension) {
  const upb_MiniTableExtension* e = upb_test_ModelExtension1_model_ext_ext;
  const upb_MiniTableField* f = upb_MiniTableExtension_ToField(e);

  EXPECT_EQ(static_cast<const void*>(f), static_cast<const void*>(e));
  EXPECT_TRUE(upb_MiniTableField_IsExtension(f));
  EXPECT_EQ(upb_MiniTableExtension_Number(e), 1547);
  EXPECT_EQ(upb_MiniTableExtension_CType(e), kUpb_CType_Message);
  EXPECT_EQ(upb_MiniTableExtension_Extendee(e),
            &upb_0test__ModelWithExtensions_msg_init);
  EXPECT_EQ(upb_MiniTableExtension_GetSubMessage(e),
            &upb_0test__ModelExtension1_msg_init);
  EXPECT_EQ(upb_MiniTableExtension_GetSubEnum(e), nullptr);
}

// The accessors only read field-level data unless the field is flagged as an
// extension, so they also work on a regular field of a message.
TEST(MiniTableExtensionTest, AccessorsOnRegularField) {
  const upb_MiniTableField* msg_f =
      upb_MiniTable_FindFieldByNumber(Proto2Mt(), kOptionalNestedMessage);
  const upb_MiniTableField* enum_f =
      upb_MiniTable_FindFieldByNumber(Proto2Mt(), kOptionalNestedEnum);
  ASSERT_NE(msg_f, nullptr);
  ASSERT_NE(enum_f, nullptr);
  ASSERT_FALSE(upb_MiniTableField_IsExtension(msg_f));
  ASSERT_FALSE(upb_MiniTableField_IsExtension(enum_f));

  const upb_MiniTableExtension* msg_e =
      reinterpret_cast<const upb_MiniTableExtension*>(msg_f);
  EXPECT_EQ(upb_MiniTableExtension_ToField(msg_e), msg_f);
  EXPECT_EQ(upb_MiniTableExtension_Number(msg_e), kOptionalNestedMessage);
  EXPECT_EQ(upb_MiniTableExtension_CType(msg_e), kUpb_CType_Message);
  EXPECT_EQ(upb_MiniTableExtension_Extendee(msg_e), nullptr);
  EXPECT_EQ(upb_MiniTableExtension_GetSubMessage(msg_e),
            upb_MiniTable_SubMessage(msg_f));
  EXPECT_NE(upb_MiniTableExtension_GetSubMessage(msg_e), nullptr);
  EXPECT_EQ(upb_MiniTableExtension_GetSubEnum(msg_e), nullptr);

  const upb_MiniTableExtension* enum_e =
      reinterpret_cast<const upb_MiniTableExtension*>(enum_f);
  EXPECT_EQ(upb_MiniTableExtension_ToField(enum_e), enum_f);
  EXPECT_EQ(upb_MiniTableExtension_Number(enum_e), kOptionalNestedEnum);
  EXPECT_EQ(upb_MiniTableExtension_CType(enum_e), kUpb_CType_Enum);
  EXPECT_EQ(upb_MiniTableExtension_Extendee(enum_e), nullptr);
  EXPECT_EQ(upb_MiniTableExtension_GetSubMessage(enum_e), nullptr);
  EXPECT_EQ(upb_MiniTableExtension_GetSubEnum(enum_e),
            upb_MiniTable_GetSubEnumTable(enum_f));
  EXPECT_NE(upb_MiniTableExtension_GetSubEnum(enum_e), nullptr);
}

TEST(MiniTableExtensionTest, SetSubEnum) {
  upb::Arena arena;
  const upb_MiniTableEnum* nested_enum = upb_MiniTable_GetSubEnumTable(
      upb_MiniTable_FindFieldByNumber(Proto2Mt(), kOptionalNestedEnum));
  const upb_MiniTableEnum* foreign_enum = upb_MiniTable_GetSubEnumTable(
      upb_MiniTable_FindFieldByNumber(Proto2Mt(), kOptionalForeignEnum));
  ASSERT_NE(nested_enum, nullptr);
  ASSERT_NE(foreign_enum, nullptr);
  ASSERT_NE(nested_enum, foreign_enum);

  upb::MtDataEncoder enc;
  ASSERT_TRUE(enc.EncodeExtension(kUpb_FieldType_Enum, 199,
                                  kUpb_FieldModifier_IsClosedEnum));
  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTableExtension* e = upb_MiniTableExtension_BuildEnum(
      enc.data().data(), enc.data().size(), Proto2Mt(), nested_enum,
      arena.ptr(), &status);
  ASSERT_NE(e, nullptr) << upb_Status_ErrorMessage(&status);

  EXPECT_EQ(upb_MiniTableExtension_Extendee(e), Proto2Mt());
  EXPECT_EQ(upb_MiniTableExtension_GetSubEnum(e), nested_enum);
  EXPECT_TRUE(upb_MiniTableExtension_SetSubEnum(e, foreign_enum));
  EXPECT_EQ(upb_MiniTableExtension_GetSubEnum(e), foreign_enum);
  EXPECT_FALSE(upb_MiniTableExtension_SetSubMessage(
      e, &upb_0test__ModelExtension1_msg_init));
  EXPECT_EQ(upb_MiniTableExtension_GetSubEnum(e), foreign_enum);
}

TEST(MiniTableExtensionTest, SetSubOnScalarExtensionFails) {
  upb::Arena arena;
  upb::MtDataEncoder enc;
  ASSERT_TRUE(enc.EncodeExtension(kUpb_FieldType_Int32, 198, 0));
  upb_Status status;
  upb_Status_Clear(&status);
  upb_MiniTableExtension* e = upb_MiniTableExtension_Build(
      enc.data().data(), enc.data().size(), Proto2Mt(), arena.ptr(), &status);
  ASSERT_NE(e, nullptr) << upb_Status_ErrorMessage(&status);

  EXPECT_EQ(upb_MiniTableExtension_CType(e), kUpb_CType_Int32);
  EXPECT_EQ(upb_MiniTableExtension_Number(e), 198);
  EXPECT_EQ(upb_MiniTableExtension_Extendee(e), Proto2Mt());
  EXPECT_FALSE(upb_MiniTableExtension_SetSubMessage(
      e, &upb_0test__ModelExtension1_msg_init));
  EXPECT_EQ(upb_MiniTableExtension_GetSubMessage(e), nullptr);
  EXPECT_EQ(upb_MiniTableExtension_GetSubEnum(e), nullptr);
}

}  // namespace
