#include <cstdint>

#include "google/protobuf/rust/test/benchmarks/bench_data.pb.h"

using benchmarks::BenchData;

extern "C" void benchmark_thunk_set_num2(void* proto, int32_t num) {
  static_cast<BenchData*>(proto)->set_num2(num);
}

extern "C" void benchmark_thunk_add_num(void* proto, int32_t num) {
  static_cast<BenchData*>(proto)->add_nums(num);
}
