// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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
//     * Neither the name of Google Inc. nor the names of its
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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// TODO(kenton):  Improve this unittest to bring it up to the standards of
//   other proto2 unittests.

#include <algorithm>

#include <google/protobuf/repeated_field.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {

// Test operations on a RepeatedField which is small enough that it does
// not allocate a separate array for storage.
TEST(RepeatedField, Small) {
  RepeatedField<int> field;

  EXPECT_EQ(field.size(), 0);

  field.Add(5);

  EXPECT_EQ(field.size(), 1);
  EXPECT_EQ(field.Get(0), 5);

  field.Add(42);

  EXPECT_EQ(field.size(), 2);
  EXPECT_EQ(field.Get(0), 5);
  EXPECT_EQ(field.Get(1), 42);

  field.Set(1, 23);

  EXPECT_EQ(field.size(), 2);
  EXPECT_EQ(field.Get(0), 5);
  EXPECT_EQ(field.Get(1), 23);

  field.RemoveLast();

  EXPECT_EQ(field.size(), 1);
  EXPECT_EQ(field.Get(0), 5);

  field.Clear();

  EXPECT_EQ(field.size(), 0);
}

// Test operations on a RepeatedField which is large enough to allocate a
// separate array.
TEST(RepeatedField, Large) {
  RepeatedField<int> field;

  for (int i = 0; i < 16; i++) {
    field.Add(i * i);
  }

  EXPECT_EQ(field.size(), 16);

  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field.Get(i), i * i);
  }
}

// Test swapping between various types of RepeatedFields.
TEST(RepeatedField, SwapSmallSmall) {
  RepeatedField<int> field1;
  RepeatedField<int> field2;

  field1.Add(5);
  field1.Add(42);

  field1.Swap(&field2);

  EXPECT_EQ(field1.size(), 0);
  EXPECT_EQ(field2.size(), 2);
  EXPECT_EQ(field2.Get(0), 5);
  EXPECT_EQ(field2.Get(1), 42);
}

TEST(RepeatedField, SwapLargeSmall) {
  RepeatedField<int> field1;
  RepeatedField<int> field2;

  for (int i = 0; i < 16; i++) {
    field1.Add(i * i);
  }
  field2.Add(5);
  field2.Add(42);
  field1.Swap(&field2);

  EXPECT_EQ(field1.size(), 2);
  EXPECT_EQ(field1.Get(0), 5);
  EXPECT_EQ(field1.Get(1), 42);
  EXPECT_EQ(field2.size(), 16);
  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field2.Get(i), i * i);
  }
}

TEST(RepeatedField, SwapLargeLarge) {
  RepeatedField<int> field1;
  RepeatedField<int> field2;

  field1.Add(5);
  field1.Add(42);
  for (int i = 0; i < 16; i++) {
    field1.Add(i);
    field2.Add(i * i);
  }
  field2.Swap(&field1);

  EXPECT_EQ(field1.size(), 16);
  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field1.Get(i), i * i);
  }
  EXPECT_EQ(field2.size(), 18);
  EXPECT_EQ(field2.Get(0), 5);
  EXPECT_EQ(field2.Get(1), 42);
  for (int i = 2; i < 18; i++) {
    EXPECT_EQ(field2.Get(i), i - 2);
  }
}

// Determines how much space was reserved by the given field by adding elements
// to it until it re-allocates its space.
static int ReservedSpace(RepeatedField<int>* field) {
  const int* ptr = field->data();
  do {
    field->Add(0);
  } while (field->data() == ptr);

  return field->size() - 1;
}

TEST(RepeatedField, ReserveMoreThanDouble) {
  // Reserve more than double the previous space in the field and expect the
  // field to reserve exactly the amount specified.
  RepeatedField<int> field;
  field.Reserve(20);

  EXPECT_EQ(20, ReservedSpace(&field));
}

