// Protocol Buffers - Google's data interchange format
// Copyright 2021 Google Inc.  All rights reserved.
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

#include "google/protobuf/bk_arena.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>  // NOLINT
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/attributes.h"
#include "absl/strings/cord.h"
#include "util/symbolize/symbolized_stacktrace.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::testing::Eq;
using ::testing::NotNull;
using ::testing::UnorderedElementsAre;

enum Scenario { kEmpty, kWithSpace, kFull };

struct TestParam {
  Scenario setup;

  static std::string ToString(testing::TestParamInfo<TestParam> param) {
    switch (param.param.setup) {
      case Scenario::kEmpty:
        return "Empty";
      case Scenario::kFull:
        return "Full";
      case Scenario::kWithSpace:
        return "WithSpace";
    }
    return "???";
  }
};

class BKArenaTest : public ::testing::TestWithParam<TestParam> {
 public:
  void SetUp() override {
    policy_.start_block_size = 4 << 10;
    policy_.max_block_size = 128 << 10;
    switch (setup()) {
      case Scenario::kEmpty:
        policy_.block_alloc = AllocateFirst;
        policy_.block_dealloc = DeallocateFirst;
        break;
      case Scenario::kFull:
        policy_.block_alloc = AllocateNext;
        policy_.block_dealloc = DeallocateNext;
        // Allocate((4 << 10) - sizeof(MemoryBlock) - sizeof(SerialArena));
        break;
      case Scenario::kWithSpace:
        policy_.block_alloc = AllocateFirst;
        policy_.block_dealloc = DeallocateFirst;
        // Allocate(16);
        break;
    }
  }

  Scenario setup() const { return GetParam().setup; }
  BKArena& arena() { return arena_; }

  void ResetArena() {
    arena_.~BKArena();
    new (&arena_) BKArena();
  }

 private:
  static void* AllocateFirst(size_t n) {
    bool unaligned = sizeof(MemoryBlock) & 0x8;
    char* p = static_cast<char*>(malloc(n + 8));
    QCHECK(ArenaAlignAs(16).IsAligned(p));
    return p + (unaligned ? 0 : 8);
  }

  static void DeallocateFirst(void* p, size_t n) {
    bool unaligned = sizeof(MemoryBlock) & 0x8;
    free(static_cast<char*>(p) - (unaligned ? 0 : 8));
  }

  static void* AllocateNext(size_t n) {
    bool unaligned = sizeof(MemoryBlock) & 0x8;
    void* p = static_cast<char*>(malloc(n)) + (unaligned ? 0 : 8);
    QCHECK(ArenaAlignAs(16).IsAligned(p));
    return p;
  }

  static void DeallocateNext(void* p, size_t n) {
    bool unaligned = sizeof(MemoryBlock) & 0x8;
    free(static_cast<char*>(p) - (unaligned ? 0 : 8));
  }

  AllocationPolicy policy_;
  BKArena arena_{nullptr, 0, policy_};
};

class BKArenaPerThreadTest : public testing::Test {};

TEST(BKArenaBasicTest, CreateDestroy) {
  BKArena arena;
  BKSerialArena* serial;
  ASSERT_TRUE(arena.GetSerialArenaFast(&serial));
  EXPECT_THAT(serial->memory(), Eq(MemoryBlock::sentinel()));
  EXPECT_THAT(serial->ptr(), Eq(MemoryBlock::sentinel()->tail()));
  //  EXPECT_THAT(serial->cleanup_pos(), Eq(Memoryblock::sentinel()->tail()));
}

TEST(BKArenaBasicTest, Allocate) {
  BKArena arena;

  void* p = arena.AllocateAligned(16);
  ASSERT_THAT(p, NotNull());
  memset(p, 0, 16);

  p = arena.AllocateAligned(32);
  ASSERT_THAT(p, NotNull());
  memset(p, 0, 32);
}

