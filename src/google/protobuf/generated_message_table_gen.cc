#include "google/protobuf/generated_message_table_gen.h"

#include <cstdint>

#include "absl/log/absl_check.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_table.h"
#include "google/protobuf/port.h"

namespace google {
namespace protobuf {
namespace internal {
namespace v2 {

using CppStringType = FieldDescriptor::CppStringType;

namespace {

uint8_t GenerateStringKind(const FieldDescriptor* field, bool is_inlined) {
  switch (field->cpp_string_type()) {
    // VIEW fields are treated as strings for now.
    case CppStringType::kView:
    case CppStringType::kString:
      return field->is_repeated() ? StringKind::kStringPtr
             : is_inlined         ? StringKind::kInlined
                                  : StringKind::kArenaPtr;
    case CppStringType::kCord:
      ABSL_CHECK(!is_inlined);
      return StringKind::kCord;
    default:
      Unreachable();
      break;
  }
}

}  // namespace

uint8_t MakeTypeCardForField(const FieldDescriptor* field, FieldTypeInfo info) {
  constexpr uint8_t field_type_to_type_card[] = {
      0,                     // placeholder as type starts from 1.
      FieldType::kDouble,    // TYPE_DOUBLE
      FieldType::kFloat,     // TYPE_FLOAT
      FieldType::kInt64,     // TYPE_INT64
      FieldType::kUInt64,    // TYPE_UINT64
      FieldType::kInt32,     // TYPE_INT32
      FieldType::kFixed64,   // TYPE_FIXED64
      FieldType::kFixed32,   // TYPE_FIXED32
      FieldType::kBool,      // TYPE_BOOL
      FieldType::kBytes,     // TYPE_STRING
      FieldType::kGroup,     // TYPE_GROUP
      FieldType::kMessage,   // TYPE_MESSAGE
      FieldType::kBytes,     // TYPE_BYTES
      FieldType::kUInt32,    // TYPE_UINT32
      FieldType::kEnum,      // TYPE_ENUM
      FieldType::kSFixed32,  // TYPE_SFIXED32
      FieldType::kSFixed64,  // TYPE_SFIXED64
      FieldType::kSInt32,    // TYPE_SINT32
      FieldType::kSInt64,    // TYPE_SINT64
  };
  static_assert(
      sizeof(field_type_to_type_card) == (FieldDescriptor::MAX_TYPE + 1), "");

  if (field->is_map()) return FieldType::kMap;

  auto field_type = field->type();
  uint8_t type_card = field_type_to_type_card[field_type];
  // Override previously set type for lazy message and UTF8 strings.
  switch (field_type) {
    case FieldDescriptor::TYPE_MESSAGE:
      if (info.is_lazy) type_card = FieldType::kLazyMessage;
      break;
    case FieldDescriptor::TYPE_STRING:
      if (field->requires_utf8_validation()) type_card = FieldType::kString;
      break;
    default:
      break;
  }

  // Set cardinality.
  if (field->is_repeated()) {
    type_card |= Cardinality::kRepeated;
  } else if (field->real_containing_oneof()) {
    type_card |= Cardinality::kOneof;
  } else if (field->has_presence()) {
    type_card |= Cardinality::kOptional;
  } else {
    type_card |= Cardinality::kSingular;
  }

  // Set StringKind for string fields. Note that numerics (signedness) and
  // messages (lazy) are already specified.
  return field->cpp_type() != FieldDescriptor::CPPTYPE_STRING
             ? type_card
             : type_card | GenerateStringKind(field, info.is_inlined);
}

}  // namespace v2
}  // namespace internal
}  // namespace protobuf
}  // namespace google
