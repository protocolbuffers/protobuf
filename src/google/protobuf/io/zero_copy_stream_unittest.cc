// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Testing strategy:  For each type of I/O (array, string, file, etc.) we
// create an output stream and write some data to it, then create a
// corresponding input stream to read the same data back and expect it to
// match.  When the data is written, it is written in several small chunks
// of varying sizes, with a BackUp() after each chunk.  It is read back
// similarly, but with chunks separated at different points.  The whole
// process is run with a variety of block sizes for both the input and
// the output.
//
// TODO(kenton):  Rewrite this test to bring it up to the standards of all
//   the other proto2 tests.  May want to wait for gTest to implement
//   "parametized tests" so that one set of tests can be used on all the
//   implementations.

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>

#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace io {
namespace {

#ifdef _WIN32
#define pipe(fds) _pipe(fds, 4096, O_BINARY)
#endif

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0     // If this isn't defined, the platform doesn't need it.
#endif
#endif

class IoTest : public testing::Test {
 protected:
  // Test helpers.

  // Helper to write an array of data to an output stream.
  bool WriteToOutput(ZeroCopyOutputStream* output, const void* data, int size);
  // Helper to read a fixed-length array of data from an input stream.
  int ReadFromInput(ZeroCopyInputStream* input, void* data, int size);
  // Write a string to the output stream.
  void WriteString(ZeroCopyOutputStream* output, const char* str);
  // Read a number of bytes equal to the size of the given string and checks
  // that it matches the string.
  void ReadString(ZeroCopyInputStream* input, const char* str);
  // Writes some text to the output stream in a particular order.  Returns
  // the number of bytes written, incase the caller needs that to set up an
  // input stream.
  int WriteStuff(ZeroCopyOutputStream* output);
  // Reads text from an input stream and expects it to match what
  // WriteStuff() writes.
  void ReadStuff(ZeroCopyInputStream* input);

