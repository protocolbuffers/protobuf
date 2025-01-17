// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/hpb/tests/child_model.upb.proto.h"
#include "google/protobuf/compiler/hpb/tests/no_package.upb.proto.h"
#include "google/protobuf/compiler/hpb/tests/set_alias.upb.proto.h"
#include "google/protobuf/compiler/hpb/tests/test_extension.upb.proto.h"
#include "google/protobuf/compiler/hpb/tests/test_model.upb.proto.h"
#include "google/protobuf/hpb/arena.h"
#include "google/protobuf/hpb/backend/upb/interop.h"
#include "google/protobuf/hpb/hpb.h"
#include "google/protobuf/hpb/ptr.h"
#include "google/protobuf/hpb/requires.h"
#include "upb/mem/arena.hpp"

namespace {

using ::hpb::internal::Requires;
using ::hpb_unittest::protos::Child;
using ::hpb_unittest::protos::ChildModel1;
using ::hpb_unittest::protos::Parent;
using ::hpb_unittest::protos::ParentWithMap;
using ::hpb_unittest::protos::ParentWithRepeated;
using ::hpb_unittest::protos::RED;
using ::hpb_unittest::protos::TestEnum;
using ::hpb_unittest::protos::TestModel;
using ::hpb_unittest::protos::TestModel_Category;
using ::hpb_unittest::protos::TestModel_Category_IMAGES;
using ::hpb_unittest::protos::TestModel_Category_NEWS;
using ::hpb_unittest::protos::TestModel_Category_VIDEO;

TEST(CppGeneratedCode, Constructor) { TestModel test_model; }

TEST(CppGeneratedCode, MessageEnum) { EXPECT_EQ(5, TestModel_Category_IMAGES); }

TEST(CppGeneratedCode, ImportedEnum) { EXPECT_EQ(3, TestEnum::DEVICE_MONITOR); }

TEST(CppGeneratedCode, Enum) { EXPECT_EQ(1, RED); }

TEST(CppGeneratedCode, EnumNoPackage) { EXPECT_EQ(1, ::hpb_CELSIUS); }

TEST(CppGeneratedCode, MessageEnumType) {
  TestModel_Category category1 = TestModel_Category_IMAGES;
  TestModel::Category category2 = TestModel::IMAGES;
  EXPECT_EQ(category1, category2);
}

TEST(CppGeneratedCode, MessageEnumValue) {
  EXPECT_EQ(TestModel_Category_IMAGES, TestModel::IMAGES);
}

TEST(CppGeneratedCode, ArenaConstructor) {
  ::hpb::Arena arena;
  auto testModel = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(false, testModel.has_b1());
}

TEST(CppGeneratedCode, Booleans) {
  ::hpb::Arena arena;
  auto testModel = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_FALSE(testModel.b1());
  testModel.set_b1(true);
  EXPECT_TRUE(testModel.b1());
  testModel.set_b1(false);
  EXPECT_FALSE(testModel.b1());
  testModel.set_b1(true);
  EXPECT_TRUE(testModel.b1());
  testModel.clear_b1();
  EXPECT_FALSE(testModel.has_b1());
}

TEST(CppGeneratedCode, ScalarInt32) {
  ::hpb::Arena arena;
  auto testModel = ::hpb::CreateMessage<TestModel>(arena);
  // Test int32 defaults.
  EXPECT_EQ(testModel.value(), 0);
  EXPECT_FALSE(testModel.has_value());
  // Floating point defaults.
  EXPECT_EQ(std::numeric_limits<float>::infinity(),
            testModel.float_value_with_default());
  EXPECT_EQ(-std::numeric_limits<double>::infinity(),
            testModel.double_value_with_default());

  // Set value.
  testModel.set_value(5);
  EXPECT_TRUE(testModel.has_value());
  EXPECT_EQ(testModel.value(), 5);
  // Change value.
  testModel.set_value(10);
  EXPECT_TRUE(testModel.has_value());
  EXPECT_EQ(testModel.value(), 10);
  // Clear value.
  testModel.clear_value();
  EXPECT_FALSE(testModel.has_value());
  EXPECT_EQ(testModel.value(), 0);
}

const char kTestStr1[] = "abcdefg";
const char kTestStr2[] = "just another test string";

TEST(CppGeneratedCode, Strings) {
  TestModel testModel;
  testModel.set_str1(kTestStr1);
  testModel.set_str2(kTestStr2);
  EXPECT_EQ(testModel.str1(), kTestStr1);
  EXPECT_EQ(testModel.str2(), kTestStr2);
  EXPECT_TRUE(testModel.has_str1());
  EXPECT_TRUE(testModel.has_str2());

  testModel.clear_str1();
  EXPECT_FALSE(testModel.has_str1());
  EXPECT_TRUE(testModel.has_str2());
}

TEST(CppGeneratedCode, ScalarUInt32) {
  ::hpb::Arena arena;
  auto testModel = ::hpb::CreateMessage<TestModel>(arena);
  // Test defaults.
  EXPECT_EQ(testModel.optional_uint32(), 0);
  EXPECT_FALSE(testModel.has_optional_uint32());
  // Set value.
  testModel.set_optional_uint32(0xA0001000);
  EXPECT_TRUE(testModel.has_optional_uint32());
  EXPECT_EQ(testModel.optional_uint32(), 0xA0001000);
  // Change value.
  testModel.set_optional_uint32(0x70002000);
  EXPECT_TRUE(testModel.has_optional_uint32());
  EXPECT_EQ(testModel.optional_uint32(), 0x70002000);
  // Clear value.
  testModel.clear_optional_uint32();
  EXPECT_FALSE(testModel.has_optional_uint32());
  EXPECT_EQ(testModel.optional_uint32(), 0);
}

TEST(CppGeneratedCode, ScalarInt64) {
  ::hpb::Arena arena;
  auto testModel = ::hpb::CreateMessage<TestModel>(arena);
  // Test defaults.
  EXPECT_EQ(testModel.optional_int64(), 0);
  EXPECT_FALSE(testModel.has_optional_int64());
  // Set value.
  testModel.set_optional_int64(static_cast<int64_t>(0xFF00CCDDA0001000));
  EXPECT_TRUE(testModel.has_optional_int64());
  EXPECT_EQ(testModel.optional_int64(), 0xFF00CCDDA0001000);
  // Change value.
  testModel.set_optional_int64(static_cast<int64_t>(0xFF00CCDD70002000));
  EXPECT_TRUE(testModel.has_optional_int64());
  EXPECT_EQ(testModel.optional_int64(), 0xFF00CCDD70002000);
  // Clear value.
  testModel.clear_optional_int64();
  EXPECT_FALSE(testModel.has_optional_int64());
  EXPECT_EQ(testModel.optional_int64(), 0);
  // Set after clear.
  testModel.set_optional_int64(static_cast<int64_t>(0xFF00CCDDA0001000));
  EXPECT_TRUE(testModel.has_optional_int64());
  EXPECT_EQ(testModel.optional_int64(), 0xFF00CCDDA0001000);
}

TEST(CppGeneratedCode, ScalarFloat) {
  ::hpb::Arena arena;
  auto testModel = ::hpb::CreateMessage<TestModel>(arena);
  // Test defaults.
  EXPECT_EQ(testModel.optional_float(), 0.0f);
  EXPECT_FALSE(testModel.has_optional_float());
  EXPECT_EQ(std::numeric_limits<float>::infinity(),
            testModel.float_value_with_default());
  EXPECT_EQ(-std::numeric_limits<double>::infinity(),
            testModel.double_value_with_default());
  // Set value.
  testModel.set_optional_float(3.14159265f);
  EXPECT_TRUE(testModel.has_optional_float());
  EXPECT_NEAR(testModel.optional_float(), 3.14159265f, 1e-9f);
  // Change value.
  testModel.set_optional_float(-2.0f);
  EXPECT_TRUE(testModel.has_optional_float());
  EXPECT_NEAR(testModel.optional_float(), -2, 1e-9f);
  // Clear value.
  testModel.clear_optional_float();
  EXPECT_FALSE(testModel.has_optional_float());
  EXPECT_EQ(testModel.optional_float(), 0.0f);
  // Set after clear.
  testModel.set_optional_float(3.14159265f);
  EXPECT_TRUE(testModel.has_optional_float());
  EXPECT_NEAR(testModel.optional_float(), 3.14159265f, 1e-9f);
}

TEST(CppGeneratedCode, ScalarDouble) {
  ::hpb::Arena arena;
  auto testModel = ::hpb::CreateMessage<TestModel>(arena);
  // Test defaults.
  EXPECT_EQ(testModel.optional_double(), 0.0);
  EXPECT_FALSE(testModel.has_optional_double());
  // Set value.
  testModel.set_optional_double(3.141592653589793);
  EXPECT_TRUE(testModel.has_optional_double());
  EXPECT_NEAR(testModel.optional_double(), 3.141592653589793, 1e-16f);
  // Change value.
  testModel.set_optional_double(-1.0);
  EXPECT_TRUE(testModel.has_optional_double());
  EXPECT_NEAR(testModel.optional_double(), -1.0, 1e-16f);
  // Clear value.
  testModel.clear_optional_double();
  EXPECT_FALSE(testModel.has_optional_double());
  EXPECT_EQ(testModel.optional_double(), 0.0f);
  // Set after clear.
  testModel.set_optional_double(3.141592653589793);
  EXPECT_TRUE(testModel.has_optional_double());
  EXPECT_NEAR(testModel.optional_double(), 3.141592653589793, 1e-16f);
}

TEST(CppGeneratedCode, Enums) {
  ::hpb::Arena arena;
  auto testModel = ::hpb::CreateMessage<TestModel>(arena);

  // Check enum default value.
  EXPECT_EQ(TestModel_Category_IMAGES, 5);

  // Test defaults.
  EXPECT_FALSE(testModel.has_category());
  EXPECT_EQ(testModel.category(), TestModel_Category_IMAGES);
  // Set value.
  testModel.set_category(TestModel_Category_NEWS);
  EXPECT_TRUE(testModel.has_category());
  EXPECT_EQ(testModel.category(), TestModel_Category_NEWS);
  // Change value.
  testModel.set_category(TestModel_Category_VIDEO);
  EXPECT_TRUE(testModel.has_category());
  EXPECT_EQ(testModel.category(), TestModel_Category_VIDEO);
  // Clear value.
  testModel.clear_category();
  EXPECT_FALSE(testModel.has_category());
  EXPECT_EQ(testModel.category(), TestModel_Category_IMAGES);
  // Set after clear.
  testModel.set_category(TestModel_Category_VIDEO);
  EXPECT_TRUE(testModel.has_category());
  EXPECT_EQ(testModel.category(), TestModel_Category_VIDEO);
}

TEST(CppGeneratedCode, FieldWithDefaultValue) {
  ::hpb::Arena arena;
  auto testModel = ::hpb::CreateMessage<TestModel>(arena);

  EXPECT_FALSE(testModel.has_int_value_with_default());
  EXPECT_EQ(testModel.int_value_with_default(), 65);
  testModel.set_int_value_with_default(10);
  EXPECT_EQ(testModel.int_value_with_default(), 10);

  EXPECT_FALSE(testModel.has_string_value_with_default());
  EXPECT_EQ(testModel.string_value_with_default(), "hello");
  testModel.set_string_value_with_default("new string");
  EXPECT_EQ(testModel.string_value_with_default(), "new string");
}

TEST(CppGeneratedCode, OneOfFields) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);

  EXPECT_FALSE(test_model.has_oneof_member1());
  EXPECT_FALSE(test_model.has_oneof_member2());
  EXPECT_EQ(TestModel::CHILD_ONEOF1_NOT_SET, test_model.child_oneof1_case());

  test_model.set_oneof_member1("one of string");
  EXPECT_TRUE(test_model.has_oneof_member1());
  EXPECT_FALSE(test_model.has_oneof_member2());
  EXPECT_EQ(test_model.oneof_member1(), "one of string");
  EXPECT_EQ(TestModel::kOneofMember1, test_model.child_oneof1_case());

  test_model.set_oneof_member2(true);
  EXPECT_FALSE(test_model.has_oneof_member1());
  EXPECT_TRUE(test_model.has_oneof_member2());
  EXPECT_EQ(test_model.oneof_member2(), true);
  EXPECT_EQ(TestModel::kOneofMember2, test_model.child_oneof1_case());

  test_model.clear_oneof_member2();
  EXPECT_FALSE(test_model.has_oneof_member1());
  EXPECT_FALSE(test_model.has_oneof_member2());
  EXPECT_EQ(test_model.oneof_member1(), "");
  EXPECT_EQ(test_model.oneof_member2(), false);
  EXPECT_EQ(TestModel::CHILD_ONEOF1_NOT_SET, test_model.child_oneof1_case());
}