TEST(RepeatedField, ReserveLessThanDouble) {
  // Reserve less than double the previous space in the field and expect the
  // field to grow by double instead.
  RepeatedField<int> field;
  field.Reserve(20);
  field.Reserve(30);

  EXPECT_EQ(40, ReservedSpace(&field));
}

TEST(RepeatedField, ReserveLessThanExisting) {
  // Reserve less than the previous space in the field and expect the
  // field to not re-allocate at all.
  RepeatedField<int> field;
  field.Reserve(20);
  const int* previous_ptr = field.data();
  field.Reserve(10);

  EXPECT_EQ(previous_ptr, field.data());
  EXPECT_EQ(20, ReservedSpace(&field));
}

TEST(RepeatedField, MergeFrom) {
  RepeatedField<int> source, destination;

  source.Add(4);
  source.Add(5);

  destination.Add(1);
  destination.Add(2);
  destination.Add(3);

  destination.MergeFrom(source);

  ASSERT_EQ(5, destination.size());

  EXPECT_EQ(1, destination.Get(0));
  EXPECT_EQ(2, destination.Get(1));
  EXPECT_EQ(3, destination.Get(2));
  EXPECT_EQ(4, destination.Get(3));
  EXPECT_EQ(5, destination.Get(4));
}

TEST(RepeatedField, MutableDataIsMutable) {
  RepeatedField<int> field;
  field.Add(1);
  EXPECT_EQ(1, field.Get(0));
  // The fact that this line compiles would be enough, but we'll check the
  // value anyway.
  *field.mutable_data() = 2;
  EXPECT_EQ(2, field.Get(0));
}

// ===================================================================
// RepeatedPtrField tests.  These pretty much just mirror the RepeatedField
// tests above.

TEST(RepeatedPtrField, Small) {
  RepeatedPtrField<string> field;

  EXPECT_EQ(field.size(), 0);

  field.Add()->assign("foo");

  EXPECT_EQ(field.size(), 1);
  EXPECT_EQ(field.Get(0), "foo");

  field.Add()->assign("bar");

  EXPECT_EQ(field.size(), 2);
  EXPECT_EQ(field.Get(0), "foo");
  EXPECT_EQ(field.Get(1), "bar");

  field.Mutable(1)->assign("baz");

  EXPECT_EQ(field.size(), 2);
  EXPECT_EQ(field.Get(0), "foo");
  EXPECT_EQ(field.Get(1), "baz");

  field.RemoveLast();

  EXPECT_EQ(field.size(), 1);
  EXPECT_EQ(field.Get(0), "foo");

  field.Clear();

  EXPECT_EQ(field.size(), 0);
}

TEST(RepeatedPtrField, Large) {
  RepeatedPtrField<string> field;

  for (int i = 0; i < 16; i++) {
    *field.Add() += 'a' + i;
  }

  EXPECT_EQ(field.size(), 16);

  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field.Get(i).size(), 1);
    EXPECT_EQ(field.Get(i)[0], 'a' + i);
  }
}

TEST(RepeatedPtrField, SwapSmallSmall) {
  RepeatedPtrField<string> field1;
  RepeatedPtrField<string> field2;

  field1.Add()->assign("foo");
  field1.Add()->assign("bar");
  field1.Swap(&field2);

  EXPECT_EQ(field1.size(), 0);
  EXPECT_EQ(field2.size(), 2);
  EXPECT_EQ(field2.Get(0), "foo");
  EXPECT_EQ(field2.Get(1), "bar");
}

TEST(RepeatedPtrField, SwapLargeSmall) {
  RepeatedPtrField<string> field1;
  RepeatedPtrField<string> field2;

  field2.Add()->assign("foo");
  field2.Add()->assign("bar");
  for (int i = 0; i < 16; i++) {
    *field1.Add() += 'a' + i;
  }
  field1.Swap(&field2);

  EXPECT_EQ(field1.size(), 2);
  EXPECT_EQ(field1.Get(0), "foo");
  EXPECT_EQ(field1.Get(1), "bar");
  EXPECT_EQ(field2.size(), 16);
  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field2.Get(i).size(), 1);
    EXPECT_EQ(field2.Get(i)[0], 'a' + i);
  }
}