  static const int kBlockSizes[];
  static const int kBlockSizeCount;
};

const int IoTest::kBlockSizes[] = {-1, 1, 2, 5, 7, 10, 23, 64};
const int IoTest::kBlockSizeCount = GOOGLE_ARRAYSIZE(IoTest::kBlockSizes);

bool IoTest::WriteToOutput(ZeroCopyOutputStream* output,
                           const void* data, int size) {
  const uint8* in = reinterpret_cast<const uint8*>(data);
  int in_size = size;

  void* out;
  int out_size;

  while (true) {
    if (!output->Next(&out, &out_size)) {
      return false;
    }

    if (in_size <= out_size) {
      memcpy(out, in, in_size);
      output->BackUp(out_size - in_size);
      return true;
    }

    memcpy(out, in, out_size);
    in += out_size;
    in_size -= out_size;
  }
}

int IoTest::ReadFromInput(ZeroCopyInputStream* input, void* data, int size) {
  uint8* out = reinterpret_cast<uint8*>(data);
  int out_size = size;

  const void* in;
  int in_size = 0;

  while (true) {
    if (!input->Next(&in, &in_size)) {
      return size - out_size;
    }

    if (out_size <= in_size) {
      memcpy(out, in, out_size);
      input->BackUp(in_size - out_size);
      return size;  // Copied all of it.
    }

    memcpy(out, in, in_size);
    out += in_size;
    out_size -= in_size;
  }
}

void IoTest::WriteString(ZeroCopyOutputStream* output, const char* str) {
  EXPECT_TRUE(WriteToOutput(output, str, strlen(str)));
}

void IoTest::ReadString(ZeroCopyInputStream* input, const char* str) {
  int length = strlen(str);
  scoped_array<char> buffer(new char[length + 1]);
  buffer[length] = '\0';
  EXPECT_EQ(ReadFromInput(input, buffer.get(), length), length);
  EXPECT_STREQ(str, buffer.get());
}

int IoTest::WriteStuff(ZeroCopyOutputStream* output) {
  WriteString(output, "Hello world!\n");
  WriteString(output, "Some te");
  WriteString(output, "xt.  Blah blah.");
  WriteString(output, "abcdefg");
  WriteString(output, "01234567890123456789");
  WriteString(output, "foobar");

  EXPECT_EQ(output->ByteCount(), 68);

  int result = output->ByteCount();
  return result;
}

// Reads text from an input stream and expects it to match what WriteStuff()
// writes.
void IoTest::ReadStuff(ZeroCopyInputStream* input) {
  ReadString(input, "Hello world!\n");
  ReadString(input, "Some text.  ");
  ReadString(input, "Blah ");
  ReadString(input, "blah.");
  ReadString(input, "abcdefg");
  EXPECT_TRUE(input->Skip(20));
  ReadString(input, "foo");
  ReadString(input, "bar");

  EXPECT_EQ(input->ByteCount(), 68);

  uint8 byte;
  EXPECT_EQ(ReadFromInput(input, &byte, 1), 0);
}

// ===================================================================

TEST_F(IoTest, ArrayIo) {
  const int kBufferSize = 256;
  uint8 buffer[kBufferSize];

  for (int i = 0; i < kBlockSizeCount; i++) {
    for (int j = 0; j < kBlockSizeCount; j++) {
      int size;
      {
        ArrayOutputStream output(buffer, kBufferSize, kBlockSizes[i]);
        size = WriteStuff(&output);
      }
      {
        ArrayInputStream input(buffer, size, kBlockSizes[j]);
        ReadStuff(&input);
      }
    }
  }
}

// There is no string input, only string output.  Also, it doesn't support
// explicit block sizes.  So, we'll only run one test and we'll use
// ArrayInput to read back the results.
TEST_F(IoTest, StringIo) {
  string str;
  {
    StringOutputStream output(&str);
    WriteStuff(&output);
  }
  {
    ArrayInputStream input(str.data(), str.size());
    ReadStuff(&input);
  }
}


// To test files, we create a temporary file, write, read, truncate, repeat.
TEST_F(IoTest, FileIo) {
  string filename = TestTempDir() + "/zero_copy_stream_test_file";

  for (int i = 0; i < kBlockSizeCount; i++) {
    for (int j = 0; j < kBlockSizeCount; j++) {
      // Make a temporary file.
      int file =
        open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0777);
      ASSERT_GE(file, 0);

      {
        FileOutputStream output(file, kBlockSizes[i]);
        WriteStuff(&output);
        EXPECT_EQ(0, output.GetErrno());
      }

      // Rewind.
      ASSERT_NE(lseek(file, 0, SEEK_SET), (off_t)-1);

      {
        FileInputStream input(file, kBlockSizes[j]);
        ReadStuff(&input);
        EXPECT_EQ(0, input.GetErrno());
      }

      close(file);
    }
  }
}

// MSVC raises various debugging exceptions if we try to use a file
// descriptor of -1, defeating our tests below.  This class will disable
// these debug assertions while in scope.
class MsvcDebugDisabler {
 public:
#ifdef _MSC_VER
  MsvcDebugDisabler() {
    old_handler_ = _set_invalid_parameter_handler(MyHandler);
    old_mode_ = _CrtSetReportMode(_CRT_ASSERT, 0);
  }
  ~MsvcDebugDisabler() {
    old_handler_ = _set_invalid_parameter_handler(old_handler_);
    old_mode_ = _CrtSetReportMode(_CRT_ASSERT, old_mode_);
  }

  static void MyHandler(const wchar_t *expr,
                        const wchar_t *func,
                        const wchar_t *file,
                        unsigned int line,
                        uintptr_t pReserved) {
    // do nothing
  }

  _invalid_parameter_handler old_handler_;
  int old_mode_;
#else
  // Dummy constructor and destructor to ensure that GCC doesn't complain
  // that debug_disabler is an unused variable.
  MsvcDebugDisabler() {}
  ~MsvcDebugDisabler() {}
#endif
};

// Test that FileInputStreams report errors correctly.
TEST_F(IoTest, FileReadError) {
  MsvcDebugDisabler debug_disabler;

  // -1 = invalid file descriptor.
  FileInputStream input(-1);

  const void* buffer;
  int size;
  EXPECT_FALSE(input.Next(&buffer, &size));
  EXPECT_EQ(EBADF, input.GetErrno());
}

// Test that FileOutputStreams report errors correctly.
TEST_F(IoTest, FileWriteError) {
  MsvcDebugDisabler debug_disabler;

  // -1 = invalid file descriptor.
  FileOutputStream input(-1);

  void* buffer;
  int size;

  // The first call to Next() succeeds because it doesn't have anything to
  // write yet.
  EXPECT_TRUE(input.Next(&buffer, &size));

  // Second call fails.
  EXPECT_FALSE(input.Next(&buffer, &size));

  EXPECT_EQ(EBADF, input.GetErrno());
}

