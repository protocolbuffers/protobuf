// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

#include <benchmark/benchmark.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_set.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/test_util/field_types.h"
#include "upb/wire/test_util/make_mini_table.h"
#include "upb/wire/test_util/wire_message.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace test {

namespace {

void BM_Decode(benchmark::State& state, const upb_MiniTable* mt,
               std::string payload, bool alias, bool initial_block) {
  char mem[4096];
  int decode_options = 0;
  for (auto s : state) {
    upb_Arena* arena = initial_block
                           ? upb_Arena_Init(mem, 4096, &upb_alloc_global)
                           : upb_Arena_New();
    upb_Message* msg = upb_Message_New(mt, arena);
    upb_DecodeStatus result = upb_Decode(payload.data(), payload.size(), msg,
                                         mt, nullptr, decode_options, arena);
    ASSERT_EQ(result, kUpb_DecodeStatus_Ok) << upb_DecodeStatus_String(result);
    upb_Arena_Free(arena);
  }
  state.SetBytesProcessed(state.iterations() * payload.size());
}

[[maybe_unused]] upb_Arena* benchmark_registration = [] {
  upb_Arena* arena = upb_Arena_New();
  std::vector<size_t> sizes{8, 64, 512};
  for (size_t size : sizes) {
    absl::flat_hash_set<upb_DecodeFast_Type> fast_types;
    ForEachType([&](auto&& type) {
      using Type = std::remove_reference_t<decltype(type)>;
      // We only need to benchmark each wire type once, since they are all
      // treated the same by the decoder.
      if (!fast_types.insert(Type::kFastType).second) return;
      std::vector<bool> initial_block{true, false};
      for (bool init : initial_block) {
        auto [mt, field] = MiniTable::MakeSingleFieldTable<Type>(
            1, kUpb_DecodeFast_Scalar, arena);
        std::string payload;
        while (payload.size() < size) {
          payload.append(ToBinaryPayload(wire_types::WireMessage{
              {1, Type::WireValue(typename Type::Value{})}}));
        }
        ::benchmark::RegisterBenchmark(
            absl::StrFormat("BM_Decode/%s/%zu/%s", Type::kName, size,
                            init ? "InitialBlock" : "NoInitialBlock")
                .c_str(),
            BM_Decode, mt, payload, false, init);
      }
    });
  }
  return arena;
}();

}  // namespace

}  // namespace test
}  // namespace upb