TEST(RepeatedPtrField, SwapLargeLarge) {
  RepeatedPtrField<string> field1;
  RepeatedPtrField<string> field2;

  field1.Add()->assign("foo");
  field1.Add()->assign("bar");
  for (int i = 0; i < 16; i++) {
    *field1.Add() += 'A' + i;
    *field2.Add() += 'a' + i;
  }
  field2.Swap(&field1);

  EXPECT_EQ(field1.size(), 16);
  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(field1.Get(i).size(), 1);
    EXPECT_EQ(field1.Get(i)[0], 'a' + i);
  }
  EXPECT_EQ(field2.size(), 18);
  EXPECT_EQ(field2.Get(0), "foo");
  EXPECT_EQ(field2.Get(1), "bar");
  for (int i = 2; i < 18; i++) {
    EXPECT_EQ(field2.Get(i).size(), 1);
    EXPECT_EQ(field2.Get(i)[0], 'A' + i - 2);
  }
}

static int ReservedSpace(RepeatedPtrField<string>* field) {
  const string* const* ptr = field->data();
  do {
    field->Add();
  } while (field->data() == ptr);

  return field->size() - 1;
}

TEST(RepeatedPtrField, ReserveMoreThanDouble) {
  RepeatedPtrField<string> field;
  field.Reserve(20);

  EXPECT_EQ(20, ReservedSpace(&field));
}

TEST(RepeatedPtrField, ReserveLessThanDouble) {
  RepeatedPtrField<string> field;
  field.Reserve(20);
  field.Reserve(30);

  EXPECT_EQ(40, ReservedSpace(&field));
}

TEST(RepeatedPtrField, ReserveLessThanExisting) {
  RepeatedPtrField<string> field;
  field.Reserve(20);
  const string* const* previous_ptr = field.data();
  field.Reserve(10);

  EXPECT_EQ(previous_ptr, field.data());
  EXPECT_EQ(20, ReservedSpace(&field));
}

TEST(RepeatedPtrField, ReserveDoesntLoseAllocated) {
  // Check that a bug is fixed:  An earlier implementation of Reserve()
  // failed to copy pointers to allocated-but-cleared objects, possibly
  // leading to segfaults.
  RepeatedPtrField<string> field;
  string* first = field.Add();
  field.RemoveLast();

  field.Reserve(20);
  EXPECT_EQ(first, field.Add());
}

// Clearing elements is tricky with RepeatedPtrFields since the memory for
// the elements is retained and reused.
TEST(RepeatedPtrField, ClearedElements) {
  RepeatedPtrField<string> field;

  string* original = field.Add();
  *original = "foo";

  EXPECT_EQ(field.ClearedCount(), 0);

  field.RemoveLast();
  EXPECT_TRUE(original->empty());
  EXPECT_EQ(field.ClearedCount(), 1);

  EXPECT_EQ(field.Add(), original);  // Should return same string for reuse.

  EXPECT_EQ(field.ReleaseLast(), original);  // We take ownership.
  EXPECT_EQ(field.ClearedCount(), 0);

  EXPECT_NE(field.Add(), original);  // Should NOT return the same string.
  EXPECT_EQ(field.ClearedCount(), 0);

  field.AddAllocated(original);  // Give ownership back.
  EXPECT_EQ(field.ClearedCount(), 0);
  EXPECT_EQ(field.Mutable(1), original);

  field.Clear();
  EXPECT_EQ(field.ClearedCount(), 2);
  EXPECT_EQ(field.ReleaseCleared(), original);  // Take ownership again.
  EXPECT_EQ(field.ClearedCount(), 1);
  EXPECT_NE(field.Add(), original);
  EXPECT_EQ(field.ClearedCount(), 0);
  EXPECT_NE(field.Add(), original);
  EXPECT_EQ(field.ClearedCount(), 0);

  field.AddCleared(original);  // Give ownership back, but as a cleared object.
  EXPECT_EQ(field.ClearedCount(), 1);
  EXPECT_EQ(field.Add(), original);
  EXPECT_EQ(field.ClearedCount(), 0);
}

