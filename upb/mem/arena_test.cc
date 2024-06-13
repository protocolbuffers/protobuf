// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mem/arena.h"

#include <stddef.h>

#include <array>
#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include "absl/random/distributions.h"
#include "absl/random/random.h"
#include "absl/synchronization/notification.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "upb/mem/alloc.h"

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
  ~Environment() {
    for (auto& atom : arenas_) {
      auto* a = atom.load(std::memory_order_relaxed);
      if (a != nullptr) upb_Arena_Free(a);
    }
  }

  void RandomNewFree(absl::BitGen& gen) {
    auto* old = SwapRandomly(gen, upb_Arena_New());
    if (old != nullptr) upb_Arena_Free(old);
  }

  void RandomIncRefCount(absl::BitGen& gen) {
    auto* a = SwapRandomly(gen, nullptr);
    if (a != nullptr) {
      upb_Arena_IncRefFor(a, nullptr);
      upb_Arena_DecRefFor(a, nullptr);
      upb_Arena_Free(a);
    }
  }

  void RandomFuse(absl::BitGen& gen) {
    std::array<upb_Arena*, 2> old;
    for (auto& o : old) {
      o = SwapRandomly(gen, nullptr);
      if (o == nullptr) o = upb_Arena_New();
    }

    EXPECT_TRUE(upb_Arena_Fuse(old[0], old[1]));
    for (auto& o : old) {
      o = SwapRandomly(gen, o);
      if (o != nullptr) upb_Arena_Free(o);
    }
  }

  void RandomPoke(absl::BitGen& gen) {
    switch (absl::Uniform(gen, 0, 2)) {
      case 0:
        RandomNewFree(gen);
        break;
      case 1:
        RandomFuse(gen);
        break;
      default:
        break;
    }
  }

 private:
  upb_Arena* SwapRandomly(absl::BitGen& gen, upb_Arena* a) {
    return arenas_[absl::Uniform<size_t>(gen, 0, arenas_.size())].exchange(
        a, std::memory_order_acq_rel);
  }

  std::array<std::atomic<upb_Arena*>, 100> arenas_ = {};
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

#endif

}  // namespace
