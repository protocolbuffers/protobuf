// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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

  using AnnotationCollector::AddAnnotation;
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
