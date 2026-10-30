// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/json/encode.h"

#include <cstddef>
#include <string>

#include "google/protobuf/field_mask.upb.h"
#include "google/protobuf/struct.upb.h"
#include <gtest/gtest.h>
#include "google/protobuf/json/json_enumval_custom_string.upb.h"
#include "google/protobuf/json/json_enumval_custom_string.upbdefs.h"
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/json/decode.h"
#include "upb/json/test.upb.h"
#include "upb/json/test.upbdefs.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/message/message.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def.hpp"
#include "upb/reflection/message.h"

static std::string JsonEncodeGeneric(
    const upb_Message* msg, const upb_MessageDef* (*getmsgdef)(upb_DefPool*),
    int options) {
  upb::Arena a;
  upb::Status status;
  upb::DefPool defpool;
  const upb_MessageDef* m = getmsgdef(defpool.ptr());
  EXPECT_TRUE(m != nullptr);

  size_t json_size =
      upb_JsonEncode(msg, m, defpool.ptr(), options, nullptr, 0, status.ptr());
  char* json_buf = (char*)upb_Arena_Malloc(a.ptr(), json_size + 1);

  size_t size = upb_JsonEncode(msg, m, defpool.ptr(), options, json_buf,
                               json_size + 1, status.ptr());
  EXPECT_EQ(size, json_size);
  return std::string(json_buf, json_size);
}

static std::string JsonEncode(const upb_test_Box* msg, int options) {
  return JsonEncodeGeneric(UPB_UPCAST(msg), upb_test_Box_getmsgdef, options);
}

static std::string JsonEncodeKnight(
    const json_enumval_custom_string_Knight* msg, int options) {
  return JsonEncodeGeneric(
      UPB_UPCAST(msg), json_enumval_custom_string_Knight_getmsgdef, options);
}

