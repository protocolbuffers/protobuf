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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <ostream>
#include <string>
#include <tuple>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

namespace google {
namespace protobuf {
namespace io {
namespace {
class PrinterDeathTest : public testing::Test {
 protected:
  ZeroCopyOutputStream* output() {
    ABSL_CHECK(stream_.has_value());
    return &*stream_;
  }
  absl::string_view written() {
    stream_.reset();
    return out_;
  }

  std::string out_;
  absl::optional<StringOutputStream> stream_{&out_};
};

// FakeDescriptorFile defines only those members that Printer uses to write out
// annotations.
struct FakeDescriptorFile {
  const std::string& name() const { return filename; }
  std::string filename;
};

// FakeDescriptor defines only those members that Printer uses to write out
// annotations.
struct FakeDescriptor {
  const FakeDescriptorFile* file() const { return &fake_file; }
  void GetLocationPath(std::vector<int>* output) const { *output = path; }

  FakeDescriptorFile fake_file;
  std::vector<int> path;
};

class FakeAnnotationCollector : public AnnotationCollector {
 public:
  ~FakeAnnotationCollector() override = default;

  void AddAnnotation(size_t begin_offset, size_t end_offset,
                     const std::string& file_path,
                     const std::vector<int>& path) override {
  }
};

#if GTEST_HAS_DEATH_TEST
TEST_F(PrinterDeathTest, Death) {
  Printer printer(output(), '$');

  EXPECT_DEBUG_DEATH(printer.Print("$nosuchvar$"), "");
  EXPECT_DEBUG_DEATH(printer.Print("$unclosed"), "");
  EXPECT_DEBUG_DEATH(printer.Outdent(), "");
}

TEST_F(PrinterDeathTest, AnnotateMultipleUsesDeath) {
  FakeAnnotationCollector collector;
  Printer printer(output(), '$', &collector);
  printer.Print("012$foo$4$foo$\n", "foo", "3");

  FakeDescriptor descriptor{{"path"}, {33}};
  EXPECT_DEBUG_DEATH(printer.Annotate("foo", "foo", &descriptor), "");
}

TEST_F(PrinterDeathTest, AnnotateNegativeLengthDeath) {
  FakeAnnotationCollector collector;
  Printer printer(output(), '$', &collector);
  printer.Print("012$foo$4$bar$\n", "foo", "3", "bar", "5");

  FakeDescriptor descriptor{{"path"}, {33}};
  EXPECT_DEBUG_DEATH(printer.Annotate("bar", "foo", &descriptor), "");
}

TEST_F(PrinterDeathTest, AnnotateUndefinedDeath) {
  FakeAnnotationCollector collector;
  Printer printer(output(), '$', &collector);
  printer.Print("012$foo$4$foo$\n", "foo", "3");

  FakeDescriptor descriptor{{"path"}, {33}};
  EXPECT_DEBUG_DEATH(printer.Annotate("bar", "bar", &descriptor), "");
}

TEST_F(PrinterDeathTest, FormatInternalUnusedArgs) {
  FakeAnnotationCollector collector;
  Printer printer(output(), '$', &collector);

  EXPECT_DEATH(printer.FormatInternal({"arg1", "arg2"}, {}, "$1$"), "");
}

TEST_F(PrinterDeathTest, FormatInternalOutOfOrderArgs) {
  FakeAnnotationCollector collector;
  Printer printer(output(), '$', &collector);

  EXPECT_DEATH(printer.FormatInternal({"arg1", "arg2"}, {}, "$2$ $1$"), "");
}

TEST_F(PrinterDeathTest, FormatInternalZeroArg) {
  FakeAnnotationCollector collector;
  Printer printer(output(), '$', &collector);

  EXPECT_DEATH(printer.FormatInternal({"arg1", "arg2"}, {}, "$0$"), "");
}

TEST_F(PrinterDeathTest, FormatInternalOutOfBounds) {
  FakeAnnotationCollector collector;
  Printer printer(output(), '$', &collector);

  EXPECT_DEATH(printer.FormatInternal({"arg1", "arg2"}, {}, "$1$ $2$ $3$"), "");
}

TEST_F(PrinterDeathTest, FormatInternalUnknownVar) {
  FakeAnnotationCollector collector;
  Printer printer(output(), '$', &collector);

  EXPECT_DEATH(printer.FormatInternal({}, {}, "$huh$"), "");
  EXPECT_DEATH(printer.FormatInternal({}, {}, "$ $"), "");
}
#endif  // GTEST_HAS_DEATH_TEST
}  // namespace
}  // namespace io
}  // namespace protobuf
}  // namespace google