TEST(CppGeneratedCode, Messages) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(false, test_model.has_child_model_1());
  auto child_model = test_model.child_model_1();
  EXPECT_EQ(false, child_model->has_child_b1());
  EXPECT_EQ(false, child_model->child_b1());
  auto mutable_child = test_model.mutable_child_model_1();
  mutable_child->set_child_b1(true);
  EXPECT_EQ(true, mutable_child->has_child_b1());
  EXPECT_EQ(true, mutable_child->child_b1());
  // The View should not change due to mutation since it
  // is default_instance.
  EXPECT_EQ(false, child_model->has_child_b1());
  // Readonly View should now show change.
  child_model = test_model.child_model_1();
  EXPECT_EQ(true, child_model->has_child_b1());
  EXPECT_EQ(true, child_model->child_b1());
  // Clear message field.
  EXPECT_EQ(true, test_model.has_child_model_1());
  test_model.clear_child_model_1();
  EXPECT_EQ(false, test_model.has_child_model_1());
}

TEST(CppGeneratedCode, NestedMessages) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  auto nested_child = test_model.nested_child_1();
  EXPECT_EQ(0, nested_child->nested_child_name().size());
  auto mutable_nested_child = test_model.mutable_nested_child_1();
  EXPECT_EQ(false, mutable_nested_child->has_nested_child_name());
  mutable_nested_child->set_nested_child_name(kTestStr1);
  EXPECT_EQ(true, mutable_nested_child->has_nested_child_name());
}

