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

#include <google/protobuf/map_proto2_unittest.pb.h>
#include <google/protobuf/map_unittest.pb.h>
#include <google/protobuf/reflection_tester.h>
#include <google/protobuf/test_util2.h>


#define BRIDGE_UNITTEST ::google::protobuf::bridge_unittest
#define UNITTEST ::protobuf_unittest
#define UNITTEST_IMPORT ::protobuf_unittest_import
#define UNITTEST_PACKAGE_NAME "protobuf_unittest"

// Must include after defining UNITTEST, etc.
// clang-format off
#include <google/protobuf/test_util.inc>
#include <google/protobuf/map_test_util.inc>
#include <google/protobuf/map_test.inc>
// clang-format on

// Must be included last.
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace internal {
namespace {


struct AlignedAsDefault {
  int x;
};
struct alignas(8) AlignedAs8 {
  int x;
};

template <typename Aligned, bool on_arena = false>
void MapTest_Aligned() {
  Arena arena;
  constexpr size_t align_mask = alignof(Aligned) - 1;
  Map<int, Aligned> map(on_arena ? &arena : nullptr);
  map.insert({1, Aligned{}});
  auto it = map.find(1);
  ASSERT_NE(it, map.end());
  ASSERT_EQ(reinterpret_cast<intptr_t>(&it->second) & align_mask, 0);
  map.clear();
}

TEST(MapTest, Aligned) { MapTest_Aligned<AlignedAsDefault>(); }
TEST(MapTest, AlignedOnArena) { MapTest_Aligned<AlignedAsDefault, true>(); }
TEST(MapTest, Aligned8) { MapTest_Aligned<AlignedAs8>(); }
TEST(MapTest, Aligned8OnArena) { MapTest_Aligned<AlignedAs8, true>(); }


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
