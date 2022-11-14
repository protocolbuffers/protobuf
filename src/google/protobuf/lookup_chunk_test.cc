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

#include "google/protobuf/lookup_chunk.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::SizeIs;

using Chunk = LookupChunk<void, void>;

std::vector<void*> ToPointers(absl::Span<const std::atomic<void*>> values) {
  std::vector<void*> pointers;
  for (auto& value : values) {
    pointers.push_back(value.load());
  }
  return pointers;
}

TEST(LookupChunk, Create) {
  void* key = &key;
  void* value = &value;
  alignas(Chunk) char instance[Chunk::AllocSize(2)];
  Chunk* chunk = new (instance) Chunk(2, nullptr);
  EXPECT_THAT(chunk->capacity(), Eq(2));
  EXPECT_THAT(chunk->size(), Eq(0));
  EXPECT_THAT(chunk->next(), Eq(nullptr));
  EXPECT_THAT(chunk->keys(), SizeIs(0));
  EXPECT_THAT(chunk->values(), SizeIs(0));

  alignas(Chunk) char instance2[Chunk::AllocSize(2)];
  Chunk* chunk2 = new (instance2) Chunk(2, chunk);
  EXPECT_THAT(chunk2->next(), Eq(chunk));
}

TEST(LookupChunk, SetNext) {
  alignas(Chunk) char instance[Chunk::AllocSize(2)];
  Chunk* chunk = new (instance) Chunk(2, nullptr);

  alignas(Chunk) char instance2[Chunk::AllocSize(2)];
  Chunk* chunk2 = new (instance2) Chunk(2, nullptr);
  chunk2->set_next(chunk);
  EXPECT_THAT(chunk2->next(), Eq(chunk));
}

TEST(LookupChunk, CreateWithKey) {
  void* key = &key;
  void* value = &value;
  alignas(Chunk) char instance[Chunk::AllocSize(2)];
  Chunk* chunk = new (instance) Chunk(2, key, value, nullptr);
  EXPECT_THAT(chunk->capacity(), Eq(2));
  EXPECT_THAT(chunk->size(), Eq(1));
  EXPECT_THAT(ToPointers(chunk->keys()), ElementsAre(key));
  EXPECT_THAT(ToPointers(chunk->values()), ElementsAre(value));
}

TEST(LookupChunk, Add) {
  void* key1 = &key1;
  void* key2 = &key2;
  void* value1 = &value1;
  void* value2 = &value2;
  alignas(Chunk) char instance[Chunk::AllocSize(2)];
  Chunk* chunk = new (instance) Chunk(2, nullptr);
  EXPECT_TRUE(chunk->Add(key1, value1));
  EXPECT_THAT(chunk->size(), Eq(1));
  EXPECT_TRUE(chunk->Add(key2, value2));
  EXPECT_THAT(chunk->size(), Eq(2));
  EXPECT_FALSE(chunk->Add(value2, key2));
  EXPECT_THAT(chunk->size(), Eq(2));
  EXPECT_THAT(ToPointers(chunk->keys()), ElementsAre(key1, key2));
  EXPECT_THAT(ToPointers(chunk->values()), ElementsAre(value1, value2));
}

TEST(LookupChunk, Find) {
  void* key1 = &key1;
  void* key2 = &key2;
  void* value1 = &value1;
  void* value2 = &value2;
  alignas(Chunk) char instance[Chunk::AllocSize(2)];
  Chunk* chunk = new (instance) Chunk(2, nullptr);
  chunk->Add(key1, value1);
  chunk->Add(key2, value2);
  EXPECT_THAT(chunk->Find(key1), Eq(value1));
  EXPECT_THAT(chunk->Find(key2), Eq(value2));
  EXPECT_THAT(chunk->Find(value1), Eq(nullptr));
  EXPECT_THAT(chunk->Find(nullptr), Eq(nullptr));
}

TEST(FixedLookupChunk, Create) {
  alignas(Chunk) char instance[Chunk::AllocSize(3)];
  Chunk* chunk = new (instance) Chunk(3, nullptr);
  EXPECT_THAT(chunk->capacity(), Eq(3));
  EXPECT_THAT(chunk->size(), Eq(0));
  EXPECT_THAT(chunk->next(), Eq(nullptr));
  EXPECT_THAT(chunk->keys(), SizeIs(0));
  EXPECT_THAT(chunk->values(), SizeIs(0));
}

TEST(LookupChunk, AddAndFind) {
  void* key1 = &key1;
  void* key2 = &key2;
  void* key3 = &key3;
  void* value1 = &value1;
  void* value2 = &value2;
  void* value3 = &value3;
  alignas(Chunk) char instance[Chunk::AllocSize(3)];
  Chunk* chunk = new (instance) Chunk(3, nullptr);
  EXPECT_TRUE(chunk->Add(key1, value1));
  EXPECT_TRUE(chunk->Add(key2, value2));
  EXPECT_TRUE(chunk->Add(key3, value3));
  EXPECT_FALSE(chunk->Add(value1, key1));
  EXPECT_THAT(chunk->Find(key1), Eq(value1));
  EXPECT_THAT(chunk->Find(key2), Eq(value2));
  EXPECT_THAT(chunk->Find(key3), Eq(value3));
  EXPECT_THAT(chunk->Find(value1), Eq(nullptr));
  EXPECT_THAT(chunk->Find(nullptr), Eq(nullptr));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