TEST(CppGeneratedCode, MessageMapInt32KeyMessageValue) {
  const int key_test_value = 3;
  ::hpb::Arena arena;
  ::hpb::Arena child_arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.child_map_size());
  test_model.clear_child_map();
  EXPECT_EQ(0, test_model.child_map_size());
  auto child_model1 = ::hpb::CreateMessage<ChildModel1>(child_arena);
  child_model1.set_child_str1("abc");
  test_model.set_child_map(key_test_value, child_model1);
  auto map_result = test_model.get_child_map(key_test_value);
  EXPECT_EQ(true, map_result.ok());
  EXPECT_EQ("abc", map_result.value()->child_str1());
  // Now mutate original child model to verify that value semantics are
  // preserved.
  child_model1.set_child_str1("abc V2");
  EXPECT_EQ("abc", map_result.value()->child_str1());
  test_model.delete_child_map(key_test_value);
  auto map_result_after_delete = test_model.get_child_map(key_test_value);
  EXPECT_EQ(false, map_result_after_delete.ok());
}

TEST(CppGeneratedCode, MapMutableValue) {
  constexpr int key = 1;
  hpb::Arena arena;
  auto msg = hpb::CreateMessage<ParentWithMap>(arena);
  auto child = hpb::CreateMessage<Child>(arena);
  child.set_peeps(12);
  msg.set_child_map(key, child);
  auto const_map_result = msg.get_child_map(key);
  EXPECT_EQ(true, const_map_result.ok());
  EXPECT_EQ(12, const_map_result.value()->peeps());

  auto mut_map_result = msg.get_mutable_child_map(key);
  EXPECT_EQ(true, mut_map_result.ok());
  mut_map_result.value()->set_peeps(9001);
  EXPECT_EQ(9001, mut_map_result.value()->peeps());
}