TEST(RepeatedPtrField, MergeFrom) {
  RepeatedPtrField<string> source, destination;

  source.Add()->assign("4");
  source.Add()->assign("5");

  destination.Add()->assign("1");
  destination.Add()->assign("2");
  destination.Add()->assign("3");

  destination.MergeFrom(source);

  ASSERT_EQ(5, destination.size());

  EXPECT_EQ("1", destination.Get(0));
  EXPECT_EQ("2", destination.Get(1));
  EXPECT_EQ("3", destination.Get(2));
  EXPECT_EQ("4", destination.Get(3));
  EXPECT_EQ("5", destination.Get(4));
}

TEST(RepeatedPtrField, MutableDataIsMutable) {
  RepeatedPtrField<string> field;
  *field.Add() = "1";
  EXPECT_EQ("1", field.Get(0));
  // The fact that this line compiles would be enough, but we'll check the
  // value anyway.
  string** data = field.mutable_data();
  **data = "2";
  EXPECT_EQ("2", field.Get(0));
}

// ===================================================================

// Iterator tests stolen from net/proto/proto-array_unittest.
class RepeatedFieldIteratorTest : public testing::Test {
 protected:
  virtual void SetUp() {
    for (int i = 0; i < 3; ++i) {
      proto_array_.Add(i);
    }
  }

  RepeatedField<int> proto_array_;
};

TEST_F(RepeatedFieldIteratorTest, Convertible) {
  RepeatedField<int>::iterator iter = proto_array_.begin();
  RepeatedField<int>::const_iterator c_iter = iter;
  EXPECT_EQ(0, *c_iter);
}

TEST_F(RepeatedFieldIteratorTest, MutableIteration) {
  RepeatedField<int>::iterator iter = proto_array_.begin();
  EXPECT_EQ(0, *iter);
  ++iter;
  EXPECT_EQ(1, *iter++);
  EXPECT_EQ(2, *iter);
  ++iter;
  EXPECT_TRUE(proto_array_.end() == iter);

  EXPECT_EQ(2, *(proto_array_.end() - 1));
}

TEST_F(RepeatedFieldIteratorTest, ConstIteration) {
  const RepeatedField<int>& const_proto_array = proto_array_;
  RepeatedField<int>::const_iterator iter = const_proto_array.begin();
  EXPECT_EQ(0, *iter);
  ++iter;
  EXPECT_EQ(1, *iter++);
  EXPECT_EQ(2, *iter);
  ++iter;
  EXPECT_TRUE(proto_array_.end() == iter);
  EXPECT_EQ(2, *(proto_array_.end() - 1));
}

TEST_F(RepeatedFieldIteratorTest, Mutation) {
  RepeatedField<int>::iterator iter = proto_array_.begin();
  *iter = 7;
  EXPECT_EQ(7, proto_array_.Get(0));
}

// -------------------------------------------------------------------

class RepeatedPtrFieldIteratorTest : public testing::Test {
 protected:
  virtual void SetUp() {
    proto_array_.Add()->assign("foo");
    proto_array_.Add()->assign("bar");
    proto_array_.Add()->assign("baz");
  }

  RepeatedPtrField<string> proto_array_;
};

TEST_F(RepeatedPtrFieldIteratorTest, Convertible) {
  RepeatedPtrField<string>::iterator iter = proto_array_.begin();
  RepeatedPtrField<string>::const_iterator c_iter = iter;
}

