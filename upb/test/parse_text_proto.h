// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef UPB_UPB_TEST_PARSE_TEXT_PROTO_H_
#define UPB_UPB_TEST_PARSE_TEXT_PROTO_H_

#include <string>

#include "gtest/gtest.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"

namespace upb_test {

// Replacement for Google ParseTextProtoOrDie.
// Only to be used in unit tests.
// Usage: MyMessage msg = ParseTextProtoOrDie(my_text_proto);
class ParseTextProtoOrDie {
 public:
  explicit ParseTextProtoOrDie(absl::string_view text_proto)
      : text_proto_(text_proto) {}

  template <class T>
  operator T() {  // NOLINT: Needed to support parsing text proto as appropriate
                  // type.
    T message;
    if (!google::protobuf::TextFormat::ParseFromString(text_proto_, &message)) {
      ADD_FAILURE() << "Failed to parse textproto: " << text_proto_;
      abort();
    }
    return message;
  }

 private:
  std::string text_proto_;
};

}  // namespace upb_test

#endif  // UPB_UPB_TEST_PARSE_TEXT_PROTO_H_