TEST(BKArenaBasicTest, AllocateWithCleanup) {
  BKArena arena;

  void* p = arena.AllocateCleanup(cleanupx::CleanupArgFor<std::string>());
  new (p) std::string(1000, 'x');

  p = arena.AllocateCleanup(cleanupx::CleanupArgFor<std::string>());
  new (p) std::string(100, 'x');

  arena.~BKArena();
  new (&arena) BKArena;

  static int dtor_count;
  static void* dtor_objects[2];
  dtor_count = 0;

  struct Class {
    ~Class() { dtor_objects[dtor_count++] = this; }
    std::string s = "Hello world, and some extra content for non SSO";
  };

  p = arena.AllocateCleanup(cleanupx::CleanupArgFor<Class>());
  ASSERT_THAT(p, NotNull());
  auto* object1 = new (p) Class;

  p = arena.AllocateCleanup(cleanupx::CleanupArgFor<Class>());
  ASSERT_THAT(p, NotNull());
  auto* object2 = new (p) Class;

  arena.~BKArena();
  new (&arena) BKArena;

  EXPECT_THAT(dtor_count, Eq(2));
  EXPECT_THAT(dtor_objects, UnorderedElementsAre(object2, object1));
}

TEST(BKArenaBasicTest, AddCleanup) {
  BKArena arena;

  void* p = arena.AllocateCleanup(cleanupx::CleanupArgFor<std::string>());
  new (p) std::string(1000, 'x');

  p = arena.AllocateAligned(sizeof(std::string));
  arena.AllocateCleanup(cleanupx::CleanupArgFor(new (p) std::string(100, 'x')));

  static int dtor_count;
  static void* dtor_objects[2];
  dtor_count = 0;

  struct Class {
    ~Class() { dtor_objects[dtor_count++] = this; }
    std::string s = "Hello world, and some extra content for non SSO";
  };

  p = arena.AllocateAligned(sizeof(Class));
  ASSERT_THAT(p, NotNull());
  auto* object1 = new (p) Class;
  arena.AllocateCleanup(cleanupx::CleanupArgFor(object1));

  p = arena.AllocateAligned(sizeof(Class));
  ASSERT_THAT(p, NotNull());
  auto* object2 = new (p) Class;
  arena.AllocateCleanup(cleanupx::CleanupArgFor(object2));

  arena.~BKArena();
  new (&arena) BKArena;

  EXPECT_THAT(dtor_count, Eq(2));
  EXPECT_THAT(dtor_objects, UnorderedElementsAre(object2, object1));
}

TEST(BKArenaBasicTest, AllocateOnNewThread) {
  BKArena arena;

  std::thread thread([&] {
    void* p = arena.AllocateAligned(16);
    ASSERT_THAT(p, NotNull());
    memset(p, 0, 16);

    p = arena.AllocateAligned(32);
    ASSERT_THAT(p, NotNull());
    memset(p, 0, 32);
  });
  thread.join();
}

TEST(BKArenaBasicTest, AllocateWithCleanupOnNewThread) {
  BKArena arena;

  std::thread thread1([&] {
    void* p = arena.AllocateCleanup(cleanupx::CleanupArgFor<std::string>());
    new (p) std::string(1000, 'x');

    p = arena.AllocateCleanup(cleanupx::CleanupArgFor<std::string>());
    new (p) std::string(100, 'x');
  });

  static int dtor_count;
  static void* dtor_objects[2];
  dtor_count = 0;
  void* object1;
  void* object2;

  std::thread thread2([&] {
    struct Class {
      ~Class() { dtor_objects[dtor_count++] = this; }
      std::string s = "Hello world, and some extra content for non SSO";
    };

    void* p = arena.AllocateCleanup(cleanupx::CleanupArgFor<Class>());
    ASSERT_THAT(p, NotNull());
    object1 = new (p) Class;

    p = arena.AllocateCleanup(cleanupx::CleanupArgFor<Class>());
    ASSERT_THAT(p, NotNull());
    object2 = new (p) Class;
  });

  thread1.join();
  thread2.join();

  arena.~BKArena();
  new (&arena) BKArena;

  EXPECT_THAT(dtor_count, Eq(2));
  EXPECT_THAT(dtor_objects, UnorderedElementsAre(object2, object1));
}