// Pipes are not seekable, so File{Input,Output}Stream ends up doing some
// different things to handle them.  We'll test by writing to a pipe and
// reading back from it.
TEST_F(IoTest, PipeIo) {
  int files[2];

  for (int i = 0; i < kBlockSizeCount; i++) {
    for (int j = 0; j < kBlockSizeCount; j++) {
      // Need to create a new pipe each time because ReadStuff() expects
      // to see EOF at the end.
      ASSERT_EQ(pipe(files), 0);

      {
        FileOutputStream output(files[1], kBlockSizes[i]);
        WriteStuff(&output);
        EXPECT_EQ(0, output.GetErrno());
      }
      close(files[1]);  // Send EOF.

      {
        FileInputStream input(files[0], kBlockSizes[j]);
        ReadStuff(&input);
        EXPECT_EQ(0, input.GetErrno());
      }
      close(files[0]);
    }
  }
}

// Test using C++ iostreams.
TEST_F(IoTest, IostreamIo) {
  for (int i = 0; i < kBlockSizeCount; i++) {
    for (int j = 0; j < kBlockSizeCount; j++) {
      stringstream stream;

      {
        OstreamOutputStream output(&stream, kBlockSizes[i]);
        WriteStuff(&output);
        EXPECT_FALSE(stream.fail());
      }

      {
        IstreamInputStream input(&stream, kBlockSizes[j]);
        ReadStuff(&input);
        EXPECT_TRUE(stream.eof());
      }
    }
  }
}

// To test ConcatenatingInputStream, we create several ArrayInputStreams
// covering a buffer and then concatenate them.
TEST_F(IoTest, ConcatenatingInputStream) {
  const int kBufferSize = 256;
  uint8 buffer[kBufferSize];

  // Fill the buffer.
  ArrayOutputStream output(buffer, kBufferSize);
  WriteStuff(&output);

  // Now split it up into multiple streams of varying sizes.
  ASSERT_EQ(68, output.ByteCount());  // Test depends on this.
  ArrayInputStream input1(buffer     , 12);
  ArrayInputStream input2(buffer + 12,  7);
  ArrayInputStream input3(buffer + 19,  6);
  ArrayInputStream input4(buffer + 25, 15);
  ArrayInputStream input5(buffer + 40,  0);
  // Note:  We want to make sure we have a stream boundary somewhere between
  // bytes 42 and 62, which is the range that it Skip()ed by ReadStuff().  This
  // tests that a bug that existed in the original code for Skip() is fixed.
  ArrayInputStream input6(buffer + 40, 10);
  ArrayInputStream input7(buffer + 50, 18);  // Total = 68 bytes.

  ZeroCopyInputStream* streams[] =
    {&input1, &input2, &input3, &input4, &input5, &input6, &input7};

  // Create the concatenating stream and read.
  ConcatenatingInputStream input(streams, GOOGLE_ARRAYSIZE(streams));
  ReadStuff(&input);
}

// To test LimitingInputStream, we write our golden text to a buffer, then
// create an ArrayInputStream that contains the whole buffer (not just the
// bytes written), then use a LimitingInputStream to limit it just to the
// bytes written.
TEST_F(IoTest, LimitingInputStream) {
  const int kBufferSize = 256;
  uint8 buffer[kBufferSize];

  // Fill the buffer.
  ArrayOutputStream output(buffer, kBufferSize);
  WriteStuff(&output);

  // Set up input.
  ArrayInputStream array_input(buffer, kBufferSize);
  LimitingInputStream input(&array_input, output.ByteCount());

  ReadStuff(&input);
}

// Check that a zero-size array doesn't confuse the code.
TEST(ZeroSizeArray, Input) {
  ArrayInputStream input(NULL, 0);
  const void* data;
  int size;
  EXPECT_FALSE(input.Next(&data, &size));
}

TEST(ZeroSizeArray, Output) {
  ArrayOutputStream output(NULL, 0);
  void* data;
  int size;
  EXPECT_FALSE(output.Next(&data, &size));
}

}  // namespace
}  // namespace io
}  // namespace protobuf
}  // namespace google
