// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/compiler/hpb/tests/child_model.upb.proto.h"
#include "google/protobuf/compiler/hpb/tests/test_extension.upb.proto.h"
#include "google/protobuf/compiler/hpb/tests/test_model.upb.proto.h"
#include "google/protobuf/hpb/requires.h"

namespace {

using ::hpb::internal::Requires;
using ::hpb_unittest::protos::ChildModel1;
using ::hpb_unittest::protos::TestModel;
using ::testing::ElementsAre;

const char kTestStr1[] = "abcdefg";
const char kTestStr2[] = "just another test string";

TEST(CppGeneratedCode, RepeatedMessages) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.child_models_size());
  // Should be able to clear repeated field when empty.
  test_model.mutable_child_models()->clear();
  EXPECT_EQ(0, test_model.child_models_size());
  // Add 2 children.
  auto new_child = test_model.add_child_models();
  EXPECT_EQ(true, new_child.ok());
  new_child.value()->set_child_str1(kTestStr1);
  new_child = test_model.add_child_models();
  EXPECT_EQ(true, new_child.ok());
  new_child.value()->set_child_str1(kTestStr2);
  EXPECT_EQ(2, test_model.child_models_size());
  // Mutable access.
  auto mutable_first = test_model.mutable_child_models(0);
  EXPECT_EQ(mutable_first->child_str1(), kTestStr1);
  mutable_first->set_child_str1("change1");
  auto mutable_second = test_model.mutable_child_models(1);
  EXPECT_EQ(mutable_second->child_str1(), kTestStr2);
  mutable_second->set_child_str1("change2");
  // Check mutations using views.
  auto view_first = test_model.child_models(0);
  EXPECT_EQ(view_first->child_str1(), "change1");
  auto view_second = test_model.child_models(1);
  EXPECT_EQ(view_second->child_str1(), "change2");
}

TEST(CppGeneratedCode, RepeatedScalar) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.value_array_size());
  // Should be able to clear repeated field when empty.
  test_model.mutable_value_array()->clear();
  EXPECT_EQ(0, test_model.value_array_size());
  // Add 2 children.
  EXPECT_EQ(true, test_model.add_value_array(5));
  EXPECT_EQ(true, test_model.add_value_array(6));
  EXPECT_EQ(2, test_model.value_array_size());
  EXPECT_EQ(5, test_model.value_array(0));
  EXPECT_EQ(6, test_model.value_array(1));
  EXPECT_EQ(true, test_model.resize_value_array(3));
  EXPECT_EQ(3, test_model.value_array_size());
  test_model.set_value_array(2, 7);
  EXPECT_EQ(5, test_model.value_array(0));
  EXPECT_EQ(6, test_model.value_array(1));
  EXPECT_EQ(7, test_model.value_array(2));
}

TEST(CppGeneratedCode, RepeatedFieldClear) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  test_model.mutable_value_array()->push_back(5);
  test_model.mutable_value_array()->push_back(16);
  test_model.mutable_value_array()->push_back(27);
  ASSERT_EQ(test_model.mutable_value_array()->size(), 3);
  test_model.mutable_value_array()->clear();
  EXPECT_EQ(test_model.mutable_value_array()->size(), 0);
}

TEST(CppGeneratedCode, RepeatedFieldProxyForScalars) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.value_array().size());
  EXPECT_EQ(0, test_model.mutable_value_array()->size());

  test_model.mutable_value_array()->push_back(5);
  test_model.mutable_value_array()->push_back(16);
  test_model.mutable_value_array()->push_back(27);

  ASSERT_EQ(test_model.mutable_value_array()->size(), 3);
  EXPECT_EQ((*test_model.mutable_value_array())[0], 5);
  EXPECT_EQ((*test_model.mutable_value_array())[1], 16);
  EXPECT_EQ((*test_model.mutable_value_array())[2], 27);

  const auto value_array = test_model.value_array();
  ASSERT_EQ(value_array.size(), 3);
  EXPECT_EQ(value_array[0], 5);
  EXPECT_EQ(value_array[1], 16);
  EXPECT_EQ(value_array[2], 27);

  EXPECT_THAT(value_array, ElementsAre(5, 16, 27));

  EXPECT_THAT(std::vector(value_array.begin(), value_array.end()),
              ElementsAre(5, 16, 27));
  EXPECT_THAT(std::vector(value_array.cbegin(), value_array.cend()),
              ElementsAre(5, 16, 27));
  EXPECT_THAT(std::vector(value_array.rbegin(), value_array.rend()),
              ElementsAre(27, 16, 5));
  EXPECT_THAT(std::vector(value_array.crbegin(), value_array.crend()),
              ElementsAre(27, 16, 5));
}

