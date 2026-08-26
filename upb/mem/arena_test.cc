// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mem/arena.h"

#include <stddef.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/thread_annotations.h"
#include "absl/cleanup/cleanup.h"
#include "absl/container/flat_hash_map.h"
#include "absl/random/distributions.h"
#include "absl/random/random.h"
#include "absl/synchronization/mutex.h"
#include "absl/synchronization/notification.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.hpp"
#include "upb/port/sanitizers.h"

// Must be last.
#include "upb/port/def.inc"

namespace {

struct CustomAlloc {
  upb_alloc alloc;
  int counter;
  bool ran_cleanup;
};

void* CustomAllocFunc(upb_alloc* alloc, void* ptr, size_t oldsize, size_t size,
                      size_t* actual_size) {
  CustomAlloc* custom_alloc = reinterpret_cast<CustomAlloc*>(alloc);
  if (size == 0) {
    custom_alloc->counter--;
  } else {
    custom_alloc->counter++;
  }
  return upb_alloc_global.func(alloc, ptr, oldsize, size, actual_size);
}

void CustomAllocCleanup(upb_alloc* alloc) {
  CustomAlloc* custom_alloc = reinterpret_cast<CustomAlloc*>(alloc);
  EXPECT_THAT(custom_alloc->counter, 0);
  custom_alloc->ran_cleanup = true;
}

TEST(ArenaTest, ArenaWithAllocCleanup) {
  CustomAlloc alloc = {{&CustomAllocFunc}, 0, false};
  upb_Arena* arena =
      upb_Arena_Init(nullptr, 0, reinterpret_cast<upb_alloc*>(&alloc));
  EXPECT_EQ(alloc.counter, 1);
  upb_Arena_SetAllocCleanup(arena, CustomAllocCleanup);
  upb_Arena_Free(arena);
  EXPECT_TRUE(alloc.ran_cleanup);
}

struct Size {
  size_t requested;
  size_t allocated;
};

struct SizeTracker {
  upb_alloc alloc;
  upb_alloc* delegate_alloc;
  absl::flat_hash_map<void*, Size>* sizes;
};

static_assert(std::is_standard_layout<SizeTracker>());

static void* size_checking_allocfunc(upb_alloc* alloc, void* ptr,
                                     size_t oldsize, size_t size,
                                     size_t* actual_size) {
  SizeTracker* size_alloc = reinterpret_cast<SizeTracker*>(alloc);
  size_t actual_size_tmp = 0;
  if (actual_size == nullptr) {
    actual_size = &actual_size_tmp;
  }
  void* result =
      size_alloc->delegate_alloc->func(alloc, ptr, oldsize, size, actual_size);
  if (ptr != nullptr) {
    Size& size_ref = size_alloc->sizes->at(ptr);
    UPB_ASSERT(size_ref.requested == oldsize || size_ref.allocated == oldsize);
    size_alloc->sizes->erase(ptr);
  }
  if (result != nullptr) {
    size_alloc->sizes->emplace(result, Size{size, UPB_MAX(size, *actual_size)});
  }
  return result;
}

TEST(ArenaTest, ShinkLastAfterReallocHwasanRegression) {
  upb_Arena_SetMaxBlockSize(UPB_MALLOC_ALIGN);
  absl::Cleanup reset_max_block_size = [] {
    upb_Arena_SetMaxBlockSize(UPB_PRIVATE(kUpbDefaultMaxBlockSize));
  };

  upb_Arena* arena = upb_Arena_Init(nullptr, 1000, &upb_alloc_global);
  EXPECT_NE(upb_Arena_Malloc(arena, 1), nullptr);
  // Will force a full-size block since the initial allocated block has tons of
  // free space and the max block size is tiny
  void* to_realloc = upb_Arena_Malloc(arena, 2000);
  // Realloc will retag to invalidate to_realloc
  void* to_shrink = upb_Arena_Realloc(arena, to_realloc, 2000, 2000);
#if UPB_HWASAN
  EXPECT_NE(to_realloc, to_shrink);
#endif
  upb_Arena_ShrinkLast(arena, to_shrink, 2000, 1);
  upb_Arena_Free(arena);
}

TEST(ArenaTest, SizedFree) {
  absl::flat_hash_map<void*, Size> sizes;
  SizeTracker alloc;
  alloc.alloc.func = size_checking_allocfunc;
  alloc.delegate_alloc = &upb_alloc_global;
  alloc.sizes = &sizes;

  char initial_block[1000];

  upb_Arena* arena = upb_Arena_Init(initial_block, 1000, &alloc.alloc);
  (void)upb_Arena_Malloc(arena, 500);
  void* to_resize = upb_Arena_Malloc(arena, 2000);
  void* resized = upb_Arena_Realloc(arena, to_resize, 2000, 4000);
  upb_Arena_ShrinkLast(arena, resized, 4000, 1);
  EXPECT_GT(sizes.size(), 0);
  upb_Arena_Free(arena);
  EXPECT_EQ(sizes.size(), 0);
}

TEST(ArenaTest, TryExtend) {
  upb_Arena* arena = upb_Arena_Init(nullptr, 1024, &upb_alloc_global);
  void* alloc = upb_Arena_Malloc(arena, 512);
  ASSERT_TRUE(upb_Arena_TryExtend(arena, alloc, 512, 700));
  ASSERT_TRUE(upb_Arena_TryExtend(arena, alloc, 700, 750));
  // If no room in block, should return false
  ASSERT_FALSE(upb_Arena_TryExtend(arena, alloc, 750, 10000));
  (void)upb_Arena_Malloc(arena, 1);
  // Can't extend past a previous alloc
  ASSERT_FALSE(upb_Arena_TryExtend(arena, alloc, 750, 900));
  upb_Arena_Free(arena);
}

TEST(ArenaTest, ReallocFastPath) {
  upb_Arena* arena = upb_Arena_Init(nullptr, 1024, &upb_alloc_global);
  void* initial = upb_Arena_Malloc(arena, 512);
  uintptr_t initial_allocated = upb_Arena_SpaceAllocated(arena, nullptr);

  void* extend = upb_Arena_Realloc(arena, initial, 512, 1024);
  EXPECT_EQ(initial_allocated, upb_Arena_SpaceAllocated(arena, nullptr));
#if UPB_HWASAN
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(initial, extend));
  EXPECT_NE(initial, extend);
#else
  EXPECT_EQ(initial, extend);
#endif

  void* shrunk = upb_Arena_Realloc(arena, extend, 1024, 512);
  EXPECT_EQ(initial_allocated, upb_Arena_SpaceAllocated(arena, nullptr));
#if UPB_HWASAN
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(initial, shrunk));
  EXPECT_NE(initial, shrunk);
  EXPECT_NE(extend, shrunk);
#else
  EXPECT_EQ(initial, shrunk);
#endif

  EXPECT_NE(nullptr, upb_Arena_Malloc(arena, 256));
  // Should have allocated into shrunk space
  EXPECT_EQ(initial_allocated, upb_Arena_SpaceAllocated(arena, nullptr));

  upb_Arena_Free(arena);
}

