#include <cstddef>
#include <string_view>

#include <gtest/gtest.h>
#include "upb/upb/base/status.hpp"
#include "upb/upb/jspb/decode.h"
#include "upb/upb/jspb/encode.h"
#include "upb/upb/mem/arena.h"
#include "upb/upb/mem/arena.hpp"
#include "upb/upb/message/message.h"
#include "upb/upb/mini_table/extension_registry.h"
#include "upb/upb/mini_table/message.h"
#include "upb/upb/test/fuzz_util.h"

// begin:google_only
// #include "testing/fuzzing/fuzztest.h"
// end:google_only

namespace {

static void DecodeEncodeArbitrarySchemaAndPayload(
    const upb::fuzz::MiniTableFuzzInput& input,
    std::string_view proto_payload) {
// Lexan does not have setenv
#ifndef _MSC_VER
  setenv("FUZZTEST_STACK_LIMIT", "262144", 1);
#endif
  upb::Arena arena;
  upb::Status decode_status;
  upb_ExtensionRegistry* exts;
  const upb_MiniTable* mini_table =
      upb::fuzz::BuildMiniTable(input, &exts, arena.ptr());
  if (!mini_table) return;
  upb_Message* msg = upb_Message_New(mini_table, arena.ptr());
  bool ok =
      upb_JspbDecode(proto_payload.data(), proto_payload.size(), msg,
                     mini_table, exts, 0, arena.ptr(), decode_status.ptr());
  if (!ok) return;

  upb::Status encode_1_status;
  size_t size = upb_JspbEncode(msg, mini_table, nullptr, 0, nullptr, 0,
                               encode_1_status.ptr());
  char* jspb_buf = (char*)upb_Arena_Malloc(arena.ptr(), size + 1);

  upb::Status encode_2_status;
  size_t written = upb_JspbEncode(msg, mini_table, nullptr, 0, jspb_buf,
                                  size + 1, encode_2_status.ptr());
  EXPECT_EQ(written, size);
}
FUZZ_TEST(FuzzTest, DecodeEncodeArbitrarySchemaAndPayload);

TEST(FuzzTest, UnclosedMessageTrailingNumber) {
  DecodeEncodeArbitrarySchemaAndPayload({{"$<$"}, {""}, "", {4133236930}},
                                        "[2");

  TEST(FuzzTest, RunoffDenseMessage) {
    DecodeEncodeArbitrarySchemaAndPayload(
        {{"\355", "$GG+", ""}, {"\352"}, "\347\232\232\232\232\232\232", {1}},
        std::string(
            "[[[[[[[][[[[[[[[[[[[[[[[8\0068\002`Q\000\000\001\000\000\000[[[[[",
            41));
  }

}  // namespace
