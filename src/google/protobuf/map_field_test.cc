// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

#include <map>
#include <memory>
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/map.h>
#include <google/protobuf/map_unittest.pb.h>
#include <google/protobuf/map_test_util.h>
#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/map_field_inl.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {

namespace internal {

using unittest::TestAllTypes;

class MapFieldBaseStub : public MapFieldBase {
 public:
  void SyncRepeatedFieldWithMap() const {
    MapFieldBase::SyncRepeatedFieldWithMap();
  }
  void SyncMapWithRepeatedField() const {
    MapFieldBase::SyncMapWithRepeatedField();
  }
  // Get underlined repeated field without synchronizing map.
  RepeatedPtrField<Message>* InternalRepeatedField() {
    return repeated_field_;
  }
  // Get underlined map without synchronizing repeated field.
  template <typename MapType>
  const MapType& GetMap() {
    return *reinterpret_cast<MapType*>(base_map_);
  }
  // Get underlined map without synchronizing repeated field.
  template <typename MapType>
  MapType* MutableMap() {
    return reinterpret_cast<MapType*>(base_map_);
  }
  bool IsMapClean() { return state_ != 0; }
  bool IsRepeatedClean() { return state_ != 1; }
  void SetMapDirty() { state_ = 0; }
  void SetRepeatedDirty() { state_ = 1; }
};

class MapFieldBasePrimitiveTest : public ::testing::Test {
 protected:
  typedef MapField<int32, int32, FieldDescriptor::TYPE_INT32,
                   FieldDescriptor::TYPE_INT32> MapFieldType;

  MapFieldBasePrimitiveTest() {
    // Get descriptors
    map_descriptor_ = unittest::TestMap::descriptor()
                          ->FindFieldByName("map_int32_int32")
                          ->message_type();
    key_descriptor_ = map_descriptor_->FindFieldByName("key");
    value_descriptor_ = map_descriptor_->FindFieldByName("value");

    // Build map field
    default_entry_ =
        MessageFactory::generated_factory()->GetPrototype(map_descriptor_);
    map_field_.reset(new MapFieldType(default_entry_));
    map_field_base_ = map_field_.get();
    map_ = map_field_->MutableMap();
    initial_value_map_[0] = 100;
    initial_value_map_[1] = 101;
    map_->insert(initial_value_map_.begin(), initial_value_map_.end());
    EXPECT_EQ(2, map_->size());
  }

