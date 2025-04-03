#include "google/protobuf/fast_parse_table_builder.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/numeric/bits.h"
#include "absl/types/optional.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/generated_message_tctable_gen.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"

namespace google {
namespace protobuf {
namespace internal {

template class FastParseTableBuilder<
    std::pair<const TailCallTableInfo::FieldEntryInfo&,
              const TailCallTableInfo::FieldOptions&>,
    TailCallTableInfo::FastFieldInfo>;

absl::optional<uint32_t> GetEndGroupTag(const Descriptor* descriptor) {
  auto* parent = descriptor->containing_type();
  if (parent == nullptr) return absl::nullopt;
  for (int i = 0; i < parent->field_count(); ++i) {
    auto* field = parent->field(i);
    if (field->type() == field->TYPE_GROUP &&
        field->message_type() == descriptor) {
      return WireFormatLite::MakeTag(field->number(),
                                     WireFormatLite::WIRETYPE_END_GROUP);
    }
  }
  return absl::nullopt;
}

uint32_t RecodeTagForFastParsing(uint32_t tag) {
  ABSL_DCHECK_LE(tag, 0x3FFFu);
  // Construct the varint-coded tag. If it is more than 7 bits, we need to
  // shift the high bits and add a continue bit.
  uint32_t hibits = tag & 0xFFFFFF80;
  if (hibits != 0) {
    // hi = tag & ~0x7F
    // lo = tag & 0x7F
    // This shifts hi to the left by 1 to the next byte and sets the
    // continuation bit.
    tag = tag + hibits + 0x80;
  }
  return tag;
}

uint32_t GetRecodedTagForFastParsing(const FieldDescriptor* field) {
  return RecodeTagForFastParsing(internal::WireFormat::MakeTag(field));
}

uint32_t TagToIdx(uint32_t tag, size_t fast_table_size) {
  // The fast table size must be a power of two.
  ABSL_DCHECK_EQ((fast_table_size & (fast_table_size - 1)), size_t{0});

  // The field index is determined by the low bits of the field number, where
  // the table size determines the width of the mask. The largest table
  // supported is 32 entries. The parse loop uses these bits directly, so that
  // the dispatch does not require arithmetic:
  //        byte 0   byte 1
  //   tag: 1nnnnttt 0nnnnnnn
  //        ^^^^^
  //         idx (table_size_log2=5)
  // This means that any field number that does not fit in the lower 4 bits
  // will always have the top bit of its table index asserted.
  uint32_t idx_mask = static_cast<uint32_t>(fast_table_size) - 1;
  return (tag >> 3) & idx_mask;
}

uint32_t FastParseTableBuilderBase::FastParseTableSize(
    size_t num_fields, absl::optional<uint32_t> end_group_tag) {
  // NOTE: The +1 is to maintain the existing behavior that does not match the
  // documented one. When the number of fields is exactly a power of two we
  // allow double that.
  return static_cast<uint32_t>(
      end_group_tag.has_value()
          ? kMaxFastFields
          : std::max(size_t{1},
                     std::min(kMaxFastFields, absl::bit_ceil(num_fields + 1))));
}

bool FastParseTableBuilderBase::IsFieldTypeEligibleForFastParsing(
    const FieldDescriptor* field) {
  // Map, oneof, weak, and split fields are not handled on the fast path.
  if (field->is_map() || field->real_containing_oneof() ||
      field->options().weak()) {
    return false;
  }

  switch (field->type()) {
      // Some bytes fields can be handled on fast path.
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES: {
      auto ctype = field->cpp_string_type();
      if (ctype == FieldDescriptor::CppStringType::kString ||
          ctype == FieldDescriptor::CppStringType::kView) {
        // strings are fine...
      } else if (ctype == FieldDescriptor::CppStringType::kCord) {
        // Cords are worth putting into the fast table, if they're not repeated
        if (field->is_repeated()) return false;
      } else {
        return false;
      }
      break;
    }

    default:
      break;
  }

  // The largest tag that can be read by the tailcall parser is two bytes
  // when varint-coded. This allows 14 bits for the numeric tag value:
  //   byte 0   byte 1
  //   1nnnnttt 0nnnnnnn
  //    ^^^^^^^  ^^^^^^^
  if (field->number() >= 1 << 11) return false;

  return true;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
