#include <cstdint>

#include <benchmark/benchmark.h>
#include "absl/random/random.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/message_benchmark.upb_minitable.h"
#include "upb/port/def.inc"

namespace {
static void BM_FindFieldByNumber(benchmark::State& state) {
  uint32_t min, max;
  if (state.range(0)) {
    min = 1;
    max = 169;
  } else {
    min = 171;
    max = 552;
  }
  const upb_MiniTable* ptr =
      third_0party_0upb_0upb_0mini_0table__TestManyFields_msg_init_ptr;
  absl::BitGen bitgen;
  uint32_t search[1024];
  for (auto& s : search) {
    s = absl::Uniform(bitgen, min, max);
  }
  uint32_t i = 0;
  for (auto _ : state) {
    uint16_t offset = upb_MiniTable_FindFieldByNumber(ptr, search[(i++ % 1024)])
                          ->UPB_PRIVATE(offset);
    benchmark::DoNotOptimize(offset);
  }
}
BENCHMARK(BM_FindFieldByNumber)->Arg(true)->Arg(false);

}  // namespace