TEST(ArenaTest, SizeHint) {
  absl::flat_hash_map<void*, Size> sizes;
  SizeTracker alloc;
  alloc.alloc.func = size_checking_allocfunc;
  alloc.delegate_alloc = &upb_alloc_global;
  alloc.sizes = &sizes;

  upb_Arena* arena = upb_Arena_Init(nullptr, 2459, &alloc.alloc);
  EXPECT_EQ(sizes.size(), 1);
  EXPECT_NE(upb_Arena_Malloc(arena, 2459), nullptr);
  EXPECT_EQ(sizes.size(), 1);
  EXPECT_NE(upb_Arena_Malloc(arena, 500), nullptr);
  EXPECT_EQ(sizes.size(), 2);
  upb_Arena_Free(arena);
  EXPECT_EQ(sizes.size(), 0);
}

class OverheadTest {
 public:
  OverheadTest(const OverheadTest&) = delete;
  OverheadTest& operator=(const OverheadTest&) = delete;

  explicit OverheadTest(size_t first = 0, size_t max_block_size = 0) {
    if (max_block_size) {
      upb_Arena_SetMaxBlockSize(max_block_size);
    }
    alloc_.alloc.func = size_checking_allocfunc;
    alloc_.delegate_alloc = &upb_alloc_global;
    alloc_.sizes = &sizes_;
    arena_ = upb_Arena_Init(nullptr, first, &alloc_.alloc);
    arena_alloced_ = 0;
    arena_alloc_count_ = 0;
  }

  void Alloc(size_t size) {
    EXPECT_NE(upb_Arena_Malloc(arena_, size), nullptr);
    arena_alloced_ += size;
    arena_alloc_count_++;
  }

  uintptr_t SpaceAllocated() {
    return upb_Arena_SpaceAllocated(arena_, nullptr);
  }

  double WastePct() {
    uintptr_t backing_alloced = upb_Arena_SpaceAllocated(arena_, nullptr);
    double waste = backing_alloced - arena_alloced_;
    return waste / backing_alloced;
  }

  double AmortizedAlloc() {
    return ((double)sizes_.size()) / arena_alloc_count_;
  }

  ~OverheadTest() {
    upb_Arena_Free(arena_);
    upb_Arena_SetMaxBlockSize(UPB_PRIVATE(kUpbDefaultMaxBlockSize));
  }
  upb_Arena* arena_;

 protected:
  absl::flat_hash_map<void*, Size> sizes_;
  SizeTracker alloc_;
  uintptr_t arena_alloced_;
  uintptr_t arena_alloc_count_;
};

TEST(OverheadTest, SingleMassiveBlockThenLittle) {
  OverheadTest test;
  // Little blocks
  for (int i = 0; i < 4; i++) {
    test.Alloc(32);
  }
  // Big block!
  test.Alloc(16000);
  for (int i = 0; i < 50; i++) {
    test.Alloc(64);
  }
  if (!UPB_ASAN) {
#ifdef __ANDROID__
    EXPECT_NEAR(test.WastePct(), 0.075, 0.025);
    EXPECT_NEAR(test.AmortizedAlloc(), 0.09, 0.025);
#else
    EXPECT_NEAR(test.WastePct(), 0.08, 0.125);
    EXPECT_NEAR(test.AmortizedAlloc(), 0.09, 0.025);
#endif
  }
}

TEST(OverheadTest, Overhead_AlternatingSmallLargeBlocks) {
  OverheadTest test(512, 4096);
  for (int i = 0; i < 100; i++) {
    test.Alloc(5000);
    test.Alloc(64);
  }
  if (!UPB_ASAN) {
    EXPECT_NEAR(test.WastePct(), 0.007, 0.0025);
    EXPECT_NEAR(test.AmortizedAlloc(), 0.52, 0.025);
  }
}

TEST(OverheadTest, PartialMaxBlocks) {
  OverheadTest test(512, 4096);
  for (int i = 0; i < 10; i++) {
    test.Alloc(2096 + i);
  }
  if (!UPB_ASAN) {
    EXPECT_NEAR(test.WastePct(), 0.16, 0.025);
    EXPECT_NEAR(test.AmortizedAlloc(), 1.1, 0.25);
  }
}

TEST(OverheadTest, SmallBlocksLargerThanInitial) {
  OverheadTest test;
  size_t initial_block_size = upb_Arena_SpaceAllocated(test.arena_, nullptr);
  for (int i = 0; i < 10; i++) {
    test.Alloc(initial_block_size * 2 + 1);
  }
  if (!UPB_ASAN && sizeof(void*) == 8) {
    EXPECT_NEAR(test.WastePct(), 0.37, 0.025);
    EXPECT_NEAR(test.AmortizedAlloc(), 0.5, 0.025);
  }
}

TEST(OverheadTest, SmallBlocksLargerThanInitial_many) {
  OverheadTest test;
  size_t initial_block_size = upb_Arena_SpaceAllocated(test.arena_, nullptr);
  for (int i = 0; i < 100; i++) {
    test.Alloc(initial_block_size * 2 + 1);
  }
  if (!UPB_ASAN && sizeof(upb_Xsan) == 0) {
#ifdef __ANDROID__
    EXPECT_NEAR(test.WastePct(), 0.09, 0.025);
    EXPECT_NEAR(test.AmortizedAlloc(), 0.12, 0.025);
#else
    EXPECT_NEAR(test.WastePct(), 0.12, 0.03);
    EXPECT_NEAR(test.AmortizedAlloc(), 0.08, 0.025);
#endif
  }
  for (int i = 0; i < 900; i++) {
    test.Alloc(initial_block_size * 2 + 1);
  }
  if (!UPB_ASAN) {
#ifdef __ANDROID__
    EXPECT_NEAR(test.WastePct(), 0.05, 0.03);
    EXPECT_NEAR(test.AmortizedAlloc(), 0.08, 0.025);
#else
    EXPECT_NEAR(test.WastePct(), 0.04, 0.025);
    EXPECT_NEAR(test.AmortizedAlloc(), 0.05, 0.025);
#endif
  }
}

TEST(OverheadTest, DefaultMaxBlockSize) {
  OverheadTest test;
  // Perform 600 1k allocations (600k total) and ensure that the amount of
  // memory allocated does not exceed 700k.
  for (int i = 0; i < 600; ++i) {
    test.Alloc(1024);
  }
  EXPECT_LE(test.SpaceAllocated(), 700 * 1024);
}

TEST(ArenaTest, ArenaFuse) {
  upb_Arena* arena1 = upb_Arena_New();
  upb_Arena* arena2 = upb_Arena_New();

  EXPECT_TRUE(upb_Arena_Fuse(arena1, arena2));

  upb_Arena_Free(arena1);
  upb_Arena_Free(arena2);
}

