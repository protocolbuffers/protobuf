// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/extension_set.h"

#include <cstdint>
#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/base/casts.h"
#include "absl/strings/cord.h"
#include "absl/strings/match.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/test_util2.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_mset.pb.h"
#include "google/protobuf/unittest_proto3_extensions.pb.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {


namespace internal {

extern bool fully_verify_message_sets_opt_out;

namespace {

using ::google::protobuf::internal::DownCast;
using TestUtil::EqualsToSerialized;

// This test closely mirrors google/protobuf/compiler/cpp/unittest.cc
// except that it uses extensions rather than regular fields.

TEST(ExtensionSetTest, Defaults) {
  // Check that all default values are set correctly in the initial message.
  unittest::TestAllExtensions message;

  TestUtil::ExpectExtensionsClear(message);

  // Messages should return pointers to default instances until first use.
  // (This is not checked by ExpectClear() since it is not actually true after
  // the fields have been set and then cleared.)
  EXPECT_EQ(&unittest::OptionalGroup_extension::default_instance(),
            &message.GetExtension(unittest::optionalgroup_extension));
  EXPECT_EQ(&unittest::TestAllTypes::NestedMessage::default_instance(),
            &message.GetExtension(unittest::optional_nested_message_extension));
  EXPECT_EQ(
      &unittest::ForeignMessage::default_instance(),
      &message.GetExtension(unittest::optional_foreign_message_extension));
  EXPECT_EQ(&unittest_import::ImportMessage::default_instance(),
            &message.GetExtension(unittest::optional_import_message_extension));
}

TEST(ExtensionSetTest, Accessors) {
  // Set every field to a unique value then go back and check all those
  // values.
  unittest::TestAllExtensions message;

  TestUtil::SetAllExtensions(&message);
  TestUtil::ExpectAllExtensionsSet(message);

  TestUtil::ModifyRepeatedExtensions(&message);
  TestUtil::ExpectRepeatedExtensionsModified(message);
}

TEST(ExtensionSetTest, Clear) {
  // Set every field to a unique value, clear the message, then check that
  // it is cleared.
  unittest::TestAllExtensions message;

  TestUtil::SetAllExtensions(&message);
  message.Clear();
  TestUtil::ExpectExtensionsClear(message);

  // Make sure setting stuff again after clearing works.  (This takes slightly
  // different code paths since the objects are reused.)
  TestUtil::SetAllExtensions(&message);
  TestUtil::ExpectAllExtensionsSet(message);
}

TEST(ExtensionSetTest, ClearOneField) {
  // Set every field to a unique value, then clear one value and insure that
  // only that one value is cleared.
  unittest::TestAllExtensions message;

  TestUtil::SetAllExtensions(&message);
  int64_t original_value =
      message.GetExtension(unittest::optional_int64_extension);

  // Clear the field and make sure it shows up as cleared.
  message.ClearExtension(unittest::optional_int64_extension);
  EXPECT_FALSE(message.HasExtension(unittest::optional_int64_extension));
  EXPECT_EQ(0, message.GetExtension(unittest::optional_int64_extension));

  // Other adjacent fields should not be cleared.
  EXPECT_TRUE(message.HasExtension(unittest::optional_int32_extension));
  EXPECT_TRUE(message.HasExtension(unittest::optional_uint32_extension));

  // Make sure if we set it again, then all fields are set.
  message.SetExtension(unittest::optional_int64_extension, original_value);
  TestUtil::ExpectAllExtensionsSet(message);
}

TEST(ExtensionSetTest, SetAllocatedExtension) {
  unittest::TestAllExtensions message;
  EXPECT_FALSE(
      message.HasExtension(unittest::optional_foreign_message_extension));
  // Add a extension using SetAllocatedExtension
  unittest::ForeignMessage* foreign_message = new unittest::ForeignMessage();
  message.SetAllocatedExtension(unittest::optional_foreign_message_extension,
                                foreign_message);
  EXPECT_TRUE(
      message.HasExtension(unittest::optional_foreign_message_extension));
  EXPECT_EQ(foreign_message, message.MutableExtension(
                                 unittest::optional_foreign_message_extension));
  EXPECT_EQ(foreign_message, &message.GetExtension(
                                 unittest::optional_foreign_message_extension));

  // SetAllocatedExtension should delete the previously existing extension.
  // (We reply on unittest to check memory leaks for this case)
  message.SetAllocatedExtension(unittest::optional_foreign_message_extension,
                                new unittest::ForeignMessage());

  // SetAllocatedExtension with nullptr is equivalent to ClearExtenion.
  message.SetAllocatedExtension(unittest::optional_foreign_message_extension,
                                nullptr);
  EXPECT_FALSE(
      message.HasExtension(unittest::optional_foreign_message_extension));
}

TEST(ExtensionSetTest, ReleaseExtension) {
  proto2_wireformat_unittest::TestMessageSet message;
  EXPECT_FALSE(message.HasExtension(
      unittest::TestMessageSetExtension1::message_set_extension));
  // Add a extension using SetAllocatedExtension
  unittest::TestMessageSetExtension1* extension =
      new unittest::TestMessageSetExtension1();
  message.SetAllocatedExtension(
      unittest::TestMessageSetExtension1::message_set_extension, extension);
  EXPECT_TRUE(message.HasExtension(
      unittest::TestMessageSetExtension1::message_set_extension));
  // Release the extension using ReleaseExtension
  unittest::TestMessageSetExtension1* released_extension =
      message.ReleaseExtension(
          unittest::TestMessageSetExtension1::message_set_extension);
  EXPECT_EQ(extension, released_extension);
  EXPECT_FALSE(message.HasExtension(
      unittest::TestMessageSetExtension1::message_set_extension));
  // ReleaseExtension will return the underlying object even after
  // ClearExtension is called.
  message.SetAllocatedExtension(
      unittest::TestMessageSetExtension1::message_set_extension,
      released_extension);
  message.ClearExtension(
      unittest::TestMessageSetExtension1::message_set_extension);
  released_extension = message.ReleaseExtension(
      unittest::TestMessageSetExtension1::message_set_extension);
  EXPECT_TRUE(released_extension != nullptr);
  delete released_extension;
}

TEST(ExtensionSetTest, ArenaUnsafeArenaSetAllocatedAndRelease) {
  Arena arena;
  unittest::TestAllExtensions* message =
      Arena::Create<unittest::TestAllExtensions>(&arena);
  unittest::ForeignMessage extension;
  message->UnsafeArenaSetAllocatedExtension(
      unittest::optional_foreign_message_extension, &extension);
  // No copy when set.
  unittest::ForeignMessage* mutable_extension =
      message->MutableExtension(unittest::optional_foreign_message_extension);
  EXPECT_EQ(&extension, mutable_extension);
  // No copy when unsafe released.
  unittest::ForeignMessage* released_extension =
      message->UnsafeArenaReleaseExtension(
          unittest::optional_foreign_message_extension);
  EXPECT_EQ(&extension, released_extension);
  EXPECT_FALSE(
      message->HasExtension(unittest::optional_foreign_message_extension));
  // Set the ownership back and let the destructors run.  It should not take
  // ownership, so this should not crash.
  message->UnsafeArenaSetAllocatedExtension(
      unittest::optional_foreign_message_extension, &extension);
}

TEST(ExtensionSetTest, UnsafeArenaSetAllocatedAndRelease) {
  unittest::TestAllExtensions message;
  unittest::ForeignMessage* extension = new unittest::ForeignMessage();
  message.UnsafeArenaSetAllocatedExtension(
      unittest::optional_foreign_message_extension, extension);
  // No copy when set.
  unittest::ForeignMessage* mutable_extension =
      message.MutableExtension(unittest::optional_foreign_message_extension);
  EXPECT_EQ(extension, mutable_extension);
  // No copy when unsafe released.
  unittest::ForeignMessage* released_extension =
      message.UnsafeArenaReleaseExtension(
          unittest::optional_foreign_message_extension);
  EXPECT_EQ(extension, released_extension);
  EXPECT_FALSE(
      message.HasExtension(unittest::optional_foreign_message_extension));
  // Set the ownership back and let the destructors run.  It should take
  // ownership, so this should not leak.
  message.UnsafeArenaSetAllocatedExtension(
      unittest::optional_foreign_message_extension, extension);
}

TEST(ExtensionSetTest, ArenaUnsafeArenaReleaseOfHeapAlloc) {
  Arena arena;
  unittest::TestAllExtensions* message =
      Arena::Create<unittest::TestAllExtensions>(&arena);
  unittest::ForeignMessage* extension = new unittest::ForeignMessage;
  message->SetAllocatedExtension(unittest::optional_foreign_message_extension,
                                 extension);
  // The arena should maintain ownership of the heap allocated proto because we
  // used UnsafeArenaReleaseExtension.  The leak checker will ensure this.
  unittest::ForeignMessage* released_extension =
      message->UnsafeArenaReleaseExtension(
          unittest::optional_foreign_message_extension);
  EXPECT_EQ(extension, released_extension);
  EXPECT_FALSE(
      message->HasExtension(unittest::optional_foreign_message_extension));
}


TEST(ExtensionSetTest, CopyFrom) {
  unittest::TestAllExtensions message1, message2;

  TestUtil::SetAllExtensions(&message1);
  message2.CopyFrom(message1);
  TestUtil::ExpectAllExtensionsSet(message2);
  message2.CopyFrom(message1);  // exercise copy when fields already exist
  TestUtil::ExpectAllExtensionsSet(message2);
}

TEST(ExtensionSetTest, CopyFromPacked) {
  unittest::TestPackedExtensions message1, message2;

  TestUtil::SetPackedExtensions(&message1);
  message2.CopyFrom(message1);
  TestUtil::ExpectPackedExtensionsSet(message2);
  message2.CopyFrom(message1);  // exercise copy when fields already exist
  TestUtil::ExpectPackedExtensionsSet(message2);
}

TEST(ExtensionSetTest, CopyFromUpcasted) {
  unittest::TestAllExtensions message1, message2;
  const Message& upcasted_message = message1;

  TestUtil::SetAllExtensions(&message1);
  message2.CopyFrom(upcasted_message);
  TestUtil::ExpectAllExtensionsSet(message2);
  // exercise copy when fields already exist
  message2.CopyFrom(upcasted_message);
  TestUtil::ExpectAllExtensionsSet(message2);
}

TEST(ExtensionSetTest, SwapWithEmpty) {
  unittest::TestAllExtensions message1, message2;
  TestUtil::SetAllExtensions(&message1);

  TestUtil::ExpectAllExtensionsSet(message1);
  TestUtil::ExpectExtensionsClear(message2);
  message1.Swap(&message2);
  TestUtil::ExpectAllExtensionsSet(message2);
  TestUtil::ExpectExtensionsClear(message1);
}

TEST(ExtensionSetTest, SwapWithSelf) {
  unittest::TestAllExtensions message;
  TestUtil::SetAllExtensions(&message);

  TestUtil::ExpectAllExtensionsSet(message);
  message.Swap(&message);
  TestUtil::ExpectAllExtensionsSet(message);
}

TEST(ExtensionSetTest, SwapExtension) {
  unittest::TestAllExtensions message1;
  unittest::TestAllExtensions message2;

  TestUtil::SetAllExtensions(&message1);
  std::vector<const FieldDescriptor*> fields;

  // Swap empty fields.
  const Reflection* reflection = message1.GetReflection();
  reflection->SwapFields(&message1, &message2, fields);
  TestUtil::ExpectAllExtensionsSet(message1);
  TestUtil::ExpectExtensionsClear(message2);

  // Swap two extensions.
  fields.push_back(reflection->FindKnownExtensionByNumber(12));
  fields.push_back(reflection->FindKnownExtensionByNumber(25));
  reflection->SwapFields(&message1, &message2, fields);

  EXPECT_TRUE(message1.HasExtension(unittest::optional_int32_extension));
  EXPECT_FALSE(message1.HasExtension(unittest::optional_double_extension));
  EXPECT_FALSE(message1.HasExtension(unittest::optional_cord_extension));

  EXPECT_FALSE(message2.HasExtension(unittest::optional_int32_extension));
  EXPECT_TRUE(message2.HasExtension(unittest::optional_double_extension));
  EXPECT_TRUE(message2.HasExtension(unittest::optional_cord_extension));
}

TEST(ExtensionSetTest, SwapExtensionWithEmpty) {
  unittest::TestAllExtensions message1;
  unittest::TestAllExtensions message2;
  unittest::TestAllExtensions message3;

  TestUtil::SetAllExtensions(&message3);

  const Reflection* reflection = message3.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message3, &fields);

