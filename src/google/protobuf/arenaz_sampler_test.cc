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

#include <memory>
#include <random>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/strutil.h>


// Must be included last.
#include <google/protobuf/port_def.inc>

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
    res.push_back(info.bytes_allocated.load(std::memory_order_acquire));
  });
  return res;
}

ThreadSafeArenaStats* Register(ThreadSafeArenazSampler* s, size_t size) {
  auto* info = s->Register();
  assert(info != nullptr);
  info->bytes_allocated.store(size);
  return info;
}

#endif  // defined(PROTOBUF_ARENAZ_SAMPLE)

namespace {

#if defined(PROTOBUF_ARENAZ_SAMPLE)

TEST(ThreadSafeArenaStatsTest, PrepareForSampling) {
  ThreadSafeArenaStats info;
  MutexLock l(&info.init_mu);
  info.PrepareForSampling();

  EXPECT_EQ(info.num_allocations.load(), 0);
  EXPECT_EQ(info.num_resets.load(), 0);
  EXPECT_EQ(info.bytes_requested.load(), 0);
  EXPECT_EQ(info.bytes_allocated.load(), 0);
  EXPECT_EQ(info.bytes_wasted.load(), 0);
  EXPECT_EQ(info.max_bytes_allocated.load(), 0);

  info.num_allocations.store(1, std::memory_order_relaxed);
  info.num_resets.store(1, std::memory_order_relaxed);
  info.bytes_requested.store(1, std::memory_order_relaxed);
  info.bytes_allocated.store(1, std::memory_order_relaxed);
  info.bytes_wasted.store(1, std::memory_order_relaxed);
  info.max_bytes_allocated.store(1, std::memory_order_relaxed);

  info.PrepareForSampling();
  EXPECT_EQ(info.num_allocations.load(), 0);
  EXPECT_EQ(info.num_resets.load(), 0);
  EXPECT_EQ(info.bytes_requested.load(), 0);
  EXPECT_EQ(info.bytes_allocated.load(), 0);
  EXPECT_EQ(info.bytes_wasted.load(), 0);
  EXPECT_EQ(info.max_bytes_allocated.load(), 0);
}

TEST(ThreadSafeArenaStatsTest, RecordAllocateSlow) {
  ThreadSafeArenaStats info;
  MutexLock l(&info.init_mu);
  info.PrepareForSampling();
  RecordAllocateSlow(&info, /*requested=*/100, /*allocated=*/128, /*wasted=*/0);
  EXPECT_EQ(info.num_allocations.load(), 1);
  EXPECT_EQ(info.num_resets.load(), 0);
  EXPECT_EQ(info.bytes_requested.load(), 100);
  EXPECT_EQ(info.bytes_allocated.load(), 128);
  EXPECT_EQ(info.bytes_wasted.load(), 0);
  EXPECT_EQ(info.max_bytes_allocated.load(), 0);
  RecordAllocateSlow(&info, /*requested=*/100, /*allocated=*/256,
                     /*wasted=*/28);
  EXPECT_EQ(info.num_allocations.load(), 2);
  EXPECT_EQ(info.num_resets.load(), 0);
  EXPECT_EQ(info.bytes_requested.load(), 200);
  EXPECT_EQ(info.bytes_allocated.load(), 384);
  EXPECT_EQ(info.bytes_wasted.load(), 28);
  EXPECT_EQ(info.max_bytes_allocated.load(), 0);
}

TEST(ThreadSafeArenaStatsTest, RecordResetSlow) {
  ThreadSafeArenaStats info;
  MutexLock l(&info.init_mu);
  info.PrepareForSampling();
  EXPECT_EQ(info.num_resets.load(), 0);
  EXPECT_EQ(info.bytes_allocated.load(), 0);
  RecordAllocateSlow(&info, /*requested=*/100, /*allocated=*/128, /*wasted=*/0);
  EXPECT_EQ(info.num_resets.load(), 0);
  EXPECT_EQ(info.bytes_allocated.load(), 128);
  RecordResetSlow(&info);
  EXPECT_EQ(info.num_resets.load(), 1);
  EXPECT_EQ(info.bytes_allocated.load(), 0);
}

TEST(ThreadSafeArenazSamplerTest, SmallSampleParameter) {
  SetThreadSafeArenazEnabled(true);
  SetThreadSafeArenazSampleParameter(100);

  for (int i = 0; i < 1000; ++i) {
    int64_t next_sample = 0;
    ThreadSafeArenaStats* sample = SampleSlow(&next_sample);
    EXPECT_GT(next_sample, 0);
    EXPECT_NE(sample, nullptr);
    UnsampleSlow(sample);
  }
}

TEST(ThreadSafeArenazSamplerTest, LargeSampleParameter) {
  SetThreadSafeArenazEnabled(true);
  SetThreadSafeArenazSampleParameter(std::numeric_limits<int32_t>::max());

  for (int i = 0; i < 1000; ++i) {
    int64_t next_sample = 0;
    ThreadSafeArenaStats* sample = SampleSlow(&next_sample);
    EXPECT_GT(next_sample, 0);
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
  ThreadSafeArenaStatsHandle h(sampler.Register());
  auto* info = ThreadSafeArenaStatsHandlePeer::GetInfo(&h);
  info->bytes_allocated.store(0x12345678, std::memory_order_relaxed);

  bool found = false;
  sampler.Iterate([&](const ThreadSafeArenaStats& h) {
    if (&h == info) {
      EXPECT_EQ(h.bytes_allocated.load(), 0x12345678);
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
      if (h.bytes_allocated.load() == 0x12345678) {
        found = true;
      }
    }
  });
  EXPECT_FALSE(found);
}

TEST(ThreadSafeArenazSamplerTest, Registration) {
  ThreadSafeArenazSampler sampler;
  auto* info1 = Register(&sampler, 1);
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(1));

  auto* info2 = Register(&sampler, 2);
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(1, 2));
  info1->bytes_allocated.store(3);
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(3, 2));

  sampler.Unregister(info1);
  sampler.Unregister(info2);
}