TEST(ArenaTest, FuseWithInitialBlock) {
  char buf1[1024];
  char buf2[1024];
  upb_Arena* arenas[] = {upb_Arena_Init(buf1, 1024, &upb_alloc_global),
                         upb_Arena_Init(buf2, 1024, &upb_alloc_global),
                         upb_Arena_Init(nullptr, 0, &upb_alloc_global)};
  int size = sizeof(arenas) / sizeof(arenas[0]);
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      if (i == j) {
        // Fuse to self is always allowed.
        EXPECT_TRUE(upb_Arena_Fuse(arenas[i], arenas[j]));
      } else {
        EXPECT_FALSE(upb_Arena_Fuse(arenas[i], arenas[j]));
      }
    }
  }

  for (int i = 0; i < size; ++i) upb_Arena_Free(arenas[i]);
}

TEST(ArenaTest, FixedInitialBlockNoAlloc) {
  char buf[1024];
  upb_Arena* arena = upb_Arena_Init(buf, sizeof(buf), nullptr);

  EXPECT_EQ(upb_Arena_Malloc(arena, 2048), nullptr);
  EXPECT_EQ(upb_Arena_Malloc(arena, 1024), nullptr);

  upb_Arena_Free(arena);
}

class Environment {
 public:
  void RandomNewFree(absl::BitGen& gen, size_t min_index = 0) {
    auto a = std::make_shared<const upb::Arena>();
    SwapRandomArena(gen, a, min_index);
  }

  void RandomIncRefCount(absl::BitGen& gen) {
    std::shared_ptr<const upb::Arena> a = RandomNonNullArena(gen);
    upb_Arena_IncRefFor(a->ptr(), nullptr);
    upb_Arena_DecRefFor(a->ptr(), nullptr);
  }

  void RandomFuse(absl::BitGen& gen) {
    std::shared_ptr<const upb::Arena> a = RandomNonNullArena(gen);
    std::shared_ptr<const upb::Arena> b = RandomNonNullArena(gen);
    EXPECT_TRUE(upb_Arena_Fuse(a->ptr(), b->ptr()));
  }

  void RandomRefArena(absl::BitGen& gen) {
    std::shared_ptr<const upb::Arena> a = RandomNonNullArena(gen);
    std::shared_ptr<const upb::Arena> b = RandomNonNullArena(gen);
    if (a->ptr() == b->ptr()) return;
    if (a->ptr() > b->ptr()) std::swap(a, b);
    EXPECT_TRUE(upb_Arena_RefArena(a->ptr(), b->ptr()));
  }

#ifndef NDEBUG
  void PartitionedHasRef(absl::BitGen& gen) {
    // Ensure refs like (0,2), (1,3), (2,4) ... (97,99).
    auto [a, b] = GetArenaPairWithOffset(gen, 2);
    (void)upb_Arena_HasRef(a->ptr(), b->ptr());
  }

  void PartitionedFuse(absl::BitGen& gen) {
    // Ensure partitions like (0,1), (2,3), (4,5) ... (98,99).
    auto [a, b] = GetArenaPairWithOffset(gen, 1);
    EXPECT_TRUE(upb_Arena_Fuse(a->ptr(), b->ptr()));
  }

  void PartitionedRefArena(absl::BitGen& gen) {
    // Ensure refs like (0,2), (1,3), (2,4) ... (97,99).
    auto [a, b] = GetArenaPairWithOffset(gen, 2);
    if (a->ptr() > b->ptr()) std::swap(a, b);
    EXPECT_TRUE(upb_Arena_RefArena(a->ptr(), b->ptr()));
  }
#endif

  void RandomPoke(absl::BitGen& gen, size_t min_index = 0) {
    switch (absl::Uniform(gen, 0, 2)) {
      case 0:
        RandomNewFree(gen, min_index);
        break;
      case 1:
        RandomFuse(gen);
        break;
      default:
        break;
    }
  }

  std::shared_ptr<const upb::Arena> IndexedNonNullArena(size_t index) {
    absl::MutexLock lock(&mutex_);
    std::shared_ptr<const upb::Arena>& ret = arenas_[index];
    if (!ret) ret = std::make_shared<const upb::Arena>();
    return ret;
  }

 private:
  using ArenaArray = std::array<std::shared_ptr<const upb::Arena>, 100>;

  std::pair<std::shared_ptr<const upb::Arena>,
            std::shared_ptr<const upb::Arena>>
  GetArenaPairWithOffset(absl::BitGen& gen, size_t offset) {
    size_t index = RandomIndex(gen, 0, std::tuple_size<ArenaArray>::value - 1);
    size_t a_index = index % 2 == 0 ? index : index + 1;
    std::shared_ptr<const upb::Arena> a = IndexedNonNullArena(a_index);
    std::shared_ptr<const upb::Arena> b = IndexedNonNullArena(
        (a_index + offset) % std::tuple_size<ArenaArray>::value);
    return {a, b};
  }

  size_t RandomIndex(absl::BitGen& gen, size_t min_index = 0,
                     size_t max_index = std::tuple_size<ArenaArray>::value) {
    return absl::Uniform<size_t>(gen, min_index, max_index);
  }

  // Swaps a random arena from the set with the given arena.
  void SwapRandomArena(absl::BitGen& gen, std::shared_ptr<const upb::Arena>& a,
                       size_t min_index) {
    size_t i = RandomIndex(gen, min_index);
    absl::MutexLock lock(&mutex_);
    arenas_[i].swap(a);
  }

  // Returns a random arena from the set, ensuring that the returned arena is
  // non-null.
  //
  // Note that the returned arena is shared and may be accessed concurrently
  // by other threads.
  std::shared_ptr<const upb::Arena> RandomNonNullArena(absl::BitGen& gen) {
    return IndexedNonNullArena(RandomIndex(gen));
  }

  ArenaArray arenas_ ABSL_GUARDED_BY(mutex_);
  absl::Mutex mutex_;
};

TEST(ArenaTest, FuzzSingleThreaded) {
  Environment env;

  absl::BitGen gen;
  auto end = absl::Now() + absl::Seconds(0.5);
  while (absl::Now() < end) {
    env.RandomPoke(gen);
  }
}

TEST(ArenaTest, LargeAlloc) {
  // Tests an allocation larger than the max block size.
  upb_Arena* arena = upb_Arena_New();
  size_t size = 100000;
  char* mem = static_cast<char*>(upb_Arena_Malloc(arena, size));
  EXPECT_NE(mem, nullptr);
  for (size_t i = 0; i < size; ++i) {
    mem[i] = static_cast<char>(i);
  }
  for (size_t i = 0; i < size; ++i) {
    EXPECT_EQ(mem[i], static_cast<char>(i));
  }
  upb_Arena_Free(arena);
}

TEST(ArenaTest, MaxBlockSize) {
  upb_Arena* arena = upb_Arena_New();
  // Perform 600 1k allocations (600k total) and ensure that the amount of
  // memory allocated does not exceed 700k.
  for (int i = 0; i < 600; ++i) {
    EXPECT_NE(upb_Arena_Malloc(arena, 1024), nullptr);
  }
  EXPECT_LE(upb_Arena_SpaceAllocated(arena, nullptr), 700 * 1024);
  upb_Arena_Free(arena);
}

