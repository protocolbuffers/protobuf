#include "google/protobuf/generated_message_table_gen.h"

#include <cctype>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>
#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/generated_message_table.h"
#include "google/protobuf/port.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_mset.pb.h"

namespace google {
namespace protobuf {
namespace internal {
namespace v2 {

class V2TableGenTester {
 public:
  static uint32_t HasBitIndex(const Reflection* reflection,
                              const FieldDescriptor* field) {
    return reflection->schema_.HasBitIndex(field);
  }
  static uint32_t GetFieldOffset(const Reflection* reflection,
                                 const FieldDescriptor* field) {
    return reflection->schema_.GetFieldOffset(field);
  }
  static bool IsLazyField(const Reflection* reflection,
                          const FieldDescriptor* field) {
    ABSL_CHECK(!field->is_extension());
    return reflection->IsLazyField(field);
  }
  static bool IsInlined(const Reflection* reflection,
                        const FieldDescriptor* field) {
    return reflection->schema_.IsFieldInlined(field);
  }
};

namespace {

using ::protobuf_unittest::TestAllTypes;
using ::protobuf_unittest::TestMessageSetExtension1;

// Creates FieldEntry that won't require AuxEntry, which requires all fields to
// fit into smaller (but common) limit. Specifically, hasbit_index for 1B,
// offset and field number for 2B.
FieldEntry CreateFieldEntryWithoutAux(const Reflection* reflection,
                                      const Message* message,
                                      const FieldDescriptor* field) {
  ABSL_CHECK_EQ(reflection, message->GetReflection());

  uint32_t hasbit_index = V2TableGenTester::HasBitIndex(reflection, field);
  uint32_t offset = V2TableGenTester::GetFieldOffset(reflection, field);

  // CHECK if "field" cannot fit into FieldEntry alone and require AuxEntry.
  static constexpr uint32_t kNoHasbit = static_cast<uint32_t>(-1);
  ABSL_CHECK(hasbit_index == kNoHasbit ||
             hasbit_index < FieldEntry::kHasbitIdxLimit);
  ABSL_CHECK_LT(offset, FieldEntry::kOffsetLimit);
  ABSL_CHECK_LT(field->number(), FieldEntry::kFieldNumberLimit);

  bool is_lazy = V2TableGenTester::IsLazyField(reflection, field);
  bool is_inlined = V2TableGenTester::IsInlined(reflection, field);

  return FieldEntry(MakeTypeCardForField(field, {is_inlined, is_lazy}),
                    hasbit_index, offset, field->number());
}

class TableGenTest : public testing::TestWithParam<const Message*> {
 public:
  TableGenTest()
      : message_(GetParam()), reflection_(message_->GetReflection()) {}

 protected:
  const Message* message_;
  const Reflection* reflection_;
};

TEST_P(TableGenTest, ValidateTypeCardForField) {
  const Descriptor* desc = message_->GetDescriptor();
  for (int i = 0, count = desc->field_count(); i < count; ++i) {
    const FieldDescriptor* field = desc->field(i);
    auto field_entry = CreateFieldEntryWithoutAux(reflection_, message_, field);

    // Validate cardinality.
    EXPECT_EQ(field->is_repeated(), field_entry.IsRepeated());
    uint8_t cardinality = field_entry.GetCardinality();
    switch (cardinality) {
      case Cardinality::kRepeated:
        EXPECT_TRUE(field->is_repeated());
        break;
      case Cardinality::kOptional:
        EXPECT_FALSE(field->is_repeated());
        EXPECT_TRUE(field->has_presence());
        break;
      case Cardinality::kSingular:
        EXPECT_FALSE(field->is_repeated());
        EXPECT_FALSE(field->has_presence());
        break;
      case Cardinality::kOneof:
        EXPECT_FALSE(field->is_repeated());
        EXPECT_TRUE(field->real_containing_oneof());
        break;
      default:
        Unreachable();
        break;
    }
    EXPECT_EQ(field->is_repeated(), field_entry.IsRepeated());

    // Validate field types, etc.
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_ENUM:
      case FieldDescriptor::CPPTYPE_INT32:
        EXPECT_EQ(field_entry.GetFieldKind(), FieldKind::kFixed32);
        EXPECT_TRUE(field_entry.IsSigned());
        break;
      case FieldDescriptor::CPPTYPE_INT64:
        EXPECT_EQ(field_entry.GetFieldKind(), FieldKind::kFixed64);
        EXPECT_TRUE(field_entry.IsSigned());
        break;
      case FieldDescriptor::CPPTYPE_FLOAT:
      case FieldDescriptor::CPPTYPE_UINT32:
        EXPECT_EQ(field_entry.GetFieldKind(), FieldKind::kFixed32);
        EXPECT_FALSE(field_entry.IsSigned());
        break;
      case FieldDescriptor::CPPTYPE_DOUBLE:
      case FieldDescriptor::CPPTYPE_UINT64:
        EXPECT_EQ(field_entry.GetFieldKind(), FieldKind::kFixed64);
        EXPECT_FALSE(field_entry.IsSigned());
        break;
      case FieldDescriptor::CPPTYPE_BOOL:
        EXPECT_EQ(field_entry.GetFieldKind(), FieldKind::kFixed8);
        EXPECT_FALSE(field_entry.IsSigned());
        break;
      case FieldDescriptor::CPPTYPE_STRING:
        EXPECT_EQ(field->requires_utf8_validation(), field_entry.IsUTF8())
            << field->full_name();

        switch (field->cpp_string_type()) {
          case FieldDescriptor::CppStringType::kView:
            EXPECT_EQ(field_entry.GetStringKind(), StringKind::kView);
            break;
          case FieldDescriptor::CppStringType::kCord:
            EXPECT_EQ(field_entry.GetStringKind(), StringKind::kCord);
            break;
          case FieldDescriptor::CppStringType::kString:
            if (field->is_repeated()) {
              EXPECT_EQ(field_entry.GetStringKind(), StringKind::kStringPtr);
            } else if (V2TableGenTester::IsInlined(reflection_, field)) {
              EXPECT_EQ(field_entry.GetStringKind(), StringKind::kInlined);
            } else {
              EXPECT_EQ(field_entry.GetStringKind(), StringKind::kArenaPtr);
            }
            break;
        }
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE:
        break;
      default:
        Unreachable();
        break;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    V2, TableGenTest,
    testing::Values(&TestAllTypes::default_instance(),
                    &TestMessageSetExtension1::default_instance()),
    [](const testing::TestParamInfo<TableGenTest::ParamType>& info) {
      std::string name = info.param->GetTypeName();
      absl::c_replace_if(name, [](char c) { return !std::isalnum(c); }, '_');
      return name;
    });

}  // namespace
}  // namespace v2
}  // namespace internal
}  // namespace protobuf
}  // namespace google
