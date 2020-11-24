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

#include <google/protobuf/arenastring.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/generated_message_util.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/strutil.h>


// Must be included last.
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {

using internal::ArenaStringPtr;

static std::string WrapString(const char* value) { return value; }

using EmptyDefault = ArenaStringPtr::EmptyDefault;

const internal::LazyString nonempty_default{{{"default", 7}}, {nullptr}};

// Test ArenaStringPtr with arena == NULL.
TEST(ArenaStringPtrTest, ArenaStringPtrOnHeap) {
  ArenaStringPtr field;
  const std::string* empty_default = &internal::GetEmptyString();
  field.UnsafeSetDefault(empty_default);
  EXPECT_EQ(std::string(""), field.Get());
  field.Set(empty_default, WrapString("Test short"), NULL);
  EXPECT_EQ(std::string("Test short"), field.Get());
  field.Set(empty_default, WrapString("Test long long long long value"), NULL);
  EXPECT_EQ(std::string("Test long long long long value"), field.Get());
  field.Set(empty_default, std::string(""), NULL);
  field.Destroy(empty_default, NULL);

  ArenaStringPtr field2;
  field2.UnsafeSetDefault(empty_default);
  std::string* mut = field2.Mutable(EmptyDefault{}, NULL);
  EXPECT_EQ(mut, field2.Mutable(EmptyDefault{}, NULL));
  EXPECT_EQ(mut, &field2.Get());
  EXPECT_NE(empty_default, mut);
  EXPECT_EQ(std::string(""), *mut);
  *mut = "Test long long long long value";  // ensure string allocates storage
  EXPECT_EQ(std::string("Test long long long long value"), field2.Get());
  field2.Destroy(empty_default, NULL);

  ArenaStringPtr field3;
  field3.UnsafeSetDefault(nullptr);
  mut = field3.Mutable(nonempty_default, NULL);
  EXPECT_EQ(mut, field3.Mutable(nonempty_default, NULL));
  EXPECT_EQ(mut, &field3.Get());
  EXPECT_NE(nullptr, mut);
  EXPECT_EQ(std::string("default"), *mut);
  *mut = "Test long long long long value";  // ensure string allocates storage
  EXPECT_EQ(std::string("Test long long long long value"), field3.Get());
  field3.Destroy(nullptr, NULL);
}

TEST(ArenaStringPtrTest, ArenaStringPtrOnArena) {
  Arena arena;
  ArenaStringPtr field;
  const std::string* empty_default = &internal::GetEmptyString();
  field.UnsafeSetDefault(empty_default);
  EXPECT_EQ(std::string(""), field.Get());
  field.Set(empty_default, WrapString("Test short"), &arena);
  EXPECT_EQ(std::string("Test short"), field.Get());
  field.Set(empty_default, WrapString("Test long long long long value"),
            &arena);
  EXPECT_EQ(std::string("Test long long long long value"), field.Get());
  field.Set(empty_default, std::string(""), &arena);
  field.Destroy(empty_default, &arena);

  ArenaStringPtr field2;
  field2.UnsafeSetDefault(empty_default);
  std::string* mut = field2.Mutable(EmptyDefault{}, &arena);
  EXPECT_EQ(mut, field2.Mutable(EmptyDefault{}, &arena));
  EXPECT_EQ(mut, &field2.Get());
  EXPECT_NE(empty_default, mut);
  EXPECT_EQ(std::string(""), *mut);
  *mut = "Test long long long long value";  // ensure string allocates storage
  EXPECT_EQ(std::string("Test long long long long value"), field2.Get());
  field2.Destroy(empty_default, &arena);

  ArenaStringPtr field3;
  field3.UnsafeSetDefault(nullptr);
  mut = field3.Mutable(nonempty_default, &arena);
  EXPECT_EQ(mut, field3.Mutable(nonempty_default, &arena));
  EXPECT_EQ(mut, &field3.Get());
  EXPECT_NE(nullptr, mut);
  EXPECT_EQ(std::string("default"), *mut);
  *mut = "Test long long long long value";  // ensure string allocates storage
  EXPECT_EQ(std::string("Test long long long long value"), field3.Get());
  field3.Destroy(nullptr, &arena);
}

TEST(ArenaStringPtrTest, ArenaStringPtrOnArenaNoSSO) {
  Arena arena;
  ArenaStringPtr field;
  const std::string* empty_default = &internal::GetEmptyString();
  field.UnsafeSetDefault(empty_default);
  EXPECT_EQ(std::string(""), field.Get());

  // Avoid triggering the SSO optimization by setting the string to something
  // larger than the internal buffer.
  field.Set(empty_default, WrapString("Test long long long long value"),
            &arena);
  EXPECT_EQ(std::string("Test long long long long value"), field.Get());
  field.Set(empty_default, std::string(""), &arena);
  field.Destroy(empty_default, &arena);

  ArenaStringPtr field2;
  field2.UnsafeSetDefault(empty_default);
  std::string* mut = field2.Mutable(EmptyDefault{}, &arena);
  EXPECT_EQ(mut, field2.Mutable(EmptyDefault{}, &arena));
  EXPECT_EQ(mut, &field2.Get());
  EXPECT_NE(empty_default, mut);
  EXPECT_EQ(std::string(""), *mut);
  *mut = "Test long long long long value";  // ensure string allocates storage
  EXPECT_EQ(std::string("Test long long long long value"), field2.Get());
  field2.Destroy(empty_default, &arena);

  ArenaStringPtr field3;
  field3.UnsafeSetDefault(nullptr);
  mut = field3.Mutable(nonempty_default, &arena);
  EXPECT_EQ(mut, field3.Mutable(nonempty_default, &arena));
  EXPECT_EQ(mut, &field3.Get());
  EXPECT_NE(nullptr, mut);
  EXPECT_EQ(std::string("default"), *mut);
  *mut = "Test long long long long value";  // ensure string allocates storage
  EXPECT_EQ(std::string("Test long long long long value"), field3.Get());
  field3.Destroy(nullptr, &arena);
}


}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>