  reflection->SwapFields(&message1, &message2, fields);

  TestUtil::ExpectExtensionsClear(message1);
  TestUtil::ExpectExtensionsClear(message2);
}

TEST(ExtensionSetTest, SwapExtensionBothFull) {
  unittest::TestAllExtensions message1;
  unittest::TestAllExtensions message2;

  TestUtil::SetAllExtensions(&message1);
  TestUtil::SetAllExtensions(&message2);

  const Reflection* reflection = message1.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message1, &fields);

  reflection->SwapFields(&message1, &message2, fields);

  TestUtil::ExpectAllExtensionsSet(message1);
  TestUtil::ExpectAllExtensionsSet(message2);
}

TEST(ExtensionSetTest, ArenaSetAllExtension) {
  Arena arena1;
  unittest::TestAllExtensions* message1 =
      Arena::Create<unittest::TestAllExtensions>(&arena1);
  TestUtil::SetAllExtensions(message1);
  TestUtil::ExpectAllExtensionsSet(*message1);
}

TEST(ExtensionSetTest, ArenaCopyConstructor) {
  Arena arena1;
  unittest::TestAllExtensions* message1 =
      Arena::Create<unittest::TestAllExtensions>(&arena1);
  TestUtil::SetAllExtensions(message1);
  unittest::TestAllExtensions message2(*message1);
  arena1.Reset();
  TestUtil::ExpectAllExtensionsSet(message2);
}

TEST(ExtensionSetTest, ArenaMergeFrom) {
  Arena arena1;
  unittest::TestAllExtensions* message1 =
      Arena::Create<unittest::TestAllExtensions>(&arena1);
  TestUtil::SetAllExtensions(message1);
  unittest::TestAllExtensions message2;
  message2.MergeFrom(*message1);
  arena1.Reset();
  TestUtil::ExpectAllExtensionsSet(message2);
}

