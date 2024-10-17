// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arenaz_sampler.h"

#include <atomic>
#include <limits>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
#if defined(PROTOBUF_ARENAZ_SAMPLE)
class ThreadSafeArenaStatsHandlePeer {
 public:
  static bool IsSampled(const ThreadSafeArenaStatsHandle& h) {
    return h.info_ != nullptr;
  }

  static ThreadSafeArenaStats* GetInfo(ThreadSafeArenaStatsHandle* h) {
    return h->info_;
  }
};

std::vector<size_t> GetBytesAllocated(ThreadSafeArenazSampler* s) {
  std::vector<size_t> res;
  s->Iterate([&](const ThreadSafeArenaStats& info) {
    for (const auto& block_stats : info.block_histogram) {
      size_t bytes_allocated =
          block_stats.bytes_allocated.load(std::memory_order_acquire);
      if (bytes_allocated != 0) {
        res.push_back(bytes_allocated);
      }
    }
  });
  return res;
}

ThreadSafeArenaStats* Register(ThreadSafeArenazSampler* s, size_t size,
                               int64_t stride) {
  auto* info = s->Register(stride);
  assert(info != nullptr);
  info->block_histogram[0].bytes_allocated.store(size,
                                                 std::memory_order_relaxed);
  return info;
}

#endif  // defined(PROTOBUF_ARENAZ_SAMPLE)

