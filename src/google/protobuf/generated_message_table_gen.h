#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_GEN_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_GEN_H__

#include <cstdint>

#include "google/protobuf/descriptor.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

// This file contains types and APIs to generate tables for v2 wireformat.

namespace google {
namespace protobuf {
namespace internal {
namespace v2 {

struct FieldTypeInfo {
  bool is_inlined;
  bool is_lazy;
};

// Returns 8 bit type card for a given field. Type cards contains information
// about field types and cardinality that are needed to iterate fields per
// message.
PROTOBUF_EXPORT uint8_t MakeTypeCardForField(const FieldDescriptor* field,
                                             FieldTypeInfo info);

}  // namespace v2
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_GEN_H__