TEST(CppGeneratedCode, MessageMapStringKeyAndStringValue) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.str_to_str_map_size());
  test_model.clear_str_to_str_map();
  EXPECT_EQ(0, test_model.str_to_str_map_size());
  test_model.set_str_to_str_map("first", "abc");
  test_model.set_str_to_str_map("second", "def");
  auto result = test_model.get_str_to_str_map("second");
  EXPECT_EQ(true, result.ok());
  EXPECT_EQ("def", result.value());
  test_model.delete_str_to_str_map("first");
  auto result_after_delete = test_model.get_str_to_str_map("first");
  EXPECT_EQ(false, result_after_delete.ok());
}

TEST(CppGeneratedCode, MessageMapStringKeyAndInt32Value) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.str_to_int_map_size());
  test_model.clear_str_to_int_map();
  EXPECT_EQ(0, test_model.str_to_int_map_size());
  test_model.set_str_to_int_map("first", 10);
  EXPECT_EQ(1, test_model.str_to_int_map_size());
  test_model.set_str_to_int_map("second", 20);
  EXPECT_EQ(2, test_model.str_to_int_map_size());
  auto result = test_model.get_str_to_int_map("second");
  EXPECT_EQ(true, result.ok());
  EXPECT_EQ(20, result.value());
  test_model.delete_str_to_int_map("first");
  auto result_after_delete = test_model.get_str_to_int_map("first");
  EXPECT_EQ(false, result_after_delete.ok());
}

