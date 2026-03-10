
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto2.upb_minitable.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/message/compare.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

namespace {

static const upb_MiniTable* kTestMiniTable =
    &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

static void TestEncodeDecodeRoundTrip(
    upb_Arena* arena,
    std::vector<protobuf_test_messages_proto2_TestAllTypesProto2*> msgs) {
  // Encode all of the messages and put their serializations contiguously.
  std::string s;
  for (auto msg : msgs) {
    char* buf;
    size_t size;
    ASSERT_TRUE(upb_EncodeLengthPrefixed(UPB_UPCAST(msg), kTestMiniTable, 0,
                                         arena, &buf,
                                         &size) == kUpb_EncodeStatus_Ok);
    ASSERT_GT(size, 0);  // Even empty messages are 1 byte in this encoding.
    s.append(std::string(buf, size));
  }

  // Now decode all of the messages contained in the contiguous block.
  std::vector<protobuf_test_messages_proto2_TestAllTypesProto2*> decoded;
  while (!s.empty()) {
    protobuf_test_messages_proto2_TestAllTypesProto2* msg =
        protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
    size_t num_bytes_read;
    ASSERT_TRUE(upb_DecodeLengthPrefixed(
                    s.data(), s.length(), UPB_UPCAST(msg), &num_bytes_read,
                    kTestMiniTable, nullptr, 0, arena) == kUpb_DecodeStatus_Ok);
    ASSERT_GT(num_bytes_read, 0);
    decoded.push_back(msg);
    s = s.substr(num_bytes_read);
  }

  // Make sure that the values round tripped correctly.
  ASSERT_EQ(msgs.size(), decoded.size());
  for (size_t i = 0; i < msgs.size(); ++i) {
    ASSERT_TRUE(upb_Message_IsEqual(UPB_UPCAST(msgs[i]), UPB_UPCAST(decoded[i]),
                                    kTestMiniTable, 0));
  }
}

TEST(LengthPrefixedTest, OneEmptyMessage) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  TestEncodeDecodeRoundTrip(arena, {msg});
  upb_Arena_Free(arena);
}

TEST(LengthPrefixedTest, AFewMessages) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* a =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  protobuf_test_messages_proto2_TestAllTypesProto2* b =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  protobuf_test_messages_proto2_TestAllTypesProto2* c =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);

  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_bool(a, true);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(b, 1);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_oneof_string(
      c, upb_StringView_FromString("string"));

  TestEncodeDecodeRoundTrip(arena, {a, b, c});
  upb_Arena_Free(arena);
}

}  // namespace
