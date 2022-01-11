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

#ifndef GOOGLE_PROTOBUF_SRC_GOOGLE_PROTOBUF_ARENAZ_SAMPLER_H__
#define GOOGLE_PROTOBUF_SRC_GOOGLE_PROTOBUF_ARENAZ_SAMPLER_H__

#include <atomic>
#include <cstddef>
#include <cstdint>


// Must be included last.
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace internal {

#if defined(PROTOBUF_ARENAZ_SAMPLE)
struct ThreadSafeArenaStats;
void RecordResetSlow(ThreadSafeArenaStats* info);
void RecordAllocateSlow(ThreadSafeArenaStats* info, size_t requested,
                        size_t allocated, size_t wasted);
// Stores information about a sampled thread safe arena.  All mutations to this
// *must* be made through `Record*` functions below.  All reads from this *must*
// only occur in the callback to `ThreadSafeArenazSampler::Iterate`.
struct ThreadSafeArenaStats
    : public absl::profiling_internal::Sample<ThreadSafeArenaStats> {
  // Constructs the object but does not fill in any fields.
  ThreadSafeArenaStats();
  ~ThreadSafeArenaStats();

  // Puts the object into a clean state, fills in the logically `const` members,
  // blocking for any readers that are currently sampling the object.
  void PrepareForSampling() ABSL_EXCLUSIVE_LOCKS_REQUIRED(init_mu);

  // These fields are mutated by the various Record* APIs and need to be
  // thread-safe.
  std::atomic<int> num_allocations;
  std::atomic<int> num_resets;
  std::atomic<size_t> bytes_requested;
  std::atomic<size_t> bytes_allocated;
  std::atomic<size_t> bytes_wasted;
  // Records the largest size an arena ever had.  Maintained across resets.
  std::atomic<size_t> max_bytes_allocated;
  // Bit i when set to 1 indicates that a thread with tid % 63 = i accessed the
  // underlying arena.  The field is maintained across resets.
  std::atomic<uint64_t> thread_ids;

  // All of the fields below are set by `PrepareForSampling`, they must not
  // be mutated in `Record*` functions.  They are logically `const` in that
  // sense. These are guarded by init_mu, but that is not externalized to
  // clients, who can only read them during
  // `ThreadSafeArenazSampler::Iterate` which will hold the lock.
  static constexpr int kMaxStackDepth = 64;
  int32_t depth;
  void* stack[kMaxStackDepth];
  static void RecordAllocateStats(ThreadSafeArenaStats* info, size_t requested,
                                  size_t allocated, size_t wasted) {
    if (PROTOBUF_PREDICT_TRUE(info == nullptr)) return;
    RecordAllocateSlow(info, requested, allocated, wasted);
  }
};

ThreadSafeArenaStats* SampleSlow(int64_t* next_sample);
void UnsampleSlow(ThreadSafeArenaStats* info);

class ThreadSafeArenaStatsHandle {
 public:
  explicit ThreadSafeArenaStatsHandle() = default;
  explicit ThreadSafeArenaStatsHandle(ThreadSafeArenaStats* info)
      : info_(info) {}

  ~ThreadSafeArenaStatsHandle() {
    if (PROTOBUF_PREDICT_TRUE(info_ == nullptr)) return;
    UnsampleSlow(info_);
  }

  ThreadSafeArenaStatsHandle(ThreadSafeArenaStatsHandle&& other) noexcept
      : info_(absl::exchange(other.info_, nullptr)) {}

  ThreadSafeArenaStatsHandle& operator=(
      ThreadSafeArenaStatsHandle&& other) noexcept {
    if (PROTOBUF_PREDICT_FALSE(info_ != nullptr)) {
      UnsampleSlow(info_);
    }
    info_ = absl::exchange(other.info_, nullptr);
    return *this;
  }

  void RecordReset() {
    if (PROTOBUF_PREDICT_TRUE(info_ == nullptr)) return;
    RecordResetSlow(info_);
  }

  ThreadSafeArenaStats* MutableStats() { return info_; }

  friend void swap(ThreadSafeArenaStatsHandle& lhs,
                   ThreadSafeArenaStatsHandle& rhs) {
    std::swap(lhs.info_, rhs.info_);
  }

  friend class ThreadSafeArenaStatsHandlePeer;

 private:
  ThreadSafeArenaStats* info_ = nullptr;
};

using ThreadSafeArenazSampler =
    ::absl::profiling_internal::SampleRecorder<ThreadSafeArenaStats>;

extern PROTOBUF_THREAD_LOCAL int64_t global_next_sample;

// Returns an RAII sampling handle that manages registration and unregistation
// with the global sampler.
inline ThreadSafeArenaStatsHandle Sample() {
  if (PROTOBUF_PREDICT_TRUE(--global_next_sample > 0)) {
    return ThreadSafeArenaStatsHandle(nullptr);
  }
  return ThreadSafeArenaStatsHandle(SampleSlow(&global_next_sample));
}

#else
struct ThreadSafeArenaStats {
  static void RecordAllocateStats(ThreadSafeArenaStats*, size_t /*requested*/,
                                  size_t /*allocated*/, size_t /*wasted*/) {}
};

ThreadSafeArenaStats* SampleSlow(int64_t* next_sample);
void UnsampleSlow(ThreadSafeArenaStats* info);

class ThreadSafeArenaStatsHandle {
 public:
  explicit ThreadSafeArenaStatsHandle() = default;
  explicit ThreadSafeArenaStatsHandle(ThreadSafeArenaStats*) {}

  void RecordReset() {}

  ThreadSafeArenaStats* MutableStats() { return nullptr; }

  friend void swap(ThreadSafeArenaStatsHandle&, ThreadSafeArenaStatsHandle&) {}

 private:
  friend class ThreadSafeArenaStatsHandlePeer;
};

class ThreadSafeArenazSampler {
 public:
  void Unregister(ThreadSafeArenaStats*) {}
  void SetMaxSamples(int32_t) {}
};

// Returns an RAII sampling handle that manages registration and unregistation
// with the global sampler.
inline ThreadSafeArenaStatsHandle Sample() {
  return ThreadSafeArenaStatsHandle(nullptr);
}
#endif  // defined(PROTOBUF_ARENAZ_SAMPLE)

// Returns a global Sampler.
ThreadSafeArenazSampler& GlobalThreadSafeArenazSampler();

// Enables or disables sampling for thread safe arenas.
void SetThreadSafeArenazEnabled(bool enabled);

// Sets the rate at which thread safe arena will be sampled.
void SetThreadSafeArenazSampleParameter(int32_t rate);

// Sets a soft max for the number of samples that will be kept.
void SetThreadSafeArenazMaxSamples(int32_t max);

// Sets the current value for when arenas should be next sampled.
void SetThreadSafeArenazGlobalNextSample(int64_t next_sample);

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_SRC_PROTOBUF_ARENAZ_SAMPLER_H__
