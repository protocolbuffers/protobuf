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

#include <string>
#include <iostream>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/arena_test_util.h>
#include <google/protobuf/map_lite_unittest.pb.h>
#include <google/protobuf/map_lite_test_util.h>
#include <google/protobuf/test_util_lite.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/stubs/strutil.h>

using namespace std;

#define EXPECT_TRUE GOOGLE_CHECK
#define ASSERT_TRUE GOOGLE_CHECK
#define EXPECT_FALSE(COND) GOOGLE_CHECK(!(COND))
#define EXPECT_EQ GOOGLE_CHECK_EQ
#define ASSERT_EQ GOOGLE_CHECK_EQ

int main(int argc, char* argv[]) {
  // Accessors
  {
    protobuf_unittest::TestMapLite message;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message);

    google::protobuf::MapLiteTestUtil::ModifyMapFields(&message);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsModified(message);
  }

  // SetMapFieldsInitialized
  {
    protobuf_unittest::TestMapLite message;

    google::protobuf::MapLiteTestUtil::SetMapFieldsInitialized(&message);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSetInitialized(message);
  }

  // Proto2SetMapFieldsInitialized
  {
    protobuf_unittest::TestEnumStartWithNonZeroMapLite message;
    EXPECT_EQ(protobuf_unittest::PROTO2_NON_ZERO_MAP_ENUM_FOO_LITE,
              (*message.mutable_map_field())[0]);
  }

  // Clear
  {
    protobuf_unittest::TestMapLite message;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message);
    message.Clear();
    google::protobuf::MapLiteTestUtil::ExpectClear(message);
  }

  // ClearMessageMap
  {
    protobuf_unittest::TestMessageMapLite message;

    // Creates a TestAllTypes with default value
    google::protobuf::TestUtilLite::ExpectClear(
        (*message.mutable_map_int32_message())[0]);
  }

  // CopyFrom
  {
    protobuf_unittest::TestMapLite message1, message2;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);
    message2.CopyFrom(message1);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);

    // Copying from self should be a no-op.
    message2.CopyFrom(message2);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }

  // CopyFromMessageMap
  {
    protobuf_unittest::TestMessageMapLite message1, message2;

    (*message1.mutable_map_int32_message())[0].add_repeated_int32(100);
    (*message2.mutable_map_int32_message())[0].add_repeated_int32(101);

    message1.CopyFrom(message2);

    // Checks repeated field is overwritten.
    EXPECT_EQ(1, message1.map_int32_message().at(0).repeated_int32_size());
    EXPECT_EQ(101, message1.map_int32_message().at(0).repeated_int32(0));
  }

  // SwapWithEmpty
  {
    protobuf_unittest::TestMapLite message1, message2;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message1);
    google::protobuf::MapLiteTestUtil::ExpectClear(message2);

    message1.Swap(&message2);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
    google::protobuf::MapLiteTestUtil::ExpectClear(message1);
  }

  // SwapWithSelf
  {
    protobuf_unittest::TestMapLite message;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message);

    message.Swap(&message);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message);
  }

  // SwapWithOther
  {
    protobuf_unittest::TestMapLite message1, message2;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);
    google::protobuf::MapLiteTestUtil::SetMapFields(&message2);
    google::protobuf::MapLiteTestUtil::ModifyMapFields(&message2);

    message1.Swap(&message2);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsModified(message1);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }

  // CopyConstructor
  {
    protobuf_unittest::TestMapLite message1;
    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);

    protobuf_unittest::TestMapLite message2(message1);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }

  // CopyAssignmentOperator
  {
    protobuf_unittest::TestMapLite message1;
    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);

    protobuf_unittest::TestMapLite message2;
    message2 = message1;
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);

    // Make sure that self-assignment does something sane.
    message2.operator=(message2);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }

  // NonEmptyMergeFrom
  {
    protobuf_unittest::TestMapLite message1, message2;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);

    // This field will test merging into an empty spot.
    (*message2.mutable_map_int32_int32())[1] = 1;
    message1.mutable_map_int32_int32()->erase(1);

    // This tests overwriting.
    (*message2.mutable_map_int32_double())[1] = 1;
    (*message1.mutable_map_int32_double())[1] = 2;

    message1.MergeFrom(message2);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message1);
  }

  // MergeFromMessageMap
  {
    protobuf_unittest::TestMessageMapLite message1, message2;

    (*message1.mutable_map_int32_message())[0].add_repeated_int32(100);
    (*message2.mutable_map_int32_message())[0].add_repeated_int32(101);

    message1.MergeFrom(message2);

    // Checks repeated field is overwritten.
    EXPECT_EQ(1, message1.map_int32_message().at(0).repeated_int32_size());
    EXPECT_EQ(101, message1.map_int32_message().at(0).repeated_int32(0));
  }

  // Test the generated SerializeWithCachedSizesToArray()
  {
    protobuf_unittest::TestMapLite message1, message2;
    string data;
    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);
    int size = message1.ByteSize();
    data.resize(size);
    google::protobuf::uint8* start =
        reinterpret_cast<google::protobuf::uint8*>(
            google::protobuf::string_as_array(&data));
    google::protobuf::uint8* end =
        message1.SerializeWithCachedSizesToArray(start);
    EXPECT_EQ(size, end - start);
    EXPECT_TRUE(message2.ParseFromString(data));
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }

  // Test the generated SerializeWithCachedSizes()
  {
    protobuf_unittest::TestMapLite message1, message2;
    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);
    int size = message1.ByteSize();
    string data;
    data.resize(size);
    {
      // Allow the output stream to buffer only one byte at a time.
      google::protobuf::io::ArrayOutputStream array_stream(
          google::protobuf::string_as_array(&data), size, 1);
      google::protobuf::io::CodedOutputStream output_stream(&array_stream);
      message1.SerializeWithCachedSizes(&output_stream);
      EXPECT_FALSE(output_stream.HadError());
      EXPECT_EQ(size, output_stream.ByteCount());
    }
    EXPECT_TRUE(message2.ParseFromString(data));
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }


  // Proto2UnknownEnum
  {
    protobuf_unittest::TestEnumMapPlusExtraLite from;
    (*from.mutable_known_map_field())[0] =
        protobuf_unittest::E_PROTO2_MAP_ENUM_FOO_LITE;
    (*from.mutable_unknown_map_field())[0] =
        protobuf_unittest::E_PROTO2_MAP_ENUM_EXTRA_LITE;
    string data;
    from.SerializeToString(&data);

    protobuf_unittest::TestEnumMapLite to;
    EXPECT_TRUE(to.ParseFromString(data));
    EXPECT_EQ(0, to.unknown_map_field().size());
    EXPECT_FALSE(to.mutable_unknown_fields()->empty());
    EXPECT_EQ(1, to.known_map_field().size());
    EXPECT_EQ(protobuf_unittest::PROTO2_MAP_ENUM_FOO_LITE,
              to.known_map_field().at(0));

    data.clear();
    from.Clear();
    to.SerializeToString(&data);
    EXPECT_TRUE(from.ParseFromString(data));
    EXPECT_EQ(1, from.known_map_field().size());
    EXPECT_EQ(protobuf_unittest::E_PROTO2_MAP_ENUM_FOO_LITE,
              from.known_map_field().at(0));
    EXPECT_EQ(1, from.unknown_map_field().size());
    EXPECT_EQ(protobuf_unittest::E_PROTO2_MAP_ENUM_EXTRA_LITE,
              from.unknown_map_field().at(0));
  }

  // StandardWireFormat
  {
    protobuf_unittest::TestMapLite message;
    string data = "\x0A\x04\x08\x01\x10\x01";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(1, message.map_int32_int32().at(1));
  }

  // UnorderedWireFormat
  {
    protobuf_unittest::TestMapLite message;

    // put value before key in wire format
    string data = "\x0A\x04\x10\x01\x08\x02";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(1, message.map_int32_int32().at(2));
  }

  // DuplicatedKeyWireFormat
  {
    protobuf_unittest::TestMapLite message;

    // Two key fields in wire format
    string data = "\x0A\x06\x08\x01\x08\x02\x10\x01";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(1, message.map_int32_int32().at(2));
  }

  // DuplicatedValueWireFormat
  {
    protobuf_unittest::TestMapLite message;

    // Two value fields in wire format
    string data = "\x0A\x06\x08\x01\x10\x01\x10\x02";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(2, message.map_int32_int32().at(1));
  }

  // MissedKeyWireFormat
  {
    protobuf_unittest::TestMapLite message;

    // No key field in wire format
    string data = "\x0A\x02\x10\x01";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(1, message.map_int32_int32().at(0));
  }

  // MissedValueWireFormat
  {
    protobuf_unittest::TestMapLite message;

    // No value field in wire format
    string data = "\x0A\x02\x08\x01";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(0, message.map_int32_int32().at(1));
  }

  // UnknownFieldWireFormat
  {
    protobuf_unittest::TestMapLite message;

    // Unknown field in wire format
    string data = "\x0A\x06\x08\x02\x10\x03\x18\x01";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(3, message.map_int32_int32().at(2));
  }

  // CorruptedWireFormat
  {
    protobuf_unittest::TestMapLite message;

    // corrupted data in wire format
    string data = "\x0A\x06\x08\x02\x11\x03";

    EXPECT_FALSE(message.ParseFromString(data));
  }

  // IsInitialized
  {
    protobuf_unittest::TestRequiredMessageMapLite map_message;

    // Add an uninitialized message.
    (*map_message.mutable_map_field())[0];
    EXPECT_FALSE(map_message.IsInitialized());

    // Initialize uninitialized message
    (*map_message.mutable_map_field())[0].set_a(0);
    (*map_message.mutable_map_field())[0].set_b(0);
    (*map_message.mutable_map_field())[0].set_c(0);
    EXPECT_TRUE(map_message.IsInitialized());
  }

  // arena support =================================================

  // ParsingAndSerializingNoHeapAllocation
  {
    // Allocate a large initial block to avoid mallocs during hooked test.
    std::vector<char> arena_block(128 * 1024);
    google::protobuf::ArenaOptions options;
    options.initial_block = arena_block.data();
    options.initial_block_size = arena_block.size();
    google::protobuf::Arena arena(options);
    string data;
    data.reserve(128 * 1024);

    {
      google::protobuf::internal::NoHeapChecker no_heap;

      protobuf_unittest::TestArenaMapLite* from =
          google::protobuf::Arena::CreateMessage<protobuf_unittest::TestArenaMapLite>(
              &arena);
      google::protobuf::MapLiteTestUtil::SetArenaMapFields(from);
      from->SerializeToString(&data);

      protobuf_unittest::TestArenaMapLite* to =
          google::protobuf::Arena::CreateMessage<protobuf_unittest::TestArenaMapLite>(
              &arena);
      to->ParseFromString(data);
      // google::protobuf::MapLiteTestUtil::ExpectArenaMapFieldsSet(*to);
    }
  }

  std::cout << "PASS" << std::endl;
  return 0;
}
