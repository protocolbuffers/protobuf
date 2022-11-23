// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
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

#include "google/protobuf/arena_cleanup.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {
namespace cleanup {
namespace {

using testing::Eq;

struct DtorTracker {
  DtorTracker() {
    count = 0;
    object = nullptr;
  }

  static void dtor(void* obj) {
    ++count;
    object = obj;
  }

  static int count;
  static void* object;
};

int DtorTracker::count;
void* DtorTracker::object;

TEST(CleanupTest, CreateDestroyDynamicNode) {
  alignas(ArenaAlignDefault::align) char buffer[1024];
  void* object = &object;
  DtorTracker dtor_tracker;

  CreateNode(buffer, object, &DtorTracker::dtor);
  EXPECT_THAT(DestroyNodeAt(buffer), Eq(sizeof(DynamicNode)));
  EXPECT_THAT(DtorTracker::count, Eq(1));
  EXPECT_THAT(DtorTracker::object, Eq(object));
}

TEST(CleanupTest, CreateDestroyEmbeddedNode) {
  alignas(ArenaAlignDefault::align) char buffer[1024];
  void* object = buffer + sizeof(DynamicNode);
  DtorTracker dtor_tracker;

  CreateNode(buffer, sizeof(DynamicNode) + 128, &DtorTracker::dtor);
  EXPECT_THAT(DestroyNodeAt(buffer), Eq(sizeof(DynamicNode) + 128));
  EXPECT_THAT(DtorTracker::count, Eq(1));
  EXPECT_THAT(DtorTracker::object, Eq(object));
}

TEST(CleanupTest, CreateDestroyStringNode) {
  alignas(ArenaAlignDefault::align) char buffer[1024];
  alignas(std::string) char instance[sizeof(std::string)];
  std::string* s = new (instance) std::string(1000, 'x');
  CreateNode(buffer, s, Tag::kString);
  EXPECT_THAT(DestroyNodeAt(buffer), Eq(sizeof(TaggedNode)));
}

TEST(CleanupTest, CreateDestroyEmbeddedStringNode) {
  alignas(ArenaAlignDefault::align) char buffer[1024];
  new (buffer + sizeof(TaggedNode)) std::string(1000, 'x');
  CreateNode(buffer, Tag::kString);
  EXPECT_THAT(DestroyNodeAt(buffer),
              Eq(sizeof(TaggedNode) + sizeof(std::string)));
}

}  // namespace
}  // namespace cleanup
}  // namespace internal
}  // namespace protobuf
}  // namespace google
