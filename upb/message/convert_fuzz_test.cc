#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>
#include "upb/mem/arena.h"
#include "upb/message/compare.h"  // Global include
#include "upb/message/internal/convert.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/sub.h"
#include "upb/mini_table/message.h"
#include "upb/test/fuzz_util.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace {

const upb_MiniTable* SubsetMiniTable(const upb_MiniTable* src, uint64_t mask,
                                     upb_Arena* arena);

const upb_MiniTable* SubsetMiniTable(const upb_MiniTable* src, uint64_t mask,
                                     upb_Arena* arena) {
  std::vector<upb_MiniTableField> new_fields;
  std::vector<upb_MiniTableSub> new_subs;

  int field_count = upb_MiniTable_FieldCount(src);
  for (int i = 0; i < field_count; ++i) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(src, i);
    if (mask & (1ULL << (i % 64))) {
      upb_MiniTableField new_f = *f;
      if (f->UPB_PRIVATE(submsg_ofs) != kUpb_NoSub) {
        new_subs.push_back(
            *UPB_PTR_AT(f, f->UPB_PRIVATE(submsg_ofs) * kUpb_SubmsgOffsetBytes,
                        const upb_MiniTableSub));
      }
      new_fields.push_back(new_f);
    }
  }

  size_t mt_size = sizeof(upb_MiniTable);
  size_t fields_bytes = new_fields.size() * sizeof(upb_MiniTableField);
  size_t subs_bytes = new_subs.size() * sizeof(upb_MiniTableSub);

  size_t mt_padded_size = UPB_ALIGN_UP(mt_size, 8);
  size_t fields_padded_size = UPB_ALIGN_UP(fields_bytes, 8);
  size_t total_size = mt_padded_size + fields_padded_size + subs_bytes;

  upb_MiniTable* new_mt = (upb_MiniTable*)upb_Arena_Malloc(arena, total_size);
  upb_MiniTableField* fields_ptr =
      UPB_PTR_AT(new_mt, mt_padded_size, upb_MiniTableField);
  upb_MiniTableSub* subs_ptr =
      UPB_PTR_AT(fields_ptr, fields_padded_size, upb_MiniTableSub);

  std::memcpy(new_mt, src, mt_size);
  if (!new_fields.empty()) {
    std::memcpy(fields_ptr, new_fields.data(), fields_bytes);
  }
  if (!new_subs.empty()) {
    std::memcpy(subs_ptr, new_subs.data(), subs_bytes);
  }

  int sub_idx = 0;
  for (size_t i = 0; i < new_fields.size(); ++i) {
    upb_MiniTableField* f = &fields_ptr[i];
    if (f->UPB_PRIVATE(submsg_ofs) != kUpb_NoSub) {
      uintptr_t f_addr = (uintptr_t)f;
      uintptr_t subs_addr = (uintptr_t)&subs_ptr[sub_idx++];

      size_t diff = subs_addr - f_addr;
      f->UPB_PRIVATE(submsg_ofs) = (uint16_t)(diff / kUpb_SubmsgOffsetBytes);
    }
  }

  new_mt->UPB_ONLYBITS(fields) = fields_ptr;
  new_mt->UPB_ONLYBITS(field_count) = (uint16_t)new_fields.size();
  new_mt->UPB_PRIVATE(dense_below) = 0;
  new_mt->UPB_PRIVATE(table_mask) = -1;

  return new_mt;
}