TEST(CppGeneratedCode, RepeatedScalarIterator) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  test_model.mutable_value_array()->push_back(5);
  test_model.mutable_value_array()->push_back(16);
  test_model.mutable_value_array()->push_back(27);
  int sum = 0;
  // Access by value.
  const ::hpb::RepeatedField<int32_t>::CProxy rep1 = test_model.value_array();
  for (auto i : rep1) {
    sum += i;
  }
  EXPECT_EQ(sum, 5 + 16 + 27);
  // Access by const reference.
  sum = 0;
  for (const auto& i : *test_model.mutable_value_array()) {
    sum += i;
  }
  EXPECT_EQ(sum, 5 + 16 + 27);
  // Access by forwarding reference.
  sum = 0;
  for (auto&& i : *test_model.mutable_value_array()) {
    sum += i;
  }
  EXPECT_EQ(sum, 5 + 16 + 27);
  // Test iterator operators.
  auto begin = test_model.value_array().begin();
  auto end = test_model.value_array().end();
  sum = 0;
  for (auto it = begin; it != end; ++it) {
    sum += *it;
  }
  EXPECT_EQ(sum, 5 + 16 + 27);
  auto it = begin;
  ++it;
  EXPECT_TRUE(begin < it);
  EXPECT_TRUE(begin <= it);
  it = end;
  EXPECT_TRUE(it == end);
  EXPECT_TRUE(it > begin);
  EXPECT_TRUE(it >= begin);
  EXPECT_TRUE(it != begin);
  // difference type
  it = end;
  --it;
  --it;
  EXPECT_EQ(end - it, 2);
  it = begin;
  EXPECT_EQ(it[0], 5);
  EXPECT_EQ(it[1], 16);
  EXPECT_EQ(it[2], 27);
  // ValueProxy.
  sum = 0;
  for (::hpb::RepeatedField<int32_t>::ValueCProxy c :
       test_model.value_array()) {
    sum += c;
  }
  EXPECT_EQ(sum, 5 + 16 + 27);
  sum = 0;
  for (::hpb::RepeatedField<int32_t>::ValueProxy c :
       *test_model.mutable_value_array()) {
    sum += c;
  }
  EXPECT_EQ(sum, 5 + 16 + 27);
}

TEST(CppGeneratedCode, RepeatedFieldProxyForStrings) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.repeated_string().size());
  EXPECT_EQ(0, test_model.mutable_repeated_string()->size());

  test_model.mutable_repeated_string()->push_back("a");
  test_model.mutable_repeated_string()->push_back("b");
  test_model.mutable_repeated_string()->push_back("c");

  ASSERT_EQ(test_model.repeated_string().size(), 3);
  EXPECT_EQ(test_model.repeated_string()[0], "a");
  EXPECT_EQ(test_model.repeated_string()[1], "b");
  EXPECT_EQ(test_model.repeated_string()[2], "c");

  EXPECT_THAT(test_model.repeated_string(), ElementsAre("a", "b", "c"));
  EXPECT_THAT(*test_model.mutable_repeated_string(),
              ElementsAre("a", "b", "c"));

  ASSERT_EQ(test_model.mutable_repeated_string()->size(), 3);
  EXPECT_EQ((*test_model.mutable_repeated_string())[0], "a");
  EXPECT_EQ((*test_model.mutable_repeated_string())[1], "b");
  EXPECT_EQ((*test_model.mutable_repeated_string())[2], "c");

  // The const accessor can't be used to modify the element
  EXPECT_FALSE((std::is_assignable<decltype(test_model.repeated_string()[1]),
                                   absl::string_view>::value));
  // But the mutable one is fine.
  (*test_model.mutable_repeated_string())[1] = "other";
  EXPECT_THAT(test_model.repeated_string(), ElementsAre("a", "other", "c"));

  test_model.mutable_repeated_string()->clear();
  EXPECT_EQ(test_model.mutable_repeated_string()->size(), 0);
}

