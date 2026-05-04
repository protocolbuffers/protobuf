// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/fuzz_impl.h"

#include <array>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/strings/escaping.h"
#include "absl/strings/string_view.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/message/compare.h"
#include "upb/message/message.h"
#include "upb/mini_table/debug_string.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"
#include "upb/test/fuzz_util.h"
#include "upb/text/debug_string.h"
#include "upb/text/options.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

namespace {

std::pair<upb_Message*, upb_DecodeStatus> Decode(
    std::string_view proto_payload, int decode_options, upb_Arena* arena,
    const upb_MiniTable* mini_table, upb_ExtensionRegistry* exts,
    bool length_prefixed, std::array<char, 64>* trace_buf = nullptr) {
  upb_Message* msg = upb_Message_New(mini_table, arena);
  upb_DecodeStatus status;
  if (length_prefixed) {
    size_t num_bytes_read = 0;
    status = upb_DecodeLengthPrefixed(
        proto_payload.data(), proto_payload.size(), msg, &num_bytes_read,
        mini_table, exts, decode_options, arena);
    EXPECT_TRUE(status != kUpb_DecodeStatus_Ok ||
                num_bytes_read <= proto_payload.size());
  } else {
    status = upb_DecodeWithTrace(proto_payload.data(), proto_payload.size(),
                                 msg, mini_table, exts, decode_options, arena,
                                 trace_buf ? trace_buf->data() : nullptr,
                                 trace_buf ? trace_buf->size() : 0);
  }
  return {msg, status};
}

std::optional<absl::string_view> Encode(const upb_Message* msg,
                                        const upb_MiniTable* mini_table,
                                        int encode_options, upb_Arena* arena,
                                        bool length_prefixed) {
  char* ptr;
  size_t size;
  upb_EncodeStatus status;
  if (length_prefixed) {
    status = upb_EncodeLengthPrefixed(msg, mini_table, encode_options, arena,
                                      &ptr, &size);
  } else {
    status = upb_Encode(msg, mini_table, encode_options, arena, &ptr, &size);
  }
  return status == kUpb_EncodeStatus_Ok
             ? std::make_optional(absl::string_view(ptr, size))
             : std::nullopt;
}

bool EncodeDepthIsMoreStrict(int encode_options, int decode_options) {
  int encode_max_depth = upb_EncodeOptions_GetEffectiveMaxDepth(encode_options);
  int decode_max_depth = upb_DecodeOptions_GetEffectiveMaxDepth(decode_options);
  return encode_max_depth <= decode_max_depth;
}

bool EncodeRequiredCheckIsMoreStrict(int encode_options, int decode_options) {
  return ((encode_options & kUpb_EncodeOption_CheckRequired) != 0) >
         ((decode_options & kUpb_DecodeOption_CheckRequired) != 0);
}

std::string DebugString(const upb_Message* msg, const upb_MiniTable* mini_table,
                        int options) {
  size_t size = upb_DebugString(msg, mini_table, options, nullptr, 0);
  std::vector<char> buf(size + 1);
  upb_DebugString(msg, mini_table, options, &buf[0], size + 1);
  return std::string(buf.data(), size);
}

}  // namespace