TEST(CppGeneratedCode, SerializeUsingArena) {
  TestModel model;
  model.set_str1("Hello World");
  ::upb::Arena arena;
  absl::StatusOr<absl::string_view> bytes = ::hpb::Serialize(&model, arena);
  EXPECT_EQ(true, bytes.ok());
  TestModel parsed_model = ::hpb::Parse<TestModel>(bytes.value()).value();
  EXPECT_EQ("Hello World", parsed_model.str1());
}

TEST(CppGeneratedCode, SerializeProxyUsingArena) {
  ::upb::Arena message_arena;
  TestModel::Proxy model_proxy = ::hpb::CreateMessage<TestModel>(message_arena);
  model_proxy.set_str1("Hello World");
  ::upb::Arena arena;
  absl::StatusOr<absl::string_view> bytes =
      ::hpb::Serialize(&model_proxy, arena);
  EXPECT_EQ(true, bytes.ok());
  TestModel parsed_model = ::hpb::Parse<TestModel>(bytes.value()).value();
  EXPECT_EQ("Hello World", parsed_model.str1());
}

TEST(CppGeneratedCode, SerializeNestedMessageUsingArena) {
  TestModel model;
  model.mutable_recursive_child()->set_str1("Hello World");
  ::upb::Arena arena;
  ::hpb::Ptr<const TestModel> child = model.recursive_child();
  absl::StatusOr<absl::string_view> bytes = ::hpb::Serialize(child, arena);
  EXPECT_EQ(true, bytes.ok());
  TestModel parsed_model = ::hpb::Parse<TestModel>(bytes.value()).value();
  EXPECT_EQ("Hello World", parsed_model.str1());
}

TEST(CppGeneratedCode, NameCollisions) {
  TestModel model;
  model.set_template_("test");
  EXPECT_EQ("test", model.template_());
  model.set_arena__("test");
  EXPECT_EQ("test", model.arena__());
}

TEST(CppGeneratedCode, SharedPointer) {
  std::shared_ptr<TestModel> model = std::make_shared<TestModel>();
  ::upb::Arena arena;
  auto bytes = ::hpb::Serialize(model.get(), arena);
  EXPECT_TRUE(::hpb::Parse(model.get(), bytes.value()));
}

