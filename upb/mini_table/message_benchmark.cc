// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>
#include "absl/random/random.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/message_benchmark.upb_minitable.h"
#include "upb/port/def.inc"

namespace {

UPB_NOINLINE static const upb_MiniTableField* FindFieldByNumberNoInline(
    const upb_MiniTable* m, uint32_t number) {
  return upb_MiniTable_FindFieldByNumber(m, number);
}

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
  constexpr uint32_t kSearchSize = 16384;
  std::vector<uint32_t> search(kSearchSize);
  for (auto& s : search) {
    s = absl::Uniform(bitgen, min, max);
  }
  uint32_t i = 0;
  uint32_t acc = 0;
  for (auto _ : state) {
    const upb_MiniTableField* field =
        upb_MiniTable_FindFieldByNumber(ptr, search[(i++ & (kSearchSize - 1))]);
    if (field) {
      acc += field->UPB_PRIVATE(offset);
    }
  }
  benchmark::DoNotOptimize(acc);
}
BENCHMARK(BM_FindFieldByNumber)->Arg(0)->Arg(1)->Arg(2);

static void BM_FindFieldByNumber_BySearchLen(benchmark::State& state) {
  const uint32_t search_len = static_cast<uint32_t>(state.range(0));
  upb_MiniTable mt =
      third_0party_0upb_0upb_0mini_0table__TestManyFields_msg_init;
  mt.UPB_ONLYBITS(field_count) = mt.UPB_PRIVATE(dense_below) + search_len;
  const upb_MiniTable* ptr = &mt;
  benchmark::DoNotOptimize(ptr);

  const uint32_t min = 171;
  const uint32_t max =
      mt.UPB_ONLYBITS(fields)[mt.UPB_ONLYBITS(field_count) - 1].UPB_ONLYBITS(
          number) +
      1;
  std::seed_seq seq{1, 2, 3};
  absl::BitGen bitgen(seq);
  constexpr uint32_t kSearchSize = 16384;
  std::vector<uint32_t> search(kSearchSize);
  for (auto& s : search) {
    s = absl::Uniform(bitgen, min, max);
  }
  uint32_t i = 0;
  uintptr_t acc = 0;
  for (auto _ : state) {
    const upb_MiniTableField* field =
        FindFieldByNumberNoInline(ptr, search[(i++ & (kSearchSize - 1))]);
    acc ^= reinterpret_cast<uintptr_t>(field);
  }
  benchmark::DoNotOptimize(acc);
}
BENCHMARK(BM_FindFieldByNumber_BySearchLen)->Arg(4)->Arg(16)->Arg(64)->Arg(382);

}  // namespace
