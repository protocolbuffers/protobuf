// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/generated_enum_util.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "google/protobuf/generated_message_util.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {

bool EnumCompareByName(const EnumEntry& a, const EnumEntry& b) {
  return absl::string_view(a.name) < absl::string_view(b.name);
}

// Gets the numeric value of the EnumEntry at the given index, but returns a
// special value for the index -1. This gives a way to use std::lower_bound on a
// sorted array of indices while searching for value that we associate with -1.
int GetValue(const EnumEntry* enums, int i, int target) {
  if (i == -1) {
    return target;
  } else {
    return enums[i].value;
  }
}

}  // namespace

bool LookUpEnumValue(const EnumEntry* enums, size_t size,
                     absl::string_view name, int* value) {
  EnumEntry target{name, 0};
  auto it = std::lower_bound(enums, enums + size, target, EnumCompareByName);
  if (it != enums + size && it->name == name) {
    *value = it->value;
    return true;
  }
  return false;
}

int LookUpEnumName(const EnumEntry* enums, const int* sorted_indices,
                   size_t size, int value) {
  auto comparator = [enums, value](int a, int b) {
    return GetValue(enums, a, value) < GetValue(enums, b, value);
  };
  auto it =
      std::lower_bound(sorted_indices, sorted_indices + size, -1, comparator);
  if (it != sorted_indices + size && enums[*it].value == value) {
    return it - sorted_indices;
  }
  return -1;
}

bool InitializeEnumStrings(
    const EnumEntry* enums, const int* sorted_indices, size_t size,
    internal::ExplicitlyConstructed<std::string>* enum_strings) {
  for (size_t i = 0; i < size; ++i) {
    enum_strings[i].Construct(enums[sorted_indices[i]].name);
    internal::OnShutdownDestroyString(enum_strings[i].get_mutable());
  }
  return true;
}

bool ValidateEnum(int value, const uint32_t* data) {
  return ValidateEnumInlined(value, data);
}

struct EytzingerLayoutSorter {
  absl::Span<const int32_t> input;
  absl::Span<uint32_t> output;
  size_t i;

  // This is recursive, but the maximum depth is log(N), so it should be safe.
  void Sort(size_t output_index = 0) {
    if (output_index < input.size()) {
      Sort(2 * output_index + 1);
      output[output_index] = input[i++];
      Sort(2 * output_index + 2);
    }
  }
};

std::vector<uint32_t> GenerateEnumData(absl::Span<const int32_t> values) {
  const auto sorted_and_unique = [&] {
    for (size_t i = 0; i + 1 < values.size(); ++i) {
      if (values[i] >= values[i + 1]) return false;
    }
    return true;
  };
  ABSL_DCHECK(sorted_and_unique());
  std::vector<int32_t> fallback_values_too_large, fallback_values_after_bitmap;
  std::vector<uint32_t> bitmap_values;
  constexpr size_t kBitmapBlockSize = 32;
  std::optional<int16_t> start_sequence;
  uint32_t sequence_length = 0;
  for (int32_t v : values) {
    // If we don't yet have a sequence, start it.
    if (!start_sequence.has_value()) {
      // But only if we can fit it in the sequence.
      if (static_cast<int16_t>(v) != v) {
        fallback_values_too_large.push_back(v);
        continue;
      }

      start_sequence = v;
      sequence_length = 1;
      continue;
    }
    // If we can extend the sequence, do so.
    if (v == static_cast<int32_t>(*start_sequence) +
                 static_cast<int32_t>(sequence_length) &&
        sequence_length < 0xFFFF) {
      ++sequence_length;
      continue;
    }

    // We adjust the bitmap values to be relative to the end of the sequence.
    const auto adjust = [&](int32_t v) -> uint32_t {
      // Cast to int64_t first to avoid overflow. The result is guaranteed to be
      // positive and fit in uint32_t.
      int64_t a = static_cast<int64_t>(v) - *start_sequence - sequence_length;
      ABSL_DCHECK(a >= 0);
      ABSL_DCHECK_EQ(a, static_cast<uint32_t>(a));
      return a;
    };
    const uint32_t adjusted = adjust(v);

    const auto add_bit = [&](uint32_t bit) {
      bitmap_values[bit / kBitmapBlockSize] |= uint32_t{1}
                                               << (bit % kBitmapBlockSize);
    };

    // If we can fit it on the already allocated bitmap, do so.
    if (adjusted < kBitmapBlockSize * bitmap_values.size()) {
      // We can fit it in the existing bitmap.
      ABSL_DCHECK_EQ(fallback_values_after_bitmap.size(), 0);
      add_bit(adjusted);
      continue;
    }

    // We can't fit in the sequence and we can't fit in the current bitmap.
    // Evaluate if it is better to add to fallback, or to collapse all the
    // fallback values after the bitmap into the bitmap.
    const size_t cost_if_fallback =
        bitmap_values.size() + (1 + fallback_values_after_bitmap.size());
    const size_t rounded_bitmap_size =
        (adjusted + kBitmapBlockSize) / kBitmapBlockSize;
    const size_t cost_if_collapse = rounded_bitmap_size;

    if (cost_if_collapse <= cost_if_fallback &&
        kBitmapBlockSize * rounded_bitmap_size < 0x10000) {
      // Collapse the existing values, and add the new one.
      ABSL_DCHECK_GT(rounded_bitmap_size, bitmap_values.size());
      bitmap_values.resize(rounded_bitmap_size);
      for (int32_t to_collapse : fallback_values_after_bitmap) {
        add_bit(adjust(to_collapse));
      }
      fallback_values_after_bitmap.clear();
      add_bit(adjusted);
    } else {
      fallback_values_after_bitmap.push_back(v);
    }
  }

  std::vector<int32_t> fallback_values;
  if (fallback_values_after_bitmap.empty()) {
    fallback_values = std::move(fallback_values_too_large);
  } else if (fallback_values_too_large.empty()) {
    fallback_values = std::move(fallback_values_after_bitmap);
  } else {
    fallback_values.resize(fallback_values_too_large.size() +
                           fallback_values_after_bitmap.size());
    std::merge(fallback_values_too_large.begin(),
               fallback_values_too_large.end(),
               fallback_values_after_bitmap.begin(),
               fallback_values_after_bitmap.end(), &fallback_values[0]);
  }

  std::vector<uint32_t> output(
      2 /* seq start + seq len + bitmap len + ordered len */ +
      bitmap_values.size() + fallback_values.size());
  uint32_t* p = output.data();

  ABSL_DCHECK_EQ(sequence_length, static_cast<uint16_t>(sequence_length));
  *p++ = uint32_t{static_cast<uint16_t>(start_sequence.value_or(0))} |
         (uint32_t{sequence_length} << 16);
  ABSL_DCHECK_EQ(
      kBitmapBlockSize * bitmap_values.size(),
      static_cast<uint16_t>(kBitmapBlockSize * bitmap_values.size()));
  ABSL_DCHECK_EQ(fallback_values.size(),
                 static_cast<uint16_t>(fallback_values.size()));
  *p++ = static_cast<uint32_t>(kBitmapBlockSize * bitmap_values.size()) |
         static_cast<uint32_t>(fallback_values.size() << 16);
  p = std::copy(bitmap_values.begin(), bitmap_values.end(), p);

  EytzingerLayoutSorter{fallback_values,
                        absl::MakeSpan(p, fallback_values.size())}
      .Sort();

  return output;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
