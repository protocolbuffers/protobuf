// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file or at https://developers.google.com/open-source/licenses/bsd

#include <atomic>
#include <limits>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "python/free_threading/access.h"
#include "python/free_threading/access_test_util.h"

namespace {

class AccessTest : public testing::Test {
 protected:
  void SetUp() override {
    access = PyUpb_Access_NewForTest();
    ASSERT_NE(access, nullptr);
  }
  void TearDown() override { PyUpb_Access_FreeForTest(access); }
  PyUpb_Access* access = nullptr;
};

TEST_F(AccessTest, AdmissionAndBalancedRetirement) {
  ASSERT_TRUE(PyUpb_Access_TryRead(access));
  ASSERT_TRUE(PyUpb_Access_TryRead(access));
  EXPECT_FALSE(PyUpb_Access_TryWrite(access));
  PyUpb_Access_EndRead(access);
  EXPECT_FALSE(PyUpb_Access_TryWrite(access));
  PyUpb_Access_EndRead(access);
  ASSERT_TRUE(PyUpb_Access_TryWrite(access));
  EXPECT_FALSE(PyUpb_Access_TryWrite(access));
  EXPECT_FALSE(PyUpb_Access_TryRead(access));
  PyUpb_Access_EndWrite(access);
  ASSERT_TRUE(PyUpb_Access_TryRead(access));
  PyUpb_Access_EndRead(access);
}

TEST_F(AccessTest, UpgradeRequiresExactlyOneReader) {
  ASSERT_TRUE(PyUpb_Access_TryRead(access));
  ASSERT_TRUE(PyUpb_Access_TryRead(access));
  EXPECT_FALSE(PyUpb_Access_TryUpgradeReadToWrite(access));
  PyUpb_Access_EndRead(access);
  ASSERT_TRUE(PyUpb_Access_TryUpgradeReadToWrite(access));
  EXPECT_FALSE(PyUpb_Access_TryRead(access));
  EXPECT_FALSE(PyUpb_Access_TryWrite(access));
  PyUpb_Access_EndWrite(access);
  ASSERT_TRUE(PyUpb_Access_TryRead(access));
  PyUpb_Access_EndRead(access);
}

TEST_F(AccessTest, FailedUpgradeRetainsReadAdmission) {
  ASSERT_TRUE(PyUpb_Access_TryRead(access));
  std::thread reader([&] {
    ASSERT_TRUE(PyUpb_Access_TryRead(access));
    EXPECT_FALSE(PyUpb_Access_TryUpgradeReadToWrite(access));
    EXPECT_FALSE(PyUpb_Access_TryWrite(access));
    PyUpb_Access_EndRead(access);
  });
  reader.join();
  // The failed upgrade must not release either caller's read ownership.
  EXPECT_FALSE(PyUpb_Access_TryWrite(access));
  ASSERT_TRUE(PyUpb_Access_TryUpgradeReadToWrite(access));
  EXPECT_FALSE(PyUpb_Access_TryRead(access));
  PyUpb_Access_EndWrite(access);
  ASSERT_TRUE(PyUpb_Access_TryWrite(access));
  PyUpb_Access_EndWrite(access);
}

TEST_F(AccessTest, ReaderCountCannotWrapIntoWriterState) {
  PyUpb_Access_SetCountForTest(access, std::numeric_limits<int32_t>::max());
  EXPECT_FALSE(PyUpb_Access_TryRead(access));
  EXPECT_FALSE(PyUpb_Access_TryWrite(access));
  EXPECT_EQ(PyUpb_Access_GetCountForTest(access), std::numeric_limits<int32_t>::max());
}

TEST_F(AccessTest, OwnershipDoesNotDependOnThreadIdentity) {
  ASSERT_TRUE(PyUpb_Access_TryWrite(access));
  std::thread rejected([&] {
    EXPECT_FALSE(PyUpb_Access_TryRead(access));
    EXPECT_FALSE(PyUpb_Access_TryWrite(access));
  });
  rejected.join();
  PyUpb_Access_EndWrite(access);
  std::thread next_owner([&] {
    ASSERT_TRUE(PyUpb_Access_TryWrite(access));
    PyUpb_Access_EndWrite(access);
  });
  next_owner.join();
}

TEST_F(AccessTest, NativeStorageIsProtectedUnderContention) {
  // Intentionally non-atomic: TSan must see the admission's happens-before
  // edges, not a separately synchronized stand-in for native message data.
  int value = 0;
  int mirror = 0;
  std::atomic<bool> start{false};
  std::atomic<int> writes{0};
  std::atomic<int> reads{0};
  std::vector<std::thread> workers;
  for (int worker = 0; worker < 8; ++worker) {
    workers.emplace_back([&, worker] {
      while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
      // Each attempted operation either obtains admission or is rejected;
      // no operation spins until its owner becomes available.
      for (int attempt = 0; attempt < 20000; ++attempt) {
        if (worker < 3) {
          if (!PyUpb_Access_TryWrite(access)) continue;
          ++value;
          mirror = value;
          PyUpb_Access_EndWrite(access);
          writes.fetch_add(1, std::memory_order_relaxed);
        } else {
          if (!PyUpb_Access_TryRead(access)) continue;
          EXPECT_EQ(value, mirror);
          PyUpb_Access_EndRead(access);
          reads.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }
  start.store(true, std::memory_order_release);
  for (auto& worker : workers) worker.join();
  // Fail-fast admission promises neither reader nor writer fairness. Either
  // group can exhaust its finite attempts while another group owns admission.
  // Require aggregate progress here; the deterministic handoff test below
  // separately requires successful reads and writes on every iteration.
  EXPECT_GT(reads.load() + writes.load(), 0);
  EXPECT_EQ(value, writes.load());
  EXPECT_EQ(value, mirror);
  // No rejected attempt may leak ownership: both modes must work after join.
  ASSERT_TRUE(PyUpb_Access_TryRead(access));
  EXPECT_EQ(value, mirror);
  PyUpb_Access_EndRead(access);
  ASSERT_TRUE(PyUpb_Access_TryWrite(access));
  PyUpb_Access_EndWrite(access);
}

TEST_F(AccessTest, NativeStorageHandoffRequiresAdmissionOrdering) {
  int value = 0;
  std::atomic<int> phase{0};
  std::atomic<bool> failed{false};
  std::thread writer([&] {
    for (int iteration = 0; iteration < 2000; ++iteration) {
      while (phase.load(std::memory_order_relaxed) != 2 * iteration) {
        if (failed.load(std::memory_order_relaxed)) return;
        std::this_thread::yield();
      }
      if (!PyUpb_Access_TryWrite(access)) {
        failed.store(true, std::memory_order_relaxed);
        ADD_FAILURE() << "Writer handoff was rejected";
        return;
      }
      value = iteration + 1;
      PyUpb_Access_EndWrite(access);
      phase.store(2 * iteration + 1, std::memory_order_relaxed);
    }
  });
  std::thread reader([&] {
    for (int iteration = 0; iteration < 2000; ++iteration) {
      while (phase.load(std::memory_order_relaxed) != 2 * iteration + 1) {
        if (failed.load(std::memory_order_relaxed)) return;
        std::this_thread::yield();
      }
      if (!PyUpb_Access_TryRead(access)) {
        failed.store(true, std::memory_order_relaxed);
        ADD_FAILURE() << "Reader handoff was rejected";
        return;
      }
      EXPECT_EQ(value, iteration + 1);
      PyUpb_Access_EndRead(access);
      // Relaxed phase operations order test attempts but supply no separate
      // happens-before edge for value. Admission must supply that edge.
      phase.store(2 * iteration + 2, std::memory_order_relaxed);
    }
  });
  writer.join();
  reader.join();
  EXPECT_EQ(value, 2000);
}

}  // namespace