#ifndef UPB_SUPPRESS_MISSING_ATOMICS

TEST(ArenaTest, FuzzFuseFreeRace) {
  Environment env;

  absl::Notification done;
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&]() {
      absl::BitGen gen;
      while (!done.HasBeenNotified()) {
        env.RandomNewFree(gen);
      }
    });
  }

  absl::BitGen gen;
  auto end = absl::Now() + absl::Seconds(2);
  while (absl::Now() < end) {
    env.RandomFuse(gen);
  }
  done.Notify();
  for (auto& t : threads) t.join();
}

TEST(ArenaTest, FuzzFuseFuseRace) {
  Environment env;

  absl::Notification done;
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&]() {
      absl::BitGen gen;
      while (!done.HasBeenNotified()) {
        env.RandomFuse(gen);
      }
    });
  }

  absl::BitGen gen;
  auto end = absl::Now() + absl::Seconds(2);
  while (absl::Now() < end) {
    env.RandomFuse(gen);
  }
  done.Notify();
  for (auto& t : threads) t.join();
}

static void* checking_global_allocfunc(upb_alloc* alloc, void* ptr,
                                       size_t oldsize, size_t size,
                                       size_t* actual_size) {
  int header_size = std::max(alignof(max_align_t), sizeof(int));
  if (ptr) {
    ptr = UPB_PTR_AT(ptr, -header_size, void);
    UPB_ASSERT(*reinterpret_cast<int*>(ptr) == 0x5AFE);
  }
  if (size == 0) {
    free(ptr);
    return nullptr;
  }
  void* ret;
  if (oldsize == 0) {
    ret = malloc(size + header_size);
  } else {
    ret = realloc(ptr, size + header_size);
  }
  if (ret) {
    *reinterpret_cast<int*>(ret) = 0x5AFE;
    return UPB_PTR_AT(ret, header_size, void);
  }
  return ret;
}

TEST(ArenaTest, FuzzFuseFreeAllocatorRace) {
  upb_Arena_SetMaxBlockSize(128);
  upb_alloc_func* old = upb_alloc_global.func;
  upb_alloc_global.func = checking_global_allocfunc;
  absl::Cleanup reset_max_block_size = [old] {
    upb_Arena_SetMaxBlockSize(UPB_PRIVATE(kUpbDefaultMaxBlockSize));
    upb_alloc_global.func = old;
  };
  absl::Notification done;
  std::vector<std::thread> threads;
  size_t thread_count = 10;
  std::vector<std::array<upb_Arena*, 11>> arenas;
  for (size_t i = 0; i < 10000; ++i) {
    std::array<upb_Arena*, 11> arr;
    arr[0] = upb_Arena_New();
    for (size_t j = 1; j < thread_count + 1; ++j) {
      arr[j] = upb_Arena_New();
      EXPECT_TRUE(upb_Arena_Fuse(arr[j - 1], arr[j]));
    }
    arenas.push_back(arr);
  }
  for (size_t i = 0; i < thread_count; ++i) {
    size_t tid = i;
    threads.emplace_back([&, tid]() {
      size_t arenaCtr = 0;
      while (!done.HasBeenNotified() && arenaCtr < arenas.size()) {
        upb_Arena* read = arenas[arenaCtr++][tid];
        (void)upb_Arena_Malloc(read, 128);
        (void)upb_Arena_Malloc(read, 128);
        upb_Arena_Free(read);
      }
      while (arenaCtr < arenas.size()) {
        upb_Arena_Free(arenas[arenaCtr++][tid]);
      }
    });
  }
  auto end = absl::Now() + absl::Seconds(2);
  size_t arenaCtr = 0;
  while (absl::Now() < end && arenaCtr < arenas.size()) {
    upb_Arena* read = arenas[arenaCtr++][thread_count];
    (void)upb_Arena_Malloc(read, 128);
    (void)upb_Arena_Malloc(read, 128);
    upb_Arena_Free(read);
  }
  done.Notify();
  while (arenaCtr < arenas.size()) {
    upb_Arena_Free(arenas[arenaCtr++][thread_count]);
  }
  for (auto& t : threads) t.join();
}

TEST(ArenaTest, FuzzFuseSpaceAllocatedRace) {
  upb_Arena_SetMaxBlockSize(128);
  absl::Cleanup reset_max_block_size = [] {
    upb_Arena_SetMaxBlockSize(UPB_PRIVATE(kUpbDefaultMaxBlockSize));
  };
  absl::Notification done;
  std::vector<std::thread> threads;
  std::vector<upb_Arena*> arenas;
  size_t thread_count = 10;
  size_t fuses_per_thread = 1000;
  size_t root_arenas_limit = 250;
  for (size_t i = 0; i < root_arenas_limit; ++i) {
    arenas.push_back(upb_Arena_New());
    for (size_t j = 0; j < thread_count; ++j) {
      upb_Arena_IncRefFor(arenas[i], nullptr);
    }
  }
  for (size_t i = 0; i < thread_count; ++i) {
    threads.emplace_back([&]() {
      size_t arenaCtr = 0;
      while (!done.HasBeenNotified() && arenaCtr < arenas.size()) {
        upb_Arena* read = arenas[arenaCtr++];
        for (size_t j = 0; j < fuses_per_thread; ++j) {
          upb_Arena* fuse = upb_Arena_New();
          EXPECT_TRUE(upb_Arena_Fuse(read, fuse));
          upb_Arena_Free(read);
          read = fuse;
        }
        upb_Arena_Free(read);
      }
      while (arenaCtr < arenas.size()) {
        upb_Arena_Free(arenas[arenaCtr++]);
      }
    });
  }

  auto end = absl::Now() + absl::Seconds(2);
  size_t arenaCtr = 0;
  uintptr_t total_allocated = 0;
  while (absl::Now() < end && arenaCtr < arenas.size()) {
    upb_Arena* read = arenas[arenaCtr++];
    size_t count;
    size_t allocated;
    do {
      allocated = upb_Arena_SpaceAllocated(read, &count);
    } while (count < fuses_per_thread * thread_count);
    upb_Arena_Free(read);
    total_allocated += allocated;
  }
  done.Notify();
  for (auto& t : threads) t.join();
  while (arenaCtr < arenas.size()) {
    upb_Arena_Free(arenas[arenaCtr++]);
  }
  ASSERT_GT(total_allocated, arenaCtr);
}

TEST(ArenaTest, FuzzAllocSpaceAllocatedRace) {
  upb_Arena_SetMaxBlockSize(128);
  absl::Cleanup reset_max_block_size = [] {
    upb_Arena_SetMaxBlockSize(UPB_PRIVATE(kUpbDefaultMaxBlockSize));
  };
  upb_Arena* arena = upb_Arena_New();
  absl::Notification done;
  std::vector<std::thread> threads;
  for (int i = 0; i < 1; ++i) {
    threads.emplace_back([&]() {
      while (!done.HasBeenNotified()) {
        size_t count;
        upb_Arena_SpaceAllocated(arena, &count);
      }
    });
  }

  auto end = absl::Now() + absl::Seconds(2);
  uintptr_t total = 0;
  while (absl::Now() < end && total < 10000000) {
    if (upb_Arena_Malloc(arena, 128) == nullptr) {
      break;
    }
    total += 128;
  }
  done.Notify();
  for (auto& t : threads) t.join();
  upb_Arena_Free(arena);
}

