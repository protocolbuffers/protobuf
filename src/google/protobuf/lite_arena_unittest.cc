// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arena_test_util.h"
#include "google/protobuf/map_lite_unittest.pb.h"
#include "google/protobuf/map_test_util.h"

#include <gtest/gtest.h>


namespace google {
namespace protobuf {
namespace {

class LiteArenaTest : public testing::Test {
 protected:
  LiteArenaTest() {
    ArenaOptions options;
    options.start_block_size = 128 * 1024;
    options.max_block_size = 128 * 1024;
    arena_.reset(new Arena(options));
    // Trigger the allocation of the first arena block, so that further use of
    // the arena will not require any heap allocations.
    Arena::CreateArray<char>(arena_.get(), 1);
  }

  std::unique_ptr<Arena> arena_;
};

TEST_F(LiteArenaTest, MapNoHeapAllocation) {
  std::string data;
  data.reserve(128 * 1024);

  {
    // TODO: Enable no heap check when ArenaStringPtr is used in
    // Map.
    // internal::NoHeapChecker no_heap;

    proto2_unittest::TestArenaMapLite* from =
        Arena::Create<proto2_unittest::TestArenaMapLite>(arena_.get());
    MapTestUtil::SetArenaMapFields(from);
    from->SerializeToString(&data);

    proto2_unittest::TestArenaMapLite* to =
        Arena::Create<proto2_unittest::TestArenaMapLite>(arena_.get());
    to->ParseFromString(data);
    MapTestUtil::ExpectArenaMapFieldsSet(*to);
  }
}

TEST_F(LiteArenaTest, UnknownFieldMemLeak) {
  proto2_unittest::ForeignMessageArenaLite* message =
      Arena::Create<proto2_unittest::ForeignMessageArenaLite>(arena_.get());
  std::string data = "\012\000";
  int original_capacity = data.capacity();
  while (data.capacity() <= original_capacity) {
    data.append("a");
  }
  data[1] = data.size() - 2;
  message->ParseFromString(data);
}

}  // namespace
}  // namespace protobuf
}  // namespace google

