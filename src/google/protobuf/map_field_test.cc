// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_format.h"
#include "absl/synchronization/barrier.h"
#include "absl/synchronization/blocking_counter.h"
#include "absl/types/optional.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_test_util.h"
#include "google/protobuf/map.h"
#include "google/protobuf/map_field_inl.h"
#include "google/protobuf/map_test_util.h"
#include "google/protobuf/map_unittest.pb.h"
#include "google/protobuf/message.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/wire_format_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

namespace internal {

using unittest::TestAllTypes;

struct MapFieldTestPeer {
  static auto GetArena(const RepeatedPtrFieldBase& v) { return v.GetArena(); }
  template <typename T>
  static auto& GetMap(T& t) {
    return t.map_;
  }
};

using TestMapField = ::google::protobuf::internal::MapField<
    unittest::TestMap_MapInt32Int32Entry_DoNotUse, ::int32_t, ::int32_t,
    ::google::protobuf::internal::WireFormatLite::TYPE_INT32,
    ::google::protobuf::internal::WireFormatLite::TYPE_INT32>;

class MapFieldBasePrimitiveTest : public testing::TestWithParam<bool> {
 protected:
  typedef unittest::TestMap_MapInt32Int32Entry_DoNotUse EntryType;
  typedef MapField<EntryType, int32_t, int32_t, WireFormatLite::TYPE_INT32,
                   WireFormatLite::TYPE_INT32>
      MapFieldType;

  MapFieldBasePrimitiveTest()
      : arena_(GetParam() ? new Arena() : nullptr),
        map_field_(arena_.get()),
        map_field_base_(map_field_.get()) {
    // Get descriptors
    map_descriptor_ = unittest::TestMap::descriptor()
                          ->FindFieldByName("map_int32_int32")
                          ->message_type();
    key_descriptor_ = map_descriptor_->map_key();
    value_descriptor_ = map_descriptor_->map_value();

    // Build map field
    map_field_base_ = map_field_.get();
    map_ = map_field_->MutableMap();
    initial_value_map_[0] = 100;
    initial_value_map_[1] = 101;
    map_->insert(initial_value_map_.begin(), initial_value_map_.end());
    EXPECT_EQ(2, map_->size());
  }

