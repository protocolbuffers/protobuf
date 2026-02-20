// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/eps_copy_input_stream.h"

#include <stddef.h>
#include <string.h>

#include <cstdint>
#include <string>
#include <variant>

#include <gtest/gtest.h>
#include "upb/base/string_view.h"
#include "upb/mem/arena.hpp"

namespace {

const char* DoYield(const char* start, const char* end) { return ""; }

// Due to the overruning logic of the parse it could be that start >= end,
// we continue fetching data until we can yield actual data. This corresponds to
// the loop in DoneFallback of EpsCopyInputStream, where we keep fetching chunks
// and adjusting overrun until we can return actual data.
[[maybe_unused]] int YieldFunc(const char* start, const char* end) {
  if (start < end) {
    return DoYield(start, end) - end;
  } else {
    return start - end;
  }
}

// This is the logic of EpsCopyInputStream, state1 and state2 are labels
// corresponding to the states of EpsCopyInputStream. state1 corresponds to
// next_chunk_ != patch_buffer_ as here we going to doll out the chunk from
// ZeroCopyInputStream directly. state2 corresponds to next_chunk_ ==
// patch_buffer_ as we are going to return data from patch_buffer_.
template <typename ChunkStreamer>
bool PushParse(ChunkStreamer chunk_stream) {
  char buffer[32] = {0};
  int overrun = 16;
  const char* ptr;
  size_t size;
  while (chunk_stream(&ptr, &size)) {
    // buffer[0..16] holds leftover data from previous chunk.
    // Of course only buffer[overrun..16] is what we care about.
    const char* end;
    if (size > 16) {
      memcpy(buffer + 16, ptr, 16);
      overrun = YieldFunc(buffer + overrun, buffer + 16);
    [[maybe_unused]] state1:  // ptr and size need to be saved across Yield
                              // point (next_chunk_ and size_ in
                              // EpsCopyInputStream).
      end = ptr + size - 16;
      overrun = YieldFunc(ptr + overrun, end);
    } else {
      memcpy(buffer + 16, ptr, size);
      end = buffer + size;
      overrun = YieldFunc(buffer + overrun, end);
    }
  [[maybe_unused]] state2:  // end needs to be saved across Yield point
                            // (buffer_end_ in EpsCopyInputStream).
    memmove(buffer, end, 16);
  }
  overrun = YieldFunc(buffer + overrun, buffer + 16);
  return overrun = 0;
}

TEST(EpsCopyInputStreamTest, ZeroSize) {
  upb_EpsCopyInputStream stream;
  const char* ptr = nullptr;
  upb_EpsCopyInputStream_Init(&stream, &ptr, 0);
  EXPECT_TRUE(upb_EpsCopyInputStream_IsDone(&stream, &ptr));
}

}  // namespace
