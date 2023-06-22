// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#include "google/protobuf/compiler/command_line_interface_tester.h"

#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace {

using ::testing::HasSubstr;

bool FileExists(const std::string& path) {
  return File::Exists(path);
}

}  // namespace

CommandLineInterfaceTester::CommandLineInterfaceTester() {
  temp_directory_ = absl::StrCat(TestTempDir(), "/proto2_cli_test_temp");

  // If the temp directory already exists, it must be left over from a
  // previous run.  Delete it.
  if (FileExists(temp_directory_)) {
    File::DeleteRecursively(temp_directory_, NULL, NULL);
  }

  // Create the temp directory.
  ABSL_CHECK_OK(File::CreateDir(temp_directory_, 0777));
}

CommandLineInterfaceTester::~CommandLineInterfaceTester() {
  // Delete the temp directory.
  if (FileExists(temp_directory_)) {
    File::DeleteRecursively(temp_directory_, NULL, NULL);
  }
}

void CommandLineInterfaceTester::RunProtoc(absl::string_view command) {
  RunProtocWithArgs(absl::StrSplit(command, ' ', absl::SkipEmpty()));
}

void CommandLineInterfaceTester::RunProtocWithArgs(
    std::vector<std::string> args) {
  std::vector<const char*> argv(args.size());

  for (size_t i = 0; i < args.size(); i++) {
    args[i] = absl::StrReplaceAll(args[i], {{"$tmpdir", temp_directory_}});
    argv[i] = args[i].c_str();
  }

  // TODO(jieluo): Cygwin doesn't work well if we try to capture stderr and
  // stdout at the same time. Need to figure out why and add this capture back
  // for Cygwin.
#if !defined(__CYGWIN__)
  CaptureTestStdout();
#endif
  CaptureTestStderr();

  return_code_ = cli_.Run(static_cast<int>(args.size()), argv.data());

  error_text_ = GetCapturedTestStderr();
#if !defined(__CYGWIN__)
  captured_stdout_ = GetCapturedTestStdout();
#endif
}

// -------------------------------------------------------------------

void CommandLineInterfaceTester::CreateTempFile(absl::string_view name,
                                                absl::string_view contents) {
  // Create parent directory, if necessary.
  std::string::size_type slash_pos = name.find_last_of('/');
  if (slash_pos != std::string::npos) {
    absl::string_view dir = name.substr(0, slash_pos);
    if (!FileExists(absl::StrCat(temp_directory_, "/", dir))) {
      ABSL_CHECK_OK(File::RecursivelyCreateDir(
          absl::StrCat(temp_directory_, "/", dir), 0777));
    }
  }

  // Write file.
  std::string full_name = absl::StrCat(temp_directory_, "/", name);
  ABSL_CHECK_OK(File::SetContents(
      full_name, absl::StrReplaceAll(contents, {{"$tmpdir", temp_directory_}}),
      true));
}

void CommandLineInterfaceTester::CreateTempDir(absl::string_view name) {
  ABSL_CHECK_OK(File::RecursivelyCreateDir(
      absl::StrCat(temp_directory_, "/", name), 0777));
}

// -------------------------------------------------------------------

void CommandLineInterfaceTester::ExpectNoErrors() {
  EXPECT_EQ(0, return_code_);
  EXPECT_EQ("", error_text_);
}

void CommandLineInterfaceTester::ExpectErrorText(
    absl::string_view expected_text) {
  EXPECT_NE(0, return_code_);
  EXPECT_EQ(absl::StrReplaceAll(expected_text, {{"$tmpdir", temp_directory_}}),
            error_text_);
}

void CommandLineInterfaceTester::ExpectErrorSubstring(
    absl::string_view expected_substring) {
  EXPECT_NE(0, return_code_);
  EXPECT_THAT(error_text_, HasSubstr(expected_substring));
}

void CommandLineInterfaceTester::ExpectWarningSubstring(
    absl::string_view expected_substring) {
  EXPECT_EQ(0, return_code_);
  EXPECT_THAT(error_text_, HasSubstr(expected_substring));
}

#if defined(_WIN32) && !defined(__CYGWIN__)
bool CommandLineInterfaceTester::HasAlternateErrorSubstring(
    const std::string& expected_substring) {
  EXPECT_NE(0, return_code_);
  return error_text_.find(expected_substring) != std::string::npos;
}
#endif  // _WIN32 && !__CYGWIN__

void CommandLineInterfaceTester::ExpectCapturedStdout(
    absl::string_view expected_text) {
  EXPECT_EQ(expected_text, captured_stdout_);
}

void CommandLineInterfaceTester::
    ExpectCapturedStdoutSubstringWithZeroReturnCode(
        absl::string_view expected_substring) {
  EXPECT_EQ(0, return_code_);
  EXPECT_THAT(captured_stdout_, HasSubstr(expected_substring));
}

void CommandLineInterfaceTester::
    ExpectCapturedStderrSubstringWithZeroReturnCode(
        absl::string_view expected_substring) {
  EXPECT_EQ(0, return_code_);
  EXPECT_THAT(error_text_, HasSubstr(expected_substring));
}

void CommandLineInterfaceTester::ExpectFileContent(absl::string_view filename,
                                                   absl::string_view content) {
  std::string path = absl::StrCat(temp_directory_, "/", filename);
  std::string file_contents;
  ABSL_CHECK_OK(File::GetContents(path, &file_contents, true));

  EXPECT_EQ(absl::StrReplaceAll(content, {{"$tmpdir", temp_directory_}}),
            file_contents);
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