TEST(ArenaTest, ArenaIncRef) {
  upb_Arena* arena1 = upb_Arena_New();
  EXPECT_EQ(upb_Arena_DebugRefCount(arena1), 1);
  upb_Arena_IncRefFor(arena1, nullptr);
  EXPECT_EQ(upb_Arena_DebugRefCount(arena1), 2);
  upb_Arena_DecRefFor(arena1, nullptr);
  EXPECT_EQ(upb_Arena_DebugRefCount(arena1), 1);
  upb_Arena_Free(arena1);
}

TEST(ArenaTest, FuzzFuseIncRefCountRace) {
  Environment env;

  absl::Notification done;
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&]() {
      absl::BitGen gen;
      while (!done.HasBeenNotified()) {
        env.RandomNewFree(gen);
      }
    });
  }

  absl::BitGen gen;
  auto end = absl::Now() + absl::Seconds(2);
  while (absl::Now() < end) {
    env.RandomFuse(gen);
    env.RandomIncRefCount(gen);
  }
  done.Notify();
  for (auto& t : threads) t.join();
}

TEST(ArenaTest, IncRefCountShouldFailForInitialBlock) {
  char buf1[1024];
  upb_Arena* arena = upb_Arena_Init(buf1, 1024, &upb_alloc_global);
  EXPECT_FALSE(upb_Arena_IncRefFor(arena, nullptr));
}

TEST(ArenaTest, FuzzFuseIsFusedRace) {
  Environment env;

  // Create two arenas and fuse them.
  std::shared_ptr<const upb::Arena> a = env.IndexedNonNullArena(0);
  std::shared_ptr<const upb::Arena> b = env.IndexedNonNullArena(1);
  EXPECT_TRUE(upb_Arena_Fuse(a->ptr(), b->ptr()));
  EXPECT_TRUE(upb_Arena_IsFused(a->ptr(), b->ptr()));

  absl::Notification done;
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&]() {
      absl::BitGen gen;
      while (!done.HasBeenNotified()) {
        env.RandomPoke(gen, 2);
      }
    });
  }

  absl::BitGen gen;
  auto end = absl::Now() + absl::Seconds(2);
  while (absl::Now() < end) {
    // Verify that the two arenas are still fused.
    EXPECT_TRUE(upb_Arena_IsFused(a->ptr(), b->ptr()));
  }
  done.Notify();
  for (auto& t : threads) t.join();
}

TEST(ArenaTest, FuzzRefArenaRace) {
  Environment env;

  absl::Notification done;
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&]() {
      absl::BitGen gen;
      while (!done.HasBeenNotified()) {
        env.RandomNewFree(gen);
      }
    });
  }

  absl::BitGen gen;
  auto end = absl::Now() + absl::Seconds(2);
  while (absl::Now() < end) {
    env.RandomRefArena(gen);
  }
  done.Notify();
  for (auto& t : threads) t.join();
}

#ifndef NDEBUG

TEST(ArenaTest, FuzzFuseRefArenaRace) {
  Environment env;

  absl::Notification done;
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&]() {
      absl::BitGen gen;
      while (!done.HasBeenNotified()) {
        env.PartitionedFuse(gen);
      }
    });
  }
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&]() {
      absl::BitGen gen;
      while (!done.HasBeenNotified()) {
        env.PartitionedHasRef(gen);
      }
    });
  }

  absl::BitGen gen;
  auto end = absl::Now() + absl::Seconds(2);
  while (absl::Now() < end) {
    env.PartitionedRefArena(gen);
    env.PartitionedHasRef(gen);
  }
  done.Notify();
  for (auto& t : threads) t.join();
}

TEST(ArenaTest, ArenaRef) {
  upb_Arena* arena1 = upb_Arena_New();
  upb_Arena* arena2 = upb_Arena_New();

  EXPECT_TRUE(upb_Arena_RefArena(arena1, arena2));
  EXPECT_TRUE(upb_Arena_HasRef(arena1, arena2));
  EXPECT_FALSE(upb_Arena_HasRef(arena2, arena1));

  upb_Arena_Free(arena1);
  upb_Arena_Free(arena2);
}
#endif

TEST(ArenaTest, ArenaRefPreventsFree) {
  upb_Arena* arena1 = upb_Arena_New();
  upb_Arena* arena2 = upb_Arena_New();

  // arena2 has refcount 1.
  EXPECT_EQ(upb_Arena_DebugRefCount(arena2), 1);

  // arena1 now owns a ref to arena2. arena2 has refcount 2.
  EXPECT_TRUE(upb_Arena_RefArena(arena1, arena2));
  EXPECT_EQ(upb_Arena_DebugRefCount(arena2), 2);

  // User of arena2 frees it. Refcount goes to 1. Arena is not freed.
  upb_Arena_Free(arena2);
  EXPECT_EQ(upb_Arena_DebugRefCount(arena2), 1);

  // We can still allocate on arena2.
  EXPECT_NE(nullptr, upb_Arena_Malloc(arena2, 1));

  // When arena1 is freed, it releases its ref on arena2, which is then freed.
  upb_Arena_Free(arena1);
}

TEST(ArenaTest, ArenaOwnerFreedFirst) {
  upb_Arena* arena1 = upb_Arena_New();
  upb_Arena* arena2 = upb_Arena_New();

  // arena2 has refcount 1.
  EXPECT_EQ(upb_Arena_DebugRefCount(arena2), 1);

  // arena1 now owns a ref to arena2. arena2 has refcount 2.
  EXPECT_TRUE(upb_Arena_RefArena(arena1, arena2));
  EXPECT_EQ(upb_Arena_DebugRefCount(arena2), 2);

  // Freeing the owner releases its ref on arena2. Refcount goes to 1.
  upb_Arena_Free(arena1);
  EXPECT_EQ(upb_Arena_DebugRefCount(arena2), 1);

  // Now when we free arena2, it is actually freed.
  upb_Arena_Free(arena2);
}

#ifndef UPB_ENABLE_REF_CYCLE_CHECKS

TEST(ArenaDeathTest, ArenaRefCycle) {
  ASSERT_DEATH(
      {
        upb_Arena* arena1 = upb_Arena_New();
        upb_Arena* arena2 = upb_Arena_New();
        upb_Arena_RefArena(arena1, arena2);
        upb_Arena_RefArena(arena2, arena1);
        upb_Arena_Free(arena1);
        upb_Arena_Free(arena2);
      },
      "");
}