TEST_F(RepeatedPtrFieldIteratorTest, MutableIteration) {
  RepeatedPtrField<string>::iterator iter = proto_array_.begin();
  EXPECT_EQ("foo", *iter);
  ++iter;
  EXPECT_EQ("bar", *(iter++));
  EXPECT_EQ("baz", *iter);
  ++iter;
  EXPECT_TRUE(proto_array_.end() == iter);
  EXPECT_EQ("baz", *(--proto_array_.end()));
}

TEST_F(RepeatedPtrFieldIteratorTest, ConstIteration) {
  const RepeatedPtrField<string>& const_proto_array = proto_array_;
  RepeatedPtrField<string>::const_iterator iter = const_proto_array.begin();
  EXPECT_EQ("foo", *iter);
  ++iter;
  EXPECT_EQ("bar", *(iter++));
  EXPECT_EQ("baz", *iter);
  ++iter;
  EXPECT_TRUE(const_proto_array.end() == iter);
  EXPECT_EQ("baz", *(--const_proto_array.end()));
}

TEST_F(RepeatedPtrFieldIteratorTest, RandomAccess) {
  RepeatedPtrField<string>::iterator iter = proto_array_.begin();
  RepeatedPtrField<string>::iterator iter2 = iter;
  ++iter2;
  ++iter2;
  EXPECT_TRUE(iter + 2 == iter2);
  EXPECT_TRUE(iter == iter2 - 2);
  EXPECT_EQ("baz", iter[2]);
  EXPECT_EQ("baz", *(iter + 2));
  EXPECT_EQ(3, proto_array_.end() - proto_array_.begin());
}

TEST_F(RepeatedPtrFieldIteratorTest, Comparable) {
  RepeatedPtrField<string>::const_iterator iter = proto_array_.begin();
  RepeatedPtrField<string>::const_iterator iter2 = iter + 1;
  EXPECT_TRUE(iter == iter);
  EXPECT_TRUE(iter != iter2);
  EXPECT_TRUE(iter < iter2);
  EXPECT_TRUE(iter <= iter2);
  EXPECT_TRUE(iter <= iter);
  EXPECT_TRUE(iter2 > iter);
  EXPECT_TRUE(iter2 >= iter);
  EXPECT_TRUE(iter >= iter);
}

// Uninitialized iterator does not point to any of the RepeatedPtrField.
// Dereferencing an uninitialized iterator crashes the process.
TEST_F(RepeatedPtrFieldIteratorTest, UninitializedIterator) {
  RepeatedPtrField<string>::iterator iter;
  EXPECT_TRUE(iter != proto_array_.begin());
  EXPECT_TRUE(iter != proto_array_.begin() + 1);
  EXPECT_TRUE(iter != proto_array_.begin() + 2);
  EXPECT_TRUE(iter != proto_array_.begin() + 3);
  EXPECT_TRUE(iter != proto_array_.end());
#ifdef GTEST_HAS_DEATH_TEST
  ASSERT_DEATH(GOOGLE_LOG(INFO) << *iter, "");
#endif
}

TEST_F(RepeatedPtrFieldIteratorTest, STLAlgorithms_lower_bound) {
  proto_array_.Clear();
  proto_array_.Add()->assign("a");
  proto_array_.Add()->assign("c");
  proto_array_.Add()->assign("d");
  proto_array_.Add()->assign("n");
  proto_array_.Add()->assign("p");
  proto_array_.Add()->assign("x");
  proto_array_.Add()->assign("y");

  string v = "f";
  RepeatedPtrField<string>::const_iterator it =
      lower_bound(proto_array_.begin(), proto_array_.end(), v);
  EXPECT_EQ(*it, "n");
  EXPECT_TRUE(it == proto_array_.begin() + 3);
}

TEST_F(RepeatedPtrFieldIteratorTest, Mutation) {
  RepeatedPtrField<string>::iterator iter = proto_array_.begin();
  *iter = "qux";
  EXPECT_EQ("qux", proto_array_.Get(0));
}

}  // namespace protobuf
}  // namespace google