namespace {

#if defined(PROTOBUF_ARENAZ_SAMPLE)

TEST(ThreadSafeArenaStatsTest, PrepareForSampling) {
  ThreadSafeArenaStats info;
  constexpr int64_t kTestStride = 107;
  absl::MutexLock l(&info.init_mu);
  info.PrepareForSampling(kTestStride);

  for (const auto& block_stats : info.block_histogram) {
    EXPECT_EQ(block_stats.num_allocations.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(block_stats.bytes_used.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(block_stats.bytes_allocated.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(block_stats.bytes_wasted.load(std::memory_order_relaxed), 0);
  }
  EXPECT_EQ(info.max_block_size.load(std::memory_order_relaxed), 0);
  EXPECT_EQ(info.weight, kTestStride);

  for (auto& block_stats : info.block_histogram) {
    block_stats.num_allocations.store(1, std::memory_order_relaxed);
    block_stats.bytes_used.store(1, std::memory_order_relaxed);
    block_stats.bytes_allocated.store(1, std::memory_order_relaxed);
    block_stats.bytes_wasted.store(1, std::memory_order_relaxed);
  }
  info.max_block_size.store(1, std::memory_order_relaxed);

  info.PrepareForSampling(2 * kTestStride);
  for (auto& block_stats : info.block_histogram) {
    EXPECT_EQ(block_stats.num_allocations.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(block_stats.bytes_used.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(block_stats.bytes_allocated.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(block_stats.bytes_wasted.load(std::memory_order_relaxed), 0);
  }
  EXPECT_EQ(info.max_block_size.load(std::memory_order_relaxed), 0);
  EXPECT_EQ(info.weight, 2 * kTestStride);
}

TEST(ThreadSafeArenaStatsTest, FindBin) {
  size_t current_bin = 0;
  size_t bytes = 1;
  while (current_bin < ThreadSafeArenaStats::kBlockHistogramBins - 1) {
    size_t next_bin = ThreadSafeArenaStats::FindBin(bytes);
    if (next_bin != current_bin) {
      // Test the bins increase linearly.
      EXPECT_EQ(next_bin, current_bin + 1);
      // Test the bins change only at values of the form 2^k + 1.
      EXPECT_EQ(absl::popcount(bytes - 1), 1);
      current_bin = next_bin;
    }
    ++bytes;
  }
}

TEST(ThreadSafeArenaStatsTest, MinMaxBlockSizeForBin) {
  std::pair<size_t, size_t> current_limits =
      ThreadSafeArenaStats::MinMaxBlockSizeForBin(0);
  EXPECT_EQ(current_limits.first, 1);
  EXPECT_LT(current_limits.first, current_limits.second);
  for (size_t i = 1; i < ThreadSafeArenaStats::kBlockHistogramBins; ++i) {
    std::pair<size_t, size_t> next_limits =
        ThreadSafeArenaStats::MinMaxBlockSizeForBin(i);
    EXPECT_LT(next_limits.first, next_limits.second);
    // Test the limits do not have gaps.
    EXPECT_EQ(next_limits.first, current_limits.second + 1);
    if (i != ThreadSafeArenaStats::kBlockHistogramBins - 1) {
      EXPECT_EQ(next_limits.second, 2 * current_limits.second);
    }
    current_limits = next_limits;
  }
  // Test the limits cover the entire range possible.
  EXPECT_EQ(current_limits.second, std::numeric_limits<size_t>::max());
}

TEST(ThreadSafeArenaStatsTest, RecordAllocateSlow) {
  ThreadSafeArenaStats info;
  constexpr int64_t kTestStride = 458;
  absl::MutexLock l(&info.init_mu);
  info.PrepareForSampling(kTestStride);
  RecordAllocateSlow(&info, /*requested=*/0, /*allocated=*/128, /*wasted=*/0);
  EXPECT_EQ(
      info.block_histogram[0].num_allocations.load(std::memory_order_relaxed),
      1);
  EXPECT_EQ(info.block_histogram[0].bytes_used.load(std::memory_order_relaxed),
            0);
  EXPECT_EQ(
      info.block_histogram[0].bytes_allocated.load(std::memory_order_relaxed),
      128);
  EXPECT_EQ(
      info.block_histogram[0].bytes_wasted.load(std::memory_order_relaxed), 0);
  EXPECT_EQ(info.max_block_size.load(std::memory_order_relaxed), 128);
  RecordAllocateSlow(&info, /*requested=*/100, /*allocated=*/256,
                     /*wasted=*/28);
  EXPECT_EQ(info.block_histogram[0].bytes_used.load(std::memory_order_relaxed),
            100);
  EXPECT_EQ(
      info.block_histogram[0].bytes_wasted.load(std::memory_order_relaxed), 28);
  EXPECT_EQ(
      info.block_histogram[1].num_allocations.load(std::memory_order_relaxed),
      1);
  EXPECT_EQ(
      info.block_histogram[1].bytes_allocated.load(std::memory_order_relaxed),
      256);
  EXPECT_EQ(info.max_block_size.load(std::memory_order_relaxed), 256);
}

TEST(ThreadSafeArenaStatsTest, RecordAllocateSlowMaxBlockSizeTest) {
  ThreadSafeArenaStats info;
  constexpr int64_t kTestStride = 458;
  absl::MutexLock l(&info.init_mu);
  info.PrepareForSampling(kTestStride);
  RecordAllocateSlow(&info, /*requested=*/100, /*allocated=*/128, /*wasted=*/0);
  EXPECT_EQ(info.max_block_size.load(std::memory_order_relaxed), 128);
  RecordAllocateSlow(&info, /*requested=*/100, /*allocated=*/256,
                     /*wasted=*/28);
  EXPECT_EQ(info.max_block_size.load(std::memory_order_relaxed), 256);
  RecordAllocateSlow(&info, /*requested=*/100, /*allocated=*/128,
                     /*wasted=*/28);
  EXPECT_EQ(info.max_block_size.load(std::memory_order_relaxed), 256);
}

TEST(ThreadSafeArenazSamplerTest, SamplingCorrectness) {
  SetThreadSafeArenazEnabled(true);
  for (int p = 0; p <= 15; ++p) {
    SetThreadSafeArenazSampleParameter(1 << p);
    SetThreadSafeArenazGlobalNextSample(1 << p);
    const int kTrials = 1000 << p;
    std::vector<ThreadSafeArenaStatsHandle> hv;
    for (int i = 0; i < kTrials; ++i) {
      ThreadSafeArenaStatsHandle h = Sample();
      if (h.MutableStats() != nullptr) hv.push_back(std::move(h));
    }
    // Ideally samples << p should be very close to kTrials.  But we keep a
    // factor of two guard band.
    EXPECT_GE(hv.size() << p, kTrials / 2);
    EXPECT_LE(hv.size() << p, 2 * kTrials);
  }
}

TEST(ThreadSafeArenazSamplerTest, SmallSampleParameter) {
  SetThreadSafeArenazEnabled(true);
  SetThreadSafeArenazSampleParameter(100);
  constexpr int64_t kTestStride = 0;

  for (int i = 0; i < 1000; ++i) {
    SamplingState sampling_state = {kTestStride, kTestStride};
    ThreadSafeArenaStats* sample = SampleSlow(sampling_state);
    EXPECT_GT(sampling_state.next_sample, 0);
    EXPECT_NE(sample, nullptr);
    UnsampleSlow(sample);
  }
}

TEST(ThreadSafeArenazSamplerTest, LargeSampleParameter) {
  SetThreadSafeArenazEnabled(true);
  SetThreadSafeArenazSampleParameter(std::numeric_limits<int32_t>::max());
  constexpr int64_t kTestStride = 0;

  for (int i = 0; i < 1000; ++i) {
    SamplingState sampling_state = {kTestStride, kTestStride};
    ThreadSafeArenaStats* sample = SampleSlow(sampling_state);
    EXPECT_GT(sampling_state.next_sample, 0);
    EXPECT_NE(sample, nullptr);
    UnsampleSlow(sample);
  }
}

TEST(ThreadSafeArenazSamplerTest, Sample) {
  SetThreadSafeArenazEnabled(true);
  SetThreadSafeArenazSampleParameter(100);
  SetThreadSafeArenazGlobalNextSample(0);
  int64_t num_sampled = 0;
  int64_t total = 0;
  double sample_rate = 0.0;
  for (int i = 0; i < 1000000; ++i) {
    ThreadSafeArenaStatsHandle h = Sample();
    ++total;
    if (ThreadSafeArenaStatsHandlePeer::IsSampled(h)) {
      ++num_sampled;
    }
    sample_rate = static_cast<double>(num_sampled) / total;
    if (0.005 < sample_rate && sample_rate < 0.015) break;
  }
  EXPECT_NEAR(sample_rate, 0.01, 0.005);
}

TEST(ThreadSafeArenazSamplerTest, Handle) {
  auto& sampler = GlobalThreadSafeArenazSampler();
  constexpr int64_t kTestStride = 17;
  ThreadSafeArenaStatsHandle h(sampler.Register(kTestStride));
  auto* info = ThreadSafeArenaStatsHandlePeer::GetInfo(&h);
  info->block_histogram[0].bytes_allocated.store(0x12345678,
                                                 std::memory_order_relaxed);

  bool found = false;
  sampler.Iterate([&](const ThreadSafeArenaStats& h) {
    if (&h == info) {
      EXPECT_EQ(
          h.block_histogram[0].bytes_allocated.load(std::memory_order_relaxed),
          0x12345678);
      EXPECT_EQ(h.weight, kTestStride);
      found = true;
    }
  });
  EXPECT_TRUE(found);

  h = ThreadSafeArenaStatsHandle();
  found = false;
  sampler.Iterate([&](const ThreadSafeArenaStats& h) {
    if (&h == info) {
      // this will only happen if some other thread has resurrected the info
      // the old handle was using.
      if (h.block_histogram[0].bytes_allocated.load(
              std::memory_order_relaxed) == 0x12345678) {
        found = true;
      }
    }
  });
  EXPECT_FALSE(found);
}

TEST(ThreadSafeArenazSamplerTest, Registration) {
  ThreadSafeArenazSampler sampler;
  constexpr int64_t kTestStride = 100;
  auto* info1 = Register(&sampler, 1, kTestStride);
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(1));

  auto* info2 = Register(&sampler, 2, kTestStride);
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(1, 2));
  info1->block_histogram[0].bytes_allocated.store(3, std::memory_order_relaxed);
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(3, 2));

  sampler.Unregister(info1);
  sampler.Unregister(info2);
}

TEST(ThreadSafeArenazSamplerTest, Unregistration) {
  ThreadSafeArenazSampler sampler;
  std::vector<ThreadSafeArenaStats*> infos;
  constexpr int64_t kTestStride = 200;
  for (size_t i = 0; i < 3; ++i) {
    infos.push_back(Register(&sampler, i + 1, kTestStride));
  }
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(1, 2, 3));

  sampler.Unregister(infos[1]);
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(1, 3));

  infos.push_back(Register(&sampler, 3, kTestStride));
  infos.push_back(Register(&sampler, 4, kTestStride));
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(1, 3, 3, 4));
  sampler.Unregister(infos[3]);
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(1, 3, 4));