TEST(CppGeneratedCode, UniquePointer) {
  auto model = std::make_unique<TestModel>();
  ::upb::Arena arena;
  auto bytes = ::hpb::Serialize(model.get(), arena);
  EXPECT_TRUE(::hpb::Parse(model.get(), bytes.value()));
}

TEST(CppGeneratedCode, Assignment) {
  TestModel model;
  model.set_category(5);
  model.mutable_child_model_1()->set_child_str1("text in child");
  TestModel model2 = model;
  EXPECT_EQ(5, model2.category());
  EXPECT_EQ(model2.child_model_1()->child_str1(), "text in child");
}

TEST(CppGeneratedCode, PtrAssignment) {
  TestModel model;
  model.mutable_child_model_1()->set_child_str1("text in child");
  ChildModel1 child_from_const_ptr = *model.child_model_1();
  EXPECT_EQ(child_from_const_ptr.child_str1(), "text in child");
  ChildModel1 child_from_ptr = *model.mutable_child_model_1();
  EXPECT_EQ(child_from_ptr.child_str1(), "text in child");
}

TEST(CppGeneratedCode, CopyConstructor) {
  TestModel model;
  model.set_category(6);
  TestModel model2(model);
  EXPECT_EQ(6, model2.category());
}

TEST(CppGeneratedCode, PtrConstructor) {
  TestModel model;
  model.mutable_child_model_1()->set_child_str1("text in child");
  ChildModel1 child_from_ptr(*model.mutable_child_model_1());
  EXPECT_EQ(child_from_ptr.child_str1(), "text in child");
  ChildModel1 child_from_const_ptr(*model.child_model_1());
  EXPECT_EQ(child_from_const_ptr.child_str1(), "text in child");
}

TEST(CppGeneratedCode, MutableToProxy) {
  TestModel model;
  ::hpb::Ptr<ChildModel1> child = model.mutable_child_model_1();
  (void)child;
}

TEST(CppGeneratedCode, ProxyToCProxy) {
  TestModel model;
  ::hpb::Ptr<ChildModel1> child = model.mutable_child_model_1();
  ::hpb::Ptr<const ChildModel1> child2 = child;
  (void)child2;
}

TEST(CppGeneratedCode, MutableAccessorsAreHiddenInCProxy) {
  TestModel model;
  ::hpb::Ptr<TestModel> proxy = &model;
  ::hpb::Ptr<const TestModel> cproxy = proxy;

  const auto test_const_accessors = [](auto p) {
    // We don't want to run it, just check it compiles.
    if (false) {
      (void)p->has_str1();
      (void)p->str1();
      (void)p->has_value();
      (void)p->value();
      (void)p->has_oneof_member1();
      (void)p->oneof_member1();
      (void)p->value_array();
      (void)p->value_array_size();
      (void)p->value_array(1);
      (void)p->has_nested_child_1();
      (void)p->nested_child_1();
      (void)p->child_models();
      (void)p->child_models_size();
      (void)p->child_models(1);
      (void)p->child_map_size();
      (void)p->get_child_map(1);
    }
  };

  test_const_accessors(proxy);
  test_const_accessors(cproxy);

  const auto test_mutable_accessors = [](auto p, bool expected) {
    const auto r = [&](auto l) { return Requires<decltype(p)>(l) == expected; };
    EXPECT_TRUE(r([](auto p) -> decltype(p->set_str1("")) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->clear_str1()) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->set_value(1)) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->clear_value()) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->set_oneof_member1("")) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->clear_oneof_member1()) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->mutable_nested_child_1()) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->clear_nested_child_1()) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->add_value_array(1)) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->mutable_value_array()) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->resize_value_array(1)) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->set_value_array(1, 1)) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->add_child_models()) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->mutable_child_models(1)) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->clear_child_map()) {}));
    EXPECT_TRUE(r([](auto p) -> decltype(p->delete_child_map(1)) {}));
    EXPECT_TRUE(r(
        [](auto p) -> decltype(p->set_child_map(1, *p->get_child_map(1))) {}));
  };
  test_mutable_accessors(proxy, true);
  test_mutable_accessors(cproxy, false);
}

