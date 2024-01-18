// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/text/encode.h"

#include <cstddef>

#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/text_format.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"
#include "upb/text/encode_test.pb.h"
#include "upb/text/encode_test.upb.h"
#include "upb/text/encode_test.upbdefs.h"

// begin:google_only
#include "testing/fuzzing/fuzztest.h"
// end:google_only

namespace {

// begin:google_only

class ReflectionFuzzTest {
 public:
  ReflectionFuzzTest() {
    message_def_ = upb::MessageDefPtr(
        upb_text_test_Proto2StringMessage_getmsgdef(def_pool_.ptr()));
  }

  void EncodeArbitraryStringField(std::string_view str_data) {
    upb::Arena arena;
    upb_text_test_Proto2StringMessage* msg =
        upb_text_test_Proto2StringMessage_new(arena.ptr());
    upb_text_test_Proto2StringMessage_set_str(
        msg, upb_StringView_FromDataAndSize(str_data.data(), str_data.size()));
    char buf[1024];
    size_t size = upb_TextEncode(UPB_UPCAST(msg), message_def_.ptr(), nullptr,
                                 0, buf, sizeof(buf));
    if (size >= sizeof(buf)) return;

    upb_text_test::Proto2StringMessage google_protobuf_msg;
    ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
        absl::string_view(buf, size), &google_protobuf_msg));
    ASSERT_EQ(google_protobuf_msg.str(), str_data);
  }

 private:
  upb::DefPool def_pool_;
  upb::MessageDefPtr message_def_;
};

FUZZ_TEST_F(ReflectionFuzzTest, EncodeArbitraryStringField);

// end:google_only

}  // namespace