// Encode a single optional enum.
TEST(JsonTest, EncodeEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_set_first_tag(foo, upb_test_Z_BAR);

  EXPECT_EQ(R"({"firstTag":"Z_BAR"})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"firstTag":1})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

namespace {

void AppendVarint(std::string* out, uint64_t v) {
  while (v >= 0x80) {
    out->push_back(static_cast<char>((v & 0x7f) | 0x80));
    v >>= 7;
  }
  out->push_back(static_cast<char>(v));
}

void AppendTagVarint(std::string* out, uint32_t field_num, uint64_t v) {
  AppendVarint(out, (static_cast<uint64_t>(field_num) << 3) | 0u);
  AppendVarint(out, v);
}

void AppendTagBytes(std::string* out, uint32_t field_num,
                    const std::string& bytes) {
  AppendVarint(out, (static_cast<uint64_t>(field_num) << 3) | 2u);
  AppendVarint(out, bytes.size());
  out->append(bytes);
}

std::string EncodeFieldProto(const char* name, uint32_t number, uint32_t label,
                             uint32_t type) {
  std::string f;
  AppendTagBytes(&f, 1, name);
  AppendTagVarint(&f, 3, number);
  AppendTagVarint(&f, 4, label);
  AppendTagVarint(&f, 5, type);
  return f;
}

std::string EncodeMessageProto(const char* name,
                               std::initializer_list<std::string> fields) {
  std::string m;
  AppendTagBytes(&m, 1, name);
  for (const auto& f : fields) {
    AppendTagBytes(&m, 2, f);
  }
  return m;
}

bool LoadSerializedFileProto(upb_DefPool* pool, const char* filename,
                             const std::string& serialized) {
  static _upb_DefPool_Init* kEmptyDeps[1] = {nullptr};
  _upb_DefPool_Init init = {
      kEmptyDeps,
      nullptr,
      filename,
      upb_StringView_FromDataAndSize(serialized.data(), serialized.size()),
  };
  return _upb_DefPool_LoadDefInitEx(pool, &init, /*rebuild_minitable=*/true);
}

}  // namespace

TEST(JsonTest, CanonicalWellKnownTypesRetainClassification) {
  // Build serialized FileDescriptorProto for canonical Duration, Timestamp,
  // and 9 wrapper types (keep alive across DefPool lifetime because
  // _upb_DefPool_LoadDefInitEx aliases descriptor strings).
  std::string extra_file;
  AppendTagBytes(&extra_file, 1, "canonical_extra_wkts.proto");
  AppendTagBytes(&extra_file, 2, "google.protobuf");
  AppendTagBytes(&extra_file, 12, "proto3");
  for (const char* tname : {"Duration", "Timestamp"}) {
    AppendTagBytes(
        &extra_file, 4,
        EncodeMessageProto(tname, {EncodeFieldProto("seconds", 1, 1, 3),
                                   EncodeFieldProto("nanos", 2, 1, 5)}));
  }
  struct WrapperWire {
    const char* name;
    uint32_t proto_type;
  };
  const WrapperWire kWrappers[] = {
      {"DoubleValue", 1},  {"FloatValue", 2},  {"Int64Value", 3},
      {"UInt64Value", 4},  {"Int32Value", 5},  {"UInt32Value", 13},
      {"BoolValue", 8},    {"StringValue", 9}, {"BytesValue", 12},
  };
  for (const auto& w : kWrappers) {
    AppendTagBytes(
        &extra_file, 4,
        EncodeMessageProto(w.name,
                           {EncodeFieldProto("value", 1, 1, w.proto_type)}));
  }

  upb::DefPool defpool;
  const upb_MessageDef* box_m = upb_test_Box_getmsgdef(defpool.ptr());
  ASSERT_NE(box_m, nullptr);
  ASSERT_TRUE(LoadSerializedFileProto(
      defpool.ptr(), "canonical_extra_wkts.proto", extra_file));

  struct ExpectedWkt {
    const char* full_name;
    upb_WellKnown expected;
  };
  const ExpectedWkt kExpected[] = {
      {"google.protobuf.Any", kUpb_WellKnown_Any},
      {"google.protobuf.FieldMask", kUpb_WellKnown_FieldMask},
      {"google.protobuf.Duration", kUpb_WellKnown_Duration},
      {"google.protobuf.Timestamp", kUpb_WellKnown_Timestamp},
      {"google.protobuf.DoubleValue", kUpb_WellKnown_DoubleValue},
      {"google.protobuf.FloatValue", kUpb_WellKnown_FloatValue},
      {"google.protobuf.Int64Value", kUpb_WellKnown_Int64Value},
      {"google.protobuf.UInt64Value", kUpb_WellKnown_UInt64Value},
      {"google.protobuf.Int32Value", kUpb_WellKnown_Int32Value},
      {"google.protobuf.UInt32Value", kUpb_WellKnown_UInt32Value},
      {"google.protobuf.BoolValue", kUpb_WellKnown_BoolValue},
      {"google.protobuf.StringValue", kUpb_WellKnown_StringValue},
      {"google.protobuf.BytesValue", kUpb_WellKnown_BytesValue},
      {"google.protobuf.Value", kUpb_WellKnown_Value},
      {"google.protobuf.ListValue", kUpb_WellKnown_ListValue},
      {"google.protobuf.Struct", kUpb_WellKnown_Struct},
  };
  for (const auto& item : kExpected) {
    const upb_MessageDef* m =
        upb_DefPool_FindMessageByName(defpool.ptr(), item.full_name);
    ASSERT_NE(m, nullptr) << item.full_name;
    EXPECT_EQ(upb_MessageDef_WellKnownType(m), item.expected) << item.full_name;
  }
}

TEST(JsonTest, NonCanonicalStructDescriptorDowngradesSafely) {
  // Define google.protobuf.Struct with `int64 fields = 1;` instead of a map.
  std::string forged_file;
  AppendTagBytes(&forged_file, 1, "forged_struct.proto");
  AppendTagBytes(&forged_file, 2, "google.protobuf");
  AppendTagBytes(&forged_file, 12, "proto3");
  AppendTagBytes(&forged_file, 4,
                 EncodeMessageProto("Struct",
                                    {EncodeFieldProto("fields", 1, 1, 3)}));

  upb::Arena arena;
  upb::DefPool pool;
  upb::Status status;
  ASSERT_TRUE(
      LoadSerializedFileProto(pool.ptr(), "forged_struct.proto", forged_file));
  const upb_MessageDef* struct_m =
      upb_DefPool_FindMessageByName(pool.ptr(), "google.protobuf.Struct");
  ASSERT_NE(struct_m, nullptr);
  EXPECT_EQ(upb_MessageDef_WellKnownType(struct_m), kUpb_WellKnown_Unspecified);

  const upb_FieldDef* f_def = upb_MessageDef_FindFieldByNumber(struct_m, 1);
  ASSERT_NE(f_def, nullptr);
  upb_Message* inst =
      upb_Message_New(upb_MessageDef_MiniTable(struct_m), arena.ptr());
  ASSERT_NE(inst, nullptr);
  upb_MessageValue v;
  v.int64_val = 0x4141414141414141LL;
  ASSERT_TRUE(upb_Message_SetFieldByDef(inst, f_def, v, arena.ptr()));

  // Without WKT validation, upb_JsonEncode union-puns 0x4141414141414141 as a
  // `const upb_Map*` in `jsonenc_struct` and crashes with a wild-pointer read.
  char out[128];
  size_t n = upb_JsonEncode(inst, struct_m, pool.ptr(), 0, out, sizeof(out),
                            status.ptr());
  ASSERT_TRUE(status.ok()) << status.error_message();
  EXPECT_EQ(std::string(out, n), R"({"fields":"4702111234474983745"})");

  upb_Message* decoded =
      upb_Message_New(upb_MessageDef_MiniTable(struct_m), arena.ptr());
  ASSERT_TRUE(upb_JsonDecode(out, n, decoded, struct_m, pool.ptr(), 0,
                             arena.ptr(), status.ptr()))
      << status.error_message();
  EXPECT_EQ(upb_Message_GetFieldByDef(decoded, f_def).int64_val,
            0x4141414141414141LL);
}

TEST(JsonTest, NonCanonicalWrapperDescriptorDowngradesSafely) {
  // Define google.protobuf.StringValue with field number 2 (missing field 1).
  std::string forged_file;
  AppendTagBytes(&forged_file, 1, "forged_wrapper.proto");
  AppendTagBytes(&forged_file, 2, "google.protobuf");
  AppendTagBytes(&forged_file, 12, "proto3");
  AppendTagBytes(&forged_file, 4,
                 EncodeMessageProto("StringValue",
                                    {EncodeFieldProto("other", 2, 1, 5)}));

  upb::Arena arena;
  upb::DefPool pool;
  upb::Status status;
  ASSERT_TRUE(
      LoadSerializedFileProto(pool.ptr(), "forged_wrapper.proto", forged_file));
  const upb_MessageDef* wrap_m =
      upb_DefPool_FindMessageByName(pool.ptr(), "google.protobuf.StringValue");
  ASSERT_NE(wrap_m, nullptr);
  EXPECT_EQ(upb_MessageDef_WellKnownType(wrap_m), kUpb_WellKnown_Unspecified);

  upb_Message* inst =
      upb_Message_New(upb_MessageDef_MiniTable(wrap_m), arena.ptr());
  const upb_FieldDef* f_def = upb_MessageDef_FindFieldByNumber(wrap_m, 2);
  upb_MessageValue v;
  v.int32_val = 42;
  ASSERT_TRUE(upb_Message_SetFieldByDef(inst, f_def, v, arena.ptr()));

  // Without WKT validation, jsonenc_wrapper looks up field 1 (NULL) and crashes.
  char out[64];
  size_t n = upb_JsonEncode(inst, wrap_m, pool.ptr(), 0, out, sizeof(out),
                            status.ptr());
  ASSERT_TRUE(status.ok()) << status.error_message();
  EXPECT_EQ(std::string(out, n), R"({"other":42})");
}

// Encode a single optional negative enum.
TEST(JsonTest, EncodeNegativeEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_set_last_tag(foo, upb_test_Z_BAZ);

  EXPECT_EQ(R"({"lastTag":"Z_BAZ"})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"lastTag":-2})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

// Encode a single repeated enum.
TEST(JsonTest, EncodeRepeatedEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_add_more_tags(foo, upb_test_Z_BAT, a.ptr());

  EXPECT_EQ(R"({"moreTags":["Z_BAT"]})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"moreTags":[13]})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

// Special case: encode null enum.
TEST(JsonTest, EncodeNullEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  google_protobuf_Value_set_null_value(upb_test_Box_mutable_val(foo, a.ptr()),
                                       google_protobuf_NULL_VALUE);

  EXPECT_EQ(R"({"val":null})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"val":null})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

TEST(JsonTest, EncodeConflictJsonName) {
  upb::Arena a;
  upb_test_Box* box = upb_test_Box_new(a.ptr());
  upb_test_Box_set_value(box, 2);
  EXPECT_EQ(R"({"old_value":2})", JsonEncode(box, 0));

  upb_test_Box* new_box = upb_test_Box_new(a.ptr());
  upb_test_Box_set_new_value(new_box, 2);
  EXPECT_EQ(R"({"value":2})", JsonEncode(new_box, 0));
}

// Test encoding when the enum value has no custom string option set, verifying
// it falls back to the default enum value name ("ARMOR_GORGET").
TEST(JsonTest, EncodeEnumMissingCustomStringOption) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GORGET);
  EXPECT_EQ(R"({"armor":"ARMOR_GORGET"})", JsonEncodeKnight(knight, 0));
}

// Test encoding when a standard custom string option is defined ("gr8 helm").
TEST(JsonTest, EncodeEnumWithCustomString) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GREAT_HELM);
  EXPECT_EQ(R"({"armor":"gr8 helm"})", JsonEncodeKnight(knight, 0));
}

// Test encoding when the custom string option contains double quotes requiring
// escaping ("a\"b").
TEST(JsonTest, EncodeEnumCustomStringWithEscapedQuotes) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GAUNTLET);
  EXPECT_EQ("{\"armor\":\"a\\\"b\"}", JsonEncodeKnight(knight, 0));
}

// Test encoding when the custom string option is enclosed within double quotes
// ("\"plate\"").
TEST(JsonTest, EncodeEnumCustomStringEnclosedInQuotes) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_PLATE);
  EXPECT_EQ("{\"armor\":\"\\\"plate\\\"\"}", JsonEncodeKnight(knight, 0));
}

