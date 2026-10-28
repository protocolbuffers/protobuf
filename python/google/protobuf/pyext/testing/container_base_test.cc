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
    SetTree(t);
    if (p == nullptr) {
      owned_proto = std::make_unique<DescriptorProto>();
      message.SetQuiescent(owned_proto.get());
    } else {
      message.SetQuiescent(&unpromoted_options);
    }
  }

  const TestMessage* Owner(const ReadAccess&) const { return this; }
  TestMessage* Owner(const WriteAccess&) { return this; }

  void MoveNodeToTree(MessageTree* new_tree, const WriteAccess& old_w,
                      const WriteAccess& new_w) {
    parent_node.UpdateGuard(old_w, new_w);
    message.UpdateGuard(old_w, new_w);
    is_mutable.UpdateGuard(old_w, new_w);
    is_frozen.UpdateGuard(old_w, new_w);
    SetTree(new_tree);
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

struct TestContainer : public TreeLockable<TestContainer> {
  Guarded<TestMessage*> parent_msg;

  explicit TestContainer(TestMessage* p)
      : parent_msg(p, p->GetTree()->guard()) {
    SetTree(p->GetTree());
  }

  const TestMessage* Owner(const ReadAccess& access) const {
    return parent_msg.Get(access);
  }
  TestMessage* Owner(const WriteAccess& access) {
    return parent_msg.Get(access);
  }
};

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
        EXPECT_NE(static_cast<const ReadAccess*>(&wa), &rb);
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

  // Same tree (a and its submessage): shares a single lock token.
  TestMessage sub(tree_a, &a);
  EXPECT_TRUE(TestMessage::WithWriteRead(
      &sub, &a, [&](const WriteAccess& w_sub, const ReadAccess& r_a) {
        EXPECT_EQ(static_cast<const ReadAccess*>(&w_sub), &r_a);
        static_cast<MessageOptions*>(sub.TargetMessage(w_sub))
            ->set_deprecated(
                !static_cast<const DescriptorProto*>(a.TargetMessage(r_a))
                     ->name()
                     .empty());
      }));

  tree_a->Unref();
  tree_b->Unref();
}

TEST_F(ContainerBaseTest, MoveToTreeRebindsGuardedFields) {
  MessageTree* orig_tree = new MessageTree();
  TestMessage root(orig_tree);
  TestMessage sub(orig_tree, &root);

  MessageTree* detached_tree = new MessageTree(orig_tree);
  EXPECT_TRUE(orig_tree->guard().WithWrite([&](const WriteAccess& old_w) {
    EXPECT_TRUE(detached_tree->guard().WithWrite([&](const WriteAccess& new_w) {
      sub.MoveNodeToTree(detached_tree, old_w, new_w);
    }));
  }));
  EXPECT_EQ(sub.GetTree(), detached_tree);

  // Accessing sub's Guarded<T> fields under detached_tree's guard succeeds.
  EXPECT_TRUE(sub.WithRead(false, [&](const ReadAccess& r) {
    return sub.TargetMessage(r) != nullptr;
  }));

  EXPECT_TRUE(detached_tree->guard().WithWrite([&](const WriteAccess& old_w) {
    EXPECT_TRUE(orig_tree->guard().WithWrite([&](const WriteAccess& new_w) {
      sub.MoveNodeToTree(orig_tree, old_w, new_w);
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