TEST(ArenaDeathTest, ArenaRefCycleThroughFuse) {
  ASSERT_DEATH(
      {
        upb_Arena* arena1 = upb_Arena_New();
        upb_Arena* arena2 = upb_Arena_New();
        upb_Arena* arena3 = upb_Arena_New();
        upb_Arena_RefArena(arena1, arena2);
        upb_Arena_Fuse(arena2, arena3);
        upb_Arena_RefArena(arena3, arena1);
        upb_Arena_Free(arena1);
        upb_Arena_Free(arena2);
        upb_Arena_Free(arena3);
      },
      "");
}

TEST(ArenaDeathTest, ArenaRefCycleThroughMultipleFuses) {
  ASSERT_DEATH(
      {
        upb_Arena* arena1 = upb_Arena_New();
        upb_Arena* arena2 = upb_Arena_New();
        upb_Arena* arena3 = upb_Arena_New();
        upb_Arena* arena4 = upb_Arena_New();
        upb_Arena* arena5 = upb_Arena_New();
        upb_Arena_RefArena(arena1, arena2);  // a -> b
        upb_Arena_Fuse(arena2, arena3);      // b + c
        upb_Arena_RefArena(arena3, arena4);  // c -> d
        upb_Arena_Fuse(arena4, arena5);      // d + e
        upb_Arena_RefArena(arena5, arena1);  // e -> a (cycle)
        upb_Arena_Free(arena1);
        upb_Arena_Free(arena2);
        upb_Arena_Free(arena3);
        upb_Arena_Free(arena4);
        upb_Arena_Free(arena5);
      },
      "");
}

TEST(ArenaDeathTest, ArenaRefFuseCycle) {
  ASSERT_DEATH(
      {
        upb::Arena a;
        upb::Arena b;
        upb::Arena c;
        c.RefArena(a);

        absl::Notification t1_started;
        absl::Notification t2_started;
        absl::Notification t1_finished;
        absl::Notification t2_finished;

        std::thread thread1([&]() {
          t1_started.Notify();
          t2_started.WaitForNotification();
          a.RefArena(b);
          t1_finished.Notify();
        });

        std::thread thread2([&]() {
          t2_started.Notify();
          t1_started.WaitForNotification();
          b.Fuse(c);
          t2_finished.Notify();
        });

        thread1.join();
        thread2.join();
      },
      "");
}

#endif  // DEBUG

#endif  // UPB_SUPPRESS_MISSING_ATOMICS

TEST(ArenaTest, AllocationCountFailureInjection) {
  if (!upb_AllocationCount_IsAvailable()) {
    return;
  }
  // Try normal scenario
  upb_AllocationCount_Reset();
  upb_Arena* arena = upb_Arena_New();
  EXPECT_NE(arena, nullptr);
  // Allocate some blocks
  for (int i = 0; i < 10; ++i) {
    void* p = upb_Arena_Malloc(arena, 500);
    EXPECT_NE(p, nullptr);
  }
  size_t total = upb_AllocationCount_Get();
  EXPECT_GT(total, 0);
  upb_Arena_Free(arena);

  // Now verify failure after i allocations
  for (size_t i = 0; i < total; ++i) {
    upb_AllocationCount_Reset();
    upb_AllocationCount_FailOn(i);
    // The i-th arena-level initial or block allocation should fail.
    upb_Arena* fail_arena = upb_Arena_New();
    if (fail_arena != nullptr) {
      bool failed = false;
      for (int j = 0; j < 10; ++j) {
        void* p = upb_Arena_Malloc(fail_arena, 500);
        if (p == nullptr) {
          failed = true;
          break;
        }
      }
      upb_Arena_Free(fail_arena);
      EXPECT_TRUE(failed);
    }
  }
  upb_AllocationCount_Reset();
}

TEST(ArenaTest, PoolAlloc) {
  upb_Arena* arena = upb_Arena_New();

  // 1. Test basic reuse (power of 2)
  void* ptr1 = upb_Arena_AllocPool(arena, 32);
  EXPECT_NE(ptr1, nullptr);

  // Free it back to the pool
  upb_Arena_FreePool(arena, ptr1, 32);

  // Allocate again, it should return the SAME block
  void* ptr2 = upb_Arena_AllocPool(arena, 32);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(ptr1, ptr2));

  // 2. Test multiple size classes (power of 2)
  void* ptr_small = upb_Arena_AllocPool(arena, 32);
  void* ptr_large = upb_Arena_AllocPool(arena, 64);
  EXPECT_NE(ptr_small, ptr_large);

  upb_Arena_FreePool(arena, ptr_small, 32);
  upb_Arena_FreePool(arena, ptr_large, 64);

  // Allocate 64 first, should get ptr_large
  void* ptr_large2 = upb_Arena_AllocPool(arena, 64);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(ptr_large2, ptr_large));

  // Allocate 32, should get ptr_small
  void* ptr_small2 = upb_Arena_AllocPool(arena, 32);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(ptr_small2, ptr_small));

  // 3. Test stack behavior (LIFO)
  void* a1 = upb_Arena_AllocPool(arena, 32);
  void* a2 = upb_Arena_AllocPool(arena, 32);
  EXPECT_NE(a1, a2);

  upb_Arena_FreePool(arena, a1, 32);
  upb_Arena_FreePool(arena, a2, 32);

  // Since it's a stack, we should get a2 first, then a1
  void* r1 = upb_Arena_AllocPool(arena, 32);
  void* r2 = upb_Arena_AllocPool(arena, 32);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(r1, a2));
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(r2, a1));

  // 4. Test other power-of-2 size classes (e.g. 16, 128, 256)
  void* p16 = upb_Arena_AllocPool(arena, 16);
  EXPECT_NE(p16, nullptr);
  upb_Arena_FreePool(arena, p16, 16);

  void* p16_2 = upb_Arena_AllocPool(arena, 16);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(p16, p16_2));

  void* p128 = upb_Arena_AllocPool(arena, 128);
  EXPECT_NE(p16_2, p128);
  upb_Arena_FreePool(arena, p16_2, 16);
  upb_Arena_FreePool(arena, p128, 128);

  upb_Arena_Free(arena);
}

TEST(ArenaTest, PoolAllocExactMatch) {
  upb_Arena* arena = upb_Arena_New();

  // 1. Allocate a 64-byte block and free it to the pool
  void* ptr_large = upb_Arena_AllocPool(arena, 64);
  EXPECT_NE(ptr_large, nullptr);
  upb_Arena_FreePool(arena, ptr_large, 64);

  // 2. Allocate 32 bytes. With O(1) exact matching, this does NOT steal or
  // destroy the 64-byte host block! It allocates from arena malloc.
  void* ptr_small = upb_Arena_AllocPool(arena, 32);
  EXPECT_FALSE(UPB_PRIVATE(upb_Xsan_PtrEq)(ptr_large, ptr_small));

  // 3. Free the 32-byte block into the pool
  upb_Arena_FreePool(arena, ptr_small, 32);

  // 4. Allocate 32 bytes again: it should reuse the 32-byte block!
  void* ptr_small2 = upb_Arena_AllocPool(arena, 32);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(ptr_small, ptr_small2));

  // 5. Allocate 64 bytes again: it should reuse the preserved 64-byte block!
  void* ptr_large2 = upb_Arena_AllocPool(arena, 64);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(ptr_large, ptr_large2));

  upb_Arena_Free(arena);
}

