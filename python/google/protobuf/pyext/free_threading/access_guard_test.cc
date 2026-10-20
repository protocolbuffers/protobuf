// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/pyext/free_threading/access_guard.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <atomic>
#include <thread>  // NOLINT
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "google/protobuf/pyext/free_threading/guarded.h"

namespace google {
namespace protobuf {
namespace python {
namespace {

// Helper to ensure Python runtime is initialized and the thread has a valid
// PyThreadState so PyErr_* functions work in tests.
class ScopedPyThreadState {
 public:
  ScopedPyThreadState() {
    if (!Py_IsInitialized()) {
      Py_Initialize();
#ifdef Py_GIL_DISABLED
      AccessGuard::SetEnabledInTsanForTesting(true);
#endif
    }
    gil_state_ = PyGILState_Ensure();
  }

  ~ScopedPyThreadState() {
    PyErr_Clear();
    PyGILState_Release(gil_state_);
  }

  ScopedPyThreadState(const ScopedPyThreadState&) = delete;
  ScopedPyThreadState& operator=(const ScopedPyThreadState&) = delete;

 private:
  PyGILState_STATE gil_state_;
};

// -----------------------------------------------------------------------------
// ReadAccess / WriteAccess / Guarded<T>
// -----------------------------------------------------------------------------

// Compile-time properties of tokens and Guarded<T>.
static_assert(!std::is_copy_constructible_v<ReadAccess>);
static_assert(!std::is_move_constructible_v<ReadAccess>);
static_assert(!std::is_copy_constructible_v<WriteAccess>);
static_assert(!std::is_move_constructible_v<WriteAccess>);
static_assert(std::is_convertible_v<const WriteAccess&, const ReadAccess&>);
static_assert(!std::is_convertible_v<const ReadAccess&, const WriteAccess&>);

// Read access to Guarded<U*> yields const U*; write access yields U*.
static_assert(std::is_same_v<decltype(std::declval<const Guarded<int*>&>().Get(
                                 std::declval<const ReadAccess&>())),
                             const int*>);
static_assert(std::is_same_v<decltype(std::declval<const Guarded<int*>&>().Get(
                                 std::declval<const WriteAccess&>())),
                             int*>);
// For non-pointer T, Get() always yields const T& (Set() modifies).
static_assert(std::is_same_v<decltype(std::declval<const Guarded<int>&>().Get(
                                 std::declval<const WriteAccess&>())),
                             const int&>);

// Set() requires write access.
template <typename G, typename A>
constexpr bool kCanSet = requires(G g, const A& a) { g.Set({}, a); };
static_assert(kCanSet<Guarded<int>, WriteAccess>);
static_assert(!kCanSet<Guarded<int>, ReadAccess>);

TEST(AccessTokenTest, WithReadAndWrite) {
  ScopedPyThreadState py_thread;
  AccessGuard guard;
  int storage = 1;
  Guarded<int*> ptr(&storage, guard);
  Guarded<int> count(0, guard);

  int read = guard.WithRead(
      -1, [&](const ReadAccess& r) { return *ptr.Get(r) + count.Get(r); });
  EXPECT_EQ(read, 1);

  // Nested readers on the same guard.
  int nested = guard.WithRead(-1, [&](const ReadAccess& r1) {
    return guard.WithRead(
        -1, [&](const ReadAccess& r2) { return *ptr.Get(r1) + *ptr.Get(r2); });
  });
  EXPECT_EQ(nested, 2);

  // Write access mutates both the referent and guarded values. The unary form
  // (void callbacks only) reports whether the callback ran.
  EXPECT_TRUE(guard.WithWrite([&](const WriteAccess& w) {
    *ptr.Get(w) = 5;
    count.Set(count.Get(w) + 1, w);
  }));
  EXPECT_EQ(storage, 5);
  EXPECT_EQ(count.GetQuiescent(), 1);

  // A WriteAccess can be passed wherever a ReadAccess is expected.
  auto read_value = [&](const ReadAccess& r) { return *ptr.Get(r); };
  EXPECT_EQ(guard.WithWrite(-1, read_value), 5);
}

TEST(AccessTokenTest, GuardedPointerIsNullable) {
  ScopedPyThreadState py_thread;
  AccessGuard guard;
  int storage = 7;
  Guarded<int*> ptr(nullptr, guard);
  EXPECT_TRUE(guard.WithWrite([&](const WriteAccess& w) {
    EXPECT_EQ(ptr.Get(w), nullptr);
    ptr.Set(&storage, w);
    EXPECT_EQ(*ptr.Get(w), 7);
    ptr.Set(nullptr, w);
  }));
  EXPECT_EQ(ptr.GetQuiescent(), nullptr);
}

#ifdef Py_GIL_DISABLED
TEST(AccessTokenTest, ContentionFailsAndInvokesOnFail) {
  ScopedPyThreadState py_thread;
  AccessGuard::ResetConcurrentAccessCountForTesting();
  AccessGuard guard;

  EXPECT_TRUE(guard.WithWrite([&](const WriteAccess&) {
    EXPECT_EQ(guard.WithRead(-1, [](const ReadAccess&) { return 1; }), -1);
    EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
    PyErr_Clear();

    bool called = false;
    EXPECT_EQ(guard.WithWrite(
                  [&] {
                    called = true;
                    return -2;
                  },
                  [](const WriteAccess&) { return 1; }),
              -2);
    EXPECT_TRUE(called);
    PyErr_Clear();

    // The unary form (void callbacks only) returns false without running f.
    EXPECT_FALSE(guard.WithRead(
        [](const ReadAccess&) { ADD_FAILURE() << "should not run"; }));
    EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
    PyErr_Clear();

    // A void callback with a callable on_fail runs on_fail on failure.
    bool void_on_fail_called = false;
    guard.WithRead(
        [&] { void_on_fail_called = true; },
        [](const ReadAccess&) { ADD_FAILURE() << "should not run"; });
    EXPECT_TRUE(void_on_fail_called);
    PyErr_Clear();
  }));

  EXPECT_TRUE(guard.WithRead([&](const ReadAccess&) {
    EXPECT_FALSE(
        guard.WithWrite(false, [](const WriteAccess&) { return true; }));
    PyErr_Clear();
  }));

  EXPECT_EQ(AccessGuard::GetConcurrentAccessCountForTesting(), 5u);
  // Everything was released: the guard is cleanly lockable.
  EXPECT_TRUE(guard.WithWrite(false, [](const WriteAccess&) { return true; }));
}
#endif  // Py_GIL_DISABLED

TEST(AccessTokenTest, WithWriteReadSameGuardSharesOneToken) {
  ScopedPyThreadState py_thread;
  AccessGuard guard;
  int dst = 1;
  int src = 2;
  Guarded<int*> dst_ptr(&dst, guard);
  Guarded<int*> src_ptr(&src, guard);

  EXPECT_TRUE(AccessGuard::WithWriteRead(
      guard, guard, [&](const WriteAccess& w, const ReadAccess& r) {
        EXPECT_EQ(static_cast<const ReadAccess*>(&w), &r);
        *dst_ptr.Get(w) += *src_ptr.Get(r);
      }));
  EXPECT_EQ(dst, 3);

  // Exactly one write lock was taken and released.
  EXPECT_TRUE(guard.WithWrite(false, [](const WriteAccess&) { return true; }));
}

TEST(AccessTokenTest, WithWriteReadDifferentGuards) {
  ScopedPyThreadState py_thread;
  AccessGuard dst_guard;
  AccessGuard src_guard;
  int dst = 1;
  int src = 2;
  Guarded<int*> dst_ptr(&dst, dst_guard);
  Guarded<int*> src_ptr(&src, src_guard);

  int result = AccessGuard::WithWriteRead(
      dst_guard, src_guard, -1, [&](const WriteAccess& w, const ReadAccess& r) {
        *dst_ptr.Get(w) = *src_ptr.Get(r) * 10;
#ifdef Py_GIL_DISABLED
        // Read lock on src still permits other readers; write lock on dst
        // excludes them.
        EXPECT_EQ(src_guard.WithRead(-1, [](const ReadAccess&) { return 0; }),
                  0);
        EXPECT_EQ(dst_guard.WithRead(-1, [](const ReadAccess&) { return 0; }),
                  -1);
        PyErr_Clear();
#endif
        return *dst_ptr.Get(w);
      });
  EXPECT_EQ(result, 20);

#ifdef Py_GIL_DISABLED
  // If the src guard is write-locked, acquisition fails and the dst write lock
  // is rolled back.
  EXPECT_TRUE(src_guard.WithWrite([&](const WriteAccess&) {
    EXPECT_EQ(AccessGuard::WithWriteRead(
                  dst_guard, src_guard, -1,
                  [](const WriteAccess&, const ReadAccess&) { return 0; }),
              -1);
    PyErr_Clear();
  }));
#endif
  EXPECT_TRUE(
      dst_guard.WithWrite(false, [](const WriteAccess&) { return true; }));
  EXPECT_TRUE(
      src_guard.WithWrite(false, [](const WriteAccess&) { return true; }));
}

TEST(AccessTokenTest, WithReadRead) {
  ScopedPyThreadState py_thread;
  AccessGuard a_guard;
  AccessGuard b_guard;
  int a = 3;
  int b = 4;
  Guarded<int*> a_ptr(&a, a_guard);
  Guarded<int*> b_ptr(&b, b_guard);

  EXPECT_EQ(AccessGuard::WithReadRead(
                a_guard, b_guard, -1,
                [&](const ReadAccess& ra, const ReadAccess& rb) {
                  return *a_ptr.Get(ra) * *b_ptr.Get(rb);
                }),
            12);
  EXPECT_EQ(AccessGuard::WithReadRead(
                a_guard, a_guard, -1,
                [&](const ReadAccess& r1, const ReadAccess& r2) {
                  EXPECT_EQ(&r1, &r2);
                  return *a_ptr.Get(r1);
                }),
            3);

#ifdef Py_GIL_DISABLED
  // Second acquisition fails: the first read lock is released.
  EXPECT_TRUE(b_guard.WithWrite([&](const WriteAccess&) {
    EXPECT_EQ(AccessGuard::WithReadRead(
                  a_guard, b_guard, -1,
                  [](const ReadAccess&, const ReadAccess&) { return 0; }),
              -1);
    PyErr_Clear();
  }));
#endif
  EXPECT_TRUE(
      a_guard.WithWrite(false, [](const WriteAccess&) { return true; }));
}

TEST(AccessTokenTest, GuardedUpdateGuard) {
  ScopedPyThreadState py_thread;
  AccessGuard old_guard;
  AccessGuard new_guard;
  int value = 9;
  Guarded<int*> ptr(&value, old_guard);

  EXPECT_TRUE(old_guard.WithWrite([&](const WriteAccess& old_w) {
    EXPECT_TRUE(new_guard.WithWrite([&](const WriteAccess& new_w) {
      ptr.UpdateGuard(old_w, new_w);
      EXPECT_EQ(*ptr.Get(new_w), 9);
    }));
  }));
  EXPECT_TRUE(new_guard.WithRead(
      [&](const ReadAccess& r) { EXPECT_EQ(*ptr.Get(r), 9); }));
}

#if GTEST_HAS_DEATH_TEST
TEST(AccessTokenDeathTest, WrongGuardFailsCheck) {
  if (!free_threading_internal::kTrackGuards) {
    GTEST_SKIP() << "Guard tracking is only enabled in sanitizer builds.";
  }
  ScopedPyThreadState py_thread;
  AccessGuard guard_a;
  AccessGuard guard_b;
  int value = 0;
  Guarded<int*> ptr(&value, guard_a);
  EXPECT_DEATH((void)guard_b.WithRead([&](const ReadAccess& r) { ptr.Get(r); }),
               "different AccessGuard");
}
#endif

#ifdef Py_GIL_DISABLED
TEST(AccessTokenTest, MultithreadedStress) {
  AccessGuard guard;
  int storage = 0;
  Guarded<int*> value(&storage, guard);

  constexpr int kNumWriters = 4;
  constexpr int kNumReaders = 8;
  constexpr int kIterations = 1000;

  std::atomic<int> active_writers{0};
  std::atomic<int> active_readers{0};
  std::vector<std::thread> threads;
  threads.reserve(kNumWriters + kNumReaders);

  for (int i = 0; i < kNumWriters; ++i) {
    threads.emplace_back([&, i] {
      ScopedPyThreadState thread_state;
      for (int step = 0; step < kIterations; ++step) {
        // May fail under contention; that is expected here.
        (void)guard.WithWrite([&](const WriteAccess& w) {
          active_writers.fetch_add(1, std::memory_order_relaxed);
          EXPECT_EQ(active_readers.load(std::memory_order_relaxed), 0);
          EXPECT_EQ(active_writers.load(std::memory_order_relaxed), 1);
          *value.Get(w) = (i * 1000) + step;
          active_writers.fetch_sub(1, std::memory_order_relaxed);
        });
        PyErr_Clear();
      }
    });
  }

  for (int i = 0; i < kNumReaders; ++i) {
    threads.emplace_back([&] {
      ScopedPyThreadState thread_state;
      for (int step = 0; step < kIterations; ++step) {
        // May fail under contention; that is expected here.
        (void)guard.WithRead([&](const ReadAccess& r) {
          active_readers.fetch_add(1, std::memory_order_relaxed);
          EXPECT_EQ(active_writers.load(std::memory_order_relaxed), 0);
          (void)*value.Get(r);
          active_readers.fetch_sub(1, std::memory_order_relaxed);
        });
        PyErr_Clear();
      }
    });
  }

  for (auto& th : threads) {
    th.join();
  }

  ScopedPyThreadState py_thread;
  // After all threads finish, the guard is cleanly lockable.
  EXPECT_TRUE(guard.WithWrite(false, [](const WriteAccess&) { return true; }));
}
#endif  // Py_GIL_DISABLED

// Mirrors MessageTree subtree detachment: while holding a write lock on the
// old guard, acquire a write lock on a new guard and hand off a subtree's
// guarded values to it.
TEST(AccessTokenTest, SubtreeDetachmentProtocol) {
  ScopedPyThreadState py_thread;
  AccessGuard old_guard;
  AccessGuard new_guard;
  int parent_val = 10;
  int child_val = 20;
  int grandchild_val = 30;
  Guarded<int*> parent(&parent_val, old_guard);
  Guarded<int*> child(&child_val, old_guard);
  Guarded<int*> grandchild(&grandchild_val, old_guard);

  EXPECT_TRUE(old_guard.WithWrite([&](const WriteAccess& old_w) {
#ifdef Py_GIL_DISABLED
    // If new_guard is already locked, the handoff cannot start and the subtree
    // stays bound to old_guard.
    EXPECT_TRUE(new_guard.WithRead([&](const ReadAccess&) {
      EXPECT_FALSE(
          new_guard.WithWrite(false, [](const WriteAccess&) { return true; }));
      EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
      PyErr_Clear();
      EXPECT_EQ(*child.Get(old_w), 20);
    }));
#endif

    EXPECT_TRUE(new_guard.WithWrite([&](const WriteAccess& new_w) {
      child.UpdateGuard(old_w, new_w);
      grandchild.UpdateGuard(old_w, new_w);
      EXPECT_EQ(*grandchild.Get(new_w), 30);

#ifdef Py_GIL_DISABLED
      // While detachment is in progress, both guards reject other accesses.
      EXPECT_EQ(old_guard.WithRead(-1, [](const ReadAccess&) { return 0; }),
                -1);
      PyErr_Clear();
      EXPECT_EQ(new_guard.WithRead(-1, [](const ReadAccess&) { return 0; }),
                -1);
      PyErr_Clear();
#endif
    }));
  }));

  // The parent and the detached subtree are now independent and can be
  // write-locked concurrently.
  EXPECT_TRUE(old_guard.WithWrite([&](const WriteAccess& old_w) {
    EXPECT_TRUE(new_guard.WithWrite([&](const WriteAccess& new_w) {
      EXPECT_EQ(*parent.Get(old_w), 10);
      EXPECT_EQ(*child.Get(new_w), 20);
      EXPECT_EQ(*grandchild.Get(new_w), 30);
    }));
  }));
}

}  // namespace
}  // namespace python
}  // namespace protobuf
}  // namespace google
