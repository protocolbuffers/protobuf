// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Adapted from the patch of kenton@google.com (Kenton Varda)
// See https://github.com/protocolbuffers/protobuf/pull/710 for details.

#include "google/protobuf/util/delimited_message_util.h"

#include <sstream>

#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "google/protobuf/test_util.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"

namespace google {
namespace protobuf {
namespace util {

TEST(DelimitedMessageUtilTest, DelimitedMessages) {
  std::stringstream stream;

  {
    proto2_unittest::TestAllTypes message1;
    TestUtil::SetAllFields(&message1);
    EXPECT_TRUE(SerializeDelimitedToOstream(message1, &stream));

    proto2_unittest::TestPackedTypes message2;
    TestUtil::SetPackedFields(&message2);
    EXPECT_TRUE(SerializeDelimitedToOstream(message2, &stream));
  }

  {
    bool clean_eof;
    io::IstreamInputStream zstream(&stream);

    proto2_unittest::TestAllTypes message1;
    clean_eof = true;
    EXPECT_TRUE(ParseDelimitedFromZeroCopyStream(&message1,
        &zstream, &clean_eof));
    EXPECT_FALSE(clean_eof);
    TestUtil::ExpectAllFieldsSet(message1);

    proto2_unittest::TestPackedTypes message2;
    clean_eof = true;
    EXPECT_TRUE(ParseDelimitedFromZeroCopyStream(&message2,
        &zstream, &clean_eof));
    EXPECT_FALSE(clean_eof);
    TestUtil::ExpectPackedFieldsSet(message2);

    clean_eof = false;
    EXPECT_FALSE(ParseDelimitedFromZeroCopyStream(&message2,
        &zstream, &clean_eof));
    EXPECT_TRUE(clean_eof);
  }
}

TEST(DelimitedMessageUtilTest, FailsAtEndOfStream) {
  std::stringstream full_stream;
  std::stringstream partial_stream;

  {
    proto2_unittest::ForeignMessage message;
    message.set_c(42);
    message.set_d(24);
    EXPECT_TRUE(SerializeDelimitedToOstream(message, &full_stream));

    std::string full_output = full_stream.str();
    ASSERT_GT(full_output.size(), size_t{2});
    ASSERT_EQ(full_output[0], 4);

    partial_stream << full_output[0] << full_output[1] << full_output[2];
  }

  {
    bool clean_eof;
    io::IstreamInputStream zstream(&partial_stream);

    proto2_unittest::ForeignMessage message;
    clean_eof = true;
    EXPECT_FALSE(ParseDelimitedFromZeroCopyStream(&message,
        &zstream, &clean_eof));
    EXPECT_FALSE(clean_eof);
  }
}

TEST(DelimitedMessageUtilTest, SerializeToFileDescriptor) {
  int fd[2];
  pipe(fd);
  {
    proto2_unittest::TestAllTypes message1;
    TestUtil::SetAllFields(&message1);

    // Note that we need to be careful of not writing too much to the pipe
    // before it is read as we could exceed buffer limits which would cause
    // this call to hang. A more robust test would create a separate thread
    // or process to read the pipe while it is being written to.
    EXPECT_TRUE(SerializeDelimitedToFileDescriptor(message1, fd[1]));
  }
  {
    bool clean_eof;
    io::FileInputStream fstream(fd[0]);
    proto2_unittest::TestAllTypes message1;
    clean_eof = true;
    EXPECT_TRUE(ParseDelimitedFromZeroCopyStream(&message1,
        &fstream, &clean_eof));
    EXPECT_FALSE(clean_eof);
    TestUtil::ExpectAllFieldsSet(message1);
  }
  close(fd[0]);  // Close read end of pipe.
  {
    proto2_unittest::TestAllTypes message2;
    TestUtil::SetAllFields(&message2);
    // Serializing should now fail since the pipe is broken.
    EXPECT_FALSE(SerializeDelimitedToFileDescriptor(message2, fd[0]));
  }
  close(fd[1]);  // Close write end of pipe.
}

}  // namespace util
}  // namespace protobuf
}  // namespace google