  google::protobuf::scoped_ptr<MapFieldType> map_field_;
  MapFieldBase* map_field_base_;
  Map<int32, int32>* map_;
  const Descriptor* map_descriptor_;
  const FieldDescriptor* key_descriptor_;
  const FieldDescriptor* value_descriptor_;
  const Message* default_entry_;
  std::map<int32, int32> initial_value_map_;  // copy of initial values inserted
};

TEST_F(MapFieldBasePrimitiveTest, SpaceUsedExcludingSelf) {
  EXPECT_LT(0, map_field_base_->SpaceUsedExcludingSelf());
}

TEST_F(MapFieldBasePrimitiveTest, GetRepeatedField) {
  const RepeatedPtrField<Message>& repeated =
      reinterpret_cast<const RepeatedPtrField<Message>&>(
          map_field_base_->GetRepeatedField());
  EXPECT_EQ(2, repeated.size());
  for (int i = 0; i < repeated.size(); i++) {
    const Message& message = repeated.Get(i);
    int key = message.GetReflection()->GetInt32(message, key_descriptor_);
    int value = message.GetReflection()->GetInt32(message, value_descriptor_);
    EXPECT_EQ(value, initial_value_map_[key]);
  }
}

TEST_F(MapFieldBasePrimitiveTest, MutableRepeatedField) {
  RepeatedPtrField<Message>* repeated =
      reinterpret_cast<RepeatedPtrField<Message>*>(
          map_field_base_->MutableRepeatedField());
  EXPECT_EQ(2, repeated->size());
  for (int i = 0; i < repeated->size(); i++) {
    const Message& message = repeated->Get(i);
    int key = message.GetReflection()->GetInt32(message, key_descriptor_);
    int value = message.GetReflection()->GetInt32(message, value_descriptor_);
    EXPECT_EQ(value, initial_value_map_[key]);
  }
}

namespace {
enum State { CLEAN, MAP_DIRTY, REPEATED_DIRTY };
}  // anonymous namespace

class MapFieldStateTest
    : public testing::TestWithParam<State> {
 public:
 protected:
  typedef MapField<int32, int32, FieldDescriptor::TYPE_INT32,
                FieldDescriptor::TYPE_INT32> MapFieldType;
  MapFieldStateTest() : state_(GetParam()) {
    // Build map field
    const Descriptor* map_descriptor =
        unittest::TestMap::descriptor()
            ->FindFieldByName("map_int32_int32")
            ->message_type();
    default_entry_ =
        MessageFactory::generated_factory()->GetPrototype(map_descriptor);
    map_field_.reset(new MapFieldType(default_entry_));
    map_field_base_ = map_field_.get();

    Expect(map_field_.get(), MAP_DIRTY, 0, 0, true);
    switch (state_) {
      case CLEAN:
        AddOneStillClean(map_field_.get());
        break;
      case MAP_DIRTY:
        MakeMapDirty(map_field_.get());
        break;
      case REPEATED_DIRTY:
        MakeRepeatedDirty(map_field_.get());
        break;
      default:
        break;
    }
  }

  void AddOneStillClean(MapFieldType* map_field) {
    MapFieldBase* map_field_base = map_field;
    Map<int32, int32>* map = map_field->MutableMap();
    (*map)[0] = 0;
    map_field_base->GetRepeatedField();
    Expect(map_field, CLEAN, 1, 1, false);
  }

  void MakeMapDirty(MapFieldType* map_field) {
    Map<int32, int32>* map = map_field->MutableMap();
    (*map)[0] = 0;
    Expect(map_field, MAP_DIRTY, 1, 0, true);
  }

  void MakeRepeatedDirty(MapFieldType* map_field) {
    MakeMapDirty(map_field);
    MapFieldBase* map_field_base = map_field;
    map_field_base->MutableRepeatedField();
    MapFieldBaseStub* stub =
        reinterpret_cast<MapFieldBaseStub*>(map_field_base);
    Map<int32, int32>* map = stub->MutableMap<Map<int32, int32> >();
    map->clear();

    Expect(map_field, REPEATED_DIRTY, 0, 1, false);
  }

  void Expect(MapFieldType* map_field, State state, int map_size,
              int repeated_size, bool is_repeated_null) {
    MapFieldBase* map_field_base = map_field;
    MapFieldBaseStub* stub =
        reinterpret_cast<MapFieldBaseStub*>(map_field_base);

    Map<int32, int32>* map = stub->MutableMap<Map<int32, int32> >();
    RepeatedPtrField<Message>* repeated_field = stub->InternalRepeatedField();

    switch (state) {
      case MAP_DIRTY:
        EXPECT_FALSE(stub->IsMapClean());
        EXPECT_TRUE(stub->IsRepeatedClean());
        break;
      case REPEATED_DIRTY:
        EXPECT_TRUE(stub->IsMapClean());
        EXPECT_FALSE(stub->IsRepeatedClean());
        break;
      case CLEAN:
        EXPECT_TRUE(stub->IsMapClean());
        EXPECT_TRUE(stub->IsRepeatedClean());
        break;
      default:
        FAIL();
    }

    EXPECT_EQ(map_size, map->size());
    if (is_repeated_null) {
      EXPECT_TRUE(repeated_field == NULL);
    } else {
      EXPECT_EQ(repeated_size, repeated_field->size());
    }
  }

  google::protobuf::scoped_ptr<MapFieldType> map_field_;
  MapFieldBase* map_field_base_;
  State state_;
  const Message* default_entry_;
};

INSTANTIATE_TEST_CASE_P(MapFieldStateTestInstance, MapFieldStateTest,
                        ::testing::Values(CLEAN, MAP_DIRTY, REPEATED_DIRTY));

TEST_P(MapFieldStateTest, GetMap) {
  map_field_->GetMap();
  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), CLEAN, 1, 1, false);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 1, 0, true);
  }
}

TEST_P(MapFieldStateTest, MutableMap) {
  map_field_->MutableMap();
  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), MAP_DIRTY, 1, 1, false);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 1, 0, true);
  }
}