bool ProxyToCProxyMethod(::hpb::Ptr<const ChildModel1> child) {
  return child->child_str1() == "text in child";
}

TEST(CppGeneratedCode, PassProxyToCProxy) {
  TestModel model;
  model.mutable_child_model_1()->set_child_str1("text in child");
  EXPECT_TRUE(ProxyToCProxyMethod(model.mutable_child_model_1()));
}

TEST(CppGeneratedCode, PtrImplicitConversion) {
  TestModel model;
  model.set_int64(5);
  ::hpb::Ptr<TestModel> model_ptr = &model;
  EXPECT_EQ(model_ptr->int64(), 5);
}

TEST(CppGeneratedCode, CanInvokeClearMessageWithPtr) {
  // Fill model.
  TestModel model;
  model.set_int64(5);
  auto new_child = model.add_child_models();
  // Clear using Ptr<T>
  auto ptr = ::hpb::Ptr<TestModel>(&model);
  ::hpb::ClearMessage(ptr);
  // Successful clear
  EXPECT_FALSE(model.has_int64());
}

TEST(CppGeneratedCode, CanInvokeClearMessageWithRawPtr) {
  // Fill model.
  TestModel model;
  model.set_int64(5);
  auto new_child = model.add_child_models();
  // Clear using T*
  ::hpb::ClearMessage(&model);
  // Successful clear
  EXPECT_FALSE(model.has_int64());
}

template <typename T>
bool CanCallClearMessage() {
  return Requires<T>([](auto x) -> decltype(::hpb::ClearMessage(x)) {});
}

TEST(CppGeneratedCode, CannotInvokeClearMessageWithConstPtr) {
  EXPECT_TRUE(CanCallClearMessage<::hpb::Ptr<TestModel>>());
  EXPECT_FALSE(CanCallClearMessage<::hpb::Ptr<const TestModel>>());
}

TEST(CppGeneratedCode, CannotInvokeClearMessageWithConstRawPtr) {
  EXPECT_TRUE(CanCallClearMessage<TestModel*>());
  EXPECT_FALSE(CanCallClearMessage<const TestModel*>());
}

TEST(CppGeneratedCode, FieldNumberConstants) {
  static_assert(TestModel::kChildMapFieldNumber == 225);
  EXPECT_EQ(225, TestModel::kChildMapFieldNumber);
}

TEST(CppGeneratedCode, ClearConstMessageShouldFailForConstChild) {
  TestModel model;
  EXPECT_FALSE(CanCallClearMessage<decltype(model.child_model_1())>());
  EXPECT_TRUE(CanCallClearMessage<decltype(model.mutable_child_model_1())>());
}

TEST(CppGeneratedCode, CloneMessage) {
  hpb::Arena arena;
  TestModel model;
  model.set_str1("Hello World");
  auto ptr = hpb::Ptr<TestModel>(&model);
  hpb::Ptr<TestModel> cloned_model = hpb::CloneMessage(ptr, arena);
  EXPECT_EQ(cloned_model->str1(), "Hello World");
}

TEST(CppGeneratedCode, SetAlias) {
  hpb::Arena arena;
  auto child = hpb::CreateMessage<Child>(arena);
  child.set_peeps(12);
  auto parent1 = hpb::CreateMessage<Parent>(arena);
  auto parent2 = hpb::CreateMessage<Parent>(arena);
  parent1.set_alias_child(child);
  parent2.set_alias_child(child);

  ASSERT_EQ(parent1.child()->peeps(), parent2.child()->peeps());
  ASSERT_EQ(hpb::interop::upb::GetMessage(parent1.child()),
            hpb::interop::upb::GetMessage(parent2.child()));
  auto childPtr = hpb::Ptr<Child>(child);
  ASSERT_EQ(hpb::interop::upb::GetMessage(childPtr),
            hpb::interop::upb::GetMessage(parent1.child()));
}