TEST(BKArenaBasicTest, AddCleanupOnNewThread) {
  BKArena arena;

  std::thread thread1([&] {
    void* p = arena.AllocateCleanup(cleanupx::CleanupArgFor<std::string>());
    new (p) std::string(1000, 'x');

    p = arena.AllocateAligned(sizeof(std::string));
    arena.AllocateCleanup(
        cleanupx::CleanupArgFor(new (p) std::string(100, 'x')));
  });
  static int dtor_count;
  static void* dtor_objects[2];
  dtor_count = 0;
  void* object1;
  void* object2;

  std::thread thread2([&] {
    struct Class {
      ~Class() { dtor_objects[dtor_count++] = this; }
      std::string s = "Hello world, and some extra content for non SSO";
    };

    object1 = arena.AllocateAligned(sizeof(Class));
    arena.AllocateCleanup(cleanupx::CleanupArgFor(new (object1) Class));
    object2 = arena.AllocateAligned(sizeof(Class));
    arena.AllocateCleanup(cleanupx::CleanupArgFor(new (object2) Class));
  });

  thread1.join();
  thread2.join();

  arena.~BKArena();
  new (&arena) BKArena;

  EXPECT_THAT(dtor_count, Eq(2));
  EXPECT_THAT(dtor_objects, UnorderedElementsAre(object2, object1));
}

TEST(BKArenaBasicTest, AllocateWithDonatedMemory) {
  alignas(MemoryBlock) uint8_t mem[1024];
  BKArena arena(mem, sizeof(mem));
  BKSerialArena* serial;
  ASSERT_TRUE(arena.GetSerialArenaFast(&serial));

  void* p = arena.AllocateAligned(16);
  EXPECT_THAT(p, Eq(mem + sizeof(MemoryBlock)));
  memset(p, 0, 16);

  p = arena.AllocateAligned(32);
  EXPECT_THAT(p, Eq(mem + sizeof(MemoryBlock) + 16));
  memset(p, 0, 32);
}

TEST(BKArenaTest, AllocateOveraligned) {
  BKArena arena;
  BKSerialArena* serial;
  ASSERT_TRUE(arena.GetSerialArenaFast(&serial));

  void* p = arena.AllocateAligned(8);
  memset(p, 0, 8);
  while (ArenaAlignAs(64).IsAligned(p)) {
    p = arena.AllocateAligned(8);
    memset(p, 0, 8);
  }

  p = arena.AllocateAligned(64, ArenaAlignAs(64));
  EXPECT_TRUE(ArenaAlignAs(64).IsAligned(p));
  memset(p, 0, 64);

  p = arena.AllocateAligned(1024, ArenaAlignAs(512));
  EXPECT_TRUE(ArenaAlignAs(512).IsAligned(p)) << (void*)p;
  memset(p, 0, 1024);
}

#ifdef THIS_IS_THE_BAR_WE_WANT_TO_LOWER

TEST_P(BKArenaTest, Allocate) {
  // Assert that we are not aligned on 16 byte boundary, which our setup
  // enforces.
  void* p = Allocate(16);
  ASSERT_THAT(p, NotNull());
  ASSERT_FALSE(ArenaAlignAs(16).IsAligned(p));
  memset(p, 0, 16);
}

TEST_P(BKArenaTest, AllocateWithFunctionCleanup) {
  static void* cleanup_ptr;
  static int cleanup_count;
  cleanup_ptr = nullptr;
  cleanup_count = 0;

  cleanup::CleanupFn cleanup = +[](void* p) {
    cleanup_ptr = p;
    ++cleanup_count;
  };

  void* p = AllocateWithCleanupX(64, cleanup);
  ASSERT_THAT(p, NotNull());
  memset(p, 0, 64);
  EXPECT_EQ(cleanup_count, 0);

  ResetArena();
  EXPECT_EQ(cleanup_count, 1);
  EXPECT_EQ(cleanup_ptr, p);
}

TEST_P(BKArenaTest, AllocateOverAlignedWithFunctionCleanup) {
  static void* cleanup_ptr;
  static int cleanup_count;
  cleanup_ptr = nullptr;
  cleanup_count = 0;

  cleanup::CleanupFn cleanup = +[](void* p) {
    cleanup_ptr = p;
    ++cleanup_count;
  };

  void* p = AllocateWithCleanupX(32, ArenaAlignAs(32), cleanup);
  ASSERT_THAT(p, NotNull());
  EXPECT_TRUE(ArenaAlignAs(32).IsAligned(p));
  memset(p, 0, 32);
  EXPECT_EQ(cleanup_count, 0);

  ResetArena();
  EXPECT_EQ(cleanup_count, 1);
  EXPECT_EQ(cleanup_ptr, p);
}

