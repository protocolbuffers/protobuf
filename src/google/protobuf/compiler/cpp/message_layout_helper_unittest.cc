#include <cstddef>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/compiler/cpp/padding_optimizer.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/test_textproto.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

using ::testing::ElementsAre;

std::vector<std::string> GetOptimizedFieldNames(const Descriptor* descriptor) {
  PaddingOptimizer optimizer(descriptor);
  std::vector<const FieldDescriptor*> fields;
  fields.reserve(static_cast<size_t>(descriptor->field_count()));
  for (int i = 0; i < descriptor->field_count(); ++i) {
    fields.push_back(descriptor->field(i));
  }
  Options options;
  auto optimized = optimizer.OptimizeLayout(fields, options);
  std::vector<std::string> names;
  names.reserve(optimized.size());
  for (const auto* field : optimized) {
    names.push_back(std::string(field->name()));
  }
  return names;
}

TEST(MessageLayoutHelperTest, TwoByteFieldAlignmentAndPairing) {
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "two_byte_test.proto"
    syntax: "proto2"
    enum_type {
      name: "Enum2Byte"
      value { name: "E2_A" number: 0 }
      value { name: "E2_B" number: 1000 }
    }
    message_type {
      name: "Test2ByteMessage"
      field { name: "int64_1" number: 1 type: TYPE_INT64 }
      field {
        name: "enum2_1"
        number: 2
        type: TYPE_ENUM
        type_name: ".Enum2Byte"
      }
      field {
        name: "enum2_2"
        number: 3
        type: TYPE_ENUM
        type_name: ".Enum2Byte"
      }
      field { name: "int32_1" number: 4 type: TYPE_INT32 }
    }
  )pb");

  DescriptorPool pool;
  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_NE(file, nullptr);

  const Descriptor* message = file->FindMessageTypeByName("Test2ByteMessage");
  ASSERT_NE(message, nullptr);

  // enum2_1 (2B) and enum2_2 (2B) consolidate into a 4B group.
  // The 4B group and int32_1 (4B) consolidate into an 8B group.
  // int64_1 is in the 8B group.
  EXPECT_THAT(GetOptimizedFieldNames(message),
              ElementsAre("int64_1", "enum2_1", "enum2_2", "int32_1"));
}

TEST(MessageLayoutHelperTest, HierarchicalConsolidation1To2To4To8) {
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "hierarchical_test.proto"
    syntax: "proto2"
    enum_type {
      name: "Enum2Byte"
      value { name: "E2_A" number: 0 }
      value { name: "E2_B" number: 1000 }
    }
    message_type {
      name: "TestHierarchyMessage"
      field { name: "int64_1" number: 1 type: TYPE_INT64 }
      field { name: "bool_1" number: 2 type: TYPE_BOOL }
      field { name: "bool_2" number: 3 type: TYPE_BOOL }
      field {
        name: "enum2_1"
        number: 4
        type: TYPE_ENUM
        type_name: ".Enum2Byte"
      }
      field { name: "int32_1" number: 5 type: TYPE_INT32 }
    }
  )pb");

  DescriptorPool pool;
  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_NE(file, nullptr);

  const Descriptor* message =
      file->FindMessageTypeByName("TestHierarchyMessage");
  ASSERT_NE(message, nullptr);

  // 1 -> 2: bool_1 (1B) + bool_2 (1B) -> 2B group
  // 2 -> 4: {bool_1, bool_2} (2B) + enum2_1 (2B) -> 4B group
  // 4 -> 8: {bool_1, bool_2, enum2_1} (4B) + int32_1 (4B) -> 8B group
  // 8B group and int64_1 (8B) form the message with 0 padding overhead.
  EXPECT_THAT(GetOptimizedFieldNames(message),
              ElementsAre("int64_1", "bool_1", "bool_2", "enum2_1", "int32_1"));
}

TEST(MessageLayoutHelperTest,
     PaddingOptimizationAcrossFamiliesWith2ByteLeftovers) {
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "family_padding_test.proto"
    syntax: "proto2"
    enum_type {
      name: "Enum2Byte"
      value { name: "E2_ZERO" number: 0 }
      value { name: "E2_VAL" number: 1000 }
    }
    message_type {
      name: "TestFamilyPaddingMessage"
      # ZERO_INITIALIZABLE family
      field { name: "z_int64" number: 1 type: TYPE_INT64 }
      field {
        name: "z_enum2"
        number: 2
        type: TYPE_ENUM
        type_name: ".Enum2Byte"
        default_value: "E2_ZERO"
      }
      # OTHER family (custom defaults)
      field {
        name: "o_enum2"
        number: 3
        type: TYPE_ENUM
        type_name: ".Enum2Byte"
        default_value: "E2_VAL"
      }
      field { name: "o_int32" number: 4 type: TYPE_INT32 default_value: "100" }
      field { name: "o_int64" number: 5 type: TYPE_INT64 default_value: "100" }
    }
  )pb");

  DescriptorPool pool;
  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_NE(file, nullptr);

  const Descriptor* message =
      file->FindMessageTypeByName("TestFamilyPaddingMessage");
  ASSERT_NE(message, nullptr);

  // In ZERO_INITIALIZABLE: z_int64 is 8B. z_enum2 is an incomplete 2B block.
  // z_enum2 is moved to the end of ZERO_INITIALIZABLE.
  // In OTHER: o_enum2 (2B) + o_int32 (4B) = 6B (incomplete block < 8B).
  // The incomplete 6B block {o_enum2, o_int32} is hoisted to the beginning of
  // OTHER. Thus, z_enum2 (2B) and {o_enum2, o_int32} (6B) meet at the boundary
  // to form an 8B block!
  EXPECT_THAT(
      GetOptimizedFieldNames(message),
      ElementsAre("z_int64", "z_enum2", "o_enum2", "o_int32", "o_int64"));
}

TEST(MessageLayoutHelperTest, MultipleMixedSizesConsolidation) {
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "mixed_sizes_test.proto"
    syntax: "proto2"
    enum_type {
      name: "Enum1Byte"
      value { name: "E1_A" number: 0 }
      value { name: "E1_B" number: 10 }
    }
    enum_type {
      name: "Enum2Byte"
      value { name: "E2_A" number: 0 }
      value { name: "E2_B" number: 500 }
    }
    enum_type {
      name: "Enum4Byte"
      value { name: "E4_A" number: 0 }
      value { name: "E4_B" number: 100000 }
    }
    message_type {
      name: "TestMixedSizesMessage"
      field { name: "i64" number: 1 type: TYPE_INT64 }
      field { name: "b1" number: 2 type: TYPE_BOOL }
      field { name: "e1" number: 3 type: TYPE_ENUM type_name: ".Enum1Byte" }
      field { name: "e2" number: 4 type: TYPE_ENUM type_name: ".Enum2Byte" }
      field { name: "e4" number: 5 type: TYPE_ENUM type_name: ".Enum4Byte" }
    }
  )pb");

  DescriptorPool pool;
  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_NE(file, nullptr);

  const Descriptor* message =
      file->FindMessageTypeByName("TestMixedSizesMessage");
  ASSERT_NE(message, nullptr);

  // b1 (1B) + e1 (1B) -> 2B
  // {b1, e1} (2B) + e2 (2B) -> 4B
  // {b1, e1, e2} (4B) + e4 (4B) -> 8B
  // i64 (8B) + {b1, e1, e2, e4} (8B)
  EXPECT_THAT(GetOptimizedFieldNames(message),
              ElementsAre("i64", "b1", "e1", "e2", "e4"));
}

}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