  std::unique_ptr<Arena> arena_;
  ArenaHolder<MapFieldType> map_field_;
  MapFieldBase* map_field_base_;
  Map<int32_t, int32_t>* map_;
  const Descriptor* map_descriptor_;
  const FieldDescriptor* key_descriptor_;
  const FieldDescriptor* value_descriptor_;
  absl::flat_hash_map<int32_t, int32_t>
      initial_value_map_;  // copy of initial values inserted
};

INSTANTIATE_TEST_SUITE_P(MapFieldBasePrimitiveTestInstance,
                         MapFieldBasePrimitiveTest,
                         testing::Values(true, false));

TEST_P(MapFieldBasePrimitiveTest, SpaceUsedExcludingSelf) {
  EXPECT_LT(0, map_field_base_->SpaceUsedExcludingSelf());
}

TEST_P(MapFieldBasePrimitiveTest, GetRepeatedField) {
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

TEST_P(MapFieldBasePrimitiveTest, MutableRepeatedField) {
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

TEST_P(MapFieldBasePrimitiveTest, Arena) {
  // Allocate a large initial block to avoid mallocs during hooked test.
  std::vector<char> arena_block(128 * 1024);
  ArenaOptions options;
  options.initial_block = &arena_block[0];
  options.initial_block_size = arena_block.size();
  Arena arena(options);

  {
    // TODO: Re-write the test to ensure the memory for the map and
    // repeated fields are allocated from arenas.
    // NoHeapChecker no_heap;

    MapFieldType* map_field = Arena::Create<MapFieldType>(&arena);

    // Set content in map
    (*map_field->MutableMap())[100] = 101;

    // Trigger conversion to repeated field.
    map_field->GetRepeatedField();
  }

  {
    // TODO: Re-write the test to ensure the memory for the map and
    // repeated fields are allocated from arenas.
    // NoHeapChecker no_heap;

    TestMapField* map_field = Arena::Create<TestMapField>(&arena);

    // Trigger conversion to repeated field.
    EXPECT_TRUE(map_field->MutableRepeatedField() != nullptr);

    EXPECT_EQ(MapFieldTestPeer::GetArena(map_field->GetRepeatedField()),
              &arena);
  }
}

TEST_P(MapFieldBasePrimitiveTest, EnforceNoArena) {
  std::unique_ptr<TestMapField> map_field(Arena::Create<TestMapField>(nullptr));
  EXPECT_EQ(MapFieldTestPeer::GetArena(map_field->GetRepeatedField()), nullptr);
}

namespace {
enum State { CLEAN, MAP_DIRTY, REPEATED_DIRTY };
}  // anonymous namespace

class MapFieldStateTest
    : public testing::TestWithParam<std::tuple<State, bool>> {
 protected:
  typedef unittest::TestMap_MapInt32Int32Entry_DoNotUse EntryType;
  typedef MapField<EntryType, int32_t, int32_t, WireFormatLite::TYPE_INT32,
                   WireFormatLite::TYPE_INT32>
      MapFieldType;
  MapFieldStateTest()
      : arena_(std::get<1>(GetParam()) ? new Arena() : nullptr),
        map_field_(arena_.get()),
        map_field_base_(map_field_.get()),
        state_(std::get<0>(GetParam())) {
    // Build map field
    Expect(map_field_.get(), MAP_DIRTY, 0, 0);
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
    Map<int32_t, int32_t>* map = map_field->MutableMap();
    (*map)[0] = 0;
    map_field_base->GetRepeatedField();
    Expect(map_field, CLEAN, 1, 1);
  }

  void MakeMapDirty(MapFieldType* map_field) {
    Map<int32_t, int32_t>* map = map_field->MutableMap();
    (*map)[0] = 0;
    Expect(map_field, MAP_DIRTY, 1, 0);
  }

  void MakeRepeatedDirty(MapFieldType* map_field) {
    MakeMapDirty(map_field);
    MapFieldBase* map_field_base = map_field;
    map_field_base->MutableRepeatedField();
    // We use map_ because we don't want to disturb the syncing
    map_field->map_.clear();

    Expect(map_field, REPEATED_DIRTY, 0, 1);
  }

  void Expect(MapFieldType* map_field, State state, int map_size,
              int repeated_size) {
    // We use map_ because we don't want to disturb the syncing
    Map<int32_t, int32_t>* map = &map_field->map_;

    switch (state) {
      case MAP_DIRTY:
        EXPECT_FALSE(map_field->state() != MapFieldType::STATE_MODIFIED_MAP);
        EXPECT_TRUE(map_field->state() !=
                    MapFieldType::STATE_MODIFIED_REPEATED);
        break;
      case REPEATED_DIRTY:
        EXPECT_TRUE(map_field->state() != MapFieldType::STATE_MODIFIED_MAP);
        EXPECT_FALSE(map_field->state() !=
                     MapFieldType::STATE_MODIFIED_REPEATED);
        break;
      case CLEAN:
        EXPECT_TRUE(map_field->state() != MapFieldType::STATE_MODIFIED_MAP);
        EXPECT_TRUE(map_field->state() !=
                    MapFieldType::STATE_MODIFIED_REPEATED);
        break;
      default:
        FAIL();
    }

    EXPECT_EQ(map_size, map->size());
    EXPECT_EQ(repeated_size,
              map_field->maybe_payload() == nullptr
                  ? 0
                  : map_field->maybe_payload()->repeated_field.size());
  }

  std::unique_ptr<Arena> arena_;
  ArenaHolder<MapFieldType> map_field_;
  MapFieldBase* map_field_base_;
  State state_;
};

INSTANTIATE_TEST_SUITE_P(MapFieldStateTestInstance, MapFieldStateTest,
                         testing::Combine(testing::Values(CLEAN, MAP_DIRTY,
                                                          REPEATED_DIRTY),
                                          testing::Values(true, false)));

TEST_P(MapFieldStateTest, GetMap) {
  map_field_->GetMap();
  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), CLEAN, 1, 1);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 1, 0);
  }
}

TEST_P(MapFieldStateTest, MutableMap) {
  map_field_->MutableMap();
  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), MAP_DIRTY, 1, 1);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 1, 0);
  }
}

TEST_P(MapFieldStateTest, MergeFromClean) {
  ArenaHolder<MapFieldType> other(arena_.get());
  AddOneStillClean(other.get());

  map_field_->MergeFrom(*other);

  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), MAP_DIRTY, 1, 1);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 1, 0);
  }

  Expect(other.get(), CLEAN, 1, 1);
}

TEST_P(MapFieldStateTest, MergeFromMapDirty) {
  ArenaHolder<MapFieldType> other(arena_.get());
  MakeMapDirty(other.get());

  map_field_->MergeFrom(*other);

  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), MAP_DIRTY, 1, 1);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 1, 0);
  }

  Expect(other.get(), MAP_DIRTY, 1, 0);
}

