// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/pyext/container_base.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "google/protobuf/pyext/free_threading/access_guard.h"
#include "google/protobuf/pyext/free_threading/guarded.h"
#include "google/protobuf/pyext/message_tree.h"

namespace google {
namespace protobuf {
namespace python {
namespace {

using ::google::protobuf::DescriptorProto;
using ::google::protobuf::MessageOptions;

// Self-contained TreeLockable test fixtures exercising TreeLockable<Derived>
// with Guarded<T> fields for both message-like (self-owning) and
// container-like (parent-owning) nodes.
struct TestMessage : public TreeLockable<TestMessage> {
  Guarded<TestMessage*> parent_node;
  Guarded<Message*> message;
  Guarded<bool> is_mutable;
  Guarded<bool> is_frozen;
  std::unique_ptr<DescriptorProto> owned_proto;
  MessageOptions unpromoted_options;

  explicit TestMessage(MessageTree* t, TestMessage* p = nullptr)
      : parent_node(p, t->guard()),
        message(nullptr, t->guard()),
        is_mutable(p == nullptr, t->guard()),
        is_frozen(false, t->guard()) {
    InitTreeQuiescent(t);
    if (p == nullptr) {
      owned_proto = std::make_unique<DescriptorProto>();
      message.SetQuiescent(owned_proto.get());
    } else {
      message.SetQuiescent(&unpromoted_options);
    }
  }

  const TestMessage* Owner(const ReadAccess&) const { return this; }
  TestMessage* Owner(const WriteAccess&) { return this; }

  // CRTP hook for TreeLockable::MoveToTree().
  void UpdateDerivedFieldsGuard(const WriteAccess& old_w,
                                const WriteAccess& new_w) {
    parent_node.UpdateGuard(old_w, new_w);
    message.UpdateGuard(old_w, new_w);
    is_mutable.UpdateGuard(old_w, new_w);
    is_frozen.UpdateGuard(old_w, new_w);
  }
};

Message* AssureWritable(TestMessage* self, const WriteAccess& lock) {
  if (self->is_frozen.Get(lock)) {
    PyErr_SetString(PyExc_RuntimeError, "Message is frozen");
    return nullptr;
  }
  if (!self->is_mutable.Get(lock)) {
    TestMessage* p = self->parent_node.Get(lock);
    ABSL_CHECK(p != nullptr);
    Message* p_msg = AssureWritable(p, lock);
    if (p_msg == nullptr) {
      return nullptr;
    }
    self->message.Set(static_cast<DescriptorProto*>(p_msg)->mutable_options(),
                      lock);
    self->is_mutable.Set(true, lock);
  }
  return self->message.Get(lock);
}

// TreeLockable::WithWriteRead() passes the source; this fixture's promotion
// never detaches anything, so it is ignored.
Message* AssureWritable(TestMessage* self, const WriteAccess& lock,
                        const WriteReadSource&) {
  return AssureWritable(self, lock);
}

struct TestContainer : public TreeLockable<TestContainer> {
  Guarded<TestMessage*> parent_msg;

  explicit TestContainer(TestMessage* p)
      : parent_msg(p, p->GetTree()->guard()) {
    InitTreeQuiescent(p->GetTree());
  }

  const TestMessage* Owner(const ReadAccess& access) const {
    return parent_msg.Get(access);
  }
  TestMessage* Owner(const WriteAccess& access) {
    return parent_msg.Get(access);
  }
};

template <typename Obj, typename Token, typename = void>
struct CanCallTargetMessage : std::false_type {};

template <typename Obj, typename Token>
struct CanCallTargetMessage<
    Obj, Token,
    std::void_t<decltype(std::declval<Obj>().TargetMessage(
        std::declval<Token>()))>> : std::true_type {};

static_assert(CanCallTargetMessage<TestMessage&, const WriteAccess&>::value);
static_assert(CanCallTargetMessage<TestMessage&, const ReadAccess&>::value);
static_assert(
    CanCallTargetMessage<const TestMessage&, const ReadAccess&>::value);
static_assert(
    !CanCallTargetMessage<const TestMessage&, const WriteAccess&>::value,
    "const wrappers must not accept WriteAccess in TargetMessage()");

class ContainerBaseTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    if (!Py_IsInitialized()) {
      Py_Initialize();
    }
#ifdef Py_GIL_DISABLED
    PyGILState_STATE gil = PyGILState_Ensure();
    AccessGuard::SetEnabledInTsanForTesting(true);
    PyGILState_Release(gil);
#endif
  }