TEST(ThreadSafeArenazSamplerTest, Unregistration) {
  ThreadSafeArenazSampler sampler;
  std::vector<ThreadSafeArenaStats*> infos;
  for (size_t i = 0; i < 3; ++i) {
    infos.push_back(Register(&sampler, i));
  }
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(0, 1, 2));

  sampler.Unregister(infos[1]);
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(0, 2));

  infos.push_back(Register(&sampler, 3));
  infos.push_back(Register(&sampler, 4));
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(0, 2, 3, 4));
  sampler.Unregister(infos[3]);
  EXPECT_THAT(GetBytesAllocated(&sampler), UnorderedElementsAre(0, 2, 4));

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
    pool.Schedule([&sampler, &stop]() {
      std::random_device rd;
      std::mt19937 gen(rd());

      std::vector<ThreadSafeArenaStats*> infoz;
      while (!stop.HasBeenNotified()) {
        if (infoz.empty()) {
          infoz.push_back(sampler.Register());
        }
        switch (std::uniform_int_distribution<>(0, 1)(gen)) {
          case 0: {
            infoz.push_back(sampler.Register());
            break;
          }
          case 1: {
            size_t p =
                std::uniform_int_distribution<>(0, infoz.size() - 1)(gen);
            ThreadSafeArenaStats* info = infoz[p];
            infoz[p] = infoz.back();
            infoz.pop_back();
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

  auto* info1 = Register(&sampler, 1);
  auto* info2 = Register(&sampler, 2);

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

class ThreadSafeArenazSamplerTestThread : public Thread {
 protected:
  void Run() override {
    google::protobuf::ArenaSafeUniquePtr<
        protobuf_test_messages::proto2::TestAllTypesProto2>
        message = google::protobuf::MakeArenaSafeUnique<
            protobuf_test_messages::proto2::TestAllTypesProto2>(arena_);
    GOOGLE_CHECK(message != nullptr);
    // Signal that a message on the arena has been created.  This should create
    // a SerialArena for this thread.
    if (barrier_->Block()) {
      delete barrier_;
    }
  }

 public:
  ThreadSafeArenazSamplerTestThread(const thread::Options& options,
                                    StringPiece name,
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
          options, StrCat("thread", i), &arena, barrier);
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
}
#endif  // defined(PROTOBUF_ARENAZ_SAMPLE)

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
