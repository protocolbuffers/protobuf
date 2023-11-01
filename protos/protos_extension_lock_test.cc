// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC nor the names of its
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

#include "protos/protos_extension_lock.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/hash/hash.h"
#include "absl/log/absl_check.h"
#include "protos/protos.h"
#include "protos_generator/tests/test_model.upb.proto.h"
#include "upb/mem/arena.hpp"

#ifndef ASSERT_OK
#define ASSERT_OK(x) ASSERT_TRUE(x.ok())
#endif  // ASSERT_OK
#ifndef EXPECT_OK
#define EXPECT_OK(x) EXPECT_TRUE(x.ok())
#endif  // EXPECT_OK

namespace protos_generator::test::protos {

namespace {

std::string GenerateTestData() {
  TestModel model;
  model.set_str1("str");
  ThemeExtension extension1;
  extension1.set_ext_name("theme");
  ABSL_CHECK_OK(::protos::SetExtension(&model, theme, extension1));
  ThemeExtension extension2;
  extension2.set_ext_name("theme_extension");
  ABSL_CHECK_OK(::protos::SetExtension(&model, ThemeExtension::theme_extension,
                                       extension2));
  ::upb::Arena arena;
  auto bytes = ::protos::Serialize(&model, arena);
  ABSL_CHECK_OK(bytes);
  return std::string(bytes->data(), bytes->size());
}

std::mutex m[8];
void unlock_func(const void* msg) { m[absl::HashOf(msg) & 0x7].unlock(); }
::protos::internal::UpbExtensionUnlocker lock_func(const void* msg) {
  m[absl::HashOf(msg) & 0x7].lock();
  return &unlock_func;
}

void TestConcurrentExtensionAccess(::protos::ExtensionRegistry registry) {
  ::protos::internal::upb_extension_locker_global.store(
      &lock_func, std::memory_order_release);
  const std::string payload = GenerateTestData();
  TestModel parsed_model =
      ::protos::Parse<TestModel>(payload, registry).value();
  const auto test_main = [&] { EXPECT_EQ("str", parsed_model.str1()); };
  const auto test_theme = [&] {
    ASSERT_TRUE(::protos::HasExtension(&parsed_model, theme));
    auto ext = ::protos::GetExtension(&parsed_model, theme);
    ASSERT_OK(ext);
    EXPECT_EQ((*ext)->ext_name(), "theme");
  };
  const auto test_theme_extension = [&] {
    auto ext =
        ::protos::GetExtension(&parsed_model, ThemeExtension::theme_extension);
    ASSERT_OK(ext);
    EXPECT_EQ((*ext)->ext_name(), "theme_extension");
  };
  const auto test_serialize = [&] {
    ::upb::Arena arena;
    EXPECT_OK(::protos::Serialize(&parsed_model, arena));
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
  ::upb::Arena arena;
  TestConcurrentExtensionAccess({{}, arena});
}

TEST(CppGeneratedCode, ConcurrentAccessDoesNotRaceOneLazyOneEager) {
  ::upb::Arena arena;
  TestConcurrentExtensionAccess({{&theme}, arena});
  TestConcurrentExtensionAccess({{&ThemeExtension::theme_extension}, arena});
}

TEST(CppGeneratedCode, ConcurrentAccessDoesNotRaceBothEager) {
  ::upb::Arena arena;
  TestConcurrentExtensionAccess(
      {{&theme, &ThemeExtension::theme_extension}, arena});
}

}  // namespace
}  // namespace protos_generator::test::protos