  void SetUp() override {
    gil_state_ = PyGILState_Ensure();
    PyErr_Clear();
  }

  void TearDown() override {
    PyErr_Clear();
    PyGILState_Release(gil_state_);
  }

 private:
  PyGILState_STATE gil_state_;
};

TEST_F(ContainerBaseTest, MessageWithReadAndWithWrite) {
  MessageTree* tree = new MessageTree();
  TestMessage msg_node(tree);

  // Initially empty.
  EXPECT_EQ(msg_node.WithRead(std::string("err"),
                              [&](const ReadAccess& r) {
                                const auto* msg =
                                    static_cast<const DescriptorProto*>(
                                        msg_node.TargetMessage(r));
                                return msg->name();
                              }),
            "");

  // Mutate under WithWrite.
  EXPECT_TRUE(msg_node.WithWrite([&](const WriteAccess& w) {
    auto* msg = static_cast<DescriptorProto*>(msg_node.TargetMessage(w));
    msg->set_name("hello");
  }));

  EXPECT_EQ(msg_node.WithRead(std::string("err"),
                              [&](const ReadAccess& r) {
                                const auto* msg =
                                    static_cast<const DescriptorProto*>(
                                        msg_node.TargetMessage(r));
                                return msg->name();
                              }),
            "hello");
  tree->Unref();
}

TEST_F(ContainerBaseTest, SubmessageWithWritePromotesToMutable) {
  MessageTree* tree = new MessageTree();
  TestMessage root(tree);
  TestMessage sub(tree, &root);
  EXPECT_EQ(sub.GetTree(), root.GetTree());

  // Before mutation, options is not set on root.
  EXPECT_FALSE(root.WithRead(true, [&](const ReadAccess& r) {
    return static_cast<const DescriptorProto*>(root.TargetMessage(r))
        ->has_options();
  }));

  // Submessage WithWrite promotes the submessage (AssureWritable) and mutates.
  EXPECT_TRUE(sub.WithWrite([&](const WriteAccess& w) {
    auto* nested = static_cast<MessageOptions*>(sub.TargetMessage(w));
    nested->set_deprecated(true);
  }));

  EXPECT_TRUE(root.WithRead(false, [&](const ReadAccess& r) {
    const auto* msg =
        static_cast<const DescriptorProto*>(root.TargetMessage(r));
    return msg->has_options() && msg->options().deprecated();
  }));
  tree->Unref();
}

TEST_F(ContainerBaseTest, ContainerTargetsParentMessage) {
  MessageTree* tree = new MessageTree();
  TestMessage root(tree);
  TestContainer container(&root);
  EXPECT_EQ(container.GetTree(), root.GetTree());

  EXPECT_TRUE(container.WithWrite([&](const WriteAccess& w) {
    EXPECT_EQ(container.Owner(w), &root);
    auto* msg = static_cast<DescriptorProto*>(container.TargetMessage(w));
    msg->add_reserved_name("a");
    msg->add_reserved_name("b");
  }));

  EXPECT_EQ(container.WithRead(-1,
                               [&](const ReadAccess& r) {
                                 const auto* msg =
                                     static_cast<const DescriptorProto*>(
                                         container.TargetMessage(r));
                                 return msg->reserved_name_size();
                               }),
            2);
  tree->Unref();
}

TEST_F(ContainerBaseTest, FrozenMessageWithWriteFails) {
  MessageTree* tree = new MessageTree();
  TestMessage root(tree);
  TestContainer container(&root);

  EXPECT_TRUE(root.WithWrite(
      [&](const WriteAccess& w) { root.is_frozen.Set(true, w); }));

  bool callback_ran = false;
  EXPECT_FALSE(
      root.WithWrite([&](const WriteAccess&) { callback_ran = true; }));
  EXPECT_FALSE(callback_ran);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  PyErr_Clear();

  EXPECT_FALSE(
      container.WithWrite([&](const WriteAccess&) { callback_ran = true; }));
  EXPECT_FALSE(callback_ran);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  PyErr_Clear();
  tree->Unref();
}

TEST_F(ContainerBaseTest, WithWriteReadAndWithReadRead) {
  MessageTree* tree_a = new MessageTree();
  MessageTree* tree_b = new MessageTree();
  TestMessage a(tree_a);
  TestMessage b(tree_b);

  // Different trees:
  EXPECT_NE(a.GetTree(), b.GetTree());
  EXPECT_TRUE(b.WithWrite([&](const WriteAccess& w) {
    static_cast<DescriptorProto*>(b.TargetMessage(w))->set_name("from_b");
  }));

  EXPECT_TRUE(TestMessage::WithWriteRead(
      &a, &b, [&](const WriteAccess& wa, const ReadAccess& rb) {
        EXPECT_EQ(wa.guard(), &tree_a->guard());
        EXPECT_EQ(rb.guard(), &tree_b->guard());
        static_cast<DescriptorProto*>(a.TargetMessage(wa))
            ->set_name(static_cast<const DescriptorProto*>(b.TargetMessage(rb))
                           ->name());
      }));

  EXPECT_TRUE(TestMessage::WithReadRead(
      &a, &b, false, [&](const ReadAccess& ra, const ReadAccess& rb) {
        return static_cast<const DescriptorProto*>(a.TargetMessage(ra))
                       ->name() == "from_b" &&
               static_cast<const DescriptorProto*>(b.TargetMessage(rb))
                       ->name() == "from_b";
      }));

  // Same tree (a and its submessage): a single write lock covers both.
  TestMessage sub(tree_a, &a);
  EXPECT_TRUE(TestMessage::WithWriteRead(
      &sub, &a, [&](const WriteAccess& w_sub, const ReadAccess& r_a) {
        EXPECT_EQ(w_sub.guard(), &tree_a->guard());
        EXPECT_EQ(r_a.guard(), &tree_a->guard());
        static_cast<MessageOptions*>(sub.TargetMessage(w_sub))
            ->set_deprecated(
                !static_cast<const DescriptorProto*>(a.TargetMessage(r_a))
                     ->name()
                     .empty());
      }));

  tree_a->Unref();
  tree_b->Unref();
}

// Mutating dst inside WithWriteRead() may detach the same-tree source onto a
// new MessageTree (e.g. AssureWritable releasing an overlapping oneof, or
// InternalClear). The new tree is born write-locked as a split of dst's write
// lock, and once src has moved, that lock is downgraded to a read lock held by
// the source's token, so the source stays readable through `r` with no
// unlocked instant.
TEST_F(ContainerBaseTest, SourceTokenFollowsSourceSplitOffDuringWrite) {
  MessageTree* orig_tree = new MessageTree();
  TestMessage dst(orig_tree);
  TestMessage src(orig_tree);
  EXPECT_TRUE(src.WithWrite([&](const WriteAccess& w) {
    static_cast<DescriptorProto*>(src.TargetMessage(w))->set_name("detached");
  }));

  MessageTree* detached_tree = nullptr;
  const TestMessage* const_src = &src;
  EXPECT_TRUE(TestMessage::WithWriteRead(
      &dst, const_src, [&](const WriteAccess& w_dst, const ReadAccess& r_src) {
        EXPECT_EQ(r_src.guard(), &orig_tree->guard());

        // Detach src, as InternalReparentFields does.
        const WriteReadSource source{const_src, r_src};
        WithSplitTree(orig_tree, w_dst, &source,
                      [&](MessageTree* new_tree, const WriteAccess& new_w) {
                        detached_tree = new_tree;
                        src.MoveToTree(new_tree, w_dst, new_w);
                      });
        EXPECT_EQ(const_src->GetTree(), detached_tree);
        EXPECT_EQ(r_src.guard(), &detached_tree->guard());
#ifdef Py_GIL_DISABLED
        // r_src holds a read lock on the split: writers are excluded.
        EXPECT_FALSE(detached_tree->guard().WithWrite(
            false, [](const WriteAccess&) { return true; }));
        PyErr_Clear();
#endif
        static_cast<DescriptorProto*>(dst.TargetMessage(w_dst))
            ->set_name(static_cast<const DescriptorProto*>(
                           const_src->TargetMessage(r_src))
                           ->name());
      }));

  EXPECT_EQ(dst.WithRead(std::string(),
                         [&](const ReadAccess& r) {
                           return static_cast<const DescriptorProto*>(
                                      dst.TargetMessage(r))
                               ->name();
                         }),
            "detached");
  // r_src released the split lock when WithWriteRead() returned.
  EXPECT_TRUE(src.WithWrite([&](const WriteAccess& w) {
    EXPECT_EQ(w.guard(), &detached_tree->guard());
  }));
  detached_tree->Unref();
  orig_tree->Unref();
}

// A split that does not contain the source is released when WithSplitTree()
// returns, and the source token is unaffected.
TEST_F(ContainerBaseTest, SplitWithoutSourceIsReleasedImmediately) {
  MessageTree* orig_tree = new MessageTree();
  TestMessage dst(orig_tree);
  TestMessage src(orig_tree);
  TestMessage other(orig_tree);

  MessageTree* detached_tree = nullptr;
  EXPECT_TRUE(TestMessage::WithWriteRead(
      &dst, &src, [&](const WriteAccess& w_dst, const ReadAccess& r_src) {
        const WriteReadSource source{&src, r_src};
        WithSplitTree(orig_tree, w_dst, &source,
                      [&](MessageTree* new_tree, const WriteAccess& new_w) {
                        detached_tree = new_tree;
                        other.MoveToTree(new_tree, w_dst, new_w);
                      });
        EXPECT_EQ(r_src.guard(), &orig_tree->guard());
        EXPECT_TRUE(other.WithRead([&](const ReadAccess& r) {
          EXPECT_EQ(r.guard(), &detached_tree->guard());
        }));
      }));
  detached_tree->Unref();
  orig_tree->Unref();
}

TEST_F(ContainerBaseTest, MoveToTreeRebindsGuardedFields) {
  MessageTree* orig_tree = new MessageTree();
  TestMessage root(orig_tree);
  TestMessage sub(orig_tree, &root);

  MessageTree* detached_tree = nullptr;
  EXPECT_TRUE(orig_tree->guard().WithWrite([&](const WriteAccess& old_w) {
    WithSplitTree(orig_tree, old_w, /*src=*/nullptr,
                  [&](MessageTree* new_tree, const WriteAccess& new_w) {
                    detached_tree = new_tree;
                    sub.MoveToTree(new_tree, old_w, new_w);
                  });
  }));
  EXPECT_EQ(sub.GetTree(), detached_tree);

  // Accessing sub's Guarded<T> fields under detached_tree's guard succeeds.
  EXPECT_TRUE(sub.WithRead(false, [&](const ReadAccess& r) {
    return sub.TargetMessage(r) != nullptr;
  }));

  EXPECT_TRUE(detached_tree->guard().WithWrite([&](const WriteAccess& old_w) {
    EXPECT_TRUE(orig_tree->guard().WithWrite([&](const WriteAccess& new_w) {
      sub.MoveToTree(orig_tree, old_w, new_w);
    }));
  }));
  EXPECT_EQ(sub.GetTree(), orig_tree);

  detached_tree->Unref();
  orig_tree->Unref();
}

}  // namespace
}  // namespace python
}  // namespace protobuf
}  // namespace google
