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

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/test_util2.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

using ::testing::ElementsAre;

// Generates Java code for the specified Java proto, returning the compiler's
// exit status.
int CompileJavaProto(std::string proto_file_name) {
  JavaGenerator java_generator;

  CommandLineInterface cli;
  cli.RegisterGenerator("--java_out", &java_generator, /*help_text=*/"");

  std::string proto_path = absl::StrCat(
      "--proto_path=",
      TestUtil::GetTestDataPath("google/protobuf/compiler/java"));
  std::string java_out = absl::StrCat("--java_out=", TestTempDir());

  const char* argv[] = {
      "protoc",
      proto_path.c_str(),
      java_out.c_str(),
      proto_file_name.c_str(),
  };

  return cli.Run(4, argv);
}

TEST(MessageSerializationTest, CollapseAdjacentExtensionRanges) {
  ABSL_CHECK_EQ(CompileJavaProto("message_serialization_unittest.proto"), 0);

  std::string java_source;
  ABSL_CHECK_OK(File::GetContents(
      // Open-source codebase does not support file::JoinPath, so we manually
      // concatenate instead.
      absl::StrCat(TestTempDir(),
                   "/TestMessageWithManyExtensionRanges.java"),
      &java_source, true));

  // Open-source codebase does not support constexpr absl::string_view.
  static constexpr const char kWriteUntilCall[] = "extensionWriter.writeUntil(";

  std::vector<std::string> range_ends;

  // Open-source codebase does not have absl::StrSplit overload taking a single
  // char delimiter.
  //
  // NOLINTNEXTLINE(abseil-faster-strsplit-delimiter)
  for (const auto& line : absl::StrSplit(java_source, "\n")) {
    // Extract end position from writeUntil call. (Open-source codebase does not
    // support RE2.)
    std::size_t write_until_pos = line.find(kWriteUntilCall);
    if (write_until_pos == std::string::npos) {
      continue;
    }
    write_until_pos += (sizeof(kWriteUntilCall) - 1);

    std::size_t comma_pos = line.find(',', write_until_pos);
    if (comma_pos == std::string::npos) {
      continue;
    }

    range_ends.push_back(
        std::string(line.substr(write_until_pos, comma_pos - write_until_pos)));
  }

  EXPECT_THAT(range_ends, ElementsAre("3", "13", "43"));
}

}  // namespace
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
