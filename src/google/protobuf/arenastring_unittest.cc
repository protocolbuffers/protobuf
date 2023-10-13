// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arenastring.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/explicitly_constructed.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

using internal::ArenaStringPtr;

const internal::LazyString nonempty_default{{{"default", 7}}, {nullptr}};
const std::string* empty_default = &internal::GetEmptyString();

class SingleArena : public testing::TestWithParam<bool> {
 public:
  std::unique_ptr<Arena> GetArena() {
    if (this->GetParam()) return nullptr;
    return std::unique_ptr<Arena>(new Arena());
  }
};

INSTANTIATE_TEST_SUITE_P(ArenaString, SingleArena, testing::Bool());

TEST_P(SingleArena, GetSet) {
  auto arena = GetArena();
  ArenaStringPtr field;
  field.InitDefault();
  EXPECT_EQ("", field.Get());
  field.Set("Test short", arena.get());
  EXPECT_EQ("Test short", field.Get());
  field.Set("Test long long long long value", arena.get());
  EXPECT_EQ("Test long long long long value", field.Get());
  field.Set("", arena.get());
  field.Destroy();
}

TEST_P(SingleArena, MutableAccessor) {
  auto arena = GetArena();
  ArenaStringPtr field;
  field.InitDefault();

  std::string* mut = field.Mutable(arena.get());
  EXPECT_EQ(mut, field.Mutable(arena.get()));
  EXPECT_EQ(mut, &field.Get());
  EXPECT_NE(empty_default, mut);
  EXPECT_EQ("", *mut);
  *mut = "Test long long long long value";  // ensure string allocates storage
  EXPECT_EQ("Test long long long long value", field.Get());
  field.Destroy();
}

TEST_P(SingleArena, NullDefault) {
  auto arena = GetArena();

  ArenaStringPtr field;
  field.InitDefault();
  std::string* mut = field.Mutable(nonempty_default, arena.get());
  EXPECT_EQ(mut, field.Mutable(nonempty_default, arena.get()));
  EXPECT_EQ(mut, &field.Get());
  EXPECT_NE(nullptr, mut);
  EXPECT_EQ("default", *mut);
  *mut = "Test long long long long value";  // ensure string allocates storage
  EXPECT_EQ("Test long long long long value", field.Get());
  field.Destroy();
}

TEST(ArenaStringPtrTest, ConstInit) {
  // Verify that we can constinit construct an ArenaStringPtr from an arbitrary
  // ExplicitlyConstructed<std::string>*.
  static internal::ExplicitlyConstructedArenaString str;
  PROTOBUF_CONSTINIT static ArenaStringPtr ptr(&str,
                                               internal::ConstantInitialized{});
  EXPECT_EQ(&ptr.Get(), str.get_mutable());

  PROTOBUF_CONSTINIT static const ArenaStringPtr ptr2(
      &internal::fixed_address_empty_string, internal::ConstantInitialized{});
  EXPECT_EQ(&ptr2.Get(), &internal::GetEmptyStringAlreadyInited());
}

TEST_P(SingleArena, ConstructEmpty) {
  auto arena = GetArena();
  ArenaStringPtr field(arena.get());

  EXPECT_EQ(field.Get(), "");
  if (internal::DebugHardenStringValues()) {
    EXPECT_FALSE(field.IsDefault());
  } else {
    EXPECT_TRUE(field.IsDefault());
  }
  if (arena == nullptr) field.Destroy();
}

TEST_P(SingleArena, ConstructEmptyWithDefault) {
  auto arena = GetArena();
  internal::LazyString default_value{{{"Hello default", 13}}, {nullptr}};
  ArenaStringPtr field(arena.get(), default_value);

  if (internal::DebugHardenStringValues()) {
    EXPECT_EQ(field.Get(), "Hello default");
    EXPECT_FALSE(field.IsDefault());
  } else {
    EXPECT_EQ(field.Get(), "");
    EXPECT_TRUE(field.IsDefault());
  }
  if (arena == nullptr) field.Destroy();
}

TEST_P(SingleArena, CopyConstructEmpty) {
  std::string empty;
  auto arena = GetArena();
  ArenaStringPtr field;
  field.InitExternal(&empty);

  ArenaStringPtr dst(arena.get(), field);
  EXPECT_EQ(dst.Get(), "");
  if (internal::DebugHardenStringValues()) {
    EXPECT_FALSE(dst.IsDefault());
  } else {
    EXPECT_TRUE(dst.IsDefault());
  }
  if (arena == nullptr) dst.Destroy();
  field.Destroy();
}

TEST_P(SingleArena, CopyConstructEmptyWithDefault) {
  std::string empty;
  auto arena = GetArena();
  ArenaStringPtr field;
  field.InitExternal(&empty);

  internal::LazyString default_value{{{"Hello default", 13}}, {nullptr}};
  ArenaStringPtr dst(arena.get(), field, default_value);
  if (internal::DebugHardenStringValues()) {
    EXPECT_EQ(dst.Get(), "Hello default");
    EXPECT_FALSE(dst.IsDefault());
  } else {
    EXPECT_EQ(dst.Get(), "");
    EXPECT_TRUE(dst.IsDefault());
  }
  if (arena == nullptr) dst.Destroy();
  field.Destroy();
}

TEST_P(SingleArena, CopyConstructValueWithDefault) {
  std::string empty;
  auto arena = GetArena();
  ArenaStringPtr field;
  field.InitExternal(&empty);
  field.Set("Hello world", nullptr);

  internal::LazyString default_value{{{"Hello default", 13}}, {nullptr}};
  ArenaStringPtr dst(arena.get(), field, default_value);
  EXPECT_EQ(dst.Get(), "Hello world");
  if (arena == nullptr) dst.Destroy();
  field.Destroy();
}

TEST_P(SingleArena, CopyConstructSSO) {
  std::string empty;
  auto arena = GetArena();
  ArenaStringPtr field;
  field.InitExternal(&empty);
  field.Set("Hello world", nullptr);

  ArenaStringPtr dst(arena.get(), field);
  EXPECT_EQ(dst.Get(), "Hello world");
  if (arena == nullptr) dst.Destroy();
  field.Destroy();
}

TEST_P(SingleArena, CopyConstructLong) {
  std::string empty;
  auto arena = GetArena();
  ArenaStringPtr field;
  field.InitExternal(&empty);
  field.Set("A string long enough to not be inlined", nullptr);

  ArenaStringPtr dst(arena.get(), field);
  EXPECT_EQ(dst.Get(), "A string long enough to not be inlined");
  if (arena == nullptr) dst.Destroy();
  field.Destroy();
}


}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