TEST_P(MapFieldStateTest, MergeFromClean) {
  MapFieldType other(default_entry_);
  AddOneStillClean(&other);

  map_field_->MergeFrom(other);

  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), MAP_DIRTY, 1, 1, false);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 1, 0, true);
  }

  Expect(&other, CLEAN, 1, 1, false);
}

TEST_P(MapFieldStateTest, MergeFromMapDirty) {
  MapFieldType other(default_entry_);
  MakeMapDirty(&other);

  map_field_->MergeFrom(other);

  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), MAP_DIRTY, 1, 1, false);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 1, 0, true);
  }

  Expect(&other, MAP_DIRTY, 1, 0, true);
}

TEST_P(MapFieldStateTest, MergeFromRepeatedDirty) {
  MapFieldType other(default_entry_);
  MakeRepeatedDirty(&other);

  map_field_->MergeFrom(other);

  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), MAP_DIRTY, 1, 1, false);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 1, 0, true);
  }

  Expect(&other, CLEAN, 1, 1, false);
}

TEST_P(MapFieldStateTest, SwapClean) {
  MapFieldType other(default_entry_);
  AddOneStillClean(&other);

  map_field_->Swap(&other);

  Expect(map_field_.get(), CLEAN, 1, 1, false);

  switch (state_) {
    case CLEAN:
      Expect(&other, CLEAN, 1, 1, false);
      break;
    case MAP_DIRTY:
      Expect(&other, MAP_DIRTY, 1, 0, true);
      break;
    case REPEATED_DIRTY:
      Expect(&other, REPEATED_DIRTY, 0, 1, false);
      break;
    default:
      break;
  }
}

TEST_P(MapFieldStateTest, SwapMapDirty) {
  MapFieldType other(default_entry_);
  MakeMapDirty(&other);

  map_field_->Swap(&other);

  Expect(map_field_.get(), MAP_DIRTY, 1, 0, true);

  switch (state_) {
    case CLEAN:
      Expect(&other, CLEAN, 1, 1, false);
      break;
    case MAP_DIRTY:
      Expect(&other, MAP_DIRTY, 1, 0, true);
      break;
    case REPEATED_DIRTY:
      Expect(&other, REPEATED_DIRTY, 0, 1, false);
      break;
    default:
      break;
  }
}

TEST_P(MapFieldStateTest, SwapRepeatedDirty) {
  MapFieldType other(default_entry_);
  MakeRepeatedDirty(&other);

  map_field_->Swap(&other);

  Expect(map_field_.get(), REPEATED_DIRTY, 0, 1, false);

  switch (state_) {
    case CLEAN:
      Expect(&other, CLEAN, 1, 1, false);
      break;
    case MAP_DIRTY:
      Expect(&other, MAP_DIRTY, 1, 0, true);
      break;
    case REPEATED_DIRTY:
      Expect(&other, REPEATED_DIRTY, 0, 1, false);
      break;
    default:
      break;
  }
}

TEST_P(MapFieldStateTest, Clear) {
  map_field_->Clear();

  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), MAP_DIRTY, 0, 1, false);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 0, 0, true);
  }
}

TEST_P(MapFieldStateTest, SpaceUsedExcludingSelf) {
  map_field_base_->SpaceUsedExcludingSelf();

  switch (state_) {
    case CLEAN:
      Expect(map_field_.get(), CLEAN, 1, 1, false);
      break;
    case MAP_DIRTY:
      Expect(map_field_.get(), MAP_DIRTY, 1, 0, true);
      break;
    case REPEATED_DIRTY:
      Expect(map_field_.get(), REPEATED_DIRTY, 0, 1, false);
      break;
    default:
      break;
  }
}

TEST_P(MapFieldStateTest, GetMapField) {
  map_field_base_->GetRepeatedField();

  if (state_ != REPEATED_DIRTY) {
    Expect(map_field_.get(), CLEAN, 1, 1, false);
  } else {
    Expect(map_field_.get(), REPEATED_DIRTY, 0, 1, false);
  }
}

TEST_P(MapFieldStateTest, MutableMapField) {
  map_field_base_->MutableRepeatedField();

  if (state_ != REPEATED_DIRTY) {
    Expect(map_field_.get(), REPEATED_DIRTY, 1, 1, false);
  } else {
    Expect(map_field_.get(), REPEATED_DIRTY, 0, 1, false);
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