TEST_P(MapFieldStateTest, MergeFromRepeatedDirty) {
  ArenaHolder<MapFieldType> other(arena_.get());
  MakeRepeatedDirty(other.get());

  map_field_->MergeFrom(*other);

  if (state_ != MAP_DIRTY) {
    Expect(map_field_.get(), MAP_DIRTY, 1, 1);
  } else {
    Expect(map_field_.get(), MAP_DIRTY, 1, 0);
  }

  Expect(other.get(), CLEAN, 1, 1);
}

TEST_P(MapFieldStateTest, SwapClean) {
  ArenaHolder<MapFieldType> other(arena_.get());
  AddOneStillClean(other.get());

  map_field_->Swap(other.get());

  Expect(map_field_.get(), CLEAN, 1, 1);

  switch (state_) {
    case CLEAN:
      Expect(other.get(), CLEAN, 1, 1);
      break;
    case MAP_DIRTY:
      Expect(other.get(), MAP_DIRTY, 1, 0);
      break;
    case REPEATED_DIRTY:
      Expect(other.get(), REPEATED_DIRTY, 0, 1);
      break;
    default:
      break;
  }
}

TEST_P(MapFieldStateTest, SwapMapDirty) {
  ArenaHolder<MapFieldType> other(arena_.get());
  MakeMapDirty(other.get());

  map_field_->Swap(other.get());

  Expect(map_field_.get(), MAP_DIRTY, 1, 0);

  switch (state_) {
    case CLEAN:
      Expect(other.get(), CLEAN, 1, 1);
      break;
    case MAP_DIRTY:
      Expect(other.get(), MAP_DIRTY, 1, 0);
      break;
    case REPEATED_DIRTY:
      Expect(other.get(), REPEATED_DIRTY, 0, 1);
      break;
    default:
      break;
  }
}

TEST_P(MapFieldStateTest, SwapRepeatedDirty) {
  ArenaHolder<MapFieldType> other(arena_.get());
  MakeRepeatedDirty(other.get());

  map_field_->Swap(other.get());

  Expect(map_field_.get(), REPEATED_DIRTY, 0, 1);

  switch (state_) {
    case CLEAN:
      Expect(other.get(), CLEAN, 1, 1);
      break;
    case MAP_DIRTY:
      Expect(other.get(), MAP_DIRTY, 1, 0);
      break;
    case REPEATED_DIRTY:
      Expect(other.get(), REPEATED_DIRTY, 0, 1);
      break;
    default:
      break;
  }
}

TEST_P(MapFieldStateTest, Clear) {
  map_field_->Clear();

  Expect(map_field_.get(), MAP_DIRTY, 0, 0);
}

TEST_P(MapFieldStateTest, SpaceUsedExcludingSelf) {
  map_field_base_->SpaceUsedExcludingSelf();

  switch (state_) {
    case CLEAN:
      Expect(map_field_.get(), CLEAN, 1, 1);
      break;
    case MAP_DIRTY:
      Expect(map_field_.get(), MAP_DIRTY, 1, 0);
      break;
    case REPEATED_DIRTY:
      Expect(map_field_.get(), REPEATED_DIRTY, 0, 1);
      break;
    default:
      break;
  }
}

TEST_P(MapFieldStateTest, GetMapField) {
  map_field_base_->GetRepeatedField();

  if (state_ != REPEATED_DIRTY) {
    Expect(map_field_.get(), CLEAN, 1, 1);
  } else {
    Expect(map_field_.get(), REPEATED_DIRTY, 0, 1);
  }
}

TEST_P(MapFieldStateTest, MutableMapField) {
  map_field_base_->MutableRepeatedField();

  if (state_ != REPEATED_DIRTY) {
    Expect(map_field_.get(), REPEATED_DIRTY, 1, 1);
  } else {
    Expect(map_field_.get(), REPEATED_DIRTY, 0, 1);
  }
}

using MyMapField =
    MapField<unittest::TestMap_MapInt32Int32Entry_DoNotUse, int32_t, int32_t,
             internal::WireFormatLite::TYPE_INT32,
             internal::WireFormatLite::TYPE_INT32>;

TEST(MapFieldTest, ConstInit) {
  // This tests that `MapField` and all its base classes can be constant
  // initialized.
  PROTOBUF_CONSTINIT static MyMapField field;  // NOLINT
  EXPECT_EQ(field.size(), 0);
}

TEST(MapFieldTest, MutableMapDoesNotAllocatePayload) {
  struct MaybePayload : MapFieldBase {
    // Use a derived type to get access to the protected method.
    // We steal the function pointer here to use below to inspect the instance.
    static constexpr auto getter() { return &MaybePayload::maybe_payload; }
  };
  MyMapField field;
  EXPECT_FALSE((field.*MaybePayload::getter())());
  field.MutableMap();
  EXPECT_FALSE((field.*MaybePayload::getter())());
}


}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
