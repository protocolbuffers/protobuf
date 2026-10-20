// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// -----------------------------------------------------------------------------
// File: message_tree.h
// -----------------------------------------------------------------------------
//
// This header file defines `MessageTree`, the shared reference-counted state
// owned by all Python wrapper objects (`CMessage` and `ContainerBase`
// subclasses) that belong to the same protobuf message hierarchy.
//
// In the C++ protobuf Python extension (pyext), mutating a parent message
// (e.g. parent.Clear(), parent.MergeFrom(other)) or promoting a lazy
// submessage (AssureWritable) directly reads or mutates the C++ protobuf
// sub-objects underneath child Python wrappers. Conversely, mutating a child
// wrapper may trigger parent promotion up the ancestor chain.
//
// Therefore, all Python wrappers in the same message hierarchy share a single
// `MessageTree` containing a tree-wide `AccessGuard`:
//
//   * Top-level root `CMessage` objects allocate a new `MessageTree` with
//     refcount = 1.
//   * All Python wrapper objects (`CMessage` and `ContainerBase` subclasses) in
//     the entire message hierarchy hold only a single shared reference on the
//     `MessageTree`, owned by the root `CMessage`.
//   * Child messages and container wrappers store a raw pointer to their
//     parent's `MessageTree` without incrementing its reference count. Because
//     each child wrapper retains a strong Python reference to its parent
//     (`Py_INCREF(parent)`), the root `CMessage` and its `MessageTree` are
//     guaranteed to remain alive in memory for at least as long as any child
//     wrapper in the hierarchy is reachable.
//   * Additional references to a `MessageTree` come solely from other
//     `MessageTree` instances via `prev_tree` links when subtrees are detached.
//   * On destruction (`tp_dealloc`), only root `CMessage` objects release their
//     reference (`tree->Unref()`). Child wrappers never call `Unref()`, and
//     drop their parent reference as their final step so child cleanup never
//     accesses a destroyed tree.
//   * Locking any node in the hierarchy locks the tree's `AccessGuard`,
//     protecting the entire tree.

#ifndef GOOGLE_PROTOBUF_PYEXT_MESSAGE_TREE_H__
#define GOOGLE_PROTOBUF_PYEXT_MESSAGE_TREE_H__

#include <atomic>
#include <cstddef>

#include "google/protobuf/pyext/free_threading/access_guard.h"

namespace google {
namespace protobuf {
namespace python {

// -----------------------------------------------------------------------------
// MessageTree
// -----------------------------------------------------------------------------
//
// Reference-counted shared state for a hierarchy of Python protobuf wrappers.
//
// Exactly one reference to `MessageTree` is held by the entire Python wrapper
// tree, owned by the root `CMessage`. Additional references come exclusively
// from detached `MessageTree` instances via `prev_tree`.
//
// When a submessage or container is detached from its parent (for example,
// when parent.ClearField("sub") or del repeated_field[i] orphans an existing
// Python wrapper as a standalone root message), the detached subtree is
// migrated to a newly allocated `MessageTree`.
//
// Under free-threaded Python (Py_GIL_DISABLED), another thread might race with
// detachment by calling `GetTree()` just before the child's tree pointer is
// updated to new_tree, and then immediately attempting to lock the old tree's
// guard. If old_tree were destroyed while that thread was between `GetTree()`
// and locking the guard, a use-after-free would occur.
//
// To prevent this without blocking locks on `GetTree()`, the newly allocated
// `MessageTree` takes a reference to old_tree as its prev_tree
// (`new MessageTree(old_tree)`). This guarantees that old_tree remains
// alive in memory for at least as long as any detached node in new_tree is
// alive.
//
// Example (creating and sharing a `MessageTree`):
//
//   // Top-level message allocates a new tree (starts with refcount == 1):
//   root_cmsg->SetTree(new MessageTree());
//
//   // Child message or container shares the parent's tree without Ref():
//   child_cmsg->SetTree(root_cmsg->GetTree());
//
//   // On destruction (tp_dealloc), only root messages release their reference:
//   if (self->parent == nullptr) {
//     self->GetTree()->Unref();
//   }
//
// Example (detaching a subtree into a new `MessageTree` while holding a write
// lock on the old tree's guard):
//
//   MessageTree* old_tree = parent->GetTree();
//   MessageTree* new_tree = new MessageTree(old_tree);  // Refs old_tree.
//   // Points child and all of its descendants at new_tree:
//   cmessage::UpdateSubtreeTree(child, new_tree);
class MessageTree {
 public:
  // Creates a new tree with a reference count of 1, owned by the caller.
  MessageTree() = default;

  // Creates the tree for a subtree being detached from `prev`. Takes a new
  // reference to `prev`, keeping it alive for as long as this tree is alive.
  explicit MessageTree(MessageTree* prev) : prev_tree_(prev->Ref()) {}

  MessageTree(const MessageTree&) = delete;
  MessageTree& operator=(const MessageTree&) = delete;

  // The guard protecting every node of this tree.
  AccessGuard& guard() { return guard_; }

  // MessageTree::Ref()
  //
  // Increments the reference count and returns this for convenience.
  MessageTree* Ref() {
    refcount_.fetch_add(1, std::memory_order_relaxed);
    return this;
  }

  // MessageTree::Unref()
  //
  // Decrements the reference count and deletes this when the count reaches
  // zero, iteratively releasing prev_tree links without recursion.
  void Unref() {
    MessageTree* curr = this;
    while (curr != nullptr &&
           curr->refcount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      MessageTree* prev = curr->prev_tree_;
      delete curr;
      curr = prev;
    }
  }

 private:
  // Only Unref() may destroy a MessageTree.
  ~MessageTree() = default;

  AccessGuard guard_;
  std::atomic<size_t> refcount_{1};

  // Holds a reference to the tree this one was detached from (if any),
  // preventing use-after-free if a concurrent reader/writer loaded `GetTree()`
  // right before detachment. Released by Unref() when this tree is destroyed.
  MessageTree* const prev_tree_ = nullptr;
};

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYEXT_MESSAGE_TREE_H__
