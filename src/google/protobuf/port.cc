// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
#include "google/protobuf/port.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <variant>

#include "absl/base/attributes.h"
#include "absl/log/absl_log.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"

// Must be included last
#include "google/protobuf/port_def.inc"

#if !defined(PROTO2_OPENSOURCE)
#if defined(PROTOBUF_INTERNAL_BOUNDS_CHECK_MODE_ABORT)
extern "C" {
#if ABSL_HAVE_ATTRIBUTE(used) && ABSL_HAVE_ATTRIBUTE(retain)
__attribute__((used, retain))
#endif  // ABSL_HAVE_ATTRIBUTE(used) && ABSL_HAVE_ATTRIBUTE(retain)
bool kVersionStampBuildHasHardeningProtobuf = true;
}
#endif  // defined(PROTOBUF_INTERNAL_BOUNDS_CHECK_MODE_ABORT)
#endif  // !defined(PROTO2_OPENSOURCE)

namespace google {
namespace protobuf {
namespace internal {

size_t StringSpaceUsedExcludingSelfLong(const std::string& str) {
  const void* start = &str;
  const void* end = &str + 1;
  if (start <= str.data() && str.data() < end) {
    // The string's data is stored inside the string object itself.
    return 0;
  } else {
    return str.capacity() + /* NULL terminator */ 1;
  }
}

// protobuf_assumption_failed() is declared and used in port_def.inc to assert
// PROTOBUF_ASSUME conditions in debug mode. This function avoids having
// port_def.inc depend on assert.h or other headers, minimizing the compilation
// footprint.
void protobuf_assumption_failed(const char* pred, const char* file, int line) {
  fprintf(stderr, "%s: %d: Assumption failed: '%s'\n", file, line, pred);
  abort();
}

static void PrintAllCounters();
static auto& CounterMap() {
  using Map = std::map<
      absl::string_view,
      std::map<std::variant<int64_t, absl::string_view>,
               std::array<std::atomic<size_t>, RealDebugCounter::kNumBuckets>>>;
  static auto* counter_map = new Map{};
  static bool dummy = std::atexit(PrintAllCounters);
  (void)dummy;
  return *counter_map;
}

static std::string CreateHistogram(
    absl::string_view category_name, absl::string_view subname, size_t subtotal,
    absl::Span<const std::atomic<size_t>> buckets) {
  std::string result;
  absl::StrAppendFormat(&result, "%s.%s:\n", category_name, subname);
  size_t sub = 0;
  for (size_t i = 0; i < buckets.size(); ++i) {
    if (sub == subtotal) {
      // No more buckets to print.
      break;
    }
    size_t v = buckets[i].load(std::memory_order_relaxed);
    if (sub == 0 && v == 0) {
      // Skip until the first non-zero bucket.
      continue;
    }

    sub += v;
    double num_chars = v * 100.0 / subtotal;
    static constexpr std::array kBlocks = {"▏", "▎", "▍", "▋", "▊", "▉"};
    std::string chars;
    while (num_chars > 1) {
      chars += kBlocks.back();
      --num_chars;
    }
    int last_char = std::round(num_chars * kBlocks.size());
    if (last_char > 0) {
      chars += kBlocks[last_char - 1];
    }

    absl::StrAppendFormat(&result, "[%2d]:%s\n", i, chars);
  }
  return result;
}

static void PrintAllCounters() {
  auto& counters = CounterMap();
  if (counters.empty()) return;
  absl::FPrintF(stderr, "Protobuf debug counters:\n");

  std::vector<std::string> histograms;
  for (auto& [category_name, category_map] : counters) {
    // Example output:
    //
    //   Category  :
    //     Value 1 : 1234 (12.34%)
    //     Value 2 : 2345 (23.45%)
    //     Total   : 3579
    absl::FPrintF(stderr, "  %-12s:\n", category_name);
    size_t total = 0;
    for (auto& entry : category_map) {
      size_t subtotal = 0;
      for (auto& counter : entry.second) {
        subtotal += counter.load(std::memory_order_relaxed);
      }
      if (subtotal != entry.second[0].load(std::memory_order_relaxed)) {
        histograms.push_back(
            CreateHistogram(category_name,
                            std::holds_alternative<int64_t>(entry.first)
                                ? absl::StrCat(std::get<int64_t>(entry.first))
                                : std::get<absl::string_view>(entry.first),
                            subtotal, entry.second));
      }
      total += subtotal;
    }

    for (auto& [subname, counter_array] : category_map) {
      size_t value = 0;
      for (auto& counter : counter_array) {
        value += counter.load(std::memory_order_relaxed);
      }
      if (std::holds_alternative<int64_t>(subname)) {
        // For integers, right align
        absl::FPrintF(stderr, "    %9d : %10zu", std::get<int64_t>(subname),
                      value);
      } else {
        // For strings, left align
        absl::FPrintF(stderr, "    %-10s: %10zu",
                      std::get<absl::string_view>(subname), value);
      }
      if (total != 0 && category_map.size() > 1) {
        absl::FPrintF(
            stderr, " (%5.2f%%)",
            100. * static_cast<double>(value) / static_cast<double>(total));
      }
      absl::FPrintF(stderr, "\n");
    }
    if (total != 0 && category_map.size() > 1) {
      absl::FPrintF(stderr, "    %-10s: %10zu\n", "Total", total);
    }
  }

  if (!histograms.empty()) {
    absl::FPrintF(stderr, "---Histograms---\n");
    for (const auto& h : histograms) {
      absl::FPrintF(stderr, "%s\n", h);
    }
  }
}

void RealDebugCounter::Register(absl::string_view name) {
  std::pair<absl::string_view, absl::string_view> parts =
      absl::StrSplit(name, '.');
  int64_t as_int;
  if (absl::SimpleAtoi(parts.second, &as_int)) {
    counters_ = CounterMap()[parts.first][as_int].data();
  } else {
    counters_ = CounterMap()[parts.first][parts.second].data();
  }
}

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 GlobalEmptyString
        fixed_address_empty_string{};

void HandleAddOverflow(int a, int b) {
  ABSL_LOG(FATAL) << "Integer overflow in CheckedAdd: " << a << " + " << b;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
