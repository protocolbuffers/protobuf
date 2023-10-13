// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#include <type_traits>

#include <gtest/gtest.h>
#include "google/protobuf/arena.h"
#include "google/protobuf/unittest.pb.h"

namespace google {
namespace protobuf {
namespace {

using protobuf_unittest::TestAllTypes;

TEST(MessageConstructorTest, RegularCtor) {
  using T = protobuf_unittest::TestAllTypes;
  EXPECT_TRUE((std::is_constructible<T>::value));
}

TEST(MessageConstructorTest, RegularCopyCtor) {
  using T = protobuf_unittest::TestAllTypes;
  EXPECT_TRUE((std::is_constructible<T, const T&>::value));
}

TEST(MessageConstructorTest, ArenaCtor) {
  using T = protobuf_unittest::TestAllTypes;
  EXPECT_FALSE((std::is_constructible<T, Arena*>::value));
}

TEST(MessageConstructorTest, ArenaCopyCtor) {
  using T = protobuf_unittest::TestAllTypes;
  EXPECT_FALSE((std::is_constructible<T, Arena*, const T&>::value));
}

}  // namespace
}  // namespace protobuf
}  // namespace google
