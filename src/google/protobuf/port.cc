// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
#include "google/protobuf/port.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"

// Must be included last
#include "google/protobuf/port_def.inc"

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
  using Map = std::map<absl::string_view,
                       std::map<std::variant<int64_t, absl::string_view>,
                                std::vector<const RealDebugCounter*>>>;
  static auto* counter_map = new Map{};
  static bool dummy = std::atexit(PrintAllCounters);
  (void)dummy;
  return *counter_map;
}

static void PrintAllCounters() {
  auto& counters = CounterMap();
  if (counters.empty()) return;
  absl::FPrintF(stderr, "Protobuf debug counters:\n");
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
      for (auto* counter : entry.second) {
        total += counter->value();
      }
    }
    for (auto& [subname, counter_vector] : category_map) {
      size_t value = 0;
      for (auto* counter : counter_vector) {
        value += counter->value();
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
}

void RealDebugCounter::Register(absl::string_view name) {
  std::pair<absl::string_view, absl::string_view> parts =
      absl::StrSplit(name, '.');
  int64_t as_int;
  if (absl::SimpleAtoi(parts.second, &as_int)) {
    CounterMap()[parts.first][as_int].push_back(this);
  } else {
    CounterMap()[parts.first][parts.second].push_back(this);
  }
}

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 GlobalEmptyString
        fixed_address_empty_string{};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