// Test encoding when the custom string option is an empty string ("").
TEST(JsonTest, EncodeEnumEmptyCustomString) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_COIF);
  EXPECT_EQ(R"({"armor":""})", JsonEncodeKnight(knight, 0));
}

// Test encoding when the custom string option contains control characters (tab
// and newline) requiring escaping ("p\taul\ndron").
TEST(JsonTest, EncodeEnumCustomStringWithEscapedControlChars) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_PAULDRON);
  EXPECT_EQ(R"({"armor":"p\taul\ndron"})", JsonEncodeKnight(knight, 0));
}

// Test encoding an enum value that has aliased values sharing a custom string
// option ("sabaton").
TEST(JsonTest, EncodeEnumAliasedCustomString) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_SABATON);
  EXPECT_EQ(R"({"armor":"sabaton"})", JsonEncodeKnight(knight, 0));
}

// Test encoding when the custom string option consists of numeric digits ("8").
TEST(JsonTest, EncodeEnumNumericCustomString) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_HACHI_MAI_DO);
  EXPECT_EQ(R"({"armor":"8"})", JsonEncodeKnight(knight, 0));
}

// Test encoding an enum value whose custom string option is identical to its
// original enum value name.
TEST(JsonTest, EncodeEnumCustomStringSameAsName) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GREAVES);
  EXPECT_EQ(R"({"armor":"ARMOR_GREAVES"})", JsonEncodeKnight(knight, 0));
}