TEST_P(BKArenaTest, AllocateWithStringCleanup) {
  void* p = AllocateWithCleanupX(sizeof(std::string), cleanup::Tag::kString);
  ASSERT_THAT(p, NotNull());
  new (p) std::string(32 << 10, 'x');
  ResetArena();
}

TEST_P(BKArenaTest, AllocateOveralignedWithStringCleanup) {
  auto align = ArenaAlignAs(32);
  size_t size = align.Ceil(sizeof(std::string));
  void* p = AllocateWithCleanupX(size, align, cleanup::Tag::kString);
  ASSERT_THAT(p, NotNull());
  EXPECT_TRUE(align.IsAligned(p));
  memset(p, 0, 32);
  new (p) std::string(32 << 10, 'x');
  ResetArena();
}

TEST_P(BKArenaTest, AllocateWithCordCleanup) {
  void* p = AllocateWithCleanupX(sizeof(absl::Cord), cleanup::Tag::kCord);
  ASSERT_THAT(p, NotNull());
  new (p) absl::Cord(std::string(2000, 'x'));
  ResetArena();
}

TEST_P(BKArenaTest, AllocateOveralignedWithCordCleanup) {
  auto align = ArenaAlignAs(32);
  size_t size = align.Ceil(sizeof(absl::Cord));
  void* p = AllocateWithCleanupX(size, align, cleanup::Tag::kCord);
  ASSERT_THAT(p, NotNull());
  EXPECT_TRUE(align.IsAligned(p));
  memset(p, 0, 32);
  new (p) absl::Cord(std::string(2000, 'x'));
  ResetArena();
}

TEST_P(BKArenaTest, AddFunctionCleanup) {
  static void* cleanup_ptr;
  static int cleanup_count;
  cleanup_ptr = nullptr;
  cleanup_count = 0;

  cleanup::CleanupFn cleanup = +[](void* p) {
    cleanup_ptr = p;
    ++cleanup_count;
  };

  AddCleanup(&cleanup_ptr, cleanup);
  EXPECT_EQ(cleanup_count, 0);

  ResetArena();
  EXPECT_EQ(cleanup_count, 1);
  EXPECT_EQ(cleanup_ptr, &cleanup_ptr);
}

TEST_P(BKArenaTest, AddStringCleanup) {
  void* p = Allocate(sizeof(std::string));
  ASSERT_THAT(p, NotNull());
  std::string* s = new (p) std::string(32 << 10, 'x');
  AddCleanup(s, cleanup::Tag::kString);
  ResetArena();
}

TEST_P(BKArenaTest, AddCordCleanup) {
  void* p = Allocate(sizeof(absl::Cord));
  ASSERT_THAT(p, NotNull());
  absl::Cord* c = new (p) absl::Cord(std::string(32 << 10, 'x'));
  AddCleanup(c, cleanup::Tag::kCord);
  ResetArena();
}