TEST(ExtensionSetTest, ArenaMergeFromWithClearedExtensions) {
  Arena arena;
  {
    auto* message1 = Arena::Create<unittest::TestAllExtensions>(&arena);
    auto* message2 = Arena::Create<unittest::TestAllExtensions>(&arena);

    // Set an extension and then clear it
    message1->SetExtension(unittest::optional_int32_extension, 1);
    message1->ClearExtension(unittest::optional_int32_extension);

    // Since all extensions in message1 have been cleared, we should be able to
    // merge it into message2 without allocating any additional memory.
    uint64_t space_used_before_merge = arena.SpaceUsed();
    message2->MergeFrom(*message1);
    EXPECT_EQ(space_used_before_merge, arena.SpaceUsed());
  }
  {
    // As more complicated case, let's have message1 and message2 share some
    // uncleared extensions in common.
    auto* message1 = Arena::Create<unittest::TestAllExtensions>(&arena);
    auto* message2 = Arena::Create<unittest::TestAllExtensions>(&arena);

    // Set int32 and uint32 on both messages.
    message1->SetExtension(unittest::optional_int32_extension, 1);
    message2->SetExtension(unittest::optional_int32_extension, 2);
    message1->SetExtension(unittest::optional_uint32_extension, 1);
    message2->SetExtension(unittest::optional_uint32_extension, 2);

    // Set and clear int64 and uint64 on message1.
    message1->SetExtension(unittest::optional_int64_extension, 0);
    message1->ClearExtension(unittest::optional_int64_extension);
    message1->SetExtension(unittest::optional_uint64_extension, 0);
    message1->ClearExtension(unittest::optional_uint64_extension);

    uint64_t space_used_before_merge = arena.SpaceUsed();
    message2->MergeFrom(*message1);
    EXPECT_EQ(space_used_before_merge, arena.SpaceUsed());
  }
}

TEST(ExtensionSetTest, ArenaSetAllocatedMessageAndRelease) {
  Arena arena;
  unittest::TestAllExtensions* message =
      Arena::Create<unittest::TestAllExtensions>(&arena);
  EXPECT_FALSE(
      message->HasExtension(unittest::optional_foreign_message_extension));
  // Add a extension using SetAllocatedExtension
  unittest::ForeignMessage* foreign_message = new unittest::ForeignMessage();
  message->SetAllocatedExtension(unittest::optional_foreign_message_extension,
                                 foreign_message);
  // foreign_message is now owned by the arena.
  EXPECT_EQ(foreign_message, message->MutableExtension(
                                 unittest::optional_foreign_message_extension));

  // Underlying message is copied, and returned.
  unittest::ForeignMessage* released_message =
      message->ReleaseExtension(unittest::optional_foreign_message_extension);
  delete released_message;
  EXPECT_FALSE(
      message->HasExtension(unittest::optional_foreign_message_extension));
}

TEST(ExtensionSetTest, SwapExtensionBothFullWithArena) {
  Arena arena1;
  std::unique_ptr<Arena> arena2(new Arena());

  unittest::TestAllExtensions* message1 =
      Arena::Create<unittest::TestAllExtensions>(&arena1);
  unittest::TestAllExtensions* message2 =
      Arena::Create<unittest::TestAllExtensions>(arena2.get());

  TestUtil::SetAllExtensions(message1);
  TestUtil::SetAllExtensions(message2);
  message1->SetExtension(unittest::optional_int32_extension, 1);
  message2->SetExtension(unittest::optional_int32_extension, 2);
  message1->Swap(message2);
  EXPECT_EQ(2, message1->GetExtension(unittest::optional_int32_extension));
  EXPECT_EQ(1, message2->GetExtension(unittest::optional_int32_extension));
  // Re-set the original values so ExpectAllExtensionsSet is happy.
  message1->SetExtension(unittest::optional_int32_extension, 101);
  message2->SetExtension(unittest::optional_int32_extension, 101);
  TestUtil::ExpectAllExtensionsSet(*message1);
  TestUtil::ExpectAllExtensionsSet(*message2);
  arena2.reset(nullptr);
  TestUtil::ExpectAllExtensionsSet(*message1);
  // Test corner cases, when one is empty and other is not.
  Arena arena3, arena4;

  unittest::TestAllExtensions* message3 =
      Arena::Create<unittest::TestAllExtensions>(&arena3);
  unittest::TestAllExtensions* message4 =
      Arena::Create<unittest::TestAllExtensions>(&arena4);
  TestUtil::SetAllExtensions(message3);
  message3->Swap(message4);
  arena3.Reset();
  TestUtil::ExpectAllExtensionsSet(*message4);
}

TEST(ExtensionSetTest, SwapFieldsOfExtensionBothFullWithArena) {
  Arena arena1;
  Arena* arena2 = new Arena();

  unittest::TestAllExtensions* message1 =
      Arena::Create<unittest::TestAllExtensions>(&arena1);
  unittest::TestAllExtensions* message2 =
      Arena::Create<unittest::TestAllExtensions>(arena2);

  TestUtil::SetAllExtensions(message1);
  TestUtil::SetAllExtensions(message2);

  const Reflection* reflection = message1->GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(*message1, &fields);
  reflection->SwapFields(message1, message2, fields);
  TestUtil::ExpectAllExtensionsSet(*message1);
  TestUtil::ExpectAllExtensionsSet(*message2);
  delete arena2;
  TestUtil::ExpectAllExtensionsSet(*message1);
}

TEST(ExtensionSetTest, SwapExtensionWithSelf) {
  unittest::TestAllExtensions message1;

  TestUtil::SetAllExtensions(&message1);

  std::vector<const FieldDescriptor*> fields;
  const Reflection* reflection = message1.GetReflection();
  reflection->ListFields(message1, &fields);
  reflection->SwapFields(&message1, &message1, fields);

  TestUtil::ExpectAllExtensionsSet(message1);
}

TEST(ExtensionSetTest, SerializationToArray) {
  // Serialize as TestAllExtensions and parse as TestAllTypes to insure wire
  // compatibility of extensions.
  //
  // This checks serialization to a flat array by explicitly reserving space in
  // the string and calling the generated message's
  // SerializeWithCachedSizesToArray.
  unittest::TestAllExtensions source;
  unittest::TestAllTypes destination;
  TestUtil::SetAllExtensions(&source);
  size_t size = source.ByteSizeLong();
  std::string data;
  data.resize(size);
  uint8_t* target = reinterpret_cast<uint8_t*>(&data[0]);
  uint8_t* end = source.SerializeWithCachedSizesToArray(target);
  EXPECT_EQ(size, end - target);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectAllFieldsSet(destination);
}

TEST(ExtensionSetTest, SerializationToStream) {
  // Serialize as TestAllExtensions and parse as TestAllTypes to insure wire
  // compatibility of extensions.
  //
  // This checks serialization to an output stream by creating an array output
  // stream that can only buffer 1 byte at a time - this prevents the message
  // from ever jumping to the fast path, ensuring that serialization happens via
  // the CodedOutputStream.
  unittest::TestAllExtensions source;
  unittest::TestAllTypes destination;
  TestUtil::SetAllExtensions(&source);
  size_t size = source.ByteSizeLong();
  std::string data;
  data.resize(size);
  {
    io::ArrayOutputStream array_stream(&data[0], size, 1);
    io::CodedOutputStream output_stream(&array_stream);
    source.SerializeWithCachedSizes(&output_stream);
    ASSERT_FALSE(output_stream.HadError());
  }
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectAllFieldsSet(destination);
}

TEST(ExtensionSetTest, PackedSerializationToArray) {
  // Serialize as TestPackedExtensions and parse as TestPackedTypes to insure
  // wire compatibility of extensions.
  //
  // This checks serialization to a flat array by explicitly reserving space in
  // the string and calling the generated message's
  // SerializeWithCachedSizesToArray.
  unittest::TestPackedExtensions source;
  unittest::TestPackedTypes destination;
  TestUtil::SetPackedExtensions(&source);
  size_t size = source.ByteSizeLong();
  std::string data;
  data.resize(size);
  uint8_t* target = reinterpret_cast<uint8_t*>(&data[0]);
  uint8_t* end = source.SerializeWithCachedSizesToArray(target);
  EXPECT_EQ(size, end - target);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectPackedFieldsSet(destination);
}

