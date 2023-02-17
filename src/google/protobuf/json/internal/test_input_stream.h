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

#ifndef GOOGLE_PROTOBUF_UTIL_TEST_INPUT_STREAM_H__
#define GOOGLE_PROTOBUF_UTIL_TEST_INPUT_STREAM_H__

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/stubs/status_macros.h"

namespace google {
namespace protobuf {
namespace json_internal {
// A ZeroCopyInputStream for writing unit tests.
class TestInputStream final : public io::ZeroCopyInputStream {
 public:
  TestInputStream(std::initializer_list<std::string> strings)
      : strings_(strings) {}
  explicit TestInputStream(std::vector<std::string> strings)
      : strings_(std::move(strings)) {}
  ~TestInputStream() override = default;

  size_t Consumed() const { return next_; }

  bool Next(const void** data, int* size) override {
    if (next_ == strings_.size()) {
      return false;
    }

    if (next_ > 0) {
      // Destroy the previous string so that ASAN can catch misbehavior
      // correctly.
      ReconstructAt(&strings_[next_ - 1]);
    }

    absl::string_view next = strings_[next_++];
    *data = next.data();
    *size = static_cast<int>(next.size());
    return true;
  }

  // TestInputStream currently does not support these members.
  void BackUp(int) override { ABSL_CHECK(false); }
  bool Skip(int) override {
    ABSL_CHECK(false);
    return false;
  }
  int64_t ByteCount() const override {
    ABSL_CHECK(false);
    return 0;
  }

 private:
  // Some versions of Clang can't figure out that
  //   x.std::string::~string()
  // is valid syntax, so we indirect through a type param, instead.
  //
  // Of course, our luck has it that std::destroy_at is a C++17 feature. :)
  template <typename T>
  static void ReconstructAt(T* p) {
    p->~T();
    new (p) T;
  }

  std::vector<std::string> strings_;
  size_t next_ = 0;
};
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_UTIL_TEST_INPUT_STREAM_H__
