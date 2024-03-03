// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/util/tracing_metrics.h"

#include <syslog.h>

#include <memory>
#include <string>

#include "absl/base/attributes.h"
#include "absl/base/const_init.h"
#include "absl/base/no_destructor.h"
#include "absl/base/thread_annotations.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "upb/mem/arena.h"
#include "upb/mem/internal/arena.h"
#include "upb/mini_table/message.h"
#include "upb/tracing/tracing.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb::tracing {

ABSL_CONST_INIT static absl::Mutex metrics_mutex(absl::kConstInit);
static absl::NoDestructor<MetricsCollector> upb_collector
    ABSL_GUARDED_BY(metrics_mutex);

void MetricsCollector::LogMessageNewHandler(const upb_MiniTable* mini_table,
                                            const upb_Arena* arena) {
  absl::MutexLock lock(&metrics_mutex);
  upb_collector->metrics_.LogMessageNew(mini_table, arena);
}

MetricsCollector& MetricsCollector::Create() {
  absl::MutexLock lock(&metrics_mutex);
#if (defined(UPB_TRACING_ENABLED) && !defined(NDEBUG))
  upb_tracing_Init(&MetricsCollector::LogMessageNewHandler);
#endif
  return *upb_collector;
}

// Returns a snapshot of collected metrics.
std::unique_ptr<Metrics> MetricsCollector::Snapshot() {
  absl::MutexLock lock(&metrics_mutex);
  return upb_collector->metrics_.Clone();
}

void Metrics::LogMessageNew(const upb_MiniTable* mini_table,
                            const upb_Arena* arena) {
#if (defined(UPB_TRACING_ENABLED) && !defined(NDEBUG))
  syslog(LOG_INFO, "Upb NewMessage %s", upb_tracing_GetName(mini_table));
#endif
}

void Metrics::DebugPrint(absl::string_view tag) const {
  syslog(LOG_INFO, "Upb %s", std::string(tag).c_str());
}

std::unique_ptr<Metrics> Metrics::Clone() const {
  return std::make_unique<Metrics>(this);
}

Metrics::Metrics(const Metrics* other) { arena_count_ = other->arena_count_; }

}  // namespace upb::tracing