void ConvertFuzz(const upb::fuzz::MiniTableFuzzInput& input, uint64_t mask1,
                 uint64_t mask2, std::string proto_payload,
                 uint32_t decode_options, uint32_t encode_options) {
  upb_Arena* arena = upb_Arena_New();

  upb_ExtensionRegistry* exts;
  const upb_MiniTable* original_mt =
      upb::fuzz::BuildMiniTable(input, &exts, arena);
  if (!original_mt) {
    upb_Arena_Free(arena);
    return;
  }

  const upb_MiniTable* src_mt = SubsetMiniTable(original_mt, mask1, arena);
  const upb_MiniTable* dst_mt = SubsetMiniTable(original_mt, mask2, arena);

  decode_options = upb_Decode_LimitDepth(decode_options, 80);
  encode_options = upb_Encode_LimitDepth(encode_options, 80);

  // We don't want to skip unknown fields or check required fields, as these
  // will cause the fuzz test to fail or exit early in ways that aren't
  // interesting.
  encode_options &=
      ~(kUpb_EncodeOption_SkipUnknown | kUpb_EncodeOption_CheckRequired);

  upb_Message* msg_orig = upb_Message_New(original_mt, arena);
  upb_DecodeStatus status =
      upb_Decode(proto_payload.data(), proto_payload.size(), msg_orig,
                 original_mt, exts, decode_options, arena);

  if (status != kUpb_DecodeStatus_Ok) {
    upb_Arena_Free(arena);
    return;
  }

  upb_Message* msg_src = upb_Message_New(src_mt, arena);
  status = upb_Decode(proto_payload.data(), proto_payload.size(), msg_src,
                      src_mt, exts, decode_options, arena);

  if (status != kUpb_DecodeStatus_Ok) {
    upb_Arena_Free(arena);
    return;
  }

  upb_Message* msg_dst = upb_Message_New(dst_mt, arena);
  if (!msg_dst || !UPB_PRIVATE(_upb_Message_Convert)(
                      msg_dst, dst_mt, msg_src, src_mt, nullptr,
                      kUpb_ConvertOption_Alias, arena)) {
    upb_Arena_Free(arena);
    return;
  }

  size_t size;
  char* bytes;
  upb_EncodeStatus enc_status =
      upb_Encode(msg_dst, dst_mt, encode_options, arena, &bytes, &size);

  if (enc_status != kUpb_EncodeStatus_Ok) {
    upb_Arena_Free(arena);
    return;
  }

  upb_Message* msg_final = upb_Message_New(original_mt, arena);
  status = upb_Decode(bytes, size, msg_final, original_mt, exts, decode_options,
                      arena);

  if (status != kUpb_DecodeStatus_Ok) {
    upb_Arena_Free(arena);
    return;
  }

  bool equal = upb_Message_IsEqual(msg_final, msg_orig, original_mt,
                                   kUpb_CompareOption_IncludeUnknownFields);
  if (!equal) {
    abort();
  }

  upb_Arena_Free(arena);
}

TEST(ConvertFuzz, Convert_IdenticalMinitables_ShallowCopy) {
  ConvertFuzz(
      upb::fuzz::MiniTableFuzzInput{
          {"\331", ""}, {"\257"}, "\351\210", {2147483645}},
      4867317803475403639ULL, 4867317803475403639ULL, "@B", 3220282077ULL,
      4189253947ULL);
}

TEST(ConvertFuzz, Convert_DifferentSubsets_DroppedFields_Complex) {
  ConvertFuzz(
      upb::fuzz::MiniTableFuzzInput{
          {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
           "", ""},
          {"\306", "", "\331", ""},
          "\314Q\345\322\024\321\017\021\016\016\016",
          {1,          1,          0,          4294967295, 4294967295,
           1,          4294967295, 4294967295, 4294967295, 4294967295,
           4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
           1,          3135226496, 3135226496, 3135226496, 0,
           3708883408}},
      14162464183153632842ULL, 2939036597827910531ULL,
      "=\032\253\342K\221MQ\nj\n\304\361\364\304", 1620786928, 2077918455);
}

TEST(ConvertFuzz, Convert_DifferentSubsets_DroppedFields_Simple) {
  ConvertFuzz(upb::fuzz::MiniTableFuzzInput{{"", "", "", "", "", "", "", "", "",
                                             "", "", "", "", "", ""},
                                            {""},
                                            "",
                                            {4294967295, 4294967295, 0}},
              4453300303650383170ULL, 13549612121351043620ULL, "]]k\031\320",
              1804506072, 3299701550);
}

TEST(ConvertFuzz,
     Convert_DifferentSubsets_DroppedFields_Regression_89982ef4cfe52331) {
  ConvertFuzz(
      upb::fuzz::MiniTableFuzzInput{
          {"", "DD", "z", "", "", "\200"}, {"\031", "\031", "\031"}, "]]", {}},
      7595264386850118164ULL, 1703605996941841092ULL, "\300D\315\316\372D",
      3067512230, 2237854510);
}

TEST(ConvertFuzz, ManualUnknownFieldRepro) {
  ConvertFuzz(upb::fuzz::MiniTableFuzzInput{{"I", "a"}}, ~0ULL, 0ULL,
              "\010\000", 0, 0);
}

TEST(ConvertFuzz, Regression_FuzzerFinding_UnknownFields) {
  ConvertFuzz(
      upb::fuzz::MiniTableFuzzInput{
          {"", "", ""}, {"", "\211", "", "{", "\232", "2"}, "", {4294967295}},
      5205171011975038739ULL, 18014293590545137538ULL, "E\261k\333\334",
      590856108, 53173774);
}

TEST(ConvertFuzz, RepeatedFieldConfusion) {
  ConvertFuzz(upb::fuzz::MiniTableFuzzInput{{"\t", "\352", "R", "$\214\214\214",
                                             "\004", "$F", "\244", "$\374P"},
                                            {""},
                                            "{GGG",
                                            {4294967295}},
              3117542645911838959, 9223372036854775807, "", 2147221503, 16);
}

TEST(ConvertFuzz, ConvertFuzzRegression) {
  ConvertFuzz(
      upb::fuzz::MiniTableFuzzInput{{""}, {"\325\242"}, "", {2147483647}},
      18446744073709551615u, 10490713252739160816u, " u", 2, 4294967231);
}

