#include "google/protobuf/generated_message_table.h"

// Must be included after other headers.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace v2 {

// Messages without any fields can just point to this special table instead of
// creating their own.
extern constexpr MessageTable<0> kEmptyMessageTable = {
    {/*has_bits_offset*/ 0, /*extension_offset*/ 0, /*field_count*/ 0,
     /*oneof_field_count*/ 0, /*split_field_count*/ 0, /*oneof_case_count*/ 0,
     /*aux_offset*/ 0}};

}  // namespace v2
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
