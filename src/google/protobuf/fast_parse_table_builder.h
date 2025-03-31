#ifndef GOOGLE_PROTOBUF_FAST_PARSE_TABLE_BUILDER_H__
#define GOOGLE_PROTOBUF_FAST_PARSE_TABLE_BUILDER_H__

#include <cstddef>
#include <cstdint>
#include <vector>

#include "absl/types/optional.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace internal {

class FastParseTableBuilderBase {
 public:
  static constexpr size_t kMaxFastFields = 32;

  static uint32_t RecodeTagForFastParsing(uint32_t tag);

  static uint32_t GetRecodedTagForFastParsing(const FieldDescriptor* field);

  static absl::optional<uint32_t> GetEndGroupTag(const Descriptor* descriptor);

  static uint32_t TagToIdx(uint32_t tag, size_t fast_table_size);

  // The largest table we allow has the same number of entries as the message
  // has fields, rounded up to the next power of 2 (e.g., a message with 5
  // fields can have a fast table of size 8). A larger table *might* cover more
  // fields in certain cases, but a larger table in that case would have mostly
  // empty entries; so, we cap the size to avoid pathologically sparse tables.
  // However, if this message uses group encoding, the tables are sometimes very
  // sparse because the fields in the group avoid using the same field numbering
  // as the parent message (even though currently, the proto compiler allows the
  // overlap, and there is no possible conflict.)
  static uint32_t FastParseTableSize(size_t num_fields,
                                     absl::optional<uint32_t> end_group_tag);

  static bool IsFieldTypeEligibleForFastParsing(const FieldDescriptor* field);
};

template <typename EntryRef, typename Output>
class FastParseTableBuilder : public FastParseTableBuilderBase {
 public:
  virtual ~FastParseTableBuilder() = default;

  std::vector<Output> Build(absl::optional<uint32_t> end_group_tag);

 private:
  virtual size_t NumFields() const = 0;
  virtual EntryRef GetEntry(size_t index) const = 0;

  virtual const FieldDescriptor* GetField(EntryRef entry) = 0;
  virtual bool IsFieldEligibleForFastParsing(EntryRef entry) = 0;

  virtual Output BuildOutputFromEntry(EntryRef entry, uint32_t tag) = 0;
  virtual Output BuildOutputFromEndGroupTag(uint32_t end_group_tag) = 0;

  virtual float PresenceProbability(EntryRef entry) = 0;
  virtual float OutputPresenceProbability(const Output& output) = 0;
};

template <typename Entry, typename Output>
std::vector<Output> FastParseTableBuilder<Entry, Output>::Build(
    absl::optional<uint32_t> end_group_tag) {
  uint32_t occupied_fields = 0;
  // Bit mask for the fields that are "important". Unimportant fields might be
  // set but it's ok if we lose them from the fast table. For example, cold
  // fields.
  uint32_t important_fields = 0;
  static_assert(sizeof(important_fields) * 8 >= kMaxFastFields, "");
  size_t num_fast_fields = FastParseTableSize(NumFields(), end_group_tag);

  std::vector<Output> fast_parse_table(num_fast_fields);

  if (end_group_tag.has_value() && (*end_group_tag >> 14) == 0) {
    // Fits in 1 or 2 varint bytes.
    const uint32_t tag = RecodeTagForFastParsing(*end_group_tag);
    const uint32_t fast_idx = TagToIdx(tag, fast_parse_table.size());

    fast_parse_table[fast_idx] = BuildOutputFromEndGroupTag(*end_group_tag);
    occupied_fields |= uint32_t{1} << fast_idx;
    important_fields |= uint32_t{1} << fast_idx;
  }

  for (size_t i = 0; i < NumFields(); ++i) {
    const auto entry = GetEntry(i);
    if (!IsFieldEligibleForFastParsing(entry)) {
      continue;
    }

    const auto* field = GetField(entry);
    const uint32_t tag = GetRecodedTagForFastParsing(field);
    const uint32_t fast_idx = TagToIdx(tag, fast_parse_table.size());

    auto& output = fast_parse_table[fast_idx];
    // Skip if previous entry is more likely present.
    const float presence_probability = PresenceProbability(entry);
    if (OutputPresenceProbability(output) >= presence_probability) {
      continue;
    }

    output = BuildOutputFromEntry(entry, tag);

    // 0.05 was selected based on load tests where 0.1 and 0.01 were also
    // evaluated and worse.
    constexpr float kMinPresence = 0.05f;
    occupied_fields |= uint32_t{1} << fast_idx;
    important_fields |= uint32_t{presence_probability >= kMinPresence}
                        << fast_idx;
  }

  // If we can halve the table without dropping important fields, do it.
  while (num_fast_fields > 1 &&
         (important_fields & (important_fields >> num_fast_fields / 2)) == 0) {
    // Half the table by merging fields.
    num_fast_fields /= 2;
    for (size_t i = 0; i < num_fast_fields; ++i) {
      size_t merge_i = i + num_fast_fields;
      // Overwrite the surviving entries if the discarded half contains an
      // important field (meaning the surviving entry is not) or the surviving
      // entry is empty.
      if (((important_fields >> merge_i) & 1) != 0 ||
          ((occupied_fields >> i) & 1) == 0) {
        fast_parse_table[i] = fast_parse_table[merge_i];
      }
    }
    important_fields |= important_fields >> num_fast_fields;
    occupied_fields |= occupied_fields >> num_fast_fields;
  }
  fast_parse_table.resize(num_fast_fields);

  return fast_parse_table;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_FAST_PARSE_TABLE_BUILDER_H__