void DecodeEncodeArbitrarySchemaAndPayload(
    const upb::fuzz::MiniTableFuzzInput& input, absl::string_view proto_payload,
    int decode_options, int encode_options, bool length_prefixed) {
  std::array<char, 64> trace_buf;
  decode_options = upb_Decode_LimitDepth(decode_options, 80);
  encode_options = upb_Encode_LimitDepth(encode_options, 80);

  int compare_options = encode_options & kUpb_EncodeOption_SkipUnknown
                            ? 0
                            : kUpb_CompareOption_IncludeUnknownFields;
  int debug_options = encode_options & kUpb_EncodeOption_SkipUnknown
                          ? UPB_TXTENC_SKIPUNKNOWN
                          : 0;

  upb::Arena arena;
  upb_ExtensionRegistry* exts;
  const upb_MiniTable* mini_table =
      upb::fuzz::BuildMiniTable(input, &exts, arena.ptr());
  if (!mini_table) return;

  // We always call DebugString() on the mini-table, so we want to ensure that
  // it works and that it doesn't crash or anything. But we only log it if
  // the special "dump" flag is set.
  size_t debug_size = 65535;
  std::unique_ptr<char[]> debug_buf(new char[debug_size]);
  upb_MiniTable_DebugString(mini_table, debug_buf.get(), debug_size);
#ifdef UPB_FUZZ_DUMP_MINITABLE
  ABSL_LOG(INFO) << "MiniTable: " << absl::string_view(debug_buf.get());
#endif

  // Always copy the input payload to a perfectly-sized buffer, so that ASAN
  // can properly check that we're not overrunning the input payload by even
  // a single byte.
  std::unique_ptr<char[]> payload_buf(new char[proto_payload.size()]);
  memcpy(payload_buf.get(), proto_payload.data(), proto_payload.size());
  proto_payload = absl::string_view(payload_buf.get(), proto_payload.size());

  // Check that we can decode the input payload into a upb_Message, without
  // crashing, no matter what the input bytes may be.
  auto [msg, status] = Decode(proto_payload, decode_options, arena.ptr(),
                              mini_table, exts, length_prefixed, &trace_buf);

  // Uncomment for debugging.
  // ABSL_LOG(INFO) << "Trace: " << absl::string_view(trace_buf.data());

  if (status != kUpb_DecodeStatus_Ok) {
    // Parsing failed. The message should still be in a valid state, so the
    // encoder should not crash, but encoding could fail (for example, if the
    // resulting message exceeds the encode depth limit).
    Encode(msg, mini_table, encode_options, arena.ptr(), length_prefixed);
    return;
  }

  auto [msg_nofast, status_nofast] =
      Decode(proto_payload, decode_options | kUpb_DecodeOption_DisableFastTable,
             arena.ptr(), mini_table, exts, length_prefixed);
  ASSERT_TRUE(status_nofast == kUpb_DecodeStatus_Ok)
      << upb_DecodeStatus_String(status_nofast);

  EXPECT_TRUE(upb_Message_IsEqual(msg, msg_nofast, mini_table, compare_options))
      << "msg: " << DebugString(msg, mini_table, debug_options)
      << "\nmsg_nofast:\n"
      << DebugString(msg_nofast, mini_table, debug_options)
      << "\nproto_payload:\n"
      << absl::CEscape(proto_payload)
      << "\ncompare_options: " << compare_options;

  std::optional<absl::string_view> encoded =
      Encode(msg, mini_table, encode_options, arena.ptr(), length_prefixed);

  if (!encoded.has_value()) {
    // Encoding failed, despite the message being valid. This should not happen,
    // except in the case where the encode depth limit or required check is more
    // strict than the decoder.
    ASSERT_TRUE(
        EncodeDepthIsMoreStrict(encode_options, decode_options) ||
        EncodeRequiredCheckIsMoreStrict(encode_options, decode_options));
    return;
  }

  // At this point we remove any depth limit on the decoder, because in some
  // cases the encoded message is deeper than the original message (eg. because
  // a map key/value were added to a map entry).
  int no_depth_limit_decode_options = upb_Decode_LimitDepth(decode_options, 0);

  auto [msg2, status2] = Decode(*encoded, no_depth_limit_decode_options,
                                arena.ptr(), mini_table, exts, length_prefixed);
  ASSERT_EQ(status2, kUpb_DecodeStatus_Ok)
      << "Original payload: " << absl::CEscape(proto_payload)
      << "\nEncoded payload (failed to parse): " << absl::CEscape(*encoded)
      << "\nParse error: " << upb_DecodeStatus_String(status2)
      << "\ndecode_options: " << decode_options
      << "\nencode_options: " << encode_options << "\nmsg:\n"
      << DebugString(msg, mini_table, 0 /*debug_options*/);

  // Check that the two messages are equal.
  EXPECT_TRUE(upb_Message_IsEqual(msg, msg2, mini_table, compare_options))
      << "Original payload (msg): " << absl::CEscape(proto_payload)
      << "\nEncoded payload (msg2): " << absl::CEscape(*encoded) << "\nmsg:\n"
      << DebugString(msg, mini_table, debug_options) << "\nmsg2:\n"
      << DebugString(msg2, mini_table, debug_options)
      << "\nencoded: " << absl::CEscape(encoded.value())
      << "\ncompare_options: " << compare_options;
}
