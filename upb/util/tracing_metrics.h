// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_UTIL_TRACING_METRICS_H_
#define UPB_UTIL_TRACING_METRICS_H_

#include <memory>

#include "absl/strings/string_view.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/message.h"

namespace upb::tracing {

class Metrics {
 public:
  Metrics() = default;
  explicit Metrics(const Metrics*);
  void DebugPrint(absl::string_view tag) const;

 private:
  Metrics& operator=(const Metrics&) = delete;
  void LogMessageNew(const upb_MiniTable* mini_table, const upb_Arena* arena);
  std::unique_ptr<Metrics> Clone() const;

  int arena_count_ = 0;

  friend class MetricsCollector;
};

// Collects allocation metrics on upb arena(s), messages and parse latency.
//
// This class is thread-safe.
// Usage:
//   On process startup call MetricsCollector::Create().
//   To read metrics use MetricsCollector::Snapshot()
class MetricsCollector {
 public:
  // Start collecting metrics.
  static MetricsCollector& Create();
  // Returns a snapshot of collected metrics.
  static std::unique_ptr<Metrics> Snapshot();

 private:
  static void LogMessageNewHandler(const upb_MiniTable* mini_table,
                                   const upb_Arena* arena);
  Metrics metrics_;
};

}  // namespace upb::tracing

#endif  // UPB_UTIL_TRACING_METRICS_H_
