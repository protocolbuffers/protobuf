/*
 * Copyright (c) 2009-2022, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Testing strategy:  For each type of I/O (array, string, file, etc.) we
// create an output stream and write some data to it, then create a
// corresponding input stream to read the same data back and expect it to
// match.  When the data is written, it is written in several small chunks
// of varying sizes, with a BackUp() after each chunk.  It is read back
// similarly, but with chunks separated at different points.  The whole
// process is run with a variety of block sizes for both the input and
// the output.

#include "gtest/gtest.h"
#include "upb/io/chunked_input_stream.h"
#include "upb/io/chunked_output_stream.h"
#include "upb/upb.hpp"

namespace upb {
namespace {

class IoTest : public testing::Test {
 protected:
  // Test helpers.

  // Helper to write an array of data to an output stream.
  bool WriteToOutput(upb_ZeroCopyOutputStream* output, const void* data,
                     int size);
  // Helper to read a fixed-length array of data from an input stream.
  int ReadFromInput(upb_ZeroCopyInputStream* input, void* data, int size);
  // Write a string to the output stream.
  void WriteString(upb_ZeroCopyOutputStream* output, const std::string& str);
  // Read a number of bytes equal to the size of the given string and checks
  // that it matches the string.
  void ReadString(upb_ZeroCopyInputStream* input, const std::string& str);
  // Writes some text to the output stream in a particular order.  Returns
  // the number of bytes written, in case the caller needs that to set up an
  // input stream.
  int WriteStuff(upb_ZeroCopyOutputStream* output);
  // Reads text from an input stream and expects it to match what
  // WriteStuff() writes.
  void ReadStuff(upb_ZeroCopyInputStream* input, bool read_eof = true);

  // Similar to WriteStuff, but performs more sophisticated testing.
  int WriteStuffLarge(upb_ZeroCopyOutputStream* output);
  // Reads and tests a stream that should have been written to
  // via WriteStuffLarge().
  void ReadStuffLarge(upb_ZeroCopyInputStream* input);

  static const int kBlockSizes[];
  static const int kBlockSizeCount;
};

const int IoTest::kBlockSizes[] = {1, 2, 5, 7, 10, 23, 64};
const int IoTest::kBlockSizeCount = sizeof(IoTest::kBlockSizes) / sizeof(int);

bool IoTest::WriteToOutput(upb_ZeroCopyOutputStream* output, const void* data,
                           int size) {
  const uint8_t* in = reinterpret_cast<const uint8_t*>(data);
  size_t in_size = size;
  size_t out_size;

  while (true) {
    upb::Status status;
    void* out = upb_ZeroCopyOutputStream_Next(output, &out_size, status.ptr());
    if (out_size == 0) return false;

    if (in_size <= out_size) {
      memcpy(out, in, in_size);
      upb_ZeroCopyOutputStream_BackUp(output, out_size - in_size);
      return true;
    }

    memcpy(out, in, out_size);
    in += out_size;
    in_size -= out_size;
  }
}

int IoTest::ReadFromInput(upb_ZeroCopyInputStream* input, void* data,
                          int size) {
  uint8_t* out = reinterpret_cast<uint8_t*>(data);
  size_t out_size = size;

  const void* in;
  size_t in_size = 0;

  while (true) {
    upb::Status status;
    in = upb_ZeroCopyInputStream_Next(input, &in_size, status.ptr());

    if (in_size == 0) {
      return size - out_size;
    }

    if (out_size <= in_size) {
      memcpy(out, in, out_size);
      if (in_size > out_size) {
        upb_ZeroCopyInputStream_BackUp(input, in_size - out_size);
      }
      return size;  // Copied all of it.
    }

    memcpy(out, in, in_size);
    out += in_size;
    out_size -= in_size;
  }
}

void IoTest::WriteString(upb_ZeroCopyOutputStream* output,
                         const std::string& str) {
  EXPECT_TRUE(WriteToOutput(output, str.c_str(), str.size()));
}

void IoTest::ReadString(upb_ZeroCopyInputStream* input,
                        const std::string& str) {
  std::unique_ptr<char[]> buffer(new char[str.size() + 1]);
  buffer[str.size()] = '\0';
  EXPECT_EQ(ReadFromInput(input, buffer.get(), str.size()), str.size());
  EXPECT_STREQ(str.c_str(), buffer.get());
}

int IoTest::WriteStuff(upb_ZeroCopyOutputStream* output) {
  WriteString(output, "Hello world!\n");
  WriteString(output, "Some te");
  WriteString(output, "xt.  Blah blah.");
  WriteString(output, "abcdefg");
  WriteString(output, "01234567890123456789");
  WriteString(output, "foobar");

  const int result = upb_ZeroCopyOutputStream_ByteCount(output);
  EXPECT_EQ(result, 68);
  return result;
}

// Reads text from an input stream and expects it to match what WriteStuff()
// writes.
void IoTest::ReadStuff(upb_ZeroCopyInputStream* input, bool read_eof) {
  ReadString(input, "Hello world!\n");
  ReadString(input, "Some text.  ");
  ReadString(input, "Blah ");
  ReadString(input, "blah.");
  ReadString(input, "abcdefg");
  EXPECT_TRUE(upb_ZeroCopyInputStream_Skip(input, 20));
  ReadString(input, "foo");
  ReadString(input, "bar");

  EXPECT_EQ(upb_ZeroCopyInputStream_ByteCount(input), 68);

  if (read_eof) {
    uint8_t byte;
    EXPECT_EQ(ReadFromInput(input, &byte, 1), 0);
  }
}

int IoTest::WriteStuffLarge(upb_ZeroCopyOutputStream* output) {
  WriteString(output, "Hello world!\n");
  WriteString(output, "Some te");
  WriteString(output, "xt.  Blah blah.");
  WriteString(output, std::string(100000, 'x'));  // A very long string
  WriteString(output, std::string(100000, 'y'));  // A very long string
  WriteString(output, "01234567890123456789");

  const int result = upb_ZeroCopyOutputStream_ByteCount(output);
  EXPECT_EQ(result, 200055);
  return result;
}

// Reads text from an input stream and expects it to match what WriteStuff()
// writes.
void IoTest::ReadStuffLarge(upb_ZeroCopyInputStream* input) {
  ReadString(input, "Hello world!\nSome text.  ");
  EXPECT_TRUE(upb_ZeroCopyInputStream_Skip(input, 5));
  ReadString(input, "blah.");
  EXPECT_TRUE(upb_ZeroCopyInputStream_Skip(input, 100000 - 10));
  ReadString(input, std::string(10, 'x') + std::string(100000 - 20000, 'y'));
  EXPECT_TRUE(upb_ZeroCopyInputStream_Skip(input, 20000 - 10));
  ReadString(input, "yyyyyyyyyy01234567890123456789");
  EXPECT_EQ(upb_ZeroCopyInputStream_ByteCount(input), 200055);

  uint8_t byte;
  EXPECT_EQ(ReadFromInput(input, &byte, 1), 0);
}

// ===================================================================

TEST_F(IoTest, ArrayIo) {
  const int kBufferSize = 256;
  uint8_t buffer[kBufferSize];

  upb::Arena arena;
  for (int i = 0; i < kBlockSizeCount; i++) {
    for (int j = 0; j < kBlockSizeCount; j++) {
      auto output = upb_ChunkedOutputStream_New(buffer, kBufferSize,
                                                kBlockSizes[j], arena.ptr());
      int size = WriteStuff(output);
      auto input =
          upb_ChunkedInputStream_New(buffer, size, kBlockSizes[j], arena.ptr());
      ReadStuff(input);
    }
  }
}

TEST(ChunkedStream, SingleInput) {
  const int kBufferSize = 256;
  uint8_t buffer[kBufferSize];
  upb::Arena arena;
  auto input =
      upb_ChunkedInputStream_New(buffer, kBufferSize, kBufferSize, arena.ptr());
  const void* data;
  size_t size;

  upb::Status status;
  data = upb_ZeroCopyInputStream_Next(input, &size, status.ptr());
  EXPECT_EQ(size, kBufferSize);

  data = upb_ZeroCopyInputStream_Next(input, &size, status.ptr());
  EXPECT_EQ(data, nullptr);
  EXPECT_EQ(size, 0);
  EXPECT_TRUE(upb_Status_IsOk(status.ptr()));
}

TEST(ChunkedStream, SingleOutput) {
  const int kBufferSize = 256;
  uint8_t buffer[kBufferSize];
  upb::Arena arena;
  auto output = upb_ChunkedOutputStream_New(buffer, kBufferSize, kBufferSize,
                                            arena.ptr());
  size_t size;
  upb::Status status;
  void* data = upb_ZeroCopyOutputStream_Next(output, &size, status.ptr());
  EXPECT_EQ(size, kBufferSize);

  data = upb_ZeroCopyOutputStream_Next(output, &size, status.ptr());
  EXPECT_EQ(data, nullptr);
  EXPECT_EQ(size, 0);
  EXPECT_TRUE(upb_Status_IsOk(status.ptr()));
}

// Check that a zero-size input array doesn't confuse the code.
TEST(ChunkedStream, InputEOF) {
  upb::Arena arena;
  char buf;
  auto input = upb_ChunkedInputStream_New(&buf, 0, 1, arena.ptr());
  size_t size;
  upb::Status status;
  const void* data = upb_ZeroCopyInputStream_Next(input, &size, status.ptr());
  EXPECT_EQ(data, nullptr);
  EXPECT_EQ(size, 0);
  EXPECT_TRUE(upb_Status_IsOk(status.ptr()));
}

// Check that a zero-size output array doesn't confuse the code.
TEST(ChunkedStream, OutputEOF) {
  upb::Arena arena;
  char buf;
  auto output = upb_ChunkedOutputStream_New(&buf, 0, 1, arena.ptr());
  size_t size;
  upb::Status status;
  void* data = upb_ZeroCopyOutputStream_Next(output, &size, status.ptr());
  EXPECT_EQ(data, nullptr);
  EXPECT_EQ(size, 0);
  EXPECT_TRUE(upb_Status_IsOk(status.ptr()));
}

}  // namespace
}  // namespace upb