TEST(ArenaTest, PoolHarvestRetiredBlock) {
  upb_Arena* arena = upb_Arena_New();

  // 1. Allocate 200 bytes. This fits in the initial block (size approx 384,
  // leaving approx 256 bytes of free space).
  // After this, the remaining free space in the first block is approx 56 bytes.
  void* p1 = upb_Arena_Malloc(arena, 200);
  EXPECT_NE(p1, nullptr);

  // 2. Allocate 100 bytes. This does NOT fit in the remaining 56 bytes.
  // Since 100 is small, it will allocate a new standard block (size 512)
  // and use it as the active block (retiring the first block).
  // It will NOT trigger a 'one-off' allocation because the new block's
  // free space (512 - 100 = 412) is much larger than the current free space
  // (56). The remaining 56 bytes in the first block will be harvested! The
  // largest power-of-2 size class <= 56 is 32.
  void* p2 = upb_Arena_Malloc(arena, 100);
  EXPECT_NE(p2, nullptr);

  // 3. Try to allocate the harvested block. Since we don't know the exact
  // size class harvested (due to compiler/reserve padding differences), we
  // search by trying to allocate various valid size classes.
  // The harvested block MUST start exactly contiguous with p1!
  uintptr_t p1_end = (uintptr_t)p1 + UPB_PRIVATE(_upb_Arena_AllocSpan)(200);

  void* p3 = nullptr;
  size_t sizes[] = {32, 16};
  for (size_t size : sizes) {
    void* ptr = upb_Arena_AllocPool(arena, size);
    if (UPB_PRIVATE(upb_Xsan_PtrEq)(ptr, (void*)p1_end)) {
      p3 = ptr;
      break;
    }
  }
  EXPECT_NE(p3, nullptr);

  upb_Arena_Free(arena);
}

TEST(OverheadTest, BlockHarvestingSavings) {
  // Set max block size to 4096 to make the test deterministic.
  upb_Arena_SetMaxBlockSize(4096);

  upb_Arena* arena = upb_Arena_New();

  // 1. Allocate 3296 bytes. This forces allocation of Block 2 (4096 bytes).
  // Block 2 has exactly 800 bytes left (current_free = 800).
  void* p1 = upb_Arena_Malloc(arena, 3296);
  EXPECT_NE(p1, nullptr);

  // 2. Allocate 1000 bytes. This does NOT fit in Block 2 (800 left).
  // It allocates Block 3 (4096 bytes).
  // Since span (1000) <= 4096 and current_free (800) < future_free (4096 - 1000
  // = 3096), it does NOT trigger 'one-off' allocation. Block 2 is retired and
  // its remaining 800 bytes are harvested (largest power-of-2 <= 800 is 512).
  // Block 3 is active and has 3096 bytes left.
  void* p2 = upb_Arena_Malloc(arena, 1000);
  EXPECT_NE(p2, nullptr);

  // 3. Allocate 3050 bytes. This fits in Block 3 (3096 left), leaving only
  // 3096 - 3056 (aligned 3050) = 40 bytes left in Block 3.
  void* p3 = upb_Arena_Malloc(arena, 3050);
  EXPECT_NE(p3, nullptr);

  // 4. Allocate 512, 256, and 32 bytes using AllocPool.
  // With iterative harvesting, 800 bytes was decomposed into 512 + 256 + 32
  // bytes. All three should HIT the harvested blocks from Block 2. No new
  // blocks are allocated! Total blocks: 3 (B1 + B2 + B3). Without harvesting,
  // these would MISS, call raw malloc, fail to fit in Block 3 (40 left), and
  // force allocation of additional 4096-byte blocks.
  void* p4_1 = upb_Arena_AllocPool(arena, 512);
  EXPECT_NE(p4_1, nullptr);
  void* p4_2 = upb_Arena_AllocPool(arena, 256);
  EXPECT_NE(p4_2, nullptr);
  void* p4_3 = upb_Arena_AllocPool(arena, 32);
  EXPECT_NE(p4_3, nullptr);

  // 5. Assert that the total space allocated is indeed small (only 3 blocks: B1
  // + B2 + B3). B1 (approx 3416) + B2 (4096) + B3 (4096) = approx 11608 bytes.
  // If harvesting failed, it would be 4+ blocks (approx 15704+ bytes).
  size_t total_allocated = upb_Arena_SpaceAllocated(arena, nullptr);

  // We assert it is strictly less than 4 blocks (15000+ bytes).
  EXPECT_LE(total_allocated, 13000);

  upb_Arena_Free(arena);
  upb_Arena_SetMaxBlockSize(UPB_PRIVATE(kUpbDefaultMaxBlockSize));
}

TEST(OverheadTest, SlowMallocPoolReuse) {
  // Set max block size to 4096 to make the test deterministic.
  upb_Arena_SetMaxBlockSize(4096);

  upb_Arena* arena = upb_Arena_New();

  // 1. Allocate 3296 bytes. This forces allocation of Block 2 (4096 bytes).
  // Block 2 has exactly 800 bytes left.
  void* p1 = upb_Arena_Malloc(arena, 3296);
  EXPECT_NE(p1, nullptr);

  // 2. Allocate 1000 bytes. This does NOT fit in Block 2 (800 left).
  // It allocates Block 3 (4096 bytes), retiring Block 2.
  // Block 2's remaining 800 bytes are harvested as 512 bytes and placed in the
  // pool.
  void* p2 = upb_Arena_Malloc(arena, 1000);
  EXPECT_NE(p2, nullptr);

  // 3. Allocate 3050 bytes. This fits in Block 3, leaving only 40 bytes left.
  void* p3 = upb_Arena_Malloc(arena, 3050);
  EXPECT_NE(p3, nullptr);

  // 4. Allocate 500 bytes using raw Malloc (which triggers SlowMalloc).
  // With SlowMalloc pool reuse, this should HIT the 512-byte harvested block in
  // the pool! It will be returned as a one-off block. No new system blocks are
  // allocated! Total blocks: 3. Without pool reuse, this would force allocation
  // of Block 4.
  void* p4 = upb_Arena_Malloc(arena, 500);
  EXPECT_NE(p4, nullptr);

  // 5. Assert that the total space allocated is indeed small (only 3 blocks).
  size_t total_allocated = upb_Arena_SpaceAllocated(arena, nullptr);
  EXPECT_LE(total_allocated, 13000);

  upb_Arena_Free(arena);
  upb_Arena_SetMaxBlockSize(UPB_PRIVATE(kUpbDefaultMaxBlockSize));
}

