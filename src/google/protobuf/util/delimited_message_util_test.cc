// Adapted from the patch of kenton@google.com (Kenton Varda)
// See https://github.com/protocolbuffers/protobuf/pull/710 for details.

#include <google/protobuf/util/delimited_message_util.h>

#include <sstream>

#include <google/protobuf/test_util.h>
#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace util {

TEST(DelimitedMessageUtilTest, DelimitedMessages) {
  std::stringstream stream;

  {
    protobuf_unittest::TestAllTypes message1;
    TestUtil::SetAllFields(&message1);
    EXPECT_TRUE(SerializeDelimitedToOstream(message1, &stream));

    protobuf_unittest::TestPackedTypes message2;
    TestUtil::SetPackedFields(&message2);
    EXPECT_TRUE(SerializeDelimitedToOstream(message2, &stream));
  }

  {
    bool clean_eof;
    io::IstreamInputStream zstream(&stream);

    protobuf_unittest::TestAllTypes message1;
    clean_eof = true;
    EXPECT_TRUE(ParseDelimitedFromZeroCopyStream(&message1,
        &zstream, &clean_eof));
    EXPECT_FALSE(clean_eof);
    TestUtil::ExpectAllFieldsSet(message1);

    protobuf_unittest::TestPackedTypes message2;
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

}  // namespace util
}  // namespace protobuf
}  // namespace google