  sampler.Unregister(infos[0]);
  sampler.Unregister(infos[2]);
  sampler.Unregister(infos[4]);
  EXPECT_THAT(GetBytesAllocated(&sampler), IsEmpty());
}

TEST(ThreadSafeArenazSamplerTest, MultiThreaded) {
  ThreadSafeArenazSampler sampler;
  absl::Notification stop;
  ThreadPool pool(10);

  for (int i = 0; i < 10; ++i) {
    const int64_t sampling_stride = 11 + i % 3;
    pool.Schedule([&sampler, &stop, sampling_stride]() {
      std::random_device rd;
      std::mt19937 gen(rd());

      std::vector<ThreadSafeArenaStats*> infoz;
      while (!stop.HasBeenNotified()) {
        if (infoz.empty()) {
          infoz.push_back(sampler.Register(sampling_stride));
        }
        switch (std::uniform_int_distribution<>(0, 1)(gen)) {
          case 0: {
            infoz.push_back(sampler.Register(sampling_stride));
            break;
          }
          case 1: {
            size_t p =
                std::uniform_int_distribution<>(0, infoz.size() - 1)(gen);
            ThreadSafeArenaStats* info = infoz[p];
            infoz[p] = infoz.back();
            infoz.pop_back();
            EXPECT_EQ(info->weight, sampling_stride);
            sampler.Unregister(info);
            break;
          }
        }
      }
    });
  }
  // The threads will hammer away.  Give it a little bit of time for tsan to
  // spot errors.
  absl::SleepFor(absl::Seconds(3));
  stop.Notify();
}

