#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

#include <benchmark/benchmark.h>
#include "absl/log/absl_check.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/rust/test/benchmarks/bench_data.pb.h"
#include "google/protobuf/rust/test/benchmarks/bench_data.upb.proto.h"
#include "protos/protos.h"

using benchmarks::BenchData;

#define EXTERN_BENCHMARK(NAME)              \
  extern "C" {                              \
  void NAME##_bench();                      \
  }                                         \
  void BM_##NAME(benchmark::State& state) { \
    for (auto s : state) {                  \
      NAME##_bench();                       \
    }                                       \
  }                                         \
  BENCHMARK(BM_##NAME);

#define PROTO_BENCHMARK(NAME)   \
  EXTERN_BENCHMARK(NAME##_cpp); \
  EXTERN_BENCHMARK(NAME##_upb);

void BM_set_string_cpp(benchmark::State& state) {
  for (auto s : state) {
    auto data = std::make_unique<BenchData>();
    data->set_name(
        "a relatively long string that will avoid any short string "
        "optimizations.");
  }
}

BENCHMARK(BM_set_string_cpp);

PROTO_BENCHMARK(set_string_rs);

void BM_set_int_cpp(benchmark::State& state) {
  for (auto s : state) {
    auto data = std::make_unique<BenchData>();
    data->set_num2(123456789);
    ABSL_CHECK_EQ(data->num2(), 123456789);
  }
}

BENCHMARK(BM_set_int_cpp);

PROTO_BENCHMARK(set_int_rs);

extern "C" {
void benchmark_thunk_set_num2_rs(void* data, int32_t num2);
}

void BM_set_int_cpp_roundtrip(benchmark::State& state) {
  for (auto s : state) {
    auto data = std::make_unique<BenchData>();
    benchmark_thunk_set_num2_rs((void*)data.get(), 123456789);
    ABSL_CHECK_EQ(data->num2(), 123456789);
  }
}

BENCHMARK(BM_set_int_cpp_roundtrip);

void BM_add_10_repeated_msg_copy_cpp(benchmark::State& state) {
  for (auto s : state) {
    auto data = std::make_unique<BenchData>();
    for (int i = 0; i < 10; ++i) {
      auto sub_data = std::make_unique<BenchData>();
      sub_data->set_num2(i);
      *data->add_subs() = *sub_data;
    }
  }
}

BENCHMARK(BM_add_10_repeated_msg_copy_cpp);

PROTO_BENCHMARK(add_10_repeated_msg_rs);

void BM_add_10_repeated_msg_direct_cpp(benchmark::State& state) {
  for (auto s : state) {
    auto data = std::make_unique<BenchData>();
    for (int i = 0; i < 10; ++i) {
      auto sub_data = data->add_subs();
      sub_data->set_num2(i);
    }
  }
}

BENCHMARK(BM_add_10_repeated_msg_direct_cpp);

void BM_copy_from_10_repeated_msg_cpp(benchmark::State& state) {
  auto source = std::make_unique<BenchData>();
  for (int i = 0; i < 10; ++i) {
    auto sub_data = source->add_subs();
    sub_data->set_num2(i);
  }
  for (auto s : state) {
    benchmarks::BenchData data;
    *data.mutable_subs() = source->subs();
  }
}

BENCHMARK(BM_copy_from_10_repeated_msg_cpp);

void BM_back_inserter_from_10_repeated_msg_cpp(benchmark::State& state) {
  auto source = std::make_unique<BenchData>();
  for (int i = 0; i < 10; ++i) {
    auto sub_data = source->add_subs();
    sub_data->set_num2(i);
  }
  for (auto s : state) {
    benchmarks::BenchData data;
    std::copy(source->subs().begin(), source->subs().end(),
              google::protobuf::RepeatedFieldBackInserter(data.mutable_subs()));
  }
}

BENCHMARK(BM_back_inserter_from_10_repeated_msg_cpp);

PROTO_BENCHMARK(copy_from_10_repeated_msg_rs);

PROTO_BENCHMARK(extend_10_repeated_msg_rs);

void BM_add_100_ints_cpp(benchmark::State& state) {
  for (auto s : state) {
    auto data = std::make_unique<BenchData>();
    for (int i = 0; i < 100; ++i) {
      data->mutable_nums()->Add(i);
    }
  }
}

BENCHMARK(BM_add_100_ints_cpp);

void BM_add_100_ints_upb(benchmark::State& state) {
  for (auto s : state) {
    ::protos::Arena arena;
    auto data = ::protos::CreateMessage<benchmarks::protos::BenchData>(arena);
    for (int i = 0; i < 100; ++i) {
      data.add_nums(i);
    }
  }
}

BENCHMARK(BM_add_100_ints_upb);

PROTO_BENCHMARK(add_100_ints_rs);

extern "C" {
void benchmark_thunk_add_num_rs(void* data, int32_t num);
}

void BM_add_100_ints_rs_roundtrip(benchmark::State& state) {
  for (auto s : state) {
    auto data = std::make_unique<BenchData>();
    for (int i = 0; i < 100; ++i) {
      benchmark_thunk_add_num_rs((void*)data.get(), i);
    }
  }
}

BENCHMARK(BM_add_100_ints_rs_roundtrip);

void BM_copy_from_100_ints_cpp(benchmark::State& state) {
  auto source = std::make_unique<BenchData>();
  for (int i = 0; i < 100; ++i) {
    source->add_nums(i);
  }
  for (auto s : state) {
    auto data = std::make_unique<BenchData>();
    *data->mutable_nums() = source->nums();
    ABSL_CHECK_EQ(data->nums()[99], 99);
  }
}

BENCHMARK(BM_copy_from_100_ints_cpp);

void BM_copy_from_100_ints_upb(benchmark::State& state) {
  ::protos::Arena arena;
  auto source = ::protos::CreateMessage<benchmarks::protos::BenchData>(arena);
  for (int i = 0; i < 100; ++i) {
    source.add_nums(i);
  }
  for (auto s : state) {
    auto data = ::protos::CreateMessage<benchmarks::protos::BenchData>(arena);
    data.resize_nums(source.nums_size());
    std::copy(source.nums().begin(), source.nums().end(),
              data.mutable_nums()->begin());
    ABSL_CHECK_EQ(data.nums()[99], 99);
  }
}

BENCHMARK(BM_copy_from_100_ints_upb);

PROTO_BENCHMARK(copy_from_100_ints_rs);

PROTO_BENCHMARK(extend_100_ints_rs);

EXTERN_BENCHMARK(extend_100_ints_vec_rs);

PROTO_BENCHMARK(sum_1000_ints_rs);

EXTERN_BENCHMARK(sum_1000_ints_vec_rs);

void BM_sum_1000_ints_cpp(benchmark::State& state) {
  auto source = std::make_unique<BenchData>();
  for (int i = 0; i < 1000; ++i) {
    source->add_nums(i);
  }

  for (auto s : state) {
    int sum = 0;
    for (auto x : source->nums()) {
      sum += x;
    }
    ABSL_CHECK_EQ(sum, 499500);
  }
}

BENCHMARK(BM_sum_1000_ints_cpp);

void BM_sum_1000_ints_upb(benchmark::State& state) {
  ::protos::Arena arena;
  auto data = ::protos::CreateMessage<benchmarks::protos::BenchData>(arena);
  for (int i = 0; i < 1000; ++i) {
    data.add_nums(i);
  }
  for (auto s : state) {
    int sum = 0;
    for (auto x : data.nums()) {
      sum += x;
    }
    ABSL_CHECK_EQ(sum, 499500);
  }
}

BENCHMARK(BM_sum_1000_ints_upb);

void BM_sum_1000_ints_vector(benchmark::State& state) {
  std::vector<int32_t> nums;
  nums.reserve(1000);
  for (int i = 0; i < 1000; ++i) {
    nums.push_back(i);
  }
  for (auto s : state) {
    int sum = 0;
    for (auto x : nums) {
      sum += x;
    }
    ABSL_CHECK_EQ(sum, 499500);
  }
}

BENCHMARK(BM_sum_1000_ints_vector);