// Test encoding when upb_JsonEncode_FormatEnumsAsIntegers is set, verifying
// integer format option overrides the custom string option.
TEST(JsonTest, EncodeEnumWithIntegerFormatOverride) {
  upb::Arena a;
  json_enumval_custom_string_Knight* knight =
      json_enumval_custom_string_Knight_new(a.ptr());
  json_enumval_custom_string_Knight_set_armor(
      knight, json_enumval_custom_string_ARMOR_GREAT_HELM);
  EXPECT_EQ(R"({"armor":1})",
            JsonEncodeKnight(knight, upb_JsonEncode_FormatEnumsAsIntegers));
}

// Test encoding when a FieldMask has paths containing characters that require
// escaping.
TEST(JsonTest, EncodeFieldMaskEscapedPaths) {
  upb::Arena a;
  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  google_protobuf_FieldMask* mask = upb_test_Box_mutable_mask_val(foo, a.ptr());
  google_protobuf_FieldMask_add_paths(
      mask, upb_StringView_FromString("foo_bar"), a.ptr());
  google_protobuf_FieldMask_add_paths(mask, upb_StringView_FromString("path\n"),
                                      a.ptr());
  google_protobuf_FieldMask_add_paths(mask, upb_StringView_FromString("path\t"),
                                      a.ptr());
  google_protobuf_FieldMask_add_paths(
      mask, upb_StringView_FromString("path_a\n"), a.ptr());
  google_protobuf_FieldMask_add_paths(
      mask, upb_StringView_FromString("path\"quote"), a.ptr());

  // "foo_bar" -> "fooBar"
  // "path\n" -> "path\n"
  // "path\t" -> "path\t"
  // "path_a\n" -> "pathA\n"
  // "path\"quote" -> "path\"quote"
  const std::string expected =
      R"({"maskVal":"fooBar,path\n,path\t,pathA\n,path\"quote"})";
  EXPECT_EQ(expected, JsonEncode(foo, 0));
}