TEST(ThreadSafeArenazSamplerTest, Callback) {
  ThreadSafeArenazSampler sampler;
  constexpr int64_t kTestStride = 203;

  auto* info1 = Register(&sampler, 1, kTestStride);
  auto* info2 = Register(&sampler, 2, kTestStride);

  static const ThreadSafeArenaStats* expected;

  auto callback = [](const ThreadSafeArenaStats& info) {
    // We can't use `info` outside of this callback because the object will be
    // disposed as soon as we return from here.
    EXPECT_EQ(&info, expected);
  };

  // Set the callback.
  EXPECT_EQ(sampler.SetDisposeCallback(callback), nullptr);
  expected = info1;
  sampler.Unregister(info1);

  // Unset the callback.
  EXPECT_EQ(callback, sampler.SetDisposeCallback(nullptr));
  expected = nullptr;  // no more calls.
  sampler.Unregister(info2);
}

TEST(ThreadSafeArenazSamplerTest, InitialBlockReportsZeroUsedAndWasted) {
  SetThreadSafeArenazEnabled(true);
  // Setting 1 as the parameter value means one in every two arenas would be
  // sampled, on average.
  int32_t oldparam = ThreadSafeArenazSampleParameter();
  SetThreadSafeArenazSampleParameter(1);
  SetThreadSafeArenazGlobalNextSample(0);
  constexpr int kSize = 571;
  int count_found_allocation = 0;
  auto& sampler = GlobalThreadSafeArenazSampler();
  for (int i = 0; i < 10; ++i) {
    char block[kSize];
    google::protobuf::Arena arena(/*initial_block=*/block, /*initial_block_size=*/kSize);
    benchmark::DoNotOptimize(&arena);
    sampler.Iterate([&](const ThreadSafeArenaStats& h) {
      const auto& histbin =
          h.block_histogram[ThreadSafeArenaStats::FindBin(kSize)];
      if (histbin.bytes_allocated.load(std::memory_order_relaxed) == kSize) {
        count_found_allocation++;
        EXPECT_EQ(histbin.bytes_used, 0);
        EXPECT_EQ(histbin.bytes_wasted, 0);
      }
    });
  }
  EXPECT_GT(count_found_allocation, 0);
  SetThreadSafeArenazSampleParameter(oldparam);
}

class ThreadSafeArenazSamplerTestThread : public Thread {
 protected:
  void Run() override {
    google::protobuf::ArenaSafeUniquePtr<
        protobuf_test_messages::proto2::TestAllTypesProto2>
        message = google::protobuf::MakeArenaSafeUnique<
            protobuf_test_messages::proto2::TestAllTypesProto2>(arena_);
    ABSL_CHECK(message != nullptr);
    // Signal that a message on the arena has been created.  This should create
    // a SerialArena for this thread.
    if (barrier_->Block()) {
      delete barrier_;
    }
  }

 public:
  ThreadSafeArenazSamplerTestThread(const thread::Options& options,
                                    absl::string_view name,
                                    google::protobuf::Arena* arena,
                                    absl::Barrier* barrier)
      : Thread(options, name), arena_(arena), barrier_(barrier) {}

 private:
  google::protobuf::Arena* arena_;
  absl::Barrier* barrier_;
};

TEST(ThreadSafeArenazSamplerTest, MultiThread) {
  SetThreadSafeArenazEnabled(true);
  // Setting 1 as the parameter value means one in every two arenas would be
  // sampled, on average.
  int32_t oldparam = ThreadSafeArenazSampleParameter();
  SetThreadSafeArenazSampleParameter(1);
  SetThreadSafeArenazGlobalNextSample(0);
  auto& sampler = GlobalThreadSafeArenazSampler();
  int count = 0;
  for (int i = 0; i < 10; ++i) {
    const int kNumThreads = 10;
    absl::Barrier* barrier = new absl::Barrier(kNumThreads + 1);
    google::protobuf::Arena arena;
    thread::Options options;
    options.set_joinable(true);
    std::vector<std::unique_ptr<ThreadSafeArenazSamplerTestThread>> threads;
    for (int i = 0; i < kNumThreads; i++) {
      auto t = std::make_unique<ThreadSafeArenazSamplerTestThread>(
          options, absl::StrCat("thread", i), &arena, barrier);
      t->Start();
      threads.push_back(std::move(t));
    }
    // Wait till each thread has created a message on the arena.
    if (barrier->Block()) {
      delete barrier;
    }
    sampler.Iterate([&](const ThreadSafeArenaStats& h) { ++count; });
    for (int i = 0; i < kNumThreads; i++) {
      threads[i]->Join();
    }
  }
  EXPECT_GT(count, 0);
  SetThreadSafeArenazSampleParameter(oldparam);
}