TEST(ExtensionSetTest, PackedSerializationToStream) {
  // Serialize as TestPackedExtensions and parse as TestPackedTypes to insure
  // wire compatibility of extensions.
  //
  // This checks serialization to an output stream by creating an array output
  // stream that can only buffer 1 byte at a time - this prevents the message
  // from ever jumping to the fast path, ensuring that serialization happens via
  // the CodedOutputStream.
  unittest::TestPackedExtensions source;
  unittest::TestPackedTypes destination;
  TestUtil::SetPackedExtensions(&source);
  size_t size = source.ByteSizeLong();
  std::string data;
  data.resize(size);
  {
    io::ArrayOutputStream array_stream(&data[0], size, 1);
    io::CodedOutputStream output_stream(&array_stream);
    source.SerializeWithCachedSizes(&output_stream);
    ASSERT_FALSE(output_stream.HadError());
  }
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectPackedFieldsSet(destination);
}

TEST(ExtensionSetTest, NestedExtensionGroup) {
  // Serialize as TestGroup and parse as TestGroupExtension.
  unittest::TestGroup source;
  unittest::TestGroupExtension destination;
  std::string data;

  source.mutable_optionalgroup()->set_a(117);
  source.set_optional_foreign_enum(unittest::FOREIGN_BAZ);
  source.SerializeToString(&data);
  EXPECT_TRUE(destination.ParseFromString(data));
  EXPECT_TRUE(
      destination
          .GetExtension(unittest::TestNestedExtension::optionalgroup_extension)
          .has_a());
  EXPECT_EQ(117, destination
                     .GetExtension(
                         unittest::TestNestedExtension::optionalgroup_extension)
                     .a());
  EXPECT_TRUE(destination.HasExtension(
      unittest::TestNestedExtension::optional_foreign_enum_extension));
  EXPECT_EQ(
      unittest::FOREIGN_BAZ,
      destination.GetExtension(
          unittest::TestNestedExtension::optional_foreign_enum_extension));
}

TEST(ExtensionSetTest, Parsing) {
  // Serialize as TestAllTypes and parse as TestAllExtensions.
  unittest::TestAllTypes source;
  unittest::TestAllExtensions destination;
  std::string data;

  TestUtil::SetAllFields(&source);
  source.SerializeToString(&data);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::SetOneofFields(&destination);
  TestUtil::ExpectAllExtensionsSet(destination);
}

TEST(ExtensionSetTest, PackedParsing) {
  // Serialize as TestPackedTypes and parse as TestPackedExtensions.
  unittest::TestPackedTypes source;
  unittest::TestPackedExtensions destination;
  std::string data;

  TestUtil::SetPackedFields(&source);
  source.SerializeToString(&data);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectPackedExtensionsSet(destination);
}

TEST(ExtensionSetTest, PackedToUnpackedParsing) {
  unittest::TestPackedTypes source;
  unittest::TestUnpackedExtensions destination;
  std::string data;

  TestUtil::SetPackedFields(&source);
  source.SerializeToString(&data);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectUnpackedExtensionsSet(destination);

  // Reserialize
  unittest::TestUnpackedTypes unpacked;
  TestUtil::SetUnpackedFields(&unpacked);
  // Serialized proto has to be the same size and parsed to the same message.
  EXPECT_EQ(unpacked.SerializeAsString().size(),
            destination.SerializeAsString().size());
  EXPECT_TRUE(EqualsToSerialized(unpacked, destination.SerializeAsString()));

  // Make sure we can add extensions.
  destination.AddExtension(unittest::unpacked_int32_extension, 1);
  destination.AddExtension(unittest::unpacked_enum_extension,
                           protobuf_unittest::FOREIGN_BAR);
}

TEST(ExtensionSetTest, UnpackedToPackedParsing) {
  unittest::TestUnpackedTypes source;
  unittest::TestPackedExtensions destination;
  std::string data;

  TestUtil::SetUnpackedFields(&source);
  source.SerializeToString(&data);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectPackedExtensionsSet(destination);

  // Reserialize
  unittest::TestPackedTypes packed;
  TestUtil::SetPackedFields(&packed);
  // Serialized proto has to be the same size and parsed to the same message.
  EXPECT_EQ(packed.SerializeAsString().size(),
            destination.SerializeAsString().size());
  EXPECT_TRUE(EqualsToSerialized(packed, destination.SerializeAsString()));

  // Make sure we can add extensions.
  destination.AddExtension(unittest::packed_int32_extension, 1);
  destination.AddExtension(unittest::packed_enum_extension,
                           protobuf_unittest::FOREIGN_BAR);
}

TEST(ExtensionSetTest, IsInitialized) {
  // Test that IsInitialized() returns false if required fields in nested
  // extensions are missing.
  unittest::TestAllExtensions message;

  EXPECT_TRUE(message.IsInitialized());

  message.MutableExtension(unittest::TestRequired::single);
  EXPECT_FALSE(message.IsInitialized());

  message.MutableExtension(unittest::TestRequired::single)->set_a(1);
  EXPECT_FALSE(message.IsInitialized());
  message.MutableExtension(unittest::TestRequired::single)->set_b(2);
  EXPECT_FALSE(message.IsInitialized());
  message.MutableExtension(unittest::TestRequired::single)->set_c(3);
  EXPECT_TRUE(message.IsInitialized());

  message.AddExtension(unittest::TestRequired::multi);
  EXPECT_FALSE(message.IsInitialized());

  message.MutableExtension(unittest::TestRequired::multi, 0)->set_a(1);
  EXPECT_FALSE(message.IsInitialized());
  message.MutableExtension(unittest::TestRequired::multi, 0)->set_b(2);
  EXPECT_FALSE(message.IsInitialized());
  message.MutableExtension(unittest::TestRequired::multi, 0)->set_c(3);
  EXPECT_TRUE(message.IsInitialized());
}

TEST(ExtensionSetTest, MutableString) {
  // Test the mutable string accessors.
  unittest::TestAllExtensions message;

  message.MutableExtension(unittest::optional_string_extension)->assign("foo");
  EXPECT_TRUE(message.HasExtension(unittest::optional_string_extension));
  EXPECT_EQ("foo", message.GetExtension(unittest::optional_string_extension));

  message.AddExtension(unittest::repeated_string_extension)->assign("bar");
  ASSERT_EQ(1, message.ExtensionSize(unittest::repeated_string_extension));
  EXPECT_EQ("bar",
            message.GetExtension(unittest::repeated_string_extension, 0));
}

TEST(ExtensionSetTest, SpaceUsedExcludingSelf) {
  // Scalar primitive extensions should increase the extension set size by a
  // minimum of the size of the primitive type.
#define TEST_SCALAR_EXTENSIONS_SPACE_USED(type, value)                       \
  do {                                                                       \
    unittest::TestAllExtensions message;                                     \
    const int base_size = message.SpaceUsedLong();                           \
    message.SetExtension(unittest::optional_##type##_extension, value);      \
    int min_expected_size =                                                  \
        base_size +                                                          \
        sizeof(message.GetExtension(unittest::optional_##type##_extension)); \
    EXPECT_LE(min_expected_size, message.SpaceUsedLong());                   \
  } while (0)

  TEST_SCALAR_EXTENSIONS_SPACE_USED(int32, 101);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(int64, 102);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(uint32, 103);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(uint64, 104);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sint32, 105);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sint64, 106);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(fixed32, 107);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(fixed64, 108);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sfixed32, 109);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sfixed64, 110);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(float, 111);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(double, 112);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(bool, true);
