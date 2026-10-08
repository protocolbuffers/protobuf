// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/pyext/free_threading/access_guard.h"

#include <optional>
#include <utility>

#include <gtest/gtest.h>
#include "google/protobuf/pyext/free_threading/guarded_ptr.h"
#include "google/protobuf/pyext/free_threading/locked_ptr.h"
#include "google/protobuf/pyext/free_threading/multi_lock.h"

namespace google {
namespace protobuf {
namespace python {
namespace {

void EnsurePythonInitialized() {
  static const bool initialized = []() {
    if (!Py_IsInitialized()) {
      Py_Initialize();
      PyEval_SaveThread();
    }
    return true;
  }();
  (void)initialized;
}

class ScopedPyThreadState {
 public:
  ScopedPyThreadState() {
    EnsurePythonInitialized();
    AccessGuard::SetEnabledInTsanForTesting(true);
    gstate_ = PyGILState_Ensure();
    PyErr_Clear();
  }
  ~ScopedPyThreadState() {
    PyErr_Clear();
    PyGILState_Release(gstate_);
  }

 private:
  PyGILState_STATE gstate_;
};

TEST(AccessGuardTest, ConcurrentReadersSucceed) {
  ScopedPyThreadState py_thread;
  AccessGuard guard;
  int value = 42;
  GuardedPtr<int> ptr(&value, guard);

  {
    std::optional<LockedPtr<const int>> r1 = ptr.LockRead(guard);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(**r1, 42);

    std::optional<LockedPtr<const int>> r2 = ptr.LockRead(guard);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(**r2, 42);
  }

  // Both read locks went out of scope; a subsequent write lock must succeed.
  std::optional<LockedPtr<int>> w = ptr.LockWrite(guard);
  ASSERT_TRUE(w.has_value());
  **w = 99;
  EXPECT_EQ(value, 99);
}

TEST(AccessGuardTest, AssignOrReturnMacro) {
  ScopedPyThreadState py_thread;
  AccessGuard guard;
  int value = 42;
  GuardedPtr<int> ptr(&value, guard);

  auto read_fn = [&]() -> int {
    PYPROTO_ASSIGN_OR_RETURN(LockedPtr<const int> r, ptr.LockRead(guard), -1);
    return *r;
  };
  EXPECT_EQ(read_fn(), 42);

  auto multi_fn = [&]() -> int {
    PYPROTO_ASSIGN_OR_RETURN((auto [w, r]),
                             LockWriteRead(ptr, guard, ptr, guard), -1);
    *w += *r;
    return *w;
  };
  EXPECT_EQ(multi_fn(), 84);

#ifdef Py_GIL_DISABLED
  std::optional<LockedPtr<int>> w = ptr.LockWrite(guard);
  ASSERT_TRUE(w.has_value());
  EXPECT_EQ(read_fn(), -1);
  PyErr_Clear();
  EXPECT_EQ(multi_fn(), -1);
  PyErr_Clear();
#endif
}

#ifdef Py_GIL_DISABLED
TEST(AccessGuardTest, WriteBlocksReadAndWrite) {
  ScopedPyThreadState py_thread;
  AccessGuard::ResetConcurrentAccessCountForTesting();
  AccessGuard guard;
  int value = 10;
  GuardedPtr<int> ptr(&value, guard);

  {
    std::optional<LockedPtr<int>> w = ptr.LockWrite(guard);
    ASSERT_TRUE(w.has_value());

    // While write lock is held, both LockRead and LockWrite must fail,
    // increment the concurrent access counter, and set a Python RuntimeError.
    EXPECT_FALSE(ptr.LockRead(guard).has_value());
    EXPECT_EQ(AccessGuard::GetConcurrentAccessCountForTesting(), 1u);
    EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
    PyErr_Clear();

    EXPECT_FALSE(ptr.LockWrite(guard).has_value());
    EXPECT_EQ(AccessGuard::GetConcurrentAccessCountForTesting(), 2u);
    EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
    PyErr_Clear();
  }

  // Once released, both LockRead and LockWrite can succeed again.
  EXPECT_TRUE(ptr.LockRead(guard).has_value());
}

TEST(AccessGuardTest, ReadBlocksWriteAndPreservesReadState) {
  ScopedPyThreadState py_thread;
  AccessGuard::ResetConcurrentAccessCountForTesting();
  AccessGuard guard;
  int value = 10;
  GuardedPtr<int> ptr(&value, guard);

  {
    std::optional<LockedPtr<const int>> r2;
    {
      std::optional<LockedPtr<const int>> r1 = ptr.LockRead(guard);
      ASSERT_TRUE(r1.has_value());

      // Attempting to acquire write while read lock is held must fail and clear
      // kWriteBit so additional readers are not blocked.
      EXPECT_FALSE(ptr.LockWrite(guard).has_value());
      EXPECT_EQ(AccessGuard::GetConcurrentAccessCountForTesting(), 1u);
      EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
      PyErr_Clear();

      r2 = ptr.LockRead(guard);
      EXPECT_TRUE(r2.has_value());
    }
    // One reader (r2) is still active, so write must still fail.
    EXPECT_FALSE(ptr.LockWrite(guard).has_value());
    EXPECT_EQ(AccessGuard::GetConcurrentAccessCountForTesting(), 2u);
    EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
    PyErr_Clear();
  }

  // All readers released; write must now succeed.
  EXPECT_TRUE(ptr.LockWrite(guard).has_value());
}
#else
TEST(AccessGuardTest, GilEnabledIsNoOp) {
  ScopedPyThreadState py_thread;
  AccessGuard guard;
  int value = 10;
  GuardedPtr<int> ptr(&value, guard);

  std::optional<LockedPtr<int>> w = ptr.LockWrite(guard);
  ASSERT_TRUE(w.has_value());

  // In GIL-enabled builds, AccessGuard is a no-op so overlapping locks succeed.
  std::optional<LockedPtr<const int>> r = ptr.LockRead(guard);
  EXPECT_TRUE(r.has_value());
}
#endif  // Py_GIL_DISABLED

TEST(AccessGuardTest, LockedPtrMoveSemantics) {
  ScopedPyThreadState py_thread;
  AccessGuard guard;
  int v1 = 1;
  int v2 = 2;
  GuardedPtr<int> p1(&v1, guard);

  AccessGuard guard2;
  GuardedPtr<int> p2(&v2, guard2);

  std::optional<LockedPtr<int>> maybe_w1 = p1.LockWrite(guard);
  ASSERT_TRUE(maybe_w1.has_value());

  // Move construct w_moved from *maybe_w1; guard must remain locked.
  LockedPtr<int> w_moved(*std::move(maybe_w1));
  EXPECT_EQ(*w_moved, 1);
#ifdef Py_GIL_DISABLED
  EXPECT_FALSE(p1.LockRead(guard).has_value());
  PyErr_Clear();
#endif

  // Move assign *maybe_w2 into w_moved; w_moved must release guard and hold
  // guard2.
  std::optional<LockedPtr<int>> maybe_w2 = p2.LockWrite(guard2);
  ASSERT_TRUE(maybe_w2.has_value());
  w_moved = *std::move(maybe_w2);
  EXPECT_EQ(*w_moved, 2);
  EXPECT_TRUE(p1.LockWrite(guard).has_value());
#ifdef Py_GIL_DISABLED
  EXPECT_FALSE(p2.LockRead(guard2).has_value());
  PyErr_Clear();
#endif
}

TEST(AccessGuardTest, SameTreeUnpackingViaGetAndGetMutable) {
  ScopedPyThreadState py_thread;
  AccessGuard tree_guard;
  int root_val = 100;
  int child_val = 200;
  int replacement_val = 300;

  GuardedPtr<int> root(&root_val, tree_guard);
  GuardedPtr<int> child(&child_val, tree_guard);

  {
    std::optional<LockedPtr<const int>> root_lock = root.LockRead(tree_guard);
    ASSERT_TRUE(root_lock.has_value());
    const int* child_ptr = child.Get(*root_lock);
    EXPECT_EQ(*child_ptr, 200);
  }

  {
    std::optional<LockedPtr<int>> root_lock = root.LockWrite(tree_guard);
    ASSERT_TRUE(root_lock.has_value());
    int* child_ptr = child.GetMutable(*root_lock);
    *child_ptr = 250;
    EXPECT_EQ(child_val, 250);

    child.Set(&replacement_val, *root_lock);
    EXPECT_EQ(*child.Get(*root_lock), 300);

    // LockedPtr::UpdatePtr replaces the pointee in-place while keeping the
    // existing lock held.
    root_lock->UpdatePtr(&replacement_val);
    EXPECT_EQ(**root_lock, 300);
    EXPECT_EQ(root_lock->get(), &replacement_val);
  }
}

TEST(AccessGuardDeathTest, CrossTreeGetFailsDCheck) {
  ScopedPyThreadState py_thread;
  AccessGuard guard_a;
  AccessGuard guard_b;
  int val_a = 1;
  int val_b = 2;
  GuardedPtr<int> ptr_a(&val_a, guard_a);
  GuardedPtr<int> ptr_b(&val_b, guard_b);

  std::optional<LockedPtr<int>> lock_a = ptr_a.LockWrite(guard_a);
  ASSERT_TRUE(lock_a.has_value());

  EXPECT_DEBUG_DEATH((void)ptr_b.Get(*lock_a), "");
  EXPECT_DEBUG_DEATH((void)ptr_b.GetMutable(*lock_a), "");
}

TEST(AccessGuardTest, LockWriteReadSameTree) {
  ScopedPyThreadState py_thread;
  AccessGuard guard;
  int dst_val = 10;
  int src_val = 20;
  int child_val = 30;
  GuardedPtr<int> dst(&dst_val, guard);
  GuardedPtr<int> src(&src_val, guard);
  GuardedPtr<int> src_child(&child_val, guard);

  {
    auto locks = LockWriteRead(dst, guard, src, guard);
    ASSERT_TRUE(locks.has_value());
    auto [dst_lock, src_lock] = *std::move(locks);
    EXPECT_EQ(*dst_lock, 10);
    EXPECT_EQ(*src_lock, 20);

    // Unpacking a child using the borrowed same-tree read lock must succeed.
    EXPECT_EQ(*src_child.Get(src_lock), 30);

#ifdef Py_GIL_DISABLED
    // Tree is write-locked, so another read lock must fail.
    EXPECT_FALSE(dst.LockRead(guard).has_value());
    PyErr_Clear();
#endif

    *dst_lock += *src_lock;
  }

  EXPECT_EQ(dst_val, 30);
  // Both dst_lock and src_lock destroyed; guard must be completely unlocked.
  EXPECT_TRUE(dst.LockWrite(guard).has_value());

#ifdef Py_GIL_DISABLED
  // Same-tree LockWriteRead must also fail if a read lock is currently held,
  // and must preserve the existing reader state.
  {
    std::optional<LockedPtr<const int>> active_reader = src.LockRead(guard);
    ASSERT_TRUE(active_reader.has_value());
    EXPECT_FALSE(LockWriteRead(dst, guard, src, guard).has_value());
    EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
    PyErr_Clear();

    // Additional readers can still acquire while active_reader is held.
    EXPECT_TRUE(dst.LockRead(guard).has_value());
  }
  EXPECT_TRUE(dst.LockWrite(guard).has_value());
#endif
}

TEST(AccessGuardTest, LockReadReadSameAndDifferentTrees) {
  ScopedPyThreadState py_thread;
  AccessGuard guard1;
  AccessGuard guard2;
  int val1 = 10;
  int val2 = 20;
  GuardedPtr<int> p1(&val1, guard1);
  GuardedPtr<int> p2(&val2, guard1);
  GuardedPtr<int> p3(&val2, guard2);

  // Same tree LockReadRead.
  {
    auto locks = LockReadRead(p1, guard1, p2, guard1);
    ASSERT_TRUE(locks.has_value());
    auto [r1, r2] = *std::move(locks);
    EXPECT_EQ(*r1, 10);
    EXPECT_EQ(*r2, 20);
#ifdef Py_GIL_DISABLED
    EXPECT_FALSE(p1.LockWrite(guard1).has_value());
    PyErr_Clear();
#endif
  }
  EXPECT_TRUE(p1.LockWrite(guard1).has_value());

  // Different trees LockReadRead.
  {
    auto locks = LockReadRead(p1, guard1, p3, guard2);
    ASSERT_TRUE(locks.has_value());
    auto [r1, r3] = *std::move(locks);
    EXPECT_EQ(*r1, 10);
    EXPECT_EQ(*r3, 20);
  }

#ifdef Py_GIL_DISABLED
  // Rollback when the second guard is write-locked: guard1 must be released.
  {
    std::optional<LockedPtr<int>> w2 = p3.LockWrite(guard2);
    ASSERT_TRUE(w2.has_value());
    EXPECT_FALSE(LockReadRead(p1, guard1, p3, guard2).has_value());
    EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
    PyErr_Clear();

    // guard1 must have been rolled back and remain write-lockable.
    EXPECT_TRUE(p1.LockWrite(guard1).has_value());
  }
#endif
}

#ifdef Py_GIL_DISABLED
TEST(AccessGuardTest, LockWriteReadDifferentTreesAndRollback) {
  ScopedPyThreadState py_thread;
  AccessGuard guard1;
  AccessGuard guard2;
  int val1 = 10;
  int val2 = 20;
  GuardedPtr<int> p1(&val1, guard1);
  GuardedPtr<int> p2(&val2, guard2);

  // Test locking in both pointer orderings: (p1, p2) and (p2, p1).
  {
    auto locks = LockWriteRead(p1, guard1, p2, guard2);
    ASSERT_TRUE(locks.has_value());
    EXPECT_FALSE(p1.LockRead(guard1).has_value());
    PyErr_Clear();
    EXPECT_TRUE(p2.LockRead(guard2).has_value());  // guard2 is only read-locked
    EXPECT_FALSE(p2.LockWrite(guard2).has_value());
    PyErr_Clear();
  }
  {
    auto locks = LockWriteRead(p2, guard2, p1, guard1);
    ASSERT_TRUE(locks.has_value());
    EXPECT_FALSE(p2.LockRead(guard2).has_value());
    PyErr_Clear();
    EXPECT_TRUE(p1.LockRead(guard1).has_value());
  }

  // Test rollback when guard2 is already write-locked.
  {
    std::optional<LockedPtr<int>> block2 = p2.LockWrite(guard2);
    ASSERT_TRUE(block2.has_value());

    EXPECT_FALSE(LockWriteRead(p1, guard1, p2, guard2).has_value());
    PyErr_Clear();

    // guard1 must have been rolled back and remain unlocked!
    EXPECT_TRUE(p1.LockWrite(guard1).has_value());
  }

  // Test rollback when guard1 is already read-locked.
  {
    std::optional<LockedPtr<const int>> block1 = p1.LockRead(guard1);
    ASSERT_TRUE(block1.has_value());

    EXPECT_FALSE(LockWriteRead(p1, guard1, p2, guard2).has_value());
    PyErr_Clear();

    // guard2 must have been rolled back and remain unlocked!
    EXPECT_TRUE(p2.LockWrite(guard2).has_value());
  }
}

TEST(AccessGuardTest, MultiThreadedStress) {
  EnsurePythonInitialized();
  AccessGuard guard;
  int shared_value = 0;
  GuardedPtr<int> ptr(&shared_value, guard);

  std::atomic<int> active_writers{0};
  std::atomic<int> active_readers{0};

  constexpr int kNumThreads = 8;
  constexpr int kIterations = 5000;
  std::vector<std::thread> threads;
  threads.reserve(kNumThreads);

  for (int t = 0; t < kNumThreads; ++t) {
    threads.emplace_back([&, t]() {
      ScopedPyThreadState py_thread;
      for (int i = 0; i < kIterations; ++i) {
        if ((i + t) % 4 == 0) {
          if (std::optional<LockedPtr<int>> w = ptr.LockWrite(guard)) {
            EXPECT_EQ(active_writers.fetch_add(1, std::memory_order_relaxed),
                      0);
            EXPECT_EQ(active_readers.load(std::memory_order_relaxed), 0);
            ++(**w);
            active_writers.fetch_sub(1, std::memory_order_relaxed);
          } else {
            PyErr_Clear();
          }
        } else {
          if (std::optional<LockedPtr<const int>> r = ptr.LockRead(guard)) {
            active_readers.fetch_add(1, std::memory_order_relaxed);
            EXPECT_EQ(active_writers.load(std::memory_order_relaxed), 0);
            volatile int observed = **r;
            (void)observed;
            active_readers.fetch_sub(1, std::memory_order_relaxed);
          } else {
            PyErr_Clear();
          }
        }
      }
    });
  }

  for (auto& th : threads) {
    th.join();
  }

  ScopedPyThreadState py_thread;
  // After all threads finish, the guard state must be 0 (cleanly lockable).
  EXPECT_TRUE(ptr.LockWrite(guard).has_value());
}
#endif  // Py_GIL_DISABLED

TEST(AccessGuardTest, SubtreeDetachmentProtocol) {
  ScopedPyThreadState py_thread;
  AccessGuard old_guard;
  AccessGuard new_guard;
  int parent_val = 10;
  int child_val = 20;
  int grandchild_val = 30;
  GuardedPtr<int> parent(&parent_val, old_guard);
  GuardedPtr<int> child(&child_val, old_guard);
  GuardedPtr<int> grandchild(&grandchild_val, old_guard);

  {
    // Step 1: Parent operation holds write lock on old_guard.
    std::optional<LockedPtr<int>> parent_lock = parent.LockWrite(old_guard);
    ASSERT_TRUE(parent_lock.has_value());

#ifdef Py_GIL_DISABLED
    // If new_guard is already locked, UpdateGuardAndLockWrite must fail and
    // leave child bound to old_guard.
    {
      int dummy_val = 0;
      GuardedPtr<int> new_guard_holder(&dummy_val, new_guard);
      std::optional<LockedPtr<const int>> block_new =
          new_guard_holder.LockRead(new_guard);
      ASSERT_TRUE(block_new.has_value());

      EXPECT_FALSE(
          child.UpdateGuardAndLockWrite(*parent_lock, new_guard).has_value());
      EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
      PyErr_Clear();

      // child must still be bound to old_guard.
      EXPECT_EQ(*child.Get(*parent_lock), 20);
    }
#endif

    {
      // Step 2: Transition child to new_guard and lock new_guard for write,
      // then hand off grandchild using both locks.
      std::optional<LockedPtr<int>> new_tree_lock =
          child.UpdateGuardAndLockWrite(*parent_lock, new_guard);
      ASSERT_TRUE(new_tree_lock.has_value());
      grandchild.UpdateGuard(*parent_lock, *new_tree_lock);

      EXPECT_EQ(*grandchild.Get(*new_tree_lock), 30);

#ifdef Py_GIL_DISABLED
      // While detachment is in progress, BOTH old_guard and new_guard reject
      // concurrent read/write attempts.
      EXPECT_FALSE(parent.LockRead(old_guard).has_value());
      PyErr_Clear();
      EXPECT_FALSE(child.LockRead(new_guard).has_value());
      PyErr_Clear();
      EXPECT_FALSE(child.LockWrite(new_guard).has_value());
      PyErr_Clear();
#endif
    }
  }

  // Step 3: Parent lock released. Now parent and detached subtree belong to
  // independent trees and can be write-locked concurrently.
  std::optional<LockedPtr<int>> p_write = parent.LockWrite(old_guard);
  std::optional<LockedPtr<int>> c_write = child.LockWrite(new_guard);
  ASSERT_TRUE(p_write.has_value());
  ASSERT_TRUE(c_write.has_value());
  EXPECT_EQ(**p_write, 10);
  EXPECT_EQ(**c_write, 20);
  EXPECT_EQ(*grandchild.GetMutable(*c_write), 30);
}

}  // namespace
}  // namespace python
}  // namespace protobuf
}  // namespace google