TEST(ConvertFuzz, ConvertFuzzRegression_Crash_ConvertInternal_2026_04_07) {
  ConvertFuzz(
      upb::fuzz::MiniTableFuzzInput{
          {""},
          {"", ">>", "", ""},
          "\357\357\357\357\357*"
          "\357\357\357\357\357\357\357\357\357\357\357\357\357\357\357\357\357"
          "\357\225\225\225\357\357\357\357\357\357\357\357\357\357\357\357\357"
          "\357\357\357\357\357\357\357\357\357",
          {4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
           4294967295, 4294967295, 4294967295, 1}},
      12411566311597166710ULL, 8799022017001952335ULL, "%%%%%%%%%%%%%%%%%ZZ%",
      2815879784ULL, 3243839465ULL);
}

TEST(ConvertFuzz, Convert_Fuzz_Crash_Regression) {
  ConvertFuzz(
      upb::fuzz::MiniTableFuzzInput{
          {"", "", ""}, {"", ""}, "\207\254", {4033684729, 4294967294}},
      7437277671727957814ULL, 9800553862402446025ULL,
      "\220Y\203\225\225\366\337\337\004", 3062476814ULL, 4041248646ULL);
}

TEST(ConvertFuzz, ConvertFuzzRegression2) {
  ConvertFuzz(upb::fuzz::MiniTableFuzzInput{{"", ""}, {""}, "", {1903095162}},
              8810965477460052779, 18446744073709551615u, "\030\030", 114210880,
              8190);
}

TEST(ConvertFuzz, ConvertFuzzRegression3) {
  ConvertFuzz(upb::fuzz::MiniTableFuzzInput{{""}, {}, "\373", {}}, 1,
              9223372036854775807, "uz\010\006\006", 4294967290, 2147483647);
}

TEST(ConvertFuzz, ConvertFuzzRegression4) {
  ConvertFuzz(
      upb::fuzz::MiniTableFuzzInput{
          {"$", "C", ""},
          {"r"},
          "\270N\270[",
          {3621539714, 3621539714, 3621539714, 3621539714, 3621539714,
           3621539714, 3621539714, 3621539714, 3621539714, 3621539714,
           3621539714, 3621539714, 3621539714, 3621539714, 3621539714,
           3621539714, 3621539714, 3621539714, 3621539714, 3621539714,
           3621539714, 3621539714, 3621539714, 3621539714, 3621539714,
           3621539714, 3621539714, 3621539714, 3621539714, 3621539714,
           3621539714, 3621539714, 3621539714, 3621539714, 3621539714,
           3621539714, 3621539714, 3621539714, 3621539714, 3621539714,
           3621539714, 3621539714, 3621539714, 3621539714}},
      13429813313754688149ULL, 6819704094736752274ULL, "\255\032\262\337))",
      1611082566, 466142871);
}

TEST(ConvertFuzz, ConvertFuzzRegression5) {
  ConvertFuzz(upb::fuzz::MiniTableFuzzInput{{""}, {"\362", "y"}, "\252", {}},
              8728619288361297649ULL, 13997967148621545864ULL,
              "%\311\311\311\303", 3383556142, 3604956055);
}

TEST(ConvertFuzz, ConvertFuzzRegression6) {
  ConvertFuzz(
      upb::fuzz::MiniTableFuzzInput{
          {"", "", "\341", "\341", "\341", "$", "", "", "\022", "\022", "", "",
           ""},
          {"\374", "\204"},
          "n\251PXq",
          {1,          1,          1,          1,          1,
           1,          1,          1413050348, 1,          1,
           1,          1,          1,          0,          0,
           1,          1,          1,          1,          1,
           1,          1,          1,          1,          1,
           1,          1,          1,          1,          4,
           4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
           4294967295, 4294967295, 4294967295, 1,          1,
           1,          1,          1,          1,          1,
           1,          1,          1,          3028396088, 1,
           1,          1,          1,          1,          4294967295,
           1,          1,          1,          1,          1,
           1,          1,          1,          1,          1,
           1,          1,          1,          1,          0,
           1,          1,          1,          1,          1,
           1,          1,          1,          1,          1}},
      304186569338573724ULL, 6555165345294515829ULL, "8W", 3970284081,
      1611359599);
}

TEST(ConvertFuzz, ConvertFuzzEncodeRegression) {
  ConvertFuzz(
      upb::fuzz::MiniTableFuzzInput{{"$$$$$$$$$$$$$$$$$$", "", "", "", "", ""},
                                    {"", ""},
                                    "D",
                                    {2462394141, 2462394141, 2462394145}},
      555217012043469213, 13507447059222749214u,
      "pm\t\t\t\t\t\t\t\t\t\t\t\005o\t\t\t\t\t\trr\375\375\375r\251r", 32766,
      16);
}

}  // namespace

}  // namespace upb