class SampleFirstArenaThread : public Thread {
 protected:
  void Run() override {
    google::protobuf::Arena arena;
    google::protobuf::ArenaSafeUniquePtr<
        protobuf_test_messages::proto2::TestAllTypesProto2>
        message = google::protobuf::MakeArenaSafeUnique<
            protobuf_test_messages::proto2::TestAllTypesProto2>(&arena);
    ABSL_CHECK(message != nullptr);
    arena_created_.Notify();
    samples_counted_.WaitForNotification();
  }

 public:
  explicit SampleFirstArenaThread(const thread::Options& options)
      : Thread(options, "SampleFirstArenaThread") {}

  absl::Notification arena_created_;
  absl::Notification samples_counted_;
};

// Test that the first arena created on a thread may and may not be chosen for
// sampling.
TEST(ThreadSafeArenazSamplerTest, SampleFirstArena) {
  SetThreadSafeArenazEnabled(true);
  auto& sampler = GlobalThreadSafeArenazSampler();

  enum class SampleResult {
    kSampled,
    kUnsampled,
    kSpoiled,
  };

  auto count_samples = [&]() {
    int count = 0;
    sampler.Iterate([&](const ThreadSafeArenaStats& h) { ++count; });
    return count;
  };

  auto run_sample_experiment = [&]() {
    int before = count_samples();
    thread::Options options;
    options.set_joinable(true);
    SampleFirstArenaThread t(options);
    t.Start();
    t.arena_created_.WaitForNotification();
    int during = count_samples();
    t.samples_counted_.Notify();
    t.Join();
    int after = count_samples();

    // If we didn't get back where we were, some other thread may have
    // created an arena and produced an invalid experiment run.
    if (before != after) return SampleResult::kSpoiled;

    switch (during - before) {
      case 1:
        return SampleResult::kSampled;
      case 0:
        return SampleResult::kUnsampled;
      default:
        return SampleResult::kSpoiled;
    }
  };

  constexpr int kTrials = 10000;
  bool sampled = false;
  bool unsampled = false;
  for (int i = 0; i < kTrials; ++i) {
    switch (run_sample_experiment()) {
      case SampleResult::kSampled:
        sampled = true;
        break;
      case SampleResult::kUnsampled:
        unsampled = true;
        break;
      default:
        break;
    }

    // This is the success criteria for the entire test.  At some point
    // we sampled the first arena and at some point we did not.
    if (sampled && unsampled) return;
  }
  EXPECT_TRUE(sampled);
  EXPECT_TRUE(unsampled);
}

TEST(ThreadSafeArenazSamplerTest, UsedAndWasted) {
  SetThreadSafeArenazEnabled(true);
  int32_t oldparam = ThreadSafeArenazSampleParameter();
  SetThreadSafeArenazSampleParameter(1);
  SetThreadSafeArenazGlobalNextSample(0);
  auto& sampler = GlobalThreadSafeArenazSampler();
  google::protobuf::Arena arena;
  // Do enough small allocations to completely fill 3 first blocks.
  // Test that they are fully used and none wasted.
  for (int i = 0; i < 1000; ++i) {
    Arena::Create<char>(&arena);
  }
  sampler.Iterate([&](const ThreadSafeArenaStats& h) {
    for (size_t i = 0; i < 3; ++i) {
      constexpr auto kSize =
          google::protobuf::internal::AllocationPolicy::kDefaultStartBlockSize;
      const auto& histbin =
          h.block_histogram[ThreadSafeArenaStats::FindBin(kSize << i)];
      EXPECT_EQ(histbin.bytes_allocated, kSize << i);
      EXPECT_EQ(histbin.bytes_used,
                (kSize << i) - google::protobuf::internal::SerialArena::kBlockHeaderSize);
      EXPECT_EQ(histbin.bytes_wasted, 0);
    }
  });
  SetThreadSafeArenazSampleParameter(oldparam);
}
#endif  // defined(PROTOBUF_ARENAZ_SAMPLE)

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
