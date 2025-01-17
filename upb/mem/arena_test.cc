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

// Must be last.
#include "upb/port/def.inc"

namespace {

struct CustomAlloc {
  upb_alloc alloc;
  int counter;
  bool ran_cleanup;
};

void* CustomAllocFunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                      size_t size) {
  CustomAlloc* custom_alloc = reinterpret_cast<CustomAlloc*>(alloc);
  if (size == 0) {
    custom_alloc->counter--;
  } else {
    custom_alloc->counter++;
  }
  return upb_alloc_global.func(alloc, ptr, oldsize, size);
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

struct SizeTracker {
  upb_alloc alloc;
  upb_alloc* delegate_alloc;
  absl::flat_hash_map<void*, size_t>* sizes;
};

static_assert(std::is_standard_layout<SizeTracker>());

static void* size_checking_allocfunc(upb_alloc* alloc, void* ptr,
                                     size_t oldsize, size_t size) {
  SizeTracker* size_alloc = reinterpret_cast<SizeTracker*>(alloc);
  void* result = size_alloc->delegate_alloc->func(alloc, ptr, oldsize, size);
  if (ptr != nullptr) {
    UPB_ASSERT(size_alloc->sizes->at(ptr) == oldsize);
    size_alloc->sizes->erase(ptr);
  }
  if (result != nullptr) {
    size_alloc->sizes->emplace(result, size);
  }
  return result;
}

TEST(ArenaTest, SizedFree) {
  absl::flat_hash_map<void*, size_t> sizes;
  SizeTracker alloc;
  alloc.alloc.func = size_checking_allocfunc;
  alloc.delegate_alloc = &upb_alloc_global;
  alloc.sizes = &sizes;

  upb_Arena* arena = upb_Arena_Init(nullptr, 0, &alloc.alloc);
  (void)upb_Arena_Malloc(arena, 500);
  void* to_resize = upb_Arena_Malloc(arena, 2000);
  void* resized = upb_Arena_Realloc(arena, to_resize, 2000, 4000);
  upb_Arena_ShrinkLast(arena, resized, 4000, 1);
  EXPECT_GT(sizes.size(), 0);
  upb_Arena_Free(arena);
  EXPECT_EQ(sizes.size(), 0);
}

TEST(ArenaTest, SizeHint) {
  absl::flat_hash_map<void*, size_t> sizes;
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
  size_t RandomIndex(absl::BitGen& gen, size_t min_index = 0) {
    return absl::Uniform<size_t>(gen, min_index,
                                 std::tuple_size<ArenaArray>::value);
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

  using ArenaArray = std::array<std::shared_ptr<const upb::Arena>, 100>;
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
    upb_Arena_Malloc(arena, 1024);
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
                                       size_t oldsize, size_t size) {
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
    upb_Arena_SetMaxBlockSize(32 << 10);
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
      upb_Arena_Fuse(arr[j - 1], arr[j]);
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
    upb_Arena_SetMaxBlockSize(32 << 10);
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
          upb_Arena_Fuse(read, fuse);
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
    upb_Arena_SetMaxBlockSize(32 << 10);
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
  upb_Arena_Fuse(a->ptr(), b->ptr());
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

#endif

}  // namespace