TEST_P(BKArenaTest, AllocateMany) {
  static void* static_ptr = &static_ptr;
  auto cleanup_check_ptr = +[](void* ptr) { GOOGLE_CHECK(ptr == static_ptr); };

  class Magic {
   public:
    Magic() {}
    ~Magic() { GOOGLE_CHECK(magic_ == 0x12345678); }

   private:
    int magic_ = 0x12345678;
  };
  auto magic_dtor = internal::cleanup::arena_destruct_object<Magic>;

  class BigMagic {
   public:
    BigMagic() = default;

    ~BigMagic() {
      GOOGLE_CHECK(magic1_ == 0x1234);
      GOOGLE_CHECK(magic2_ == 0x5678);
    }

   private:
    int magic1_ = 0x1234;
    char buf[112];
    int magic2_ = 0x5678;
  };
  auto big_magic_dtor = internal::cleanup::arena_destruct_object<BigMagic>;

  for (int loops = 0; loops < 100; ++loops) {
    for (int i = loops; i < (16 << 10); ++i) {
      void* p;
      switch (i & 15) {
        case 0:
          AddCleanup(static_ptr, cleanup_check_ptr);
          break;
        case 1:
          p = Allocate(sizeof(std::string));
          new (p) std::string(40, 'x');
          AddCleanup(p, cleanup::Tag::kString);
          break;
        case 2:
          p = Allocate(sizeof(absl::Cord));
          new (p) absl::Cord(std::string(40, 'x'));
          AddCleanup(p, cleanup::Tag::kCord);
          break;

        case 3:
          p = AllocateWithCleanupX(sizeof(std::string), cleanup::Tag::kString);
          new (p) std::string(100, 'x');
          break;
        case 4:
          p = AllocateWithCleanupX(sizeof(absl::Cord), cleanup::Tag::kCord);
          new (p) absl::Cord(std::string(100, 'x'));
          break;

        case 5:
          p = AllocateWithCleanupX(ArenaAlignDefault::Ceil(sizeof(Magic)),
                                   magic_dtor);
          new (p) Magic;
          break;
        case 6:
          static_assert(ArenaAlignDefault::IsAligned(sizeof(BigMagic)), "");
          p = AllocateWithCleanupX(sizeof(BigMagic), big_magic_dtor);
          new (p) BigMagic;
          break;

        case 7: {
          SerialArena* serial;
          if (arena().GetSerialArenaFast(&serial)) {
            p = serial->MaybeAllocateWithCleanup<Magic>();
            if (p) new (p) Magic;
          }
        } break;
        case 8: {
          SerialArena* serial;
          if (arena().GetSerialArenaFast(&serial)) {
            p = serial->MaybeAllocateWithCleanup<std::string>();
            if (p) new (p) std::string(100, 'x');
          }
        } break;

        default:
          p = Allocate(16 + (i % 16) * 8);
          memset(p, 0, 16 + (i % 16) * 8);
          break;
      }
    }
    ResetArena();
  }
}

TEST_P(BKArenaTest, AllocateManyOnNewThread) {
  return;
  BKArena arena;

  auto cleanup = +[](void* p) {};

  for (int i = 0; i < (64 << 10); ++i) {
    //    std::cout << i << "\n";
    void* p = arena.AllocateAligned(16);
    ASSERT_THAT(p, NotNull());
    memset(p, 0, 16);

    //    std::cout << i << " X \n";
    p = arena.AllocateWithCleanupX(32, ArenaAlignDefault(), cleanup);
    ASSERT_THAT(p, NotNull());
    memset(p, 0, 32);
  }

  return;
  std::thread thread([&] {
    for (int i = 0; i < (64 << 10); ++i) {
      void* p = arena.AllocateAligned(16);
      ASSERT_THAT(p, NotNull());
      memset(p, 0, 16);

      p = arena.AllocateWithCleanupX(32, ArenaAlignDefault(), cleanup);
      ASSERT_THAT(p, NotNull());
      memset(p, 0, 32);
    }
  });
  thread.join();
}

TEST_P(BKArenaTest, ManyThreads) {
  constexpr int kThreadCount = 100;
  constexpr int kAllocateCount = (32 << 10) * 100 / 32;  // ~100 blocks @ 32b

  std::vector<std::thread> threads;
  absl::Notification off_to_the_races;
  std::vector<int64_t*> datas(kThreadCount * kAllocateCount);

  for (int i = 0; i < kThreadCount; ++i) {
    threads.emplace_back([&, i] {
      int64_t sequence = i * kAllocateCount;
      off_to_the_races.WaitForNotification();
      for (int j = 0; j < kAllocateCount; ++j) {
        auto* p = static_cast<int64_t*>(Allocate(32));
        QCHECK(p);
        *p = sequence;
        datas[static_cast<size_t>(sequence++)] = p;
      }
    });
  }

  off_to_the_races.Notify();

  for (auto& thread : threads) {
    thread.join();
  }

  int64_t sequence = 0;
  for (int64_t* data : datas) {
    ASSERT_NE(data, nullptr) << " at " << sequence;
    ASSERT_EQ(*data, sequence++) << " at " << sequence;
  }
}

#endif

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
