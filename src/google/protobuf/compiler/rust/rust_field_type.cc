#include "google/protobuf/compiler/rust/rust_field_type.h"

#include "absl/log/absl_log.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/port.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

RustFieldType GetRustFieldType(const FieldDescriptor& field) {
  return GetRustFieldType(field.type());
}

RustFieldType GetRustFieldType(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_BOOL:
      return RustFieldType::BOOL;
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32:
      return RustFieldType::INT32;
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64:
      return RustFieldType::INT64;
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
      return RustFieldType::UINT32;
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64:
      return RustFieldType::UINT64;
    case FieldDescriptor::TYPE_FLOAT:
      return RustFieldType::FLOAT;
    case FieldDescriptor::TYPE_DOUBLE:
      return RustFieldType::DOUBLE;
    case FieldDescriptor::TYPE_BYTES:
      return RustFieldType::BYTES;
    case FieldDescriptor::TYPE_STRING:
      return RustFieldType::STRING;
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP:
      return RustFieldType::MESSAGE;
    case FieldDescriptor::TYPE_ENUM:
      return RustFieldType::ENUM;
  }
  ABSL_LOG(ERROR) << "Unknown field type: " << type;
  internal::Unreachable();
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
