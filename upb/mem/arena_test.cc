// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mem/arena.h"

#include <stddef.h>

#include <array>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include "absl/base/thread_annotations.h"
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

TEST(ArenaTest, ArenaFuse) {
  upb_Arena* arena1 = upb_Arena_New();
  upb_Arena* arena2 = upb_Arena_New();

  EXPECT_TRUE(upb_Arena_Fuse(arena1, arena2));

  upb_Arena_Free(arena1);
  upb_Arena_Free(arena2);
}

// Do-nothing allocator for testing.
extern "C" void* TestAllocFunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                               size_t size) {
  return upb_alloc_global.func(alloc, ptr, oldsize, size);
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

TEST(ArenaTest, Contains) {
  upb_Arena* arena1 = upb_Arena_New();
  upb_Arena* arena2 = upb_Arena_New();
  void* ptr1a = upb_Arena_Malloc(arena1, 8);
  void* ptr2a = upb_Arena_Malloc(arena2, 8);

  EXPECT_TRUE(UPB_PRIVATE(_upb_Arena_Contains)(arena1, ptr1a));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Arena_Contains)(arena2, ptr2a));
  EXPECT_FALSE(UPB_PRIVATE(_upb_Arena_Contains)(arena1, ptr2a));
  EXPECT_FALSE(UPB_PRIVATE(_upb_Arena_Contains)(arena2, ptr1a));

  void* ptr1b = upb_Arena_Malloc(arena1, 1000000);
  void* ptr2b = upb_Arena_Malloc(arena2, 1000000);

  EXPECT_TRUE(UPB_PRIVATE(_upb_Arena_Contains)(arena1, ptr1a));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Arena_Contains)(arena1, ptr1b));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Arena_Contains)(arena2, ptr2a));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Arena_Contains)(arena2, ptr2b));
  EXPECT_FALSE(UPB_PRIVATE(_upb_Arena_Contains)(arena1, ptr2a));
  EXPECT_FALSE(UPB_PRIVATE(_upb_Arena_Contains)(arena1, ptr2b));
  EXPECT_FALSE(UPB_PRIVATE(_upb_Arena_Contains)(arena2, ptr1a));
  EXPECT_FALSE(UPB_PRIVATE(_upb_Arena_Contains)(arena2, ptr1b));

  upb_Arena_Fuse(arena1, arena2);

  EXPECT_TRUE(UPB_PRIVATE(_upb_Arena_Contains)(arena1, ptr1a));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Arena_Contains)(arena1, ptr1b));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Arena_Contains)(arena2, ptr2a));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Arena_Contains)(arena2, ptr2b));
  EXPECT_FALSE(UPB_PRIVATE(_upb_Arena_Contains)(arena1, ptr2a));
  EXPECT_FALSE(UPB_PRIVATE(_upb_Arena_Contains)(arena1, ptr2b));
  EXPECT_FALSE(UPB_PRIVATE(_upb_Arena_Contains)(arena2, ptr1a));
  EXPECT_FALSE(UPB_PRIVATE(_upb_Arena_Contains)(arena2, ptr1b));

  upb_Arena_Free(arena1);
  upb_Arena_Free(arena2);
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

#ifdef UPB_USE_C11_ATOMICS

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
