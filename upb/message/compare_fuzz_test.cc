#include <string>

#include <gtest/gtest.h>
#include "testing/fuzzing/fuzztest.h"
#include "absl/base/throw_delegate.h"
#include "google/protobuf/util/message_differencer.h"
#include "upb/mem/arena.h"
#include "upb/message/compare.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"
#include "upb/test/test.pb.h"
#include "upb/test/test.upb_minitable.h"
#include "upb/wire/decode.h"

namespace upb {
namespace {

static void CheckExtRegStatus(upb_ExtensionRegistryStatus status) {
  if (status == kUpb_ExtensionRegistryStatus_OutOfMemory) {
    absl::ThrowStdBadAlloc();
  } else {
    ASSERT_EQ(status, kUpb_ExtensionRegistryStatus_Ok);
  }
}
const upb_ExtensionRegistry* CreateTestExtensionRegistry(upb_Arena* arena) {
  upb_ExtensionRegistry* ext_registry = upb_ExtensionRegistry_New(arena);
  CheckExtRegStatus(upb_ExtensionRegistry_Add(
      ext_registry, upb_test_ModelExtension1_model_ext_ext));
  CheckExtRegStatus(upb_ExtensionRegistry_Add(
      ext_registry, upb_test_ModelExtension2_model_ext_2_ext));
  CheckExtRegStatus(upb_ExtensionRegistry_Add(
      ext_registry, upb_test_ModelExtension2_model_ext_3_ext));
  CheckExtRegStatus(upb_ExtensionRegistry_Add(
      ext_registry, upb_test_ModelExtension2_model_ext_4_ext));
  CheckExtRegStatus(upb_ExtensionRegistry_Add(
      ext_registry, upb_test_ModelExtension2_model_ext_5_ext));
  return ext_registry;
}

template <typename T>
void ComparePartialEqualityProto(const upb_MiniTable* mini_table,
                                 const upb_ExtensionRegistry* ext_registry,
                                 const T& cpp_actual, const T& cpp_expected) {
  std::string payload_actual = cpp_actual.SerializeAsString();
  std::string payload_expected = cpp_expected.SerializeAsString();

  // 1. C++ MessageDifferencer partial equality.
  // MessageDifferencer::PARTIAL checks fields present in the first argument.
  // UPB partial equality (upb_Message_IsEqual(actual, expected, ..., PARTIAL))
  // checks fields present in expected (msg2).
  // Therefore, cpp_expected is passed as the first argument.
  google::protobuf::util::MessageDifferencer differencer;
  differencer.set_scope(google::protobuf::util::MessageDifferencer::PARTIAL);
  bool cpp_equals = differencer.Compare(cpp_expected, cpp_actual);

  // 2. UPB partial equality.
  upb_Arena* arena = upb_Arena_New();

  upb_Message* upb_actual = upb_Message_New(mini_table, arena);
  upb_Message* upb_expected = upb_Message_New(mini_table, arena);

  upb_DecodeStatus status_actual =
      upb_Decode(payload_actual.data(), payload_actual.size(), upb_actual,
                 mini_table, ext_registry, 0, arena);
  upb_DecodeStatus status_expected =
      upb_Decode(payload_expected.data(), payload_expected.size(), upb_expected,
                 mini_table, ext_registry, 0, arena);

  ASSERT_EQ(status_actual, kUpb_DecodeStatus_Ok);
  ASSERT_EQ(status_expected, kUpb_DecodeStatus_Ok);

  bool upb_equals = upb_Message_IsEqual(upb_actual, upb_expected, mini_table,
                                        kUpb_CompareOption_Partial);

  EXPECT_EQ(upb_equals, cpp_equals)
      << "Mismatch between UPB and C++ partial equality!\n"
      << "cpp_equals: " << cpp_equals << ", upb_equals: " << upb_equals << "\n"
      << "cpp_actual: " << cpp_actual.DebugString() << "\n"
      << "cpp_expected: " << cpp_expected.DebugString();

  upb_Arena_Free(arena);
}

void DifferentialPartialEqualsScalarFuzz(
    const upb_test::HelloRequest& base_expected,
    const upb_test::HelloRequest& extra_actual) {
  upb_test::HelloRequest cpp_actual = base_expected;
  cpp_actual.MergeFrom(extra_actual);
  ComparePartialEqualityProto<upb_test::HelloRequest>(
      &upb_0test__HelloRequest_msg_init, nullptr, cpp_actual, base_expected);
}

void DifferentialPartialEqualsMapFuzz(
    const upb_test::ModelWithMaps& base_expected,
    const upb_test::ModelWithMaps& extra_actual) {
  upb_Arena* arena = upb_Arena_New();
  const upb_ExtensionRegistry* ext_registry =
      CreateTestExtensionRegistry(arena);

  upb_test::ModelWithMaps cpp_actual = base_expected;
  cpp_actual.MergeFrom(extra_actual);
  ComparePartialEqualityProto<upb_test::ModelWithMaps>(
      &upb_0test__ModelWithMaps_msg_init, ext_registry, cpp_actual,
      base_expected);

  upb_Arena_Free(arena);
}

void DifferentialPartialEqualsExtensionFuzz(
    const upb_test::ModelWithExtensions& base_expected,
    const upb_test::ModelWithExtensions& extra_actual) {
  upb_test::ModelWithExtensions cpp_actual = base_expected;
  cpp_actual.MergeFrom(extra_actual);

  upb_Arena* arena = upb_Arena_New();
  const upb_ExtensionRegistry* ext_registry =
      CreateTestExtensionRegistry(arena);

  ComparePartialEqualityProto<upb_test::ModelWithExtensions>(
      &upb_0test__ModelWithExtensions_msg_init, ext_registry, cpp_actual,
      base_expected);

  upb_Arena_Free(arena);
}

FUZZ_TEST(CompareFuzzTest, DifferentialPartialEqualsScalarFuzz);
FUZZ_TEST(CompareFuzzTest, DifferentialPartialEqualsMapFuzz);
FUZZ_TEST(CompareFuzzTest, DifferentialPartialEqualsExtensionFuzz);

TEST(CompareFuzzTest, PartialMapComparisonExtraKeysInActual) {
  upb_test::ModelWithMaps cpp_expected;
  (*cpp_expected.mutable_map_ss())["k1"] = "v1";

  upb_test::ModelWithMaps cpp_actual = cpp_expected;
  (*cpp_actual.mutable_map_ss())["k2"] = "v2";

  ComparePartialEqualityProto<upb_test::ModelWithMaps>(
      &upb_0test__ModelWithMaps_msg_init, nullptr, cpp_actual, cpp_expected);
}

TEST(CompareFuzzTest, PartialPresenceFieldMissingInActual) {
  upb_test::ModelWithExtensions cpp_expected;
  cpp_expected.set_random_name("");  // Presence is set to default value.

  upb_test::ModelWithExtensions cpp_actual;  // Presence is NOT set in actual.

  ComparePartialEqualityProto<upb_test::ModelWithExtensions>(
      &upb_0test__ModelWithExtensions_msg_init, nullptr, cpp_actual,
      cpp_expected);
}

}  // namespace
}  // namespace upb
