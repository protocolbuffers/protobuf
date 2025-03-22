// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// This test is testing a lot more than just the UnknownFieldSet class.  It
// tests handling of unknown fields throughout the system.

#include "google/protobuf/unknown_field_set.h"

#include <cstddef>
#include <string>
#include <vector>

#include "google/protobuf/stubs/callback.h"
#include "google/protobuf/stubs/common.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_set.h"
#include "absl/functional/bind_front.h"
#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/port.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_lite.pb.h"
#include "google/protobuf/wire_format.h"

namespace google {
namespace protobuf {
namespace internal {
struct UnknownFieldSetTestPeer {
  static auto AddLengthDelimited(UnknownFieldSet& set, int number) {
    return set.AddLengthDelimited(number);
  }
};
}  // namespace internal

using internal::WireFormat;
using ::testing::ElementsAre;

template <typename T>
T UnknownToProto(const UnknownFieldSet& set) {
  T message;
  std::string serialized_message;
  ABSL_CHECK(set.SerializeToString(&serialized_message));
  ABSL_CHECK(message.ParseFromString(serialized_message));
  return message;
}

class UnknownFieldSetTest : public testing::Test {
 protected:
  void SetUp() override {
    descriptor_ = unittest::TestAllTypes::descriptor();
    TestUtil::SetAllFields(&all_fields_);
    all_fields_.SerializeToString(&all_fields_data_);
    ASSERT_TRUE(empty_message_.ParseFromString(all_fields_data_));
    unknown_fields_ = empty_message_.mutable_unknown_fields();
  }

  const UnknownField* GetField(const std::string& name) {
    const FieldDescriptor* field = descriptor_->FindFieldByName(name);
    if (field == nullptr) return nullptr;
    for (int i = 0; i < unknown_fields_->field_count(); i++) {
      if (unknown_fields_->field(i).number() == field->number()) {
        return &unknown_fields_->field(i);
      }
    }
    return nullptr;
  }

  // Constructs a protocol buffer which contains fields with all the same
  // numbers as all_fields_data_ except that each field is some other wire
  // type.
  std::string GetBizarroData() {
    unittest::TestEmptyMessage bizarro_message;
    UnknownFieldSet* bizarro_unknown_fields =
        bizarro_message.mutable_unknown_fields();
    for (int i = 0; i < unknown_fields_->field_count(); i++) {
      const UnknownField& unknown_field = unknown_fields_->field(i);
      if (unknown_field.type() == UnknownField::TYPE_VARINT) {
        bizarro_unknown_fields->AddFixed32(unknown_field.number(), 1);
      } else {
        bizarro_unknown_fields->AddVarint(unknown_field.number(), 1);
      }
    }

    std::string data;
    EXPECT_TRUE(bizarro_message.SerializeToString(&data));
    return data;
  }

  const Descriptor* descriptor_;
  unittest::TestAllTypes all_fields_;
  std::string all_fields_data_;

  // An empty message that has been parsed from all_fields_data_.  So, it has
  // unknown fields of every type.
  unittest::TestEmptyMessage empty_message_;
  UnknownFieldSet* unknown_fields_;
};

namespace {

TEST_F(UnknownFieldSetTest, AllFieldsPresent) {
  // Verifies the following:
  // --all unknown tags belong to TestAllTypes.
  // --all fields in TestAllTypes is present in UnknownFieldSet except unset
  //   oneof fields.
  //
  // Should handle repeated fields that may appear multiple times in
  // UnknownFieldSet.

  int non_oneof_count = 0;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!descriptor_->field(i)->containing_oneof()) {
      non_oneof_count++;
    }
  }

  absl::flat_hash_set<uint32_t> unknown_tags;
  for (int i = 0; i < unknown_fields_->field_count(); i++) {
    unknown_tags.insert(unknown_fields_->field(i).number());
  }

  for (uint32_t t : unknown_tags) {
    EXPECT_NE(descriptor_->FindFieldByNumber(t), nullptr);
  }

