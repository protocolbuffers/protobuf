// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/binary_test_util.h"

#include <set>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ElementsAreArray;
using ::testing::UnorderedElementsAreArray;

// Enumerates FieldDescriptor::Type independently of the table.
TEST(AllFieldTypesExceptGroupTest, IsEveryTypeButGroup) {
  std::set<FieldDescriptor::Type> all_but_group;
  for (int i = 1; i <= FieldDescriptor::MAX_TYPE; ++i) {
    all_but_group.insert(static_cast<FieldDescriptor::Type>(i));
  }
  all_but_group.erase(FieldDescriptor::TYPE_GROUP);
  EXPECT_THAT(AllFieldTypesExceptGroup(),
              UnorderedElementsAreArray(all_but_group));
}

TEST(PackableFieldTypesTest, IsThePackableSubsetInTableOrder) {
  std::vector<FieldDescriptor::Type> packable;
  for (FieldDescriptor::Type type : AllFieldTypesExceptGroup()) {
    if (FieldDescriptor::IsTypePackable(type)) packable.push_back(type);
  }
  EXPECT_THAT(PackableFieldTypes(), ElementsAreArray(packable));
}

TEST(LengthDelimitedFieldTypesTest, IsTheNonPackableSubsetInTableOrder) {
  std::vector<FieldDescriptor::Type> non_packable;
  for (FieldDescriptor::Type type : AllFieldTypesExceptGroup()) {
    if (!FieldDescriptor::IsTypePackable(type)) non_packable.push_back(type);
  }
  EXPECT_THAT(LengthDelimitedFieldTypes(), ElementsAreArray(non_packable));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
