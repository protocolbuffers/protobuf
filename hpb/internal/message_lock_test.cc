// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/hpb/internal/message_lock.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/hash/hash.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/compiler/hpb/tests/test_model.upb.proto.h"
#include "google/protobuf/hpb/extension.h"
#include "google/protobuf/hpb/hpb.h"
#include "upb/mem/arena.hpp"
#include "upb/mini_table/extension.h"

#ifndef ASSERT_OK
#define ASSERT_OK(x) ASSERT_TRUE(x.ok())
#endif  // ASSERT_OK
#ifndef EXPECT_OK
#define EXPECT_OK(x) EXPECT_TRUE(x.ok())
#endif  // EXPECT_OK

namespace hpb_unittest::protos {

namespace {

std::string GenerateTestData() {
  TestModel model;
  model.set_str1("str");
  ThemeExtension extension1;
  extension1.set_ext_name("theme");
  ABSL_CHECK_OK(::hpb::SetExtension(&model, theme, extension1));
  ThemeExtension extension2;
  extension2.set_ext_name("theme_extension");
  ABSL_CHECK_OK(
      ::hpb::SetExtension(&model, ThemeExtension::theme_extension, extension2));
  ::upb::Arena arena;
  auto bytes = ::hpb::Serialize(&model, arena);
  ABSL_CHECK_OK(bytes);
  return std::string(bytes->data(), bytes->size());
}

std::mutex m[8];
void unlock_func(const void* msg) { m[absl::HashOf(msg) & 0x7].unlock(); }
::hpb::internal::UpbExtensionUnlocker lock_func(const void* msg) {
  m[absl::HashOf(msg) & 0x7].lock();
  return &unlock_func;
}

void TestConcurrentExtensionAccess(::hpb::ExtensionRegistry registry) {
  ::hpb::internal::upb_extension_locker_global.store(&lock_func,
                                                     std::memory_order_release);
  const std::string payload = GenerateTestData();
  TestModel parsed_model = ::hpb::Parse<TestModel>(payload, registry).value();
  const auto test_main = [&] { EXPECT_EQ("str", parsed_model.str1()); };
  const auto test_theme = [&] {
    ASSERT_TRUE(::hpb::HasExtension(&parsed_model, theme));
    auto ext = hpb::GetExtension(&parsed_model, theme);
    ASSERT_OK(ext);
    EXPECT_EQ((*ext)->ext_name(), "theme");
  };
  const auto test_theme_extension = [&] {
    auto ext =
        hpb::GetExtension(&parsed_model, ThemeExtension::theme_extension);
    ASSERT_OK(ext);
    EXPECT_EQ((*ext)->ext_name(), "theme_extension");
  };
  const auto test_serialize = [&] {
    ::upb::Arena arena;
    EXPECT_OK(::hpb::Serialize(&parsed_model, arena));
  };
  const auto test_copy_constructor = [&] {
    TestModel copy_a = parsed_model;
    TestModel copy_b = parsed_model;
    EXPECT_EQ(copy_a.has_str1(), copy_b.has_str1());
  };
  std::thread t1(test_main);
  std::thread t2(test_main);
  std::thread t3(test_theme);
  std::thread t4(test_theme);
  std::thread t5(test_theme_extension);
  std::thread t6(test_theme_extension);
  std::thread t7(test_serialize);
  std::thread t8(test_copy_constructor);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
  t6.join();
  t7.join();
  t8.join();
  test_main();
  test_theme();
  test_theme_extension();
}

TEST(CppGeneratedCode, ConcurrentAccessDoesNotRaceBothLazy) {
  upb::Arena arena;
  hpb::ExtensionRegistry registry(arena);
  TestConcurrentExtensionAccess(registry);
}

TEST(CppGeneratedCode, ConcurrentAccessDoesNotRaceOneLazyOneEager) {
  upb::Arena arena;
  hpb::ExtensionRegistry r1(arena);
  r1.AddExtension(theme);
  TestConcurrentExtensionAccess(r1);
  hpb::ExtensionRegistry r2(arena);
  r2.AddExtension(ThemeExtension::theme_extension);
  TestConcurrentExtensionAccess(r2);
}

TEST(CppGeneratedCode, ConcurrentAccessDoesNotRaceBothEager) {
  upb::Arena arena;
  hpb::ExtensionRegistry registry(arena);
  registry.AddExtension(theme);
  registry.AddExtension(ThemeExtension::theme_extension);
  TestConcurrentExtensionAccess(registry);
}

TEST(CppGeneratedCode, ConcurrentAccessDoesNotRaceGlobalInstance) {
  upb::Arena arena;
  TestConcurrentExtensionAccess(hpb::ExtensionRegistry::generated_registry());
}

}  // namespace
}  // namespace hpb_unittest::protos