TEST(CppGeneratedCode, RepeatedFieldProxyForMessages) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.child_models().size());
  ChildModel1 child1;
  child1.set_child_str1(kTestStr1);
  test_model.mutable_child_models()->push_back(child1);
  ChildModel1 child2;
  child2.set_child_str1(kTestStr2);
  test_model.mutable_child_models()->push_back(std::move(child2));

  int i = 0;
  for (auto child : test_model.child_models()) {
    EXPECT_FALSE(Requires<decltype(child)>(
        [](auto&& x) -> decltype(x.set_child_str1("")) {}));
    if (i++ == 0) {
      EXPECT_EQ(child.child_str1(), kTestStr1);
    } else {
      EXPECT_EQ(child.child_str1(), kTestStr2);
    }
  }

  i = 0;
  for (const auto& child : *test_model.mutable_child_models()) {
    if (i++ == 0) {
      EXPECT_EQ(child.child_str1(), kTestStr1);
    } else {
      EXPECT_EQ(child.child_str1(), kTestStr2);
    }
  }

  using testing::_;
  EXPECT_THAT(test_model.child_models(), ElementsAre(_, _));

  EXPECT_EQ(test_model.child_models().size(), 2);
  EXPECT_EQ(test_model.child_models()[0].child_str1(), kTestStr1);
  EXPECT_EQ(test_model.child_models()[1].child_str1(), kTestStr2);
  EXPECT_EQ((*test_model.mutable_child_models())[0].child_str1(), kTestStr1);
  EXPECT_EQ((*test_model.mutable_child_models())[1].child_str1(), kTestStr2);
  (*test_model.mutable_child_models())[0].set_child_str1("change1");
  EXPECT_EQ((*test_model.mutable_child_models())[0].child_str1(), "change1");
  test_model.mutable_child_models()->clear();
  EXPECT_EQ(test_model.mutable_child_models()->size(), 0);
}

TEST(CppGeneratedCode, EmptyRepeatedFieldProxyForMessages) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.child_models().size());
  ChildModel1 child1;
  child1.set_child_str1(kTestStr1);

  EXPECT_EQ(test_model.child_models().size(), 0);
  EXPECT_EQ(std::distance(test_model.child_models().begin(),
                          test_model.child_models().end()),
            0);
}

TEST(CppGeneratedCode, RepeatedFieldProxyForMessagesIndexOperator) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.child_models().size());
  ChildModel1 child1;
  child1.set_child_str1(kTestStr1);
  test_model.mutable_child_models()->push_back(child1);
  ChildModel1 child2;

  child2.set_child_str1(kTestStr2);
  test_model.mutable_child_models()->push_back(std::move(child2));
  ASSERT_EQ(test_model.child_models().size(), 2);

  // test_model.child_models()[0].set_child_str1("change1");
  (*test_model.mutable_child_models())[0].set_child_str1("change1");
  EXPECT_EQ((*test_model.mutable_child_models())[0].child_str1(), "change1");
}

TEST(CppGeneratedCode, RepeatedStrings) {
  ::hpb::Arena arena;
  auto test_model = ::hpb::CreateMessage<TestModel>(arena);
  EXPECT_EQ(0, test_model.repeated_string_size());
  // Should be able to clear repeated field when empty.
  test_model.mutable_repeated_string()->clear();
  EXPECT_EQ(0, test_model.repeated_string_size());
  // Add 2 children.
  EXPECT_EQ(true, test_model.add_repeated_string("Hello"));
  EXPECT_EQ(true, test_model.add_repeated_string("World"));
  EXPECT_EQ(2, test_model.repeated_string_size());
  EXPECT_EQ("Hello", test_model.repeated_string(0));
  EXPECT_EQ("World", test_model.repeated_string(1));
  EXPECT_EQ(true, test_model.resize_repeated_string(3));
  EXPECT_EQ(3, test_model.repeated_string_size());
  test_model.set_repeated_string(2, "Test");
  EXPECT_EQ("Hello", test_model.repeated_string(0));
  EXPECT_EQ("World", test_model.repeated_string(1));
  EXPECT_EQ("Test", test_model.repeated_string(2));
}


}  // namespace
