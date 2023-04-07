#include "upb/mem/arena.h"

#include <array>
#include <atomic>
#include <thread>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/random/distributions.h"
#include "absl/random/random.h"
#include "absl/synchronization/notification.h"
#include "upb/upb.hpp"

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

/* Do nothing allocator for testing */
extern "C" void* TestAllocFunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                               size_t size) {
  return upb_alloc_global.func(alloc, ptr, oldsize, size);
}

TEST(ArenaTest, FuseWithInitialBlock) {
  char buf1[1024];
  char buf2[1024];
  upb_Arena* arenas[] = {upb_Arena_Init(buf1, 1024, &upb_alloc_global),
                         upb_Arena_Init(buf2, 1024, &upb_alloc_global),
                         upb_Arena_Init(NULL, 0, &upb_alloc_global)};
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

#endif

}  // namespace