  EXPECT_EQ(non_oneof_count + descriptor_->oneof_decl_count(),
            unknown_tags.size());
}

TEST_F(UnknownFieldSetTest, Varint) {
  const UnknownField* field = GetField("optional_int32");
  ASSERT_TRUE(field != nullptr);

  ASSERT_EQ(UnknownField::TYPE_VARINT, field->type());
  EXPECT_EQ(all_fields_.optional_int32(), field->varint());
}

TEST_F(UnknownFieldSetTest, Fixed32) {
  const UnknownField* field = GetField("optional_fixed32");
  ASSERT_TRUE(field != nullptr);

  ASSERT_EQ(UnknownField::TYPE_FIXED32, field->type());
  EXPECT_EQ(all_fields_.optional_fixed32(), field->fixed32());
}

TEST_F(UnknownFieldSetTest, Fixed64) {
  const UnknownField* field = GetField("optional_fixed64");
  ASSERT_TRUE(field != nullptr);

  ASSERT_EQ(UnknownField::TYPE_FIXED64, field->type());
  EXPECT_EQ(all_fields_.optional_fixed64(), field->fixed64());
}

TEST_F(UnknownFieldSetTest, LengthDelimited) {
  const UnknownField* field = GetField("optional_string");
  ASSERT_TRUE(field != nullptr);

  ASSERT_EQ(UnknownField::TYPE_LENGTH_DELIMITED, field->type());
  EXPECT_EQ(all_fields_.optional_string(), field->length_delimited());
}

TEST_F(UnknownFieldSetTest, Group) {
  const UnknownField* field = GetField("optionalgroup");
  ASSERT_TRUE(field != nullptr);

  ASSERT_EQ(UnknownField::TYPE_GROUP, field->type());
  ASSERT_EQ(1, field->group().field_count());

  const UnknownField& nested_field = field->group().field(0);
  const FieldDescriptor* nested_field_descriptor =
      unittest::TestAllTypes::OptionalGroup::descriptor()->FindFieldByName("a");
  ASSERT_TRUE(nested_field_descriptor != nullptr);

  EXPECT_EQ(nested_field_descriptor->number(), nested_field.number());
  ASSERT_EQ(UnknownField::TYPE_VARINT, nested_field.type());
  EXPECT_EQ(all_fields_.optionalgroup().a(), nested_field.varint());
}

static void PopulateUFS(UnknownFieldSet& set) {
  UnknownFieldSet* node = &set;
  for (int i = 0; i < 3; ++i) {
    node->AddVarint(1, 100);
    const char* long_str = "This is a very long string, not sso";
    node->AddLengthDelimited(2, long_str);
    node->AddLengthDelimited(2, std::string(long_str));
    node->AddLengthDelimited(2, absl::Cord(long_str));
    *internal::UnknownFieldSetTestPeer::AddLengthDelimited(*node, 3) = long_str;
    // Test some recursion too.
    node = node->AddGroup(4);
  }
}

TEST_F(UnknownFieldSetTest, ArenaSupportWorksWithMergeFrom) {
  Arena arena;

  for (bool lhs_arena : {false, true}) {
    for (bool rhs_arena : {false, true}) {
      UnknownFieldSet lhs_stack, rhs_stack;
      auto& lhs =
          lhs_arena ? *Arena::Create<UnknownFieldSet>(&arena) : lhs_stack;
      auto& rhs =
          rhs_arena ? *Arena::Create<UnknownFieldSet>(&arena) : rhs_stack;
      PopulateUFS(rhs);
      lhs.MergeFrom(rhs);
    }
  }
}

TEST_F(UnknownFieldSetTest, ArenaSupportWorksWithMergeAndDestroy) {
  Arena arena;

  for (bool lhs_arena : {false, true}) {
    for (bool populate_lhs : {false, true}) {
      for (bool rhs_arena : {false, true}) {
        for (bool populate_rhs : {false, true}) {
          UnknownFieldSet lhs_stack, rhs_stack;
          auto& lhs =
              lhs_arena ? *Arena::Create<UnknownFieldSet>(&arena) : lhs_stack;
          auto& rhs =
              rhs_arena ? *Arena::Create<UnknownFieldSet>(&arena) : rhs_stack;
          if (populate_lhs) PopulateUFS(lhs);
          if (populate_rhs) PopulateUFS(rhs);
          lhs.MergeFromAndDestroy(&rhs);
        }
      }
    }
  }
}

TEST_F(UnknownFieldSetTest, ArenaSupportWorksWithSwap) {
  Arena arena;

  for (bool lhs_arena : {false, true}) {
    for (bool rhs_arena : {false, true}) {
      UnknownFieldSet lhs_stack, rhs_stack;
      auto& lhs =
          lhs_arena ? *Arena::Create<UnknownFieldSet>(&arena) : lhs_stack;
      auto& rhs =
          rhs_arena ? *Arena::Create<UnknownFieldSet>(&arena) : rhs_stack;
      PopulateUFS(lhs);
      lhs.Swap(&rhs);
    }
  }
}

TEST_F(UnknownFieldSetTest, ArenaSupportWorksWithClear) {
  Arena arena;
  auto* ufs = Arena::Create<UnknownFieldSet>(&arena);
  PopulateUFS(*ufs);
  // Clear should not try to delete memory from the arena.
  ufs->Clear();
}

TEST_F(UnknownFieldSetTest, ArenaSupportWorksDelete) {
  Arena arena;

  auto* ufs = Arena::Create<UnknownFieldSet>(&arena);
  PopulateUFS(*ufs);

  while (ufs->field_count() != 0) {
    ufs->DeleteByNumber(ufs->field(0).number());
  }

  ufs = Arena::Create<UnknownFieldSet>(&arena);
  PopulateUFS(*ufs);
  ufs->DeleteSubrange(0, ufs->field_count());
}

TEST_F(UnknownFieldSetTest, SerializeFastAndSlowAreEquivalent) {
  int size =
      WireFormat::ComputeUnknownFieldsSize(empty_message_.unknown_fields());
  std::string slow_buffer;
  std::string fast_buffer;
  slow_buffer.resize(size);
  fast_buffer.resize(size);

  uint8_t* target = reinterpret_cast<uint8_t*>(&fast_buffer[0]);
  uint8_t* result = WireFormat::SerializeUnknownFieldsToArray(
      empty_message_.unknown_fields(), target);
  EXPECT_EQ(size, result - target);

  {
    io::ArrayOutputStream raw_stream(&slow_buffer[0], size, 1);
    io::CodedOutputStream output_stream(&raw_stream);
    WireFormat::SerializeUnknownFields(empty_message_.unknown_fields(),
                                       &output_stream);
    ASSERT_FALSE(output_stream.HadError());
  }
  EXPECT_TRUE(fast_buffer == slow_buffer);
}

TEST_F(UnknownFieldSetTest, Serialize) {
  // Check that serializing the UnknownFieldSet produces the original data
  // again.

  std::string data;
  empty_message_.SerializeToString(&data);

  // Don't use EXPECT_EQ because we don't want to dump raw binary data to
  // stdout.
  EXPECT_TRUE(data == all_fields_data_);
}

TEST_F(UnknownFieldSetTest, ParseViaReflection) {
  // Make sure fields are properly parsed to the UnknownFieldSet when parsing
  // via reflection.

  unittest::TestEmptyMessage message;
  io::ArrayInputStream raw_input(all_fields_data_.data(),
                                 all_fields_data_.size());
  io::CodedInputStream input(&raw_input);
  ASSERT_TRUE(WireFormat::ParseAndMergePartial(&input, &message));

  EXPECT_EQ(message.DebugString(), empty_message_.DebugString());
}

TEST_F(UnknownFieldSetTest, SerializeViaReflection) {
  // Make sure fields are properly written from the UnknownFieldSet when
  // serializing via reflection.

  std::string data;

  {
    io::StringOutputStream raw_output(&data);
    io::CodedOutputStream output(&raw_output);
    size_t size = WireFormat::ByteSize(empty_message_);
    WireFormat::SerializeWithCachedSizes(empty_message_, size, &output);
    ASSERT_FALSE(output.HadError());
  }

  // Don't use EXPECT_EQ because we don't want to dump raw binary data to
  // stdout.
  EXPECT_TRUE(data == all_fields_data_);
}

TEST_F(UnknownFieldSetTest, CopyFrom) {
  unittest::TestEmptyMessage message;

  message.CopyFrom(empty_message_);

  EXPECT_EQ(empty_message_.DebugString(), message.DebugString());
}

TEST_F(UnknownFieldSetTest, Swap) {
  unittest::TestEmptyMessage other_message;
  ASSERT_TRUE(other_message.ParseFromString(GetBizarroData()));

  EXPECT_GT(empty_message_.unknown_fields().field_count(), 0);
  EXPECT_GT(other_message.unknown_fields().field_count(), 0);
  const std::string debug_string = empty_message_.DebugString();
  const std::string other_debug_string = other_message.DebugString();
  EXPECT_NE(debug_string, other_debug_string);

  empty_message_.Swap(&other_message);
  EXPECT_EQ(debug_string, other_message.DebugString());
  EXPECT_EQ(other_debug_string, empty_message_.DebugString());
}

TEST_F(UnknownFieldSetTest, SwapWithSelf) {
  const std::string debug_string = empty_message_.DebugString();
  EXPECT_GT(empty_message_.unknown_fields().field_count(), 0);

  empty_message_.Swap(&empty_message_);
  EXPECT_GT(empty_message_.unknown_fields().field_count(), 0);
  EXPECT_EQ(debug_string, empty_message_.DebugString());
}

TEST_F(UnknownFieldSetTest, MergeFrom) {
  unittest::TestEmptyMessage source, destination;

  destination.mutable_unknown_fields()->AddVarint(1, 1);
  destination.mutable_unknown_fields()->AddVarint(3, 2);
  source.mutable_unknown_fields()->AddVarint(2, 3);
  source.mutable_unknown_fields()->AddVarint(3, 4);

  destination.MergeFrom(source);

  std::string destination_text;
  TextFormat::PrintToString(destination, &destination_text);
  EXPECT_EQ(
      // Note:  The ordering of fields here depends on the ordering of adds
      //   and merging, above.
      "1: 1\n"
      "3: 2\n"
      "2: 3\n"
      "3: 4\n",
      destination_text);
}

TEST_F(UnknownFieldSetTest, MergeFromMessage) {
  unittest::TestEmptyMessage source, destination;

  destination.mutable_unknown_fields()->AddVarint(1, 1);
  destination.mutable_unknown_fields()->AddVarint(3, 2);
  source.mutable_unknown_fields()->AddVarint(2, 3);
  source.mutable_unknown_fields()->AddVarint(3, 4);

  destination.mutable_unknown_fields()->MergeFromMessage(source);

  std::string destination_text;
  TextFormat::PrintToString(destination, &destination_text);
  EXPECT_EQ(
      // Note:  The ordering of fields here depends on the ordering of adds
      //   and merging, above.
      "1: 1\n"
      "3: 2\n"
      "2: 3\n"
      "3: 4\n",
      destination_text);
}

TEST_F(UnknownFieldSetTest, MergeFromMessageLite) {
  unittest::TestAllTypesLite source;
  unittest::TestEmptyMessageLite destination;

  source.set_optional_fixed32(42);
  destination.ParseFromString(source.SerializeAsString());

  UnknownFieldSet unknown_field_set;
  EXPECT_TRUE(unknown_field_set.MergeFromMessage(destination));
  EXPECT_EQ(unknown_field_set.field_count(), 1);

  const UnknownField& unknown_field = unknown_field_set.field(0);
  EXPECT_EQ(unknown_field.number(), 7);
  EXPECT_EQ(unknown_field.fixed32(), 42);
}

TEST_F(UnknownFieldSetTest, Clear) {
  // Clear the set.
  empty_message_.Clear();
  EXPECT_EQ(0, unknown_fields_->field_count());
}

TEST_F(UnknownFieldSetTest, ClearAndFreeMemory) {
  EXPECT_GT(unknown_fields_->field_count(), 0);
  unknown_fields_->ClearAndFreeMemory();
  EXPECT_EQ(0, unknown_fields_->field_count());
  unknown_fields_->AddVarint(123456, 654321);
  EXPECT_EQ(1, unknown_fields_->field_count());
}

TEST_F(UnknownFieldSetTest, ParseKnownAndUnknown) {
  // Test mixing known and unknown fields when parsing.

  unittest::TestEmptyMessage source;
  source.mutable_unknown_fields()->AddVarint(123456, 654321);
  std::string data;
  ASSERT_TRUE(source.SerializeToString(&data));

  unittest::TestAllTypes destination;
  ASSERT_TRUE(destination.ParseFromString(all_fields_data_ + data));

  TestUtil::ExpectAllFieldsSet(destination);
  ASSERT_EQ(1, destination.unknown_fields().field_count());
  ASSERT_EQ(UnknownField::TYPE_VARINT,
            destination.unknown_fields().field(0).type());
  EXPECT_EQ(654321, destination.unknown_fields().field(0).varint());
}

TEST_F(UnknownFieldSetTest, WrongTypeTreatedAsUnknown) {
  // Test that fields of the wrong wire type are treated like unknown fields
  // when parsing.

  unittest::TestAllTypes all_types_message;
  unittest::TestEmptyMessage empty_message;
  std::string bizarro_data = GetBizarroData();
  ASSERT_TRUE(all_types_message.ParseFromString(bizarro_data));
  ASSERT_TRUE(empty_message.ParseFromString(bizarro_data));

  // All fields should have been interpreted as unknown, so the debug strings
  // should be the same.
  EXPECT_EQ(empty_message.DebugString(), all_types_message.DebugString());
}

TEST_F(UnknownFieldSetTest, WrongTypeTreatedAsUnknownViaReflection) {
  // Same as WrongTypeTreatedAsUnknown but via the reflection interface.

  unittest::TestAllTypes all_types_message;
  unittest::TestEmptyMessage empty_message;
  std::string bizarro_data = GetBizarroData();
  io::ArrayInputStream raw_input(bizarro_data.data(), bizarro_data.size());
  io::CodedInputStream input(&raw_input);
  ASSERT_TRUE(WireFormat::ParseAndMergePartial(&input, &all_types_message));
  ASSERT_TRUE(empty_message.ParseFromString(bizarro_data));

  EXPECT_EQ(empty_message.DebugString(), all_types_message.DebugString());
}

TEST_F(UnknownFieldSetTest, UnknownExtensions) {
  // Make sure fields are properly parsed to the UnknownFieldSet even when
  // they are declared as extension numbers.

  unittest::TestEmptyMessageWithExtensions message;
  ASSERT_TRUE(message.ParseFromString(all_fields_data_));

  EXPECT_EQ(message.DebugString(), empty_message_.DebugString());
}

TEST_F(UnknownFieldSetTest, UnknownExtensionsReflection) {
  // Same as UnknownExtensions except parsing via reflection.

  unittest::TestEmptyMessageWithExtensions message;
  io::ArrayInputStream raw_input(all_fields_data_.data(),
                                 all_fields_data_.size());
  io::CodedInputStream input(&raw_input);
  ASSERT_TRUE(WireFormat::ParseAndMergePartial(&input, &message));

  EXPECT_EQ(message.DebugString(), empty_message_.DebugString());
}

TEST_F(UnknownFieldSetTest, WrongExtensionTypeTreatedAsUnknown) {
  // Test that fields of the wrong wire type are treated like unknown fields
  // when parsing extensions.

  unittest::TestAllExtensions all_extensions_message;
  unittest::TestEmptyMessage empty_message;
  std::string bizarro_data = GetBizarroData();
  ASSERT_TRUE(all_extensions_message.ParseFromString(bizarro_data));
  ASSERT_TRUE(empty_message.ParseFromString(bizarro_data));

  // All fields should have been interpreted as unknown, so the debug strings
  // should be the same.
  EXPECT_EQ(empty_message.DebugString(), all_extensions_message.DebugString());
}

TEST_F(UnknownFieldSetTest, UnknownEnumValue) {
  using unittest::TestAllExtensions;
  using unittest::TestAllTypes;
  using unittest::TestEmptyMessage;

  const FieldDescriptor* singular_field =
      TestAllTypes::descriptor()->FindFieldByName("optional_nested_enum");
  const FieldDescriptor* repeated_field =
      TestAllTypes::descriptor()->FindFieldByName("repeated_nested_enum");
  ASSERT_TRUE(singular_field != nullptr);
  ASSERT_TRUE(repeated_field != nullptr);

  std::string data;

  {
    TestEmptyMessage empty_message;
    UnknownFieldSet* unknown_fields = empty_message.mutable_unknown_fields();
    unknown_fields->AddVarint(singular_field->number(), TestAllTypes::BAR);
    unknown_fields->AddVarint(singular_field->number(), 5);  // not valid
    unknown_fields->AddVarint(repeated_field->number(), TestAllTypes::FOO);
    unknown_fields->AddVarint(repeated_field->number(), 4);  // not valid
    unknown_fields->AddVarint(repeated_field->number(), TestAllTypes::BAZ);
    unknown_fields->AddVarint(repeated_field->number(), 6);  // not valid
    empty_message.SerializeToString(&data);
  }

  {
    TestAllTypes message;
    ASSERT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(TestAllTypes::BAR, message.optional_nested_enum());
    ASSERT_EQ(2, message.repeated_nested_enum_size());
    EXPECT_EQ(TestAllTypes::FOO, message.repeated_nested_enum(0));
    EXPECT_EQ(TestAllTypes::BAZ, message.repeated_nested_enum(1));

    const UnknownFieldSet& unknown_fields = message.unknown_fields();
    ASSERT_EQ(3, unknown_fields.field_count());

    EXPECT_EQ(singular_field->number(), unknown_fields.field(0).number());
    ASSERT_EQ(UnknownField::TYPE_VARINT, unknown_fields.field(0).type());
    EXPECT_EQ(5, unknown_fields.field(0).varint());

    EXPECT_EQ(repeated_field->number(), unknown_fields.field(1).number());
    ASSERT_EQ(UnknownField::TYPE_VARINT, unknown_fields.field(1).type());
    EXPECT_EQ(4, unknown_fields.field(1).varint());

    EXPECT_EQ(repeated_field->number(), unknown_fields.field(2).number());
    ASSERT_EQ(UnknownField::TYPE_VARINT, unknown_fields.field(2).type());
    EXPECT_EQ(6, unknown_fields.field(2).varint());
  }

  {
    using unittest::optional_nested_enum_extension;
    using unittest::repeated_nested_enum_extension;

    TestAllExtensions message;
    ASSERT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(TestAllTypes::BAR,
              message.GetExtension(optional_nested_enum_extension));
    ASSERT_EQ(2, message.ExtensionSize(repeated_nested_enum_extension));
    EXPECT_EQ(TestAllTypes::FOO,
              message.GetExtension(repeated_nested_enum_extension, 0));
    EXPECT_EQ(TestAllTypes::BAZ,
              message.GetExtension(repeated_nested_enum_extension, 1));

    const UnknownFieldSet& unknown_fields = message.unknown_fields();
    ASSERT_EQ(3, unknown_fields.field_count());

    EXPECT_EQ(singular_field->number(), unknown_fields.field(0).number());
    ASSERT_EQ(UnknownField::TYPE_VARINT, unknown_fields.field(0).type());
    EXPECT_EQ(5, unknown_fields.field(0).varint());

    EXPECT_EQ(repeated_field->number(), unknown_fields.field(1).number());
    ASSERT_EQ(UnknownField::TYPE_VARINT, unknown_fields.field(1).type());
    EXPECT_EQ(4, unknown_fields.field(1).varint());

    EXPECT_EQ(repeated_field->number(), unknown_fields.field(2).number());
    ASSERT_EQ(UnknownField::TYPE_VARINT, unknown_fields.field(2).type());
    EXPECT_EQ(6, unknown_fields.field(2).varint());
  }
}

TEST_F(UnknownFieldSetTest, SpaceUsedExcludingSelf) {
  UnknownFieldSet empty;
  empty.AddVarint(1, 0);
  RepeatedField<UnknownField> rep;
  rep.Add();
  EXPECT_EQ(rep.SpaceUsedExcludingSelf(), empty.SpaceUsedExcludingSelf());
}

TEST_F(UnknownFieldSetTest, SpaceUsed) {
  // Keep shadow vectors to avoid making assumptions about its capacity growth.
  // We imitate the push back calls here to determine the expected capacity.
  RepeatedField<UnknownField> shadow_vector, shadow_vector_group;
  unittest::TestEmptyMessage empty_message;

  // Make sure an unknown field set has zero space used until a field is
  // actually added.
  const size_t base = empty_message.SpaceUsedLong();
  std::string* str = nullptr;
  UnknownFieldSet* group = nullptr;
  const auto total = [&] {
    size_t result = base;
    result += shadow_vector.SpaceUsedExcludingSelfLong();
    result += shadow_vector_group.SpaceUsedExcludingSelfLong();
    if (str != nullptr) {
      result += sizeof(std::string) +
                internal::StringSpaceUsedExcludingSelfLong(*str);
    }
    if (group != nullptr) {
      result += sizeof(UnknownFieldSet);
    }
    return result;
  };

  UnknownFieldSet* unknown_fields = empty_message.mutable_unknown_fields();
  EXPECT_EQ(total(), empty_message.SpaceUsedLong());

  // Make sure each thing we add to the set increases the SpaceUsedLong().
  unknown_fields->AddVarint(1, 0);
  shadow_vector.Add();
  EXPECT_EQ(total(), empty_message.SpaceUsedLong()) << "Var";

  str =
      internal::UnknownFieldSetTestPeer::AddLengthDelimited(*unknown_fields, 1);
  shadow_vector.Add();
  EXPECT_EQ(total(), empty_message.SpaceUsedLong()) << "Str";

  str->assign(sizeof(std::string) + 1, 'x');
  EXPECT_EQ(total(), empty_message.SpaceUsedLong()) << "Str2";

  group = unknown_fields->AddGroup(1);
  shadow_vector.Add();
  EXPECT_EQ(total(), empty_message.SpaceUsedLong()) << "Group";

  group->AddVarint(1, 0);
  shadow_vector_group.Add();
  EXPECT_EQ(total(), empty_message.SpaceUsedLong()) << "Group2";

  unknown_fields->AddVarint(1, 0);
  shadow_vector.Add();
  EXPECT_EQ(total(), empty_message.SpaceUsedLong()) << "Var2";
}

TEST_F(UnknownFieldSetTest, Empty) {
  UnknownFieldSet unknown_fields;
  EXPECT_TRUE(unknown_fields.empty());
  unknown_fields.AddVarint(6, 123);
  EXPECT_FALSE(unknown_fields.empty());
  unknown_fields.Clear();
  EXPECT_TRUE(unknown_fields.empty());
}

TEST_F(UnknownFieldSetTest, DeleteSubrange) {
  // Exhaustively test the deletion of every possible subrange in arrays of all
  // sizes from 0 through 9.
  for (int size = 0; size < 10; ++size) {
    for (int num = 0; num <= size; ++num) {
      for (int start = 0; start < size - num; ++start) {
        // Create a set with "size" fields.
        UnknownFieldSet unknown;
        for (int i = 0; i < size; ++i) {
          unknown.AddFixed32(i, i);
        }
        // Delete the specified subrange.
        unknown.DeleteSubrange(start, num);
        // Make sure the resulting field values are still correct.
        EXPECT_EQ(size - num, unknown.field_count());
        for (int i = 0; i < unknown.field_count(); ++i) {
          if (i < start) {
            EXPECT_EQ(i, unknown.field(i).fixed32());
          } else {
            EXPECT_EQ(i + num, unknown.field(i).fixed32());
          }
        }
      }
    }
  }
}

void CheckDeleteByNumber(const std::vector<int>& field_numbers,
                         int deleted_number,
                         const std::vector<int>& expected_field_numbers) {
  UnknownFieldSet unknown_fields;
  for (int i = 0; i < field_numbers.size(); ++i) {
    unknown_fields.AddFixed32(field_numbers[i], i);
  }
  unknown_fields.DeleteByNumber(deleted_number);
  ASSERT_EQ(expected_field_numbers.size(), unknown_fields.field_count());
  for (int i = 0; i < expected_field_numbers.size(); ++i) {
    EXPECT_EQ(expected_field_numbers[i], unknown_fields.field(i).number());
  }
}

#define MAKE_VECTOR(x) std::vector<int>(x, x + ABSL_ARRAYSIZE(x))
TEST_F(UnknownFieldSetTest, DeleteByNumber) {
  CheckDeleteByNumber(std::vector<int>(), 1, std::vector<int>());
  static const int kTestFieldNumbers1[] = {1, 2, 3};
  static const int kFieldNumberToDelete1 = 1;
  static const int kExpectedFieldNumbers1[] = {2, 3};
  CheckDeleteByNumber(MAKE_VECTOR(kTestFieldNumbers1), kFieldNumberToDelete1,
                      MAKE_VECTOR(kExpectedFieldNumbers1));
  static const int kTestFieldNumbers2[] = {1, 2, 3};
  static const int kFieldNumberToDelete2 = 2;
  static const int kExpectedFieldNumbers2[] = {1, 3};
  CheckDeleteByNumber(MAKE_VECTOR(kTestFieldNumbers2), kFieldNumberToDelete2,
                      MAKE_VECTOR(kExpectedFieldNumbers2));
  static const int kTestFieldNumbers3[] = {1, 2, 3};
  static const int kFieldNumberToDelete3 = 3;
  static const int kExpectedFieldNumbers3[] = {1, 2};
  CheckDeleteByNumber(MAKE_VECTOR(kTestFieldNumbers3), kFieldNumberToDelete3,
                      MAKE_VECTOR(kExpectedFieldNumbers3));
  static const int kTestFieldNumbers4[] = {1, 2, 1, 4, 1};
  static const int kFieldNumberToDelete4 = 1;
  static const int kExpectedFieldNumbers4[] = {2, 4};
  CheckDeleteByNumber(MAKE_VECTOR(kTestFieldNumbers4), kFieldNumberToDelete4,
                      MAKE_VECTOR(kExpectedFieldNumbers4));
  static const int kTestFieldNumbers5[] = {1, 2, 3, 4, 5};
  static const int kFieldNumberToDelete5 = 6;
  static const int kExpectedFieldNumbers5[] = {1, 2, 3, 4, 5};
  CheckDeleteByNumber(MAKE_VECTOR(kTestFieldNumbers5), kFieldNumberToDelete5,
                      MAKE_VECTOR(kExpectedFieldNumbers5));
}
#undef MAKE_VECTOR

TEST_F(UnknownFieldSetTest, SerializeToString) {
  UnknownFieldSet field_set;
  field_set.AddVarint(3, 3);
  field_set.AddVarint(4, 4);
  field_set.AddVarint(1, -1);
  field_set.AddVarint(2, -2);
  field_set.AddLengthDelimited(44, "str");
  field_set.AddLengthDelimited(44, std::string("byv"));
  field_set.AddLengthDelimited(44,
                               absl::Cord("this came from cord and is long"));
  *internal::UnknownFieldSetTestPeer::AddLengthDelimited(field_set, 44) =
      "0123456789";

  field_set.AddFixed32(7, 7);
  field_set.AddFixed64(8, 8);

  UnknownFieldSet* group_field_set = field_set.AddGroup(46);
  group_field_set->AddVarint(47, 1024);
  group_field_set = field_set.AddGroup(46);
  group_field_set->AddVarint(47, 2048);

  unittest::TestAllTypes message =
      UnknownToProto<unittest::TestAllTypes>(field_set);

  EXPECT_EQ(message.optional_int32(), -1);
  EXPECT_EQ(message.optional_int64(), -2);
  EXPECT_EQ(message.optional_uint32(), 3);
  EXPECT_EQ(message.optional_uint64(), 4);
  EXPECT_EQ(message.optional_fixed32(), 7);
  EXPECT_EQ(message.optional_fixed64(), 8);
  EXPECT_THAT(message.repeated_string(),
              ElementsAre("str", "byv", "this came from cord and is long",
                          "0123456789"));
  EXPECT_EQ(message.repeatedgroup(0).a(), 1024);
  EXPECT_EQ(message.repeatedgroup(1).a(), 2048);
}

TEST_F(UnknownFieldSetTest, SerializeToCodedStream_TestPackedTypes) {
  UnknownFieldSet field_set;
  field_set.AddVarint(90, -1);
  field_set.AddVarint(90, -2);
  field_set.AddVarint(90, -3);
  field_set.AddVarint(90, -4);
  field_set.AddVarint(93, 5);
  field_set.AddVarint(93, 6);
  field_set.AddVarint(93, 7);

  unittest::TestPackedTypes message;
  std::string serialized_message;
  {
    io::StringOutputStream string_output(&serialized_message);
    io::CodedOutputStream coded_output(&string_output);
    ASSERT_TRUE(field_set.SerializeToCodedStream(&coded_output));
  }
  ASSERT_TRUE(message.ParseFromString(serialized_message));
  EXPECT_THAT(message.packed_int32(), ElementsAre(-1, -2, -3, -4));
  EXPECT_THAT(message.packed_uint64(), ElementsAre(5, 6, 7));
}

TEST_F(UnknownFieldSetTest, SerializeToCord_TestPackedTypes) {
  UnknownFieldSet field_set;
  field_set.AddVarint(90, -1);
  field_set.AddVarint(90, -2);
  field_set.AddVarint(90, -3);
  field_set.AddVarint(90, -4);
  field_set.AddVarint(93, 5);
  field_set.AddVarint(93, 6);
  field_set.AddVarint(93, 7);

  absl::Cord cord;
  ASSERT_TRUE(field_set.SerializeToCord(&cord));

  unittest::TestPackedTypes message;
  ASSERT_TRUE(message.ParseFromString(cord));
  EXPECT_THAT(message.packed_int32(), ElementsAre(-1, -2, -3, -4));
  EXPECT_THAT(message.packed_uint64(), ElementsAre(5, 6, 7));
}

TEST(UnknownFieldTest, SettersOverrideTheDataProperly) {
  using T = unittest::TestAllTypes;
  UnknownFieldSet set;
  set.AddVarint(T::kOptionalInt32FieldNumber, 2);
  set.AddFixed32(T::kOptionalFixed32FieldNumber, 3);
  set.AddFixed64(T::kOptionalFixed64FieldNumber, 4);
  set.AddLengthDelimited(T::kOptionalStringFieldNumber, "5");

  T message = UnknownToProto<T>(set);

  EXPECT_EQ(message.optional_int32(), 2);
  EXPECT_EQ(message.optional_fixed32(), 3);
  EXPECT_EQ(message.optional_fixed64(), 4);
  EXPECT_EQ(message.optional_string(), "5");

  set.mutable_field(0)->set_varint(22);
  set.mutable_field(1)->set_fixed32(33);
  set.mutable_field(2)->set_fixed64(44);
  set.mutable_field(3)->set_length_delimited("55");

  message = UnknownToProto<T>(set);

  EXPECT_EQ(message.optional_int32(), 22);
  EXPECT_EQ(message.optional_fixed32(), 33);
  EXPECT_EQ(message.optional_fixed64(), 44);
  EXPECT_EQ(message.optional_string(), "55");

  set.mutable_field(3)->set_length_delimited(std::string("555"));
  message = UnknownToProto<T>(set);
  EXPECT_EQ(message.optional_string(), "555");

  set.mutable_field(3)->set_length_delimited(absl::Cord("5555"));
  message = UnknownToProto<T>(set);
  EXPECT_EQ(message.optional_string(), "5555");
}

}  // namespace
}  // namespace protobuf
}  // namespace google
