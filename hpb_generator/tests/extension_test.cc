// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb/extension.h"

#include <cstdint>
#include <type_traits>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/hpb/tests/child_model.hpb.h"
#include "google/protobuf/compiler/hpb/tests/test_extension.hpb.h"
#include "google/protobuf/compiler/hpb/tests/test_model.hpb.h"
#include "hpb/arena.h"
#include "hpb/backend/upb/interop.h"
#include "hpb/hpb.h"
#include "hpb/requires.h"
#include "upb/mem/arena.h"

namespace {
using ::hpb::internal::Requires;

using ::hpb_unittest::protos::container_ext;
using ::hpb_unittest::protos::ContainerExtension;
using ::hpb_unittest::protos::TestModel;
using ::hpb_unittest::protos::theme;
using ::hpb_unittest::protos::ThemeExtension;
using ::hpb_unittest::someotherpackage::protos::bool_ext;
using ::hpb_unittest::someotherpackage::protos::double_ext;
using ::hpb_unittest::someotherpackage::protos::float_ext;
using ::hpb_unittest::someotherpackage::protos::int32_ext;
using ::hpb_unittest::someotherpackage::protos::int64_ext;
using ::hpb_unittest::someotherpackage::protos::repeated_int32_ext;
using ::hpb_unittest::someotherpackage::protos::repeated_int64_ext;
using ::hpb_unittest::someotherpackage::protos::repeated_string_ext;
using ::hpb_unittest::someotherpackage::protos::string_ext;
using ::hpb_unittest::someotherpackage::protos::string_trigraph_ext;
using ::hpb_unittest::someotherpackage::protos::uint32_ext;
using ::hpb_unittest::someotherpackage::protos::uint64_ext;

using ::testing::status::IsOkAndHolds;

TEST(CppGeneratedCode, HasExtension) {
  TestModel model;
  EXPECT_EQ(false, ::hpb::HasExtension(&model, theme));
}

TEST(CppGeneratedCode, HasExtensionPtr) {
  TestModel model;
  EXPECT_EQ(false, ::hpb::HasExtension(model.recursive_child(), theme));
}

TEST(CppGeneratedCode, ClearExtensionWithEmptyExtension) {
  TestModel model;
  EXPECT_EQ(false, ::hpb::HasExtension(&model, theme));
  ::hpb::ClearExtension(&model, theme);
  EXPECT_EQ(false, ::hpb::HasExtension(&model, theme));
}

TEST(CppGeneratedCode, ClearExtensionWithEmptyExtensionPtr) {
  TestModel model;
  ::hpb::Ptr<TestModel> recursive_child = model.mutable_recursive_child();
  ::hpb::ClearExtension(recursive_child, theme);
  EXPECT_EQ(false, ::hpb::HasExtension(recursive_child, theme));
}

TEST(CppGeneratedCode, GetSetExtensionInt32) {
  TestModel model;
  EXPECT_EQ(false, hpb::HasExtension(&model, int32_ext));
  int32_t val = 55;
  auto x = hpb::SetExtension(&model, int32_ext, val);
  EXPECT_EQ(true, hpb::HasExtension(&model, int32_ext));
  EXPECT_THAT(hpb::GetExtension(&model, int32_ext), IsOkAndHolds(val));
}

TEST(CppGeneratedCode, GetSetExtensionInt64) {
  TestModel model;
  EXPECT_EQ(false, hpb::HasExtension(&model, int64_ext));
  int64_t val = std::numeric_limits<int32_t>::max() + int64_t{1};
  auto x = hpb::SetExtension(&model, int64_ext, val);
  EXPECT_EQ(true, hpb::HasExtension(&model, int64_ext));
  EXPECT_THAT(hpb::GetExtension(&model, int64_ext), IsOkAndHolds(val));
}

TEST(CppGeneratedCode, GetSetExtensionUInt32) {
  TestModel model;
  EXPECT_EQ(false, hpb::HasExtension(&model, uint32_ext));
  uint32_t val = std::numeric_limits<int32_t>::max() + uint32_t{5};
  auto x = hpb::SetExtension(&model, uint32_ext, val);
  EXPECT_EQ(true, hpb::HasExtension(&model, uint32_ext));
  EXPECT_THAT(hpb::GetExtension(&model, uint32_ext), IsOkAndHolds(val));
}

TEST(CppGeneratedCode, GetSetExtensionUInt64) {
  TestModel model;
  EXPECT_EQ(false, hpb::HasExtension(&model, uint64_ext));
  uint64_t val = std::numeric_limits<int64_t>::max() + uint64_t{5};
  auto x = hpb::SetExtension(&model, uint64_ext, val);
  EXPECT_EQ(true, hpb::HasExtension(&model, uint64_ext));
  EXPECT_THAT(hpb::GetExtension(&model, uint64_ext), IsOkAndHolds(val));
}

TEST(CppGeneratedCode, GetSetExtensionFloat) {
  TestModel model;
  EXPECT_EQ(false, hpb::HasExtension(&model, float_ext));
  float val = 2.78;
  auto x = hpb::SetExtension(&model, float_ext, val);
  EXPECT_EQ(true, hpb::HasExtension(&model, float_ext));
  EXPECT_THAT(hpb::GetExtension(&model, float_ext), IsOkAndHolds(val));
}

TEST(CppGeneratedCode, GetSetExtensionDouble) {
  TestModel model;
  EXPECT_EQ(false, hpb::HasExtension(&model, double_ext));
  double val = std::numeric_limits<float>::max() + 1.23;
  auto x = hpb::SetExtension(&model, double_ext, val);
  EXPECT_EQ(true, hpb::HasExtension(&model, double_ext));
  EXPECT_THAT(hpb::GetExtension(&model, double_ext), IsOkAndHolds(val));
}

TEST(CppGeneratedCode, GetSetExtensionBool) {
  TestModel model;
  EXPECT_EQ(false, hpb::HasExtension(&model, bool_ext));
  auto x = hpb::SetExtension(&model, bool_ext, true);
  EXPECT_EQ(true, hpb::HasExtension(&model, bool_ext));
  EXPECT_THAT(hpb::GetExtension(&model, bool_ext), IsOkAndHolds(true));
}

TEST(CppGeneratedCode, GetSetExtensionString) {
  TestModel model;
  EXPECT_EQ(false, hpb::HasExtension(&model, string_ext));
  absl::string_view val = "Hello World";
  auto x = hpb::SetExtension(&model, string_ext, val);
  EXPECT_EQ(true, hpb::HasExtension(&model, string_ext));
  EXPECT_THAT(hpb::GetExtension(&model, string_ext), IsOkAndHolds(val));
}

TEST(CppGeneratedCode, SetExtension) {
  TestModel model;
  void* prior_message;
  {
    // Use a nested scope to make sure the arenas are fused correctly.
    ThemeExtension extension1;
    extension1.set_ext_name("Hello World");
    prior_message = hpb::interop::upb::GetMessage(&extension1);
    EXPECT_EQ(false, ::hpb::HasExtension(&model, theme));
    EXPECT_EQ(true,
              ::hpb::SetExtension(&model, theme, std::move(extension1)).ok());
  }
  EXPECT_EQ(true, ::hpb::HasExtension(&model, theme));
  auto ext = hpb::GetExtension(&model, theme);
  EXPECT_TRUE(ext.ok());
  EXPECT_EQ(hpb::interop::upb::GetMessage(*ext), prior_message);
}

TEST(CppGeneratedCode, SetExtensionWithPtr) {
  ::hpb::Arena arena_model;
  ::hpb::Ptr<TestModel> model = ::hpb::CreateMessage<TestModel>(arena_model);
  void* prior_message;
  {
    // Use a nested scope to make sure the arenas are fused correctly.
    ::hpb::Arena arena;
    ::hpb::Ptr<ThemeExtension> extension1 =
        ::hpb::CreateMessage<ThemeExtension>(arena);
    extension1->set_ext_name("Hello World");
    prior_message = hpb::interop::upb::GetMessage(extension1);
    EXPECT_EQ(false, ::hpb::HasExtension(model, theme));
    auto res = ::hpb::SetExtension(model, theme, extension1);
    EXPECT_EQ(true, res.ok());
  }
  EXPECT_EQ(true, ::hpb::HasExtension(model, theme));
  auto ext = hpb::GetExtension(model, theme);
  EXPECT_TRUE(ext.ok());
  EXPECT_NE(hpb::interop::upb::GetMessage(*ext), prior_message);
}

#ifndef _MSC_VER
TEST(CppGeneratedCode, SetExtensionShouldNotCompileForWrongType) {
  ::hpb::Arena arena;
  ::hpb::Ptr<TestModel> model = ::hpb::CreateMessage<TestModel>(arena);
  ThemeExtension extension1;
  ContainerExtension extension2;

  const auto canSetExtension = [&](auto l) {
    return Requires<decltype(model)>(l);
  };
  EXPECT_TRUE(canSetExtension(
      [](auto p) -> decltype(::hpb::SetExtension(p, theme, extension1)) {}));
  // Wrong extension value type should fail to compile.
  EXPECT_TRUE(!canSetExtension(
      [](auto p) -> decltype(::hpb::SetExtension(p, theme, extension2)) {}));
  // Wrong extension id with correct extension type should fail to compile.
  EXPECT_TRUE(
      !canSetExtension([](auto p) -> decltype(::hpb::SetExtension(
                                      p, container_ext, extension1)) {}));
}
#endif

TEST(CppGeneratedCode, SetExtensionWithPtrSameArena) {
  ::hpb::Arena arena;
  ::hpb::Ptr<TestModel> model = ::hpb::CreateMessage<TestModel>(arena);
  void* prior_message;
  {
    // Use a nested scope to make sure the arenas are fused correctly.
    ::hpb::Ptr<ThemeExtension> extension1 =
        ::hpb::CreateMessage<ThemeExtension>(arena);
    extension1->set_ext_name("Hello World");
    prior_message = hpb::interop::upb::GetMessage(extension1);
    EXPECT_EQ(false, ::hpb::HasExtension(model, theme));
    auto res = ::hpb::SetExtension(model, theme, extension1);
    EXPECT_EQ(true, res.ok());
  }
  EXPECT_EQ(true, ::hpb::HasExtension(model, theme));
  auto ext = hpb::GetExtension(model, theme);
  EXPECT_TRUE(ext.ok());
  EXPECT_NE(hpb::interop::upb::GetMessage(*ext), prior_message);
}

TEST(CppGeneratedCode, SetExtensionFusingFailureShouldCopy) {
  // Use an initial block to disallow fusing.
  char initial_block[1000];
  hpb::Arena arena(initial_block, sizeof(initial_block));

  hpb::Ptr<TestModel> model = ::hpb::CreateMessage<TestModel>(arena);

  ThemeExtension extension1;
  extension1.set_ext_name("Hello World");
  ASSERT_FALSE(upb_Arena_Fuse(hpb::interop::upb::UnwrapArena(arena),
                              hpb::interop::upb::GetArena(&extension1)));
  EXPECT_FALSE(::hpb::HasExtension(model, theme));
  auto status = ::hpb::SetExtension(model, theme, std::move(extension1));
  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(::hpb::HasExtension(model, theme));
  EXPECT_TRUE(hpb::GetExtension(model, theme).ok());
}

TEST(CppGeneratedCode, SetExtensionShouldClone) {
  TestModel model;
  ThemeExtension extension1;
  extension1.set_ext_name("Hello World");
  EXPECT_EQ(false, ::hpb::HasExtension(&model, theme));
  EXPECT_EQ(true, ::hpb::SetExtension(&model, theme, extension1).ok());
  extension1.set_ext_name("Goodbye");
  EXPECT_EQ(true, ::hpb::HasExtension(&model, theme));
  auto ext = hpb::GetExtension(&model, theme);
  EXPECT_TRUE(ext.ok());
  EXPECT_EQ((*ext)->ext_name(), "Hello World");
}

TEST(CppGeneratedCode, SetExtensionShouldCloneConst) {
  TestModel model;
  ThemeExtension extension1;
  extension1.set_ext_name("Hello World");
  EXPECT_EQ(false, ::hpb::HasExtension(&model, theme));
  EXPECT_EQ(true,
            ::hpb::SetExtension(&model, theme, std::as_const(extension1)).ok());
  extension1.set_ext_name("Goodbye");
  EXPECT_EQ(true, ::hpb::HasExtension(&model, theme));
  auto ext = hpb::GetExtension(&model, theme);
  EXPECT_TRUE(ext.ok());
  EXPECT_EQ((*ext)->ext_name(), "Hello World");
}

TEST(CppGeneratedCode, SetExtensionOnMutableChild) {
  TestModel model;
  ThemeExtension extension1;
  extension1.set_ext_name("Hello World");
  EXPECT_EQ(false, ::hpb::HasExtension(model.mutable_recursive_child(), theme));
  EXPECT_EQ(true, ::hpb::SetExtension(model.mutable_recursive_child(), theme,
                                      extension1)
                      .ok());
  EXPECT_EQ(true, ::hpb::HasExtension(model.mutable_recursive_child(), theme));
}

TEST(CppGeneratedCode, SetAliasExtensionOnMutableChild) {
  hpb::Arena arena;
  hpb::Ptr<TestModel> model = hpb::CreateMessage<TestModel>(arena);
  hpb::Ptr<ThemeExtension> extension1 =
      hpb::CreateMessage<ThemeExtension>(arena);
  extension1->set_ext_name("Hello World");
  EXPECT_EQ(false,
            ::hpb::HasExtension(model->mutable_recursive_child(), theme));
  ::hpb::SetAliasExtension(model->mutable_recursive_child(), theme, extension1);
  EXPECT_EQ(true, ::hpb::HasExtension(model->mutable_recursive_child(), theme));
}

TEST(CppGeneratedCode, SetAliasExtensionOnTwoParents) {
  hpb::Arena arena;
  hpb::Ptr<TestModel> model1 = hpb::CreateMessage<TestModel>(arena);
  hpb::Ptr<TestModel> model2 = hpb::CreateMessage<TestModel>(arena);
  hpb::Ptr<ThemeExtension> extension1 =
      hpb::CreateMessage<ThemeExtension>(arena);
  extension1->set_ext_name("Hello World");
  ::hpb::SetAliasExtension(model1->mutable_recursive_child(), theme,
                           extension1);
  ::hpb::SetAliasExtension(model2->mutable_recursive_child(), theme,
                           extension1);
  extension1->set_ext_name("Goodbye");
  EXPECT_EQ("Goodbye",
            hpb::GetExtension(model1->mutable_recursive_child(), theme)
                .value()
                ->ext_name());
  EXPECT_EQ("Goodbye",
            hpb::GetExtension(model2->mutable_recursive_child(), theme)
                .value()
                ->ext_name());
}

TEST(CppGeneratedCode, SetAliasExtensionOnDifferentArenaShouldCrash) {
  hpb::Arena arena1;
  hpb::Arena arena2;
  hpb::Ptr<TestModel> model = hpb::CreateMessage<TestModel>(arena1);
  hpb::Ptr<ThemeExtension> extension1 =
      hpb::CreateMessage<ThemeExtension>(arena2);
  extension1->set_ext_name("Hello World");
  EXPECT_DEATH(::hpb::SetAliasExtension(model->mutable_recursive_child(), theme,
                                        extension1),
               "");
}

TEST(CppGeneratedCode, GetExtension) {
  TestModel model;
  ThemeExtension extension1;
  extension1.set_ext_name("Hello World");
  EXPECT_EQ(false, ::hpb::HasExtension(&model, theme));
  EXPECT_EQ(true, ::hpb::SetExtension(&model, theme, extension1).ok());
  EXPECT_EQ("Hello World",
            hpb::GetExtension(&model, theme).value()->ext_name());
}

TEST(CppGeneratedCode, GetExtensionInt32WithDefault) {
  TestModel model;
  auto res = hpb::GetExtension(&model, int32_ext);
  EXPECT_TRUE(res.ok());
  EXPECT_EQ(*res, 644);
}

TEST(CppGeneratedCode, GetExtensionInt64WithDefault) {
  TestModel model;
  auto res = hpb::GetExtension(&model, int64_ext);
  EXPECT_TRUE(res.ok());
  int64_t expected = std::numeric_limits<int32_t>::max() + int64_t{1};
  EXPECT_EQ(*res, expected);
}

TEST(CppGeneratedCode, GetExtensionUInt32WithDefault) {
  TestModel model;
  auto res = hpb::GetExtension(&model, uint32_ext);
  EXPECT_THAT(res, IsOkAndHolds(12));
}

TEST(CppGeneratedCode, GetExtensionUInt64WithDefault) {
  TestModel model;
  auto res = hpb::GetExtension(&model, uint64_ext);
  EXPECT_THAT(res, IsOkAndHolds(4294967296));
}

TEST(CppGeneratedCode, GetExtensionFloatWithDefault) {
  TestModel model;
  auto res = hpb::GetExtension(&model, float_ext);
  static_assert(std::is_same_v<decltype(res), absl::StatusOr<float>>);
  EXPECT_THAT(res, IsOkAndHolds(3.14f));
}

TEST(CppGeneratedCode, GetExtensionDoubleWithDefault) {
  TestModel model;
  auto res = hpb::GetExtension(&model, double_ext);
  static_assert(std::is_same_v<decltype(res), absl::StatusOr<double>>);
  EXPECT_THAT(res, IsOkAndHolds(340282000000000000000000000000000000001.23));
}

TEST(CppGeneratedCode, GetExtensionBoolWithDefault) {
  TestModel model;
  auto res = hpb::GetExtension(&model, bool_ext);
  EXPECT_THAT(res, IsOkAndHolds(true));
}

TEST(CppGeneratedCode, GetExtensionStringWithDefault) {
  TestModel model;
  auto res = hpb::GetExtension(&model, string_ext);
  EXPECT_TRUE(res.ok());
  EXPECT_THAT(res, IsOkAndHolds("mishpacha"));
}

TEST(CppGeneratedCode, GetExtensionStringWithDefaultAndTrigraph) {
  TestModel model;
  auto res = hpb::GetExtension(&model, string_trigraph_ext);
  EXPECT_TRUE(res.ok());
  EXPECT_THAT(res, IsOkAndHolds("bseder??!bseder"));
}

TEST(CppGeneratedCode, GetExtensionOnMutableChild) {
  TestModel model;
  ThemeExtension extension1;
  extension1.set_ext_name("Hello World");
  ::hpb::Ptr<TestModel> mutable_recursive_child =
      model.mutable_recursive_child();
  EXPECT_EQ(false, ::hpb::HasExtension(mutable_recursive_child, theme));
  EXPECT_EQ(
      true,
      ::hpb::SetExtension(mutable_recursive_child, theme, extension1).ok());
  EXPECT_EQ(
      "Hello World",
      hpb::GetExtension(mutable_recursive_child, theme).value()->ext_name());
}

TEST(CppGeneratedCode, GetExtensionOnImmutableChild) {
  TestModel model;
  ThemeExtension extension1;
  extension1.set_ext_name("Hello World");
  ::hpb::Ptr<TestModel> mutable_recursive_child =
      model.mutable_recursive_child();
  EXPECT_EQ(false, ::hpb::HasExtension(mutable_recursive_child, theme));
  EXPECT_EQ(
      true,
      ::hpb::SetExtension(mutable_recursive_child, theme, extension1).ok());
  ::hpb::Ptr<const TestModel> recursive_child = model.recursive_child();
  EXPECT_EQ("Hello World",
            hpb::GetExtension(recursive_child, theme).value()->ext_name());
}

TEST(CppGeneratedCode, Parse) {
  TestModel model;
  model.set_str1("Test123");
  ThemeExtension extension1;
  extension1.set_ext_name("Hello World");
  EXPECT_EQ(true, ::hpb::SetExtension(&model, theme, extension1).ok());
  hpb::Arena arena;
  auto bytes = hpb::Serialize(&model, arena);
  EXPECT_EQ(true, bytes.ok());
  TestModel parsed_model = ::hpb::Parse<TestModel>(bytes.value()).value();
  EXPECT_EQ("Test123", parsed_model.str1());
  EXPECT_EQ(true, hpb::GetExtension(&parsed_model, theme).ok());
}

TEST(CppGeneratedCode, ParseIntoPtrToModel) {
  TestModel model;
  model.set_str1("Test123");
  ThemeExtension extension1;
  extension1.set_ext_name("Hello World");
  EXPECT_EQ(true, ::hpb::SetExtension(&model, theme, extension1).ok());
  hpb::Arena arena;
  auto bytes = ::hpb::Serialize(&model, arena);
  EXPECT_EQ(true, bytes.ok());
  ::hpb::Ptr<TestModel> parsed_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_TRUE(::hpb::Parse(parsed_model, bytes.value()));
  EXPECT_EQ("Test123", parsed_model->str1());
  // Should return an extension even if we don't pass ExtensionRegistry
  // by promoting unknown.
  EXPECT_EQ(true, hpb::GetExtension(parsed_model, theme).ok());
}

TEST(CppGeneratedCode, ParseWithExtensionRegistry) {
  TestModel model;
  model.set_str1("Test123");
  ThemeExtension extension1;
  extension1.set_ext_name("Hello World");
  EXPECT_EQ(true, ::hpb::SetExtension(&model, theme, extension1).ok());
  EXPECT_EQ(true, ::hpb::SetExtension(&model, ThemeExtension::theme_extension,
                                      extension1)
                      .ok());
  hpb::Arena arena;
  auto bytes = ::hpb::Serialize(&model, arena);
  EXPECT_EQ(true, bytes.ok());

  TestModel parsed_model =
      ::hpb::Parse<TestModel>(bytes.value(),
                              hpb::ExtensionRegistry::generated_registry())
          .value();
  EXPECT_EQ("Test123", parsed_model.str1());
  EXPECT_EQ(true, hpb::GetExtension(&parsed_model, theme).ok());
  EXPECT_EQ(
      true,
      hpb::GetExtension(&parsed_model, ThemeExtension::theme_extension).ok());
  EXPECT_EQ("Hello World",
            hpb::GetExtension(&parsed_model, ThemeExtension::theme_extension)
                .value()
                ->ext_name());
}

TEST(CppGeneratedCode, ClearSubMessage) {
  // Fill model.
  TestModel model;
  model.set_int64(5);
  auto new_child = model.mutable_child_model_1();
  new_child->set_child_str1("text in child");
  ThemeExtension extension1;
  extension1.set_ext_name("name in extension");
  EXPECT_TRUE(::hpb::SetExtension(&model, theme, extension1).ok());
  EXPECT_TRUE(model.mutable_child_model_1()->has_child_str1());
  // Clear using Ptr<T>
  ::hpb::ClearMessage(model.mutable_child_model_1());
  EXPECT_FALSE(model.mutable_child_model_1()->has_child_str1());
}

TEST(CppGeneratedCode, ClearMessage) {
  // Fill model.
  TestModel model;
  model.set_int64(5);
  model.set_str2("Hello");
  auto new_child = model.add_child_models();
  ASSERT_TRUE(new_child.ok());
  new_child.value()->set_child_str1("text in child");
  ThemeExtension extension1;
  extension1.set_ext_name("name in extension");
  EXPECT_TRUE(::hpb::SetExtension(&model, theme, extension1).ok());
  // Clear using T*
  ::hpb::ClearMessage(&model);
  // Verify that scalars, repeated fields and extensions are cleared.
  EXPECT_FALSE(model.has_int64());
  EXPECT_FALSE(model.has_str2());
  EXPECT_TRUE(model.child_models().empty());
  EXPECT_FALSE(::hpb::HasExtension(&model, theme));
}

TEST(CppGeneratedCode, DeepCopy) {
  // Fill model.
  TestModel model;
  model.set_int64(5);
  model.set_str2("Hello");
  auto new_child = model.add_child_models();
  ASSERT_TRUE(new_child.ok());
  new_child.value()->set_child_str1("text in child");
  ThemeExtension extension1;
  extension1.set_ext_name("name in extension");
  EXPECT_TRUE(::hpb::SetExtension(&model, theme, extension1).ok());
  TestModel target;
  target.set_b1(true);
  ::hpb::DeepCopy(&model, &target);
  EXPECT_FALSE(target.b1()) << "Target was not cleared before copying content ";
  EXPECT_EQ(target.str2(), "Hello");
  EXPECT_TRUE(::hpb::HasExtension(&target, theme));
}

TEST(CppGeneratedCode, HasExtensionAndRegistry) {
  // Fill model.
  TestModel source;
  source.set_int64(5);
  source.set_str2("Hello");
  auto new_child = source.add_child_models();
  ASSERT_TRUE(new_child.ok());
  new_child.value()->set_child_str1("text in child");
  ThemeExtension extension1;
  extension1.set_ext_name("name in extension");
  ASSERT_TRUE(::hpb::SetExtension(&source, theme, extension1).ok());

  // Now that we have a source model with extension data, serialize.
  ::hpb::Arena arena;
  std::string data = std::string(::hpb::Serialize(&source, arena).value());

  // Test with ExtensionRegistry
  TestModel parsed_model =
      ::hpb::Parse<TestModel>(data,
                              hpb::ExtensionRegistry::generated_registry())
          .value();
  EXPECT_TRUE(::hpb::HasExtension(&parsed_model, theme));
}

TEST(CppGeneratedCode, ExtensionFieldNumberConstant) {
  EXPECT_EQ(12003, ::hpb::ExtensionNumber(ThemeExtension::theme_extension));
}

TEST(CppGeneratedCode, GetExtensionRepeatedi32) {
  TestModel model;
  hpb::Arena arena;
  hpb::ExtensionRegistry extensions(arena);
  extensions.AddExtension(repeated_int32_ext);
  // These bytes are the serialized form of a repeated int32 field
  // with two elements: [2, 3] @index 13004
  auto bytes = "\342\254\006\002\002\003";
  auto parsed_model = hpb::Parse<TestModel>(bytes, extensions).value();
  auto res = hpb::GetExtension(&parsed_model, repeated_int32_ext);
  EXPECT_EQ(true, res.ok());
  EXPECT_EQ(res->size(), 2);
  EXPECT_EQ((*res)[0], 2);
  EXPECT_EQ((*res)[1], 3);
}

TEST(CppGeneratedCode, GetExtensionRepeatedi64) {
  TestModel model;
  hpb::Arena arena;
  hpb::ExtensionRegistry extensions(arena);
  extensions.AddExtension(repeated_int64_ext);
  // These bytes represent a repeated int64 field with one element: [322].
  auto bytes = "\352\254\006\002\302\002";
  auto parsed_model = hpb::Parse<TestModel>(bytes, extensions).value();
  auto res = hpb::GetExtension(&parsed_model, repeated_int64_ext);
  EXPECT_EQ(true, res.ok());
  EXPECT_EQ(res->size(), 1);
  EXPECT_EQ((*res)[0], 322);
}

TEST(CppGeneratedCode, GetExtensionSingularString) {
  TestModel model;
  hpb::Arena arena;
  hpb::ExtensionRegistry extensions(arena);
  extensions.AddExtension(string_ext);
  // These bytes represent a singular string field: "todaraba" @index 13012.
  auto bytes = "\242\255\006\010todaraba";
  auto parsed_model = hpb::Parse<TestModel>(bytes, extensions).value();
  auto res = hpb::GetExtension(&parsed_model, string_ext);
  EXPECT_THAT(res, IsOkAndHolds("todaraba"));
}

TEST(CppGeneratedCode, GetExtensionRepeatedString) {
  TestModel model;
  hpb::Arena arena;
  hpb::ExtensionRegistry extensions(arena);
  extensions.AddExtension(repeated_string_ext);
  // These bytes represent a repeated string field with two elements:
  // ["hello", "world"] @index 13006.
  auto bytes = "\362\254\006\005hello\362\254\006\005world";
  auto parsed_model = hpb::Parse<TestModel>(bytes, extensions).value();
  auto res = hpb::GetExtension(&parsed_model, repeated_string_ext);
  EXPECT_EQ(true, res.ok());
  EXPECT_EQ(res->size(), 2);
  EXPECT_EQ((*res)[0], "hello");
  EXPECT_EQ((*res)[1], "world");
}

TEST(CppGeneratedCode, ConstExprExtensionNumber) {
  constexpr auto ext_num = hpb::ExtensionNumber(int32_ext);
  EXPECT_EQ(ext_num, 13002);
}

}  // namespace