TEST(CppGeneratedCode, SetAliasFieldsOutofOrder) {
  hpb::Arena arena;
  auto child = hpb::CreateMessage<Child>(arena);
  child.set_peeps(12);
  auto parent1 = hpb::CreateMessage<Parent>(arena);
  auto parent2 = hpb::CreateMessage<Parent>(arena);
  parent1.set_alias_child(child);
  parent2.set_alias_child(child);
  ASSERT_EQ(parent1.child()->peeps(), parent2.child()->peeps());
  ASSERT_EQ(parent1.child()->peeps(), 12);
}

TEST(CppGeneratedCode, SetAliasFailsForDifferentArena) {
  hpb::Arena arena;
  auto child = hpb::CreateMessage<Child>(arena);
  hpb::Arena different_arena;
  auto parent = hpb::CreateMessage<Parent>(different_arena);
  EXPECT_DEATH(parent.set_alias_child(child), "hpb::interop::upb::GetArena");
}

TEST(CppGeneratedCode, SetAliasSucceedsForDifferentArenaFused) {
  hpb::Arena arena;
  auto parent1 = hpb::CreateMessage<Parent>(arena);
  auto child = parent1.mutable_child();
  child->set_peeps(12);

  hpb::Arena other_arena;
  auto parent2 = hpb::CreateMessage<Parent>(other_arena);
  arena.Fuse(other_arena);

  parent2.set_alias_child(child);

  ASSERT_EQ(parent1.child()->peeps(), parent2.child()->peeps());
  ASSERT_EQ(hpb::interop::upb::GetMessage(parent1.child()),
            hpb::interop::upb::GetMessage(parent2.child()));
  auto childPtr = hpb::Ptr<Child>(child);
  ASSERT_EQ(hpb::interop::upb::GetMessage(childPtr),
            hpb::interop::upb::GetMessage(parent1.child()));
}

TEST(CppGeneratedCode, SetAliasRepeated) {
  hpb::Arena arena;
  auto child = hpb::CreateMessage<Child>(arena);
  child.set_peeps(1611);
  auto parent1 = hpb::CreateMessage<ParentWithRepeated>(arena);
  auto parent2 = hpb::CreateMessage<ParentWithRepeated>(arena);
  parent1.add_alias_children(child);
  parent2.add_alias_children(child);

  ASSERT_EQ(parent1.children(0)->peeps(), parent2.children(0)->peeps());
  ASSERT_EQ(hpb::interop::upb::GetMessage(parent1.children(0)),
            hpb::interop::upb::GetMessage(parent2.children(0)));
  auto childPtr = hpb::Ptr<Child>(child);
  ASSERT_EQ(hpb::interop::upb::GetMessage(childPtr),
            hpb::interop::upb::GetMessage(parent1.children(0)));
}

TEST(CppGeneratedCode, SetAliasRepeatedFailsForDifferentArena) {
  hpb::Arena arena;
  auto child = hpb::CreateMessage<Child>(arena);
  hpb::Arena different_arena;
  auto parent = hpb::CreateMessage<ParentWithRepeated>(different_arena);
  EXPECT_DEATH(parent.add_alias_children(child), "hpb::interop::upb::GetArena");
}

TEST(CppGeneratedCode, SetAliasMap) {
  hpb::Arena arena;
  auto parent1 = hpb::CreateMessage<ParentWithMap>(arena);
  auto parent2 = hpb::CreateMessage<ParentWithMap>(arena);

  auto child = hpb::CreateMessage<Child>(arena);

  constexpr int key = 1;
  parent1.set_alias_child_map(key, child);
  parent2.set_alias_child_map(key, child);
  auto c1 = parent1.get_child_map(key);
  auto c2 = parent2.get_child_map(key);

  EXPECT_TRUE(c1.ok());
  EXPECT_TRUE(c2.ok());
  ASSERT_EQ(hpb::interop::upb::GetMessage(c1.value()),
            hpb::interop::upb::GetMessage(c2.value()));
}

}  // namespace