static upb_test_Box* NewBoxWithData(upb_Arena* arena, const char* data,
                                    size_t size) {
  upb_test_Box* box = upb_test_Box_new(arena);
  upb_test_Box_set_data(box, upb_StringView_FromDataAndSize(data, size));
  return box;
}

TEST(JsonTest, EncodeBytesUsesStandardBase64ByDefault) {
  // Each encoding uses alphabet index 62 and/or 63, the only entries that
  // differ between the standard ('+', '/') and web-safe ('-', '_') alphabets.
  // Input lengths 3, 2 and 1 cover no padding, "=" and "==".
  upb::Arena a;
  EXPECT_EQ(R"({"data":"+/+/"})",
            JsonEncode(NewBoxWithData(a.ptr(), "\xfb\xff\xbf", 3), 0));
  EXPECT_EQ(R"({"data":"+/8="})",
            JsonEncode(NewBoxWithData(a.ptr(), "\xfb\xff", 2), 0));
  EXPECT_EQ(R"({"data":"/w=="})",
            JsonEncode(NewBoxWithData(a.ptr(), "\xff", 1), 0));
}

TEST(JsonTest, EncodeBytesWebSafeBase64) {
  // Same inputs as EncodeBytesUsesStandardBase64ByDefault.
  upb::Arena a;
  EXPECT_EQ(R"({"data":"-_-_"})",
            JsonEncode(NewBoxWithData(a.ptr(), "\xfb\xff\xbf", 3),
                       upb_JsonEncode_WebSafeBase64));
  EXPECT_EQ(R"({"data":"-_8="})",
            JsonEncode(NewBoxWithData(a.ptr(), "\xfb\xff", 2),
                       upb_JsonEncode_WebSafeBase64));
  EXPECT_EQ(R"({"data":"_w=="})", JsonEncode(NewBoxWithData(a.ptr(), "\xff", 1),
                                             upb_JsonEncode_WebSafeBase64));
}

TEST(JsonTest, EncodeBytesWebSafeBase64RoundTripsThroughDecoder) {
  upb::Arena a;
  const char kData[] = "\x00\xfb\xff\xbf\x10";
  const size_t kSize = sizeof(kData) - 1;
  const std::string json = JsonEncode(NewBoxWithData(a.ptr(), kData, kSize),
                                      upb_JsonEncode_WebSafeBase64);
  EXPECT_EQ(R"({"data":"APv_vxA="})", json);

  upb::DefPool defpool;
  upb::Status status;
  const upb_MessageDef* m = upb_test_Box_getmsgdef(defpool.ptr());
  upb_test_Box* decoded = upb_test_Box_new(a.ptr());
  ASSERT_TRUE(upb_JsonDecode(json.data(), json.size(), UPB_UPCAST(decoded), m,
                             defpool.ptr(), 0, a.ptr(), status.ptr()))
      << status.error_message();
  const upb_StringView data = upb_test_Box_data(decoded);
  EXPECT_EQ(std::string(kData, kSize), std::string(data.data, data.size));
}
