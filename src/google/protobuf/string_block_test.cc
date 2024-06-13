// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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