#undef TEST_SCALAR_EXTENSIONS_SPACE_USED
  {
    unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsedLong();
    message.SetExtension(unittest::optional_nested_enum_extension,
                         unittest::TestAllTypes::FOO);
    int min_expected_size =
        base_size +
        sizeof(message.GetExtension(unittest::optional_nested_enum_extension));
    EXPECT_LE(min_expected_size, message.SpaceUsedLong());
  }
  {
    // Strings may cause extra allocations depending on their length; ensure
    // that gets included as well.
    unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsedLong();
    const std::string s(
        "this is a fairly large string that will cause some "
        "allocation in order to store it in the extension");
    message.SetExtension(unittest::optional_string_extension, s);
    int min_expected_size = base_size + s.length();
    EXPECT_LE(min_expected_size, message.SpaceUsedLong());
  }
  {
    // Messages also have additional allocation that need to be counted.
    unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsedLong();
    unittest::ForeignMessage foreign;
    foreign.set_c(42);
    message.MutableExtension(unittest::optional_foreign_message_extension)
        ->CopyFrom(foreign);
    int min_expected_size = base_size + foreign.SpaceUsedLong();
    EXPECT_LE(min_expected_size, message.SpaceUsedLong());
  }

  // Repeated primitive extensions will increase space used by at least a
  // RepeatedField<T>, and will cause additional allocations when the array
  // gets too big for the initial space.
  // This macro:
  //   - Adds a value to the repeated extension, then clears it, establishing
  //     the base size.
  //   - Adds a small number of values, testing that it doesn't increase the
  //     SpaceUsedLong()
  //   - Adds a large number of values (requiring allocation in the repeated
  //     field), and ensures that that allocation is included in SpaceUsedLong()
