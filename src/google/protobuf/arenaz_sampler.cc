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

#include <google/protobuf/arenaz_sampler.h>

#include <atomic>
#include <cstdint>
#include <limits>


// Must be included last.
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace internal {

ThreadSafeArenazSampler& GlobalThreadSafeArenazSampler() {
  static auto* sampler = new ThreadSafeArenazSampler();
  return *sampler;
}

void UnsampleSlow(ThreadSafeArenaStats* info) {
  GlobalThreadSafeArenazSampler().Unregister(info);
}

#if defined(PROTOBUF_ARENAZ_SAMPLE)
namespace {

PROTOBUF_CONSTINIT std::atomic<bool> g_arenaz_enabled{true};
PROTOBUF_CONSTINIT std::atomic<int32_t> g_arenaz_sample_parameter{1 << 10};
PROTOBUF_THREAD_LOCAL absl::profiling_internal::ExponentialBiased
    g_exponential_biased_generator;

}  // namespace

PROTOBUF_THREAD_LOCAL int64_t global_next_sample = 1LL << 10;

ThreadSafeArenaStats::ThreadSafeArenaStats() { PrepareForSampling(); }
ThreadSafeArenaStats::~ThreadSafeArenaStats() = default;

void ThreadSafeArenaStats::PrepareForSampling() {
  num_allocations.store(0, std::memory_order_relaxed);
  num_resets.store(0, std::memory_order_relaxed);
  bytes_requested.store(0, std::memory_order_relaxed);
  bytes_allocated.store(0, std::memory_order_relaxed);
  bytes_wasted.store(0, std::memory_order_relaxed);
  max_bytes_allocated.store(0, std::memory_order_relaxed);
  thread_ids.store(0, std::memory_order_relaxed);
  // The inliner makes hardcoded skip_count difficult (especially when combined
  // with LTO).  We use the ability to exclude stacks by regex when encoding
  // instead.
  depth = absl::GetStackTrace(stack, kMaxStackDepth, /* skip_count= */ 0);
}

void RecordResetSlow(ThreadSafeArenaStats* info) {
  const size_t max_bytes =
      info->max_bytes_allocated.load(std::memory_order_relaxed);
  const size_t allocated_bytes =
      info->bytes_allocated.load(std::memory_order_relaxed);
  if (max_bytes < allocated_bytes) {
    info->max_bytes_allocated.store(allocated_bytes);
  }
  info->bytes_requested.store(0, std::memory_order_relaxed);
  info->bytes_allocated.store(0, std::memory_order_relaxed);
  info->bytes_wasted.fetch_add(0, std::memory_order_relaxed);
  info->num_allocations.fetch_add(0, std::memory_order_relaxed);
  info->num_resets.fetch_add(1, std::memory_order_relaxed);
}

void RecordAllocateSlow(ThreadSafeArenaStats* info, size_t requested,
                        size_t allocated, size_t wasted) {
  info->bytes_requested.fetch_add(requested, std::memory_order_relaxed);
  info->bytes_allocated.fetch_add(allocated, std::memory_order_relaxed);
  info->bytes_wasted.fetch_add(wasted, std::memory_order_relaxed);
  info->num_allocations.fetch_add(1, std::memory_order_relaxed);
  const uint64_t tid = (1ULL << (GetCachedTID() % 63));
  const uint64_t thread_ids = info->thread_ids.load(std::memory_order_relaxed);
  if (!(thread_ids & tid)) {
    info->thread_ids.store(thread_ids | tid, std::memory_order_relaxed);
  }
}

ThreadSafeArenaStats* SampleSlow(int64_t* next_sample) {
  bool first = *next_sample < 0;
  *next_sample = g_exponential_biased_generator.GetStride(
      g_arenaz_sample_parameter.load(std::memory_order_relaxed));
  // Small values of interval are equivalent to just sampling next time.
  ABSL_ASSERT(*next_sample >= 1);

  // g_arenaz_enabled can be dynamically flipped, we need to set a threshold low
  // enough that we will start sampling in a reasonable time, so we just use the
  // default sampling rate.
  if (!g_arenaz_enabled.load(std::memory_order_relaxed)) return nullptr;
  // We will only be negative on our first count, so we should just retry in
  // that case.
  if (first) {
    if (PROTOBUF_PREDICT_TRUE(--*next_sample > 0)) return nullptr;
    return SampleSlow(next_sample);
  }

  return GlobalThreadSafeArenazSampler().Register();
}

void SetThreadSafeArenazEnabled(bool enabled) {
  g_arenaz_enabled.store(enabled, std::memory_order_release);
}

void SetThreadSafeArenazSampleParameter(int32_t rate) {
  if (rate > 0) {
    g_arenaz_sample_parameter.store(rate, std::memory_order_release);
  } else {
    ABSL_RAW_LOG(ERROR, "Invalid thread safe arenaz sample rate: %lld",
                 static_cast<long long>(rate));  // NOLINT(runtime/int)
  }
}

void SetThreadSafeArenazMaxSamples(int32_t max) {
  if (max > 0) {
    GlobalThreadSafeArenazSampler().SetMaxSamples(max);
  } else {
    ABSL_RAW_LOG(ERROR, "Invalid thread safe arenaz max samples: %lld",
                 static_cast<long long>(max));  // NOLINT(runtime/int)
  }
}

void SetThreadSafeArenazGlobalNextSample(int64_t next_sample) {
  if (next_sample >= 0) {
    global_next_sample = next_sample;
  } else {
    ABSL_RAW_LOG(ERROR, "Invalid thread safe arenaz next sample: %lld",
                 static_cast<long long>(next_sample));  // NOLINT(runtime/int)
  }
}

#else
ThreadSafeArenaStats* SampleSlow(int64_t* next_sample) {
  *next_sample = std::numeric_limits<int64_t>::max();
  return nullptr;
}

void SetThreadSafeArenazEnabled(bool enabled) {}
void SetThreadSafeArenazSampleParameter(int32_t rate) {}
void SetThreadSafeArenazMaxSamples(int32_t max) {}
void SetThreadSafeArenazGlobalNextSample(int64_t next_sample) {}
#endif  // defined(PROTOBUF_ARENAZ_SAMPLE)

}  // namespace internal
}  // namespace protobuf
}  // namespace google
