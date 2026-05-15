#include <cstdint>
#include <random>

#include <benchmark/benchmark.h>
#include "absl/random/random.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/message_benchmark.upb_minitable.h"
#include "upb/port/def.inc"

namespace {
static void BM_FindFieldByNumber(benchmark::State& state) {
  uint32_t min, max;
  switch (state.range(0)) {
    case 0:
      min = 1;
      max = 169;
      break;
    case 1:
      min = 171;
      max = 552;
      break;
    default:
      min = 100;  // Some dense fields
      max = 570;  // some unknowns
      break;
  }
  const upb_MiniTable* ptr =
      &third_0party_0upb_0upb_0mini_0table__TestManyFields_msg_init;
  std::seed_seq seq{1, 2, 3};
  benchmark::DoNotOptimize(seq);
  absl::BitGen bitgen(seq);
  uint32_t search[1024];
  for (auto& s : search) {
    s = absl::Uniform(bitgen, min, max);
  }
  uint32_t i = 0;
  for (auto _ : state) {
    const upb_MiniTableField* field =
        upb_MiniTable_FindFieldByNumber(ptr, search[(i++ % 1024)]);
    if (field) {
      uint16_t offset = field->UPB_PRIVATE(offset);
      benchmark::DoNotOptimize(offset);
    }
  }
}
BENCHMARK(BM_FindFieldByNumber)->Arg(0)->Arg(1)->Arg(2);

}  // namespace