#define TEST_REPEATED_EXTENSIONS_SPACE_USED(type, cpptype, value)              \
  do {                                                                         \
    std::unique_ptr<unittest::TestAllExtensions> message(                      \
        Arena::Create<unittest::TestAllExtensions>(nullptr));                  \
    const size_t base_size = message->SpaceUsedLong();                         \
    size_t min_expected_size = sizeof(RepeatedField<cpptype>) + base_size;     \
    message->AddExtension(unittest::repeated_##type##_extension, value);       \
    message->ClearExtension(unittest::repeated_##type##_extension);            \
    const size_t empty_repeated_field_size = message->SpaceUsedLong();         \
    EXPECT_LE(min_expected_size, empty_repeated_field_size) << #type;          \
    message->AddExtension(unittest::repeated_##type##_extension, value);       \
    EXPECT_EQ(empty_repeated_field_size, message->SpaceUsedLong()) << #type;   \
    message->ClearExtension(unittest::repeated_##type##_extension);            \
    const size_t old_capacity =                                                \
        message->GetRepeatedExtension(unittest::repeated_##type##_extension)   \
            .Capacity();                                                       \
    EXPECT_GE(                                                                 \
        old_capacity,                                                          \
        (RepeatedFieldLowerClampLimit<cpptype, std::max(sizeof(cpptype),       \
                                                        sizeof(void*))>()));   \
    for (int i = 0; i < 16; ++i) {                                             \
      message->AddExtension(unittest::repeated_##type##_extension, value);     \
    }                                                                          \
    int expected_size =                                                        \
        sizeof(cpptype) *                                                      \
            (message                                                           \
                 ->GetRepeatedExtension(unittest::repeated_##type##_extension) \
                 .Capacity() -                                                 \
             old_capacity) +                                                   \
        empty_repeated_field_size;                                             \
    EXPECT_LE(expected_size, message->SpaceUsedLong()) << #type;               \
  } while (0)

  TEST_REPEATED_EXTENSIONS_SPACE_USED(int32, int32_t, 101);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(int64, int64_t, 102);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(uint32, uint32_t, 103);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(uint64, uint64_t, 104);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sint32, int32_t, 105);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sint64, int64_t, 106);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(fixed32, uint32_t, 107);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(fixed64, uint64_t, 108);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sfixed32, int32_t, 109);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sfixed64, int64_t, 110);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(float, float, 111);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(double, double, 112);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(bool, bool, true);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(nested_enum, int,
                                      unittest::TestAllTypes::FOO);
#undef TEST_REPEATED_EXTENSIONS_SPACE_USED
  // Repeated strings
  {
    std::unique_ptr<unittest::TestAllExtensions> message(
        Arena::Create<unittest::TestAllExtensions>(nullptr));
    const size_t base_size = message->SpaceUsedLong();
    size_t min_expected_size =
        sizeof(RepeatedPtrField<std::string>) + base_size;
    const std::string value(256, 'x');
    // Once items are allocated, they may stick around even when cleared so
    // without the hardcore memory management accessors there isn't a notion of
    // the empty repeated field memory usage as there is with primitive types.
    for (int i = 0; i < 16; ++i) {
      message->AddExtension(unittest::repeated_string_extension, value);
    }
    min_expected_size +=
        (sizeof(value) + value.size()) *
        (16 - RepeatedFieldLowerClampLimit<void*, sizeof(void*)>());
    EXPECT_LE(min_expected_size, message->SpaceUsedLong());
  }
  // Repeated messages
  {
    std::unique_ptr<unittest::TestAllExtensions> message(
        Arena::Create<unittest::TestAllExtensions>(nullptr));
    const size_t base_size = message->SpaceUsedLong();
    size_t min_expected_size =
        sizeof(RepeatedPtrField<unittest::ForeignMessage>) + base_size;
    unittest::ForeignMessage prototype;
    prototype.set_c(2);
    for (int i = 0; i < 16; ++i) {
      *message->AddExtension(unittest::repeated_foreign_message_extension) =
          prototype;
    }
    min_expected_size +=
        (16 - RepeatedFieldLowerClampLimit<void*, sizeof(void*)>()) *
        prototype.SpaceUsedLong();
    EXPECT_LE(min_expected_size, message->SpaceUsedLong());
  }
}

// N.B.: We do not test range-based for here because we remain C++03 compatible.
template <typename T, typename M, typename ID>
inline T SumAllExtensions(const M& message, ID extension, T zero) {
  T sum = zero;
  typename RepeatedField<T>::const_iterator iter =
      message.GetRepeatedExtension(extension).begin();
  typename RepeatedField<T>::const_iterator end =
      message.GetRepeatedExtension(extension).end();
  for (; iter != end; ++iter) {
    sum += *iter;
  }
  return sum;
}

template <typename T, typename M, typename ID>
inline void IncAllExtensions(M* message, ID extension, T val) {
  typename RepeatedField<T>::iterator iter =
      message->MutableRepeatedExtension(extension)->begin();
  typename RepeatedField<T>::iterator end =
      message->MutableRepeatedExtension(extension)->end();
  for (; iter != end; ++iter) {
    *iter += val;
  }
}

TEST(ExtensionSetTest, RepeatedFields) {
  unittest::TestAllExtensions message;

  // Test empty repeated-field case (b/12926163)
  ASSERT_EQ(
      0,
      message.GetRepeatedExtension(unittest::repeated_int32_extension).size());
  ASSERT_EQ(
      0, message.GetRepeatedExtension(unittest::repeated_nested_enum_extension)
             .size());
  ASSERT_EQ(
      0,
      message.GetRepeatedExtension(unittest::repeated_string_extension).size());
  ASSERT_EQ(
      0,
      message.GetRepeatedExtension(unittest::repeated_nested_message_extension)
          .size());

  unittest::TestAllTypes::NestedMessage nested_message;
  nested_message.set_bb(42);
  unittest::TestAllTypes::NestedEnum nested_enum =
      unittest::TestAllTypes::NestedEnum_MIN;

  for (int i = 0; i < 10; ++i) {
    message.AddExtension(unittest::repeated_int32_extension, 1);
    message.AddExtension(unittest::repeated_int64_extension, 2);
    message.AddExtension(unittest::repeated_uint32_extension, 3);
    message.AddExtension(unittest::repeated_uint64_extension, 4);
    message.AddExtension(unittest::repeated_sint32_extension, 5);
    message.AddExtension(unittest::repeated_sint64_extension, 6);
    message.AddExtension(unittest::repeated_fixed32_extension, 7);
    message.AddExtension(unittest::repeated_fixed64_extension, 8);
    message.AddExtension(unittest::repeated_sfixed32_extension, 7);
    message.AddExtension(unittest::repeated_sfixed64_extension, 8);
    message.AddExtension(unittest::repeated_float_extension, 9.0);
    message.AddExtension(unittest::repeated_double_extension, 10.0);
    message.AddExtension(unittest::repeated_bool_extension, true);
    message.AddExtension(unittest::repeated_nested_enum_extension, nested_enum);
    message.AddExtension(unittest::repeated_string_extension,
                         std::string("test"));
    message.AddExtension(unittest::repeated_bytes_extension,
                         std::string("test\xFF"));
    message.AddExtension(unittest::repeated_nested_message_extension)
        ->CopyFrom(nested_message);
    message.AddExtension(unittest::repeated_nested_enum_extension, nested_enum);
  }

  ASSERT_EQ(10, SumAllExtensions<int32_t>(
                    message, unittest::repeated_int32_extension, 0));
  IncAllExtensions<int32_t>(&message, unittest::repeated_int32_extension, 1);
  ASSERT_EQ(20, SumAllExtensions<int32_t>(
                    message, unittest::repeated_int32_extension, 0));

  ASSERT_EQ(20, SumAllExtensions<int64_t>(
                    message, unittest::repeated_int64_extension, 0));
  IncAllExtensions<int64_t>(&message, unittest::repeated_int64_extension, 1);
  ASSERT_EQ(30, SumAllExtensions<int64_t>(
                    message, unittest::repeated_int64_extension, 0));

  ASSERT_EQ(30, SumAllExtensions<uint32_t>(
                    message, unittest::repeated_uint32_extension, 0));
  IncAllExtensions<uint32_t>(&message, unittest::repeated_uint32_extension, 1);
  ASSERT_EQ(40, SumAllExtensions<uint32_t>(
                    message, unittest::repeated_uint32_extension, 0));

  ASSERT_EQ(40, SumAllExtensions<uint64_t>(
                    message, unittest::repeated_uint64_extension, 0));
  IncAllExtensions<uint64_t>(&message, unittest::repeated_uint64_extension, 1);
  ASSERT_EQ(50, SumAllExtensions<uint64_t>(
                    message, unittest::repeated_uint64_extension, 0));

  ASSERT_EQ(50, SumAllExtensions<int32_t>(
                    message, unittest::repeated_sint32_extension, 0));
  IncAllExtensions<int32_t>(&message, unittest::repeated_sint32_extension, 1);
  ASSERT_EQ(60, SumAllExtensions<int32_t>(
                    message, unittest::repeated_sint32_extension, 0));

  ASSERT_EQ(60, SumAllExtensions<int64_t>(
                    message, unittest::repeated_sint64_extension, 0));
  IncAllExtensions<int64_t>(&message, unittest::repeated_sint64_extension, 1);
  ASSERT_EQ(70, SumAllExtensions<int64_t>(
                    message, unittest::repeated_sint64_extension, 0));

  ASSERT_EQ(70, SumAllExtensions<uint32_t>(
                    message, unittest::repeated_fixed32_extension, 0));
  IncAllExtensions<uint32_t>(&message, unittest::repeated_fixed32_extension, 1);
  ASSERT_EQ(80, SumAllExtensions<uint32_t>(
                    message, unittest::repeated_fixed32_extension, 0));

  ASSERT_EQ(80, SumAllExtensions<uint64_t>(
                    message, unittest::repeated_fixed64_extension, 0));
  IncAllExtensions<uint64_t>(&message, unittest::repeated_fixed64_extension, 1);
  ASSERT_EQ(90, SumAllExtensions<uint64_t>(
                    message, unittest::repeated_fixed64_extension, 0));

  // Usually, floating-point arithmetic cannot be trusted to be exact, so it is
  // a Bad Idea to assert equality in a test like this. However, we're dealing
  // with integers with a small number of significant mantissa bits, so we
  // should actually have exact precision here.
  ASSERT_EQ(90, SumAllExtensions<float>(message,
                                        unittest::repeated_float_extension, 0));
  IncAllExtensions<float>(&message, unittest::repeated_float_extension, 1);
  ASSERT_EQ(100, SumAllExtensions<float>(
                     message, unittest::repeated_float_extension, 0));

  ASSERT_EQ(100, SumAllExtensions<double>(
                     message, unittest::repeated_double_extension, 0));
  IncAllExtensions<double>(&message, unittest::repeated_double_extension, 1);
  ASSERT_EQ(110, SumAllExtensions<double>(
                     message, unittest::repeated_double_extension, 0));

  RepeatedPtrField<std::string>::iterator string_iter;
  RepeatedPtrField<std::string>::iterator string_end;
  for (string_iter =
           message
               .MutableRepeatedExtension(unittest::repeated_string_extension)
               ->begin(),
      string_end =
           message
               .MutableRepeatedExtension(unittest::repeated_string_extension)
               ->end();
       string_iter != string_end; ++string_iter) {
    string_iter->append("test");
  }
  RepeatedPtrField<std::string>::const_iterator string_const_iter;
  RepeatedPtrField<std::string>::const_iterator string_const_end;
  for (string_const_iter =
           message.GetRepeatedExtension(unittest::repeated_string_extension)
               .begin(),
      string_const_end =
           message.GetRepeatedExtension(unittest::repeated_string_extension)
               .end();
       string_iter != string_end; ++string_iter) {
    ASSERT_TRUE(*string_iter == "testtest");
  }

  RepeatedField<unittest::TestAllTypes_NestedEnum>::iterator enum_iter;
  RepeatedField<unittest::TestAllTypes_NestedEnum>::iterator enum_end;
  for (enum_iter = message
                       .MutableRepeatedExtension(
                           unittest::repeated_nested_enum_extension)
                       ->begin(),
      enum_end = message
                     .MutableRepeatedExtension(
                         unittest::repeated_nested_enum_extension)
                     ->end();
       enum_iter != enum_end; ++enum_iter) {
    *enum_iter = unittest::TestAllTypes::NestedEnum_MAX;
  }
  RepeatedField<unittest::TestAllTypes_NestedEnum>::const_iterator
      enum_const_iter;
  RepeatedField<unittest::TestAllTypes_NestedEnum>::const_iterator
      enum_const_end;
  for (enum_const_iter =
           message
               .GetRepeatedExtension(unittest::repeated_nested_enum_extension)
               .begin(),
      enum_const_end =
           message
               .GetRepeatedExtension(unittest::repeated_nested_enum_extension)
               .end();
       enum_const_iter != enum_const_end; ++enum_const_iter) {
    ASSERT_EQ(*enum_const_iter, unittest::TestAllTypes::NestedEnum_MAX);
  }

  RepeatedPtrField<unittest::TestAllTypes_NestedMessage>::iterator msg_iter;
  RepeatedPtrField<unittest::TestAllTypes_NestedMessage>::iterator msg_end;
  for (msg_iter = message
                      .MutableRepeatedExtension(
                          unittest::repeated_nested_message_extension)
                      ->begin(),
      msg_end = message
                    .MutableRepeatedExtension(
                        unittest::repeated_nested_message_extension)
                    ->end();
       msg_iter != msg_end; ++msg_iter) {
    msg_iter->set_bb(1234);
  }
  RepeatedPtrField<unittest::TestAllTypes_NestedMessage>::const_iterator
      msg_const_iter;
  RepeatedPtrField<unittest::TestAllTypes_NestedMessage>::const_iterator
      msg_const_end;
  for (msg_const_iter = message
                            .GetRepeatedExtension(
                                unittest::repeated_nested_message_extension)
                            .begin(),
      msg_const_end = message
                          .GetRepeatedExtension(
                              unittest::repeated_nested_message_extension)
                          .end();
       msg_const_iter != msg_const_end; ++msg_const_iter) {
    ASSERT_EQ(msg_const_iter->bb(), 1234);
  }

  // Test one primitive field.
  for (auto& x :
       *message.MutableRepeatedExtension(unittest::repeated_int32_extension)) {
    x = 4321;
  }
  for (const auto& x :
       message.GetRepeatedExtension(unittest::repeated_int32_extension)) {
    ASSERT_EQ(x, 4321);
  }
  // Test one string field.
  for (auto& x :
       *message.MutableRepeatedExtension(unittest::repeated_string_extension)) {
    x = "test_range_based_for";
  }
  for (const auto& x :
       message.GetRepeatedExtension(unittest::repeated_string_extension)) {
    ASSERT_TRUE(x == "test_range_based_for");
  }
  // Test one message field.
  for (auto& x : *message.MutableRepeatedExtension(
           unittest::repeated_nested_message_extension)) {
    x.set_bb(4321);
  }
  for (const auto& x : *message.MutableRepeatedExtension(
           unittest::repeated_nested_message_extension)) {
    ASSERT_EQ(x.bb(), 4321);
  }
}

// From b/12926163
TEST(ExtensionSetTest, AbsentExtension) {
  unittest::TestAllExtensions message;
  message.MutableRepeatedExtension(unittest::repeated_nested_message_extension)
      ->Add()
      ->set_bb(123);
  ASSERT_EQ(1,
            message.ExtensionSize(unittest::repeated_nested_message_extension));
  EXPECT_EQ(123,
            message.GetExtension(unittest::repeated_nested_message_extension, 0)
                .bb());
}

#if GTEST_HAS_DEATH_TEST

TEST(ExtensionSetTest, InvalidEnumDeath) {
  unittest::TestAllExtensions message;
  EXPECT_DEBUG_DEATH(
      message.SetExtension(unittest::optional_foreign_enum_extension,
                           static_cast<unittest::ForeignEnum>(53)),
      "IsValid");
}

#endif  // GTEST_HAS_DEATH_TEST

TEST(ExtensionSetTest, DynamicExtensions) {
  // Test adding a dynamic extension to a compiled-in message object.

  FileDescriptorProto dynamic_proto;
  dynamic_proto.set_name("dynamic_extensions_test.proto");
  dynamic_proto.add_dependency(
      unittest::TestAllExtensions::descriptor()->file()->name());
  dynamic_proto.set_package("dynamic_extensions");

  // Copy the fields and nested types from TestDynamicExtensions into our new
  // proto, converting the fields into extensions.
  const Descriptor* template_descriptor =
      unittest::TestDynamicExtensions::descriptor();
  DescriptorProto template_descriptor_proto;
  template_descriptor->CopyTo(&template_descriptor_proto);
  dynamic_proto.mutable_message_type()->MergeFrom(
      template_descriptor_proto.nested_type());
  dynamic_proto.mutable_enum_type()->MergeFrom(
      template_descriptor_proto.enum_type());
  dynamic_proto.mutable_extension()->MergeFrom(
      template_descriptor_proto.field());

  // For each extension that we added...
  for (int i = 0; i < dynamic_proto.extension_size(); i++) {
    // Set its extendee to TestAllExtensions.
    FieldDescriptorProto* extension = dynamic_proto.mutable_extension(i);
    extension->set_extendee(
        unittest::TestAllExtensions::descriptor()->full_name());

    // If the field refers to one of the types nested in TestDynamicExtensions,
    // make it refer to the type in our dynamic proto instead.
    std::string prefix =
        absl::StrCat(".", template_descriptor->full_name(), ".");
    if (extension->has_type_name()) {
      std::string* type_name = extension->mutable_type_name();
      if (absl::StartsWith(*type_name, prefix)) {
        type_name->replace(0, prefix.size(), ".dynamic_extensions.");
      }
    }
  }

  // Now build the file, using the generated pool as an underlay.
  DescriptorPool dynamic_pool(DescriptorPool::generated_pool());
  const FileDescriptor* file = dynamic_pool.BuildFile(dynamic_proto);
  ASSERT_TRUE(file != nullptr);
  DynamicMessageFactory dynamic_factory(&dynamic_pool);
  dynamic_factory.SetDelegateToGeneratedFactory(true);

  // Construct a message that we can parse with the extensions we defined.
  // Since the extensions were based off of the fields of TestDynamicExtensions,
  // we can use that message to create this test message.
  std::string data;
  unittest::TestDynamicExtensions dynamic_extension;
  {
    unittest::TestDynamicExtensions message;
    message.set_scalar_extension(123);
    message.set_enum_extension(unittest::FOREIGN_BAR);
    message.set_dynamic_enum_extension(
        unittest::TestDynamicExtensions::DYNAMIC_BAZ);
    message.mutable_message_extension()->set_c(456);
    message.mutable_dynamic_message_extension()->set_dynamic_field(789);
    message.add_repeated_extension("foo");
    message.add_repeated_extension("bar");
    message.add_packed_extension(12);
    message.add_packed_extension(-34);
    message.add_packed_extension(56);
    message.add_packed_extension(-78);

    // Also add some unknown fields.

    // An unknown enum value (for a known field).
    message.mutable_unknown_fields()->AddVarint(
        unittest::TestDynamicExtensions::kDynamicEnumExtensionFieldNumber,
        12345);
    // A regular unknown field.
    message.mutable_unknown_fields()->AddLengthDelimited(54321, "unknown");

    message.SerializeToString(&data);
    dynamic_extension = message;
  }

  // Now we can parse this using our dynamic extension definitions...
  unittest::TestAllExtensions message;
  {
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetExtensionRegistry(&dynamic_pool, &dynamic_factory);
    ASSERT_TRUE(message.ParseFromCodedStream(&input));
    ASSERT_TRUE(input.ConsumedEntireMessage());
  }

  // Can we print it?
  std::string message_text;
  TextFormat::PrintToString(message, &message_text);
  EXPECT_EQ(
      "[dynamic_extensions.scalar_extension]: 123\n"
      "[dynamic_extensions.enum_extension]: FOREIGN_BAR\n"
      "[dynamic_extensions.dynamic_enum_extension]: DYNAMIC_BAZ\n"
      "[dynamic_extensions.message_extension] {\n"
      "  c: 456\n"
      "}\n"
      "[dynamic_extensions.dynamic_message_extension] {\n"
      "  dynamic_field: 789\n"
      "}\n"
      "[dynamic_extensions.repeated_extension]: \"foo\"\n"
      "[dynamic_extensions.repeated_extension]: \"bar\"\n"
      "[dynamic_extensions.packed_extension]: 12\n"
      "[dynamic_extensions.packed_extension]: -34\n"
      "[dynamic_extensions.packed_extension]: 56\n"
      "[dynamic_extensions.packed_extension]: -78\n"
      "2002: 12345\n"
      "54321: \"unknown\"\n",
      message_text);

  // Can we serialize it?
  EXPECT_TRUE(
      EqualsToSerialized(dynamic_extension, message.SerializeAsString()));

  // What if we parse using the reflection-based parser?
  {
    unittest::TestAllExtensions message2;
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetExtensionRegistry(&dynamic_pool, &dynamic_factory);
    ASSERT_TRUE(WireFormat::ParseAndMergePartial(&input, &message2));
    ASSERT_TRUE(input.ConsumedEntireMessage());
    EXPECT_EQ(message.DebugString(), message2.DebugString());
  }

  // Are the embedded generated types actually using the generated objects?
  {
    const FieldDescriptor* message_extension =
        file->FindExtensionByName("message_extension");
    ASSERT_TRUE(message_extension != nullptr);
    const Message& sub_message =
        message.GetReflection()->GetMessage(message, message_extension);
    const unittest::ForeignMessage* typed_sub_message =
#if PROTOBUF_RTTI
        dynamic_cast<const unittest::ForeignMessage*>(&sub_message);
#else
        static_cast<const unittest::ForeignMessage*>(&sub_message);
#endif
    ASSERT_TRUE(typed_sub_message != nullptr);
    EXPECT_EQ(456, typed_sub_message->c());
  }

  // What does GetMessage() return for the embedded dynamic type if it isn't
  // present?
  {
    const FieldDescriptor* dynamic_message_extension =
        file->FindExtensionByName("dynamic_message_extension");
    ASSERT_TRUE(dynamic_message_extension != nullptr);
    const Message& parent = unittest::TestAllExtensions::default_instance();
    const Message& sub_message = parent.GetReflection()->GetMessage(
        parent, dynamic_message_extension, &dynamic_factory);
    const Message* prototype =
        dynamic_factory.GetPrototype(dynamic_message_extension->message_type());
    EXPECT_EQ(prototype, &sub_message);
  }
}

TEST(ExtensionSetTest, Proto3PackedDynamicExtensions) {
  // Regression test for b/271121265. This test case verifies that
  // packed-by-default repeated custom options in proto3 are correctly
  // serialized in packed form when dynamic extensions are used.

  // Create a custom option in proto3 and load this into an overlay
  // DescriptorPool with a DynamicMessageFactory.
  google::protobuf::FileDescriptorProto file_descriptor_proto;
  file_descriptor_proto.set_syntax("proto3");
  file_descriptor_proto.set_name(
      "google/protobuf/unittest_proto3_packed_extension.proto");
  file_descriptor_proto.set_package("proto3_unittest");
  file_descriptor_proto.add_dependency(
      DescriptorProto::descriptor()->file()->name());
  FieldDescriptorProto* extension = file_descriptor_proto.add_extension();
  extension->set_name("repeated_int32_option");
  extension->set_extendee(MessageOptions().GetTypeName());
  extension->set_number(50009);
  extension->set_label(FieldDescriptorProto::LABEL_REPEATED);
  extension->set_type(FieldDescriptorProto::TYPE_INT32);
  extension->set_json_name("repeatedInt32Option");
  google::protobuf::DescriptorPool pool(DescriptorPool::generated_pool());
  ASSERT_NE(pool.BuildFile(file_descriptor_proto), nullptr);
  DynamicMessageFactory factory;
  factory.SetDelegateToGeneratedFactory(true);

  // Create a serialized MessageOptions proto equivalent to:
  // [proto3_unittest.repeated_int32_option]: 1
  UnknownFieldSet unknown_fields;
  unknown_fields.AddVarint(50009, 1);
  std::string serialized_options;
  ASSERT_TRUE(unknown_fields.SerializeToString(&serialized_options));

  // Parse the MessageOptions using our custom extension registry.
  io::ArrayInputStream input_stream(serialized_options.data(),
                                    serialized_options.size());
  io::CodedInputStream coded_stream(&input_stream);
  coded_stream.SetExtensionRegistry(&pool, &factory);
  MessageOptions message_options;
  ASSERT_TRUE(message_options.ParseFromCodedStream(&coded_stream));

  // Finally, serialize the proto again and verify that the repeated option has
  // been correctly serialized in packed form.
  std::string reserialized_options;
  ASSERT_TRUE(message_options.SerializeToString(&reserialized_options));
  EXPECT_EQ(reserialized_options, "\xca\xb5\x18\x01\x01");
}

TEST(ExtensionSetTest, Proto3ExtensionPresenceSingular) {
  using protobuf_unittest::Proto3FileExtensions;
  FileDescriptorProto file;

  EXPECT_FALSE(file.options().HasExtension(Proto3FileExtensions::singular_int));
  EXPECT_EQ(file.options().GetExtension(Proto3FileExtensions::singular_int), 0);

  file.mutable_options()->SetExtension(Proto3FileExtensions::singular_int, 1);

  EXPECT_TRUE(file.options().HasExtension(Proto3FileExtensions::singular_int));
  EXPECT_EQ(file.options().GetExtension(Proto3FileExtensions::singular_int), 1);
}

TEST(ExtensionSetTest, BoolExtension) {
  unittest::TestAllExtensions msg;
  uint8_t wire_bytes[2] = {13 * 8, 42 /* out of bounds payload for bool */};
  EXPECT_TRUE(msg.ParseFromArray(wire_bytes, 2));
  EXPECT_TRUE(msg.GetExtension(protobuf_unittest::optional_bool_extension));
}

TEST(ExtensionSetTest, ConstInit) {
  PROTOBUF_CONSTINIT static ExtensionSet set{};
  EXPECT_EQ(set.NumExtensions(), 0);
}

TEST(ExtensionSetTest, ExtensionSetSpaceUsed) {
  unittest::TestAllExtensions msg;
  size_t l = msg.SpaceUsedLong();
  msg.SetExtension(unittest::optional_int32_extension, 100);
  unittest::TestAllExtensions msg2(msg);
  size_t l2 = msg2.SpaceUsedLong();
  msg.ClearExtension(unittest::optional_int32_extension);
  unittest::TestAllExtensions msg3(msg);
  size_t l3 = msg3.SpaceUsedLong();
  EXPECT_TRUE((l2 - l) > (l3 - l));
}

TEST(ExtensionSetTest, Descriptor) {
  EXPECT_EQ(
      GetExtensionReflection(unittest::optional_int32_extension),
      unittest::TestAllExtensions::descriptor()->file()->FindExtensionByName(
          "optional_int32_extension"));
  EXPECT_NE(GetExtensionReflection(unittest::optional_int32_extension),
            nullptr);
  EXPECT_EQ(GetExtensionReflection(pb::cpp),
            pb::CppFeatures::descriptor()->file()->FindExtensionByName("cpp"));
  EXPECT_NE(GetExtensionReflection(pb::cpp), nullptr);
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