TEST(ArenaTest, PoolHostBlockEvacuation) {
  upb_Arena* arena = upb_Arena_New();

  // 1. Allocate and free blocks in ascending order: 16, 32, 64, 128, 256
  // The 256-byte block becomes the host block.
  void* p16 = upb_Arena_AllocPool(arena, 16);
  void* p32 = upb_Arena_AllocPool(arena, 32);
  void* p64 = upb_Arena_AllocPool(arena, 64);
  void* p128 = upb_Arena_AllocPool(arena, 128);
  void* p256 = upb_Arena_AllocPool(arena, 256);

  upb_Arena_FreePool(arena, p16, 16);
  upb_Arena_FreePool(arena, p32, 32);
  upb_Arena_FreePool(arena, p64, 64);
  upb_Arena_FreePool(arena, p128, 128);
  upb_Arena_FreePool(arena, p256, 256);

  // 2. Popping 256 pops the host block itself!
  // Backward scan must evacuate the index to the 128-byte block.
  void* r256 = upb_Arena_AllocPool(arena, 256);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(r256, p256));

  // 3. Popping 128 pops the new host block!
  // Evacuates index to the 64-byte block.
  void* r128 = upb_Arena_AllocPool(arena, 128);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(r128, p128));

  // 4. Pop remaining blocks: 64, 32, 16
  void* r64 = upb_Arena_AllocPool(arena, 64);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(r64, p64));

  void* r32 = upb_Arena_AllocPool(arena, 32);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(r32, p32));

  void* r16 = upb_Arena_AllocPool(arena, 16);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(r16, p16));

  // 5. Pool is now empty, next allocation allocates a new block
  void* r_new = upb_Arena_AllocPool(arena, 16);
  EXPECT_NE(r_new, nullptr);
  EXPECT_FALSE(UPB_PRIVATE(upb_Xsan_PtrEq)(r_new, p16));

  upb_Arena_Free(arena);
}

TEST(ArenaTest, PoolMultipleBlocksSameBinLIFO) {
  upb_Arena* arena = upb_Arena_New();

  const int kNumBlocks = 10;
  void* blocks[kNumBlocks];

  // Allocate 10 blocks of 64 bytes
  for (int i = 0; i < kNumBlocks; ++i) {
    blocks[i] = upb_Arena_AllocPool(arena, 64);
    EXPECT_NE(blocks[i], nullptr);
  }

  // Free them in order 0, 1, ..., 9
  for (int i = 0; i < kNumBlocks; ++i) {
    upb_Arena_FreePool(arena, blocks[i], 64);
  }

  // Pop them back: must come out in exact LIFO reverse order 9, 8, ..., 0
  for (int i = kNumBlocks - 1; i >= 0; --i) {
    void* p = upb_Arena_AllocPool(arena, 64);
    EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(p, blocks[i]));
  }

  upb_Arena_Free(arena);
}

TEST(ArenaTest, PoolMultipleHostSizedBlocks) {
  upb_Arena* arena = upb_Arena_New();

  void* h1 = upb_Arena_AllocPool(arena, 512);
  void* h2 = upb_Arena_AllocPool(arena, 512);
  void* h3 = upb_Arena_AllocPool(arena, 512);

  // Free h1 (becomes host), h2 (enters bins[5]), h3 (enters bins[5])
  upb_Arena_FreePool(arena, h1, 512);
  upb_Arena_FreePool(arena, h2, 512);
  upb_Arena_FreePool(arena, h3, 512);

  // Pop: h3 comes first, then h2, then host block h1 itself
  void* r3 = upb_Arena_AllocPool(arena, 512);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(r3, h3));

  void* r2 = upb_Arena_AllocPool(arena, 512);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(r2, h2));

  void* r1 = upb_Arena_AllocPool(arena, 512);
  EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(r1, h1));

  upb_Arena_Free(arena);
}

TEST(ArenaTest, PoolLargePowerOfTwoSizes) {
  upb_Arena* arena = upb_Arena_New();

  // Test unbounded large power of 2 sizes (e.g. 64KB, 128KB, 256KB)
  size_t large_sizes[] = {65536, 131072, 262144};
  void* ptrs[3];

  for (int i = 0; i < 3; ++i) {
    ptrs[i] = upb_Arena_AllocPool(arena, large_sizes[i]);
    EXPECT_NE(ptrs[i], nullptr);
  }

  for (int i = 0; i < 3; ++i) {
    upb_Arena_FreePool(arena, ptrs[i], large_sizes[i]);
  }

  // Pop in reverse order
  for (int i = 2; i >= 0; --i) {
    void* p = upb_Arena_AllocPool(arena, large_sizes[i]);
    EXPECT_TRUE(UPB_PRIVATE(upb_Xsan_PtrEq)(p, ptrs[i]));
  }

  upb_Arena_Free(arena);
}

TEST(ArenaTest, PoolInvalidSizes) {
  upb_Arena* arena = upb_Arena_New();

  // Null pointer is safe no-op
  upb_Arena_FreePool(arena, nullptr, 32);

  // Non-power of 2 is ignored by pool
  void* ptr = upb_Arena_Malloc(arena, 48);
  upb_Arena_FreePool(arena, ptr, 48);

  // Too small size (< 16) is ignored
  void* p8 = upb_Arena_Malloc(arena, 8);
  upb_Arena_FreePool(arena, p8, 8);

  upb_Arena_Free(arena);
}

TEST(OverheadTest, PoolStressRepetitiveRealloc) {
  upb_Arena* arena = upb_Arena_New();

  // Simulate heavy churn: 1,000 cycles of simulated dynamic array resizing.
  // In each cycle:
  // Allocate buffer of size 32 -> resize to 64 (free 32) -> resize to 128 (free
  // 64) -> resize to 256 (free 128) -> resize to 512 (free 256) -> resize to
  // 1024 (free 512) -> finally free 1024. Without pooling, 1000 cycles * 2048
  // bytes = ~2 MB allocated. With pooling, all sizes (32, 64, 128, 256, 512,
  // 1024) are reused across all 1000 iterations!
  for (int cycle = 0; cycle < 1000; ++cycle) {
    void* p32 = upb_Arena_AllocPool(arena, 32);
    void* p64 = upb_Arena_AllocPool(arena, 64);
    upb_Arena_FreePool(arena, p32, 32);

    void* p128 = upb_Arena_AllocPool(arena, 128);
    upb_Arena_FreePool(arena, p64, 64);

    void* p256 = upb_Arena_AllocPool(arena, 256);
    upb_Arena_FreePool(arena, p128, 128);

    void* p512 = upb_Arena_AllocPool(arena, 512);
    upb_Arena_FreePool(arena, p256, 256);

    void* p1024 = upb_Arena_AllocPool(arena, 1024);
    upb_Arena_FreePool(arena, p512, 512);

    // Free the final buffer
    upb_Arena_FreePool(arena, p1024, 1024);
  }

  size_t total_allocated = upb_Arena_SpaceAllocated(arena, nullptr);

  // Despite 6,000 allocations across 1,000 cycles (totaling >2MB of churn),
  // the memory pooled and allocated should be bounded under 32KB.
  EXPECT_LE(total_allocated, 32768);

  upb_Arena_Free(arena);
}
}  // namespace
