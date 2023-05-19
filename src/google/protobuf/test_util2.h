// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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
//     * Neither the name of Google Inc. nor the names of its
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

#ifndef GOOGLE_PROTOBUF_TEST_UTIL2_H__
#define GOOGLE_PROTOBUF_TEST_UTIL2_H__

#include <string>

#include "google/protobuf/testing/googletest.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/util/message_differencer.h"

namespace google {
namespace protobuf {
namespace TestUtil {

// Translate net/proto2/* or third_party/protobuf/* to google/protobuf/*.
inline std::string TranslatePathToOpensource(absl::string_view google3_path) {
  constexpr absl::string_view net_proto2 = "net/proto2/";
  constexpr absl::string_view third_party_protobuf = "third_party/protobuf/";
  if (!absl::ConsumePrefix(&google3_path, net_proto2)) {
    ABSL_CHECK(absl::ConsumePrefix(&google3_path, third_party_protobuf))
        << google3_path;
  }

  absl::ConsumePrefix(&google3_path, "internal/");
  absl::ConsumePrefix(&google3_path, "proto/");
  absl::ConsumeSuffix(&google3_path, "public/");
  return absl::StrCat("google/protobuf/", google3_path);
}

inline std::string MaybeTranslatePath(absl::string_view google3_path) {
  return TranslatePathToOpensource(google3_path);
  return std::string(google3_path);
}

inline std::string TestSourceDir() {
  return google::protobuf::TestSourceDir();
}

inline std::string GetTestDataPath(absl::string_view google3_path) {
  return absl::StrCat(TestSourceDir(), "/", MaybeTranslatePath(google3_path));
}

// Checks the equality of "message" and serialized proto of type "ProtoType".
// Do not directly compare two serialized protos.
template <typename ProtoType>
bool EqualsToSerialized(const ProtoType& message, const std::string& data) {
  ProtoType other;
  other.ParsePartialFromString(data);
  return util::MessageDifferencer::Equals(message, other);
}

// Wraps io::ArrayInputStream while checking against bound. When a blocking
// stream is used with bounded length, proto parsing must not access beyond the
// bound. Otherwise, it can result in unintended block, then deadlock.
class BoundedArrayInputStream : public io::ZeroCopyInputStream {
 public:
  BoundedArrayInputStream(const void* data, int size)
      : stream_(data, size), bound_(size) {}
  ~BoundedArrayInputStream() override {}

  bool Next(const void** data, int* size) override {
    ABSL_CHECK_LT(stream_.ByteCount(), bound_);
    return stream_.Next(data, size);
  }
  void BackUp(int count) override { stream_.BackUp(count); }
  bool Skip(int count) override { return stream_.Skip(count); }
  int64_t ByteCount() const override { return stream_.ByteCount(); }

 private:
  io::ArrayInputStream stream_;
  int bound_;
};

}  // namespace TestUtil
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_TEST_UTIL2_H__
