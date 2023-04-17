// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/generated_enum_util.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/types/optional.h"
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

bool ValidateEnum(int value, const uint16_t* data) {
  return ValidateEnumInlined(value, data);
}

struct EytzingerLayoutSorter {
  absl::Span<const int32_t> input;
  absl::Span<uint16_t> output;
  int i;

  void Sort(size_t output_index = 1) {
    if (output_index <= input.size()) {
      Sort(2 * output_index);
      output[output_index * 2 - 2] = input[i];
      output[output_index * 2 - 1] = input[i] >> 16;
      i++;
      Sort(2 * output_index + 1);
    }
  }
};

std::vector<uint16_t> GenerateEnumData(const std::vector<int32_t>& values) {
  const auto sorted_and_unique = [&] {
    for (size_t i = 0; i + 1 < values.size(); ++i) {
      if (values[i] >= values[i + 1]) return false;
    }
    return true;
  };
  ABSL_DCHECK(sorted_and_unique());
  std::vector<int32_t> fallback_values_too_large, fallback_values_after_bitmap;
  std::vector<uint16_t> bitmap_values;
  absl::optional<int16_t> start_sequence;
  uint16_t sequence_length = 0;
  for (int32_t v : values) {
    if (static_cast<int16_t>(v) != v) {
      fallback_values_too_large.push_back(v);
      continue;
    }
    // If we don't yet have a sequence, start it.
    if (!start_sequence.has_value()) {
      start_sequence = v;
      sequence_length = 1;
      continue;
    }
    // If we can extend the sequence, do so.
    if (v == *start_sequence + sequence_length && sequence_length < 0xFFFF) {
      ++sequence_length;
      continue;
    }

    // We adjust the bitmap values to be relative to the end of the sequence.
    const auto adjust = [&](int32_t v) -> uint32_t {
      // Cast to int64_t first to avoid overflow. The result is guaranteed to be
      // positive and fit in uint32_t.
      return static_cast<int64_t>(v) - *start_sequence - sequence_length;
    };
    const uint32_t adjusted = adjust(v);

    const auto add_bit = [&](uint32_t bit) {
      bitmap_values[bit / 16] |= 1 << (bit % 16);
    };

    // If we can fit it on the already allocated bitmap, do so.
    if (adjusted < 16 * bitmap_values.size()) {
      // We can fit it in the existing bitmap.
      ABSL_DCHECK_EQ(fallback_values_after_bitmap.size(), 0);
      add_bit(adjusted);
      continue;
    }

    // We can't fit in the sequence and we can't fit in the current bitmap.
    // Evaluate if it is better to add to fallback, or to collapse all the
    // fallback values after the bitmap into the bitmap.
    const size_t cost_if_fallback =
        sizeof(uint16_t) * bitmap_values.size() +
        sizeof(int32_t) * (1 + fallback_values_after_bitmap.size());
    const size_t rounded_bitmap_size = (adjusted + 1 + 15) / 16;
    const size_t cost_if_collapse = sizeof(uint16_t) * rounded_bitmap_size;

    if (cost_if_collapse <= cost_if_fallback) {
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

  std::vector<uint16_t> output(
      4 /* seq start + seq len + bitmap len + ordered len */ +
      bitmap_values.size() + 2 * fallback_values.size());

  output[0] = start_sequence.value_or(0);
  output[1] = sequence_length;
  output[2] = 16 * bitmap_values.size();
  output[3] = fallback_values.size();
  auto sorted_start =
      std::copy(bitmap_values.begin(), bitmap_values.end(), output.data() + 4);

  EytzingerLayoutSorter{
      fallback_values, absl::MakeSpan(sorted_start, 2 * fallback_values.size())}
      .Sort();

  return output;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
