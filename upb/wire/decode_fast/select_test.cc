#include "upb/wire/decode_fast/select.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <gtest/gtest.h>
#include "absl/base/macros.h"
#include "upb/wire/decode_fast/combinations.h"

namespace {

TEST(SelectTest, FunctionIndicesMatch) {
  // Verify that UPB_DECODEFAST_FUNCTION_IDX() and UPB_DECODEFAST_FUNCTIONS()
  // use the same ordering and the indexes correctly match.
#define IDX(type, card, size)                                                 \
  UPB_DECODEFAST_FUNCTION_IDX(kUpb_DecodeFast_##type, kUpb_DecodeFast_##card, \
                              kUpb_DecodeFast_##size),
  static const uint32_t numbers[] = {UPB_DECODEFAST_FUNCTIONS(IDX)};
  for (size_t i = 0; i < ABSL_ARRAYSIZE(numbers); i++) {
    EXPECT_EQ(numbers[i], i) << upb_DecodeFast_GetFunctionName(i) << " vs "
                             << upb_DecodeFast_GetFunctionName(numbers[i]);
  }
#undef IDX
}  // namespace

}  // namespace
