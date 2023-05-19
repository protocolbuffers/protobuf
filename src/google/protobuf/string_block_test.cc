// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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
//
// This file defines tests for the internal StringBlock class

#include "google/protobuf/string_block.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Must be included last
#include "google/protobuf/port_def.inc"

using ::testing::Eq;
using ::testing::Ne;

namespace google {
namespace protobuf {
namespace internal {
namespace {

size_t EffectiveSizeFor(int size) {
  size -= sizeof(StringBlock);
  return static_cast<size_t>(size - (size % sizeof(std::string)));
}

size_t AllocatedSizeFor(int size) {
  return EffectiveSizeFor(size) + sizeof(StringBlock);
}

TEST(StringBlockTest, HeapAllocateOneBlock) {
  StringBlock* block = StringBlock::New(nullptr);

  ASSERT_THAT(block, Ne(nullptr));
  EXPECT_THAT(block->next(), Eq(nullptr));
  ASSERT_TRUE(block->heap_allocated());
  EXPECT_THAT(block->allocated_size(), Eq(AllocatedSizeFor(256)));
  EXPECT_THAT(block->effective_size(), Eq(EffectiveSizeFor(256)));
  EXPECT_THAT(block->begin(), Eq(block->AtOffset(0)));
  EXPECT_THAT(block->end(), Eq(block->AtOffset(block->effective_size())));

  EXPECT_THAT(StringBlock::Delete(block), Eq(AllocatedSizeFor(256)));
}

TEST(StringBlockTest, EmplaceOneBlock) {
  // NextSize() returns unrounded 'min_size()' on first call.
  size_t size = StringBlock::NextSize(nullptr);
  EXPECT_THAT(size, Eq(256));

  auto buffer = std::make_unique<char[]>(size);
  StringBlock* block = StringBlock::Emplace(buffer.get(), size, nullptr);

  ASSERT_THAT(block, Ne(nullptr));
  EXPECT_THAT(block->next(), Eq(nullptr));
  ASSERT_FALSE(block->heap_allocated());
  EXPECT_THAT(block->allocated_size(), Eq(AllocatedSizeFor(256)));
  EXPECT_THAT(block->effective_size(), Eq(EffectiveSizeFor(256)));
  EXPECT_THAT(block->begin(), Eq(block->AtOffset(0)));
  EXPECT_THAT(block->end(), Eq(block->AtOffset(block->effective_size())));

  EXPECT_THAT(StringBlock::Delete(block), Eq(0));
}

TEST(StringBlockTest, HeapAllocateMultipleBlocks) {
  // Note: first two blocks are 256
  StringBlock* previous = StringBlock::New(nullptr);

  for (int size = 256; size <= 8192; size *= 2) {
    StringBlock* block = StringBlock::New(previous);
    ASSERT_THAT(block, Ne(nullptr));
    ASSERT_THAT(block->next(), Eq(previous));
    ASSERT_TRUE(block->heap_allocated());
    ASSERT_THAT(block->allocated_size(), Eq(AllocatedSizeFor(size)));
    ASSERT_THAT(block->effective_size(), Eq(EffectiveSizeFor(size)));
    ASSERT_THAT(block->begin(), Eq(block->AtOffset(0)));
    ASSERT_THAT(block->end(), Eq(block->AtOffset(block->effective_size())));
    previous = block;
  }

  // Capped at 8K from here on
  StringBlock* block = StringBlock::New(previous);
  ASSERT_THAT(block, Ne(nullptr));
  ASSERT_THAT(block->next(), Eq(previous));
  ASSERT_TRUE(block->heap_allocated());
  ASSERT_THAT(block->allocated_size(), Eq(AllocatedSizeFor(8192)));
  ASSERT_THAT(block->effective_size(), Eq(EffectiveSizeFor(8192)));
  ASSERT_THAT(block->begin(), Eq(block->AtOffset(0)));
  ASSERT_THAT(block->end(), Eq(block->AtOffset(block->effective_size())));

  while (block) {
    size_t size = block->allocated_size();
    StringBlock* next = block->next();
    EXPECT_THAT(StringBlock::Delete(block), Eq(AllocatedSizeFor(size)));
    block = next;
  }
}

TEST(StringBlockTest, EmplaceMultipleBlocks) {
  std::vector<std::unique_ptr<char[]>> buffers;

  // Convenience lambda to allocate a buffer and invoke Emplace on it.
  auto EmplaceBlock = [&](StringBlock* previous) {
    size_t size = StringBlock::NextSize(previous);
    buffers.push_back(std::make_unique<char[]>(size));
    return StringBlock::Emplace(buffers.back().get(), size, previous);
  };

  // Note: first two blocks are 256
  StringBlock* previous = EmplaceBlock(nullptr);

  for (int size = 256; size <= 8192; size *= 2) {
    StringBlock* block = EmplaceBlock(previous);
    ASSERT_THAT(block, Ne(nullptr));
    ASSERT_THAT(block->next(), Eq(previous));
    ASSERT_FALSE(block->heap_allocated());
    ASSERT_THAT(block->allocated_size(), Eq(AllocatedSizeFor(size)));
    ASSERT_THAT(block->effective_size(), Eq(EffectiveSizeFor(size)));
    ASSERT_THAT(block->begin(), Eq(block->AtOffset(0)));
    ASSERT_THAT(block->end(), Eq(block->AtOffset(block->effective_size())));
    previous = block;
  }

  // Capped at 8K from here on
  StringBlock* block = EmplaceBlock(previous);
  ASSERT_THAT(block, Ne(nullptr));
  EXPECT_THAT(block->next(), Eq(previous));
  ASSERT_FALSE(block->heap_allocated());
  ASSERT_THAT(block->allocated_size(), Eq(AllocatedSizeFor(8192)));
  ASSERT_THAT(block->effective_size(), Eq(EffectiveSizeFor(8192)));
  ASSERT_THAT(block->begin(), Eq(block->AtOffset(0)));
  ASSERT_THAT(block->end(), Eq(block->AtOffset(block->effective_size())));

  while (block) {
    StringBlock* next = block->next();
    EXPECT_THAT(StringBlock::Delete(block), Eq(0));
    block = next;
  }
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
