// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/command_line_interface_tester.h"

#include <cstddef>
#include <string>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"

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

  // TODO: Cygwin doesn't work well if we try to capture stderr and
  // stdout at the same time. Need to figure out why and add this capture back
  // for Cygwin.
#if !defined(__CYGWIN__)
  CaptureTestStdout();
#endif
  CaptureTestStderr();

  return_code_ = cli_.Run(static_cast<int>(args.size()), argv.data());

  captured_stderr_ = GetCapturedTestStderr();
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

  // Note: since warnings and errors are both simply printed to stderr, we
  // can't holistically distinguish them here; in practice we don't have
  // multiline warnings so just counting any line with 'warning:' in it
  // is sufficient to separate warnings and errors in practice.
  for (const auto& line :
       absl::StrSplit(captured_stderr_, '\n', absl::SkipEmpty())) {
    EXPECT_THAT(line, HasSubstr("warning:"));
  }
}

void CommandLineInterfaceTester::ExpectErrorText(
    absl::string_view expected_text) {
  EXPECT_NE(0, return_code_);
  EXPECT_THAT(captured_stderr_,
              HasSubstr(absl::StrReplaceAll(expected_text,
                                            {{"$tmpdir", temp_directory_}})));
}

void CommandLineInterfaceTester::ExpectErrorSubstring(
    absl::string_view expected_substring) {
  EXPECT_NE(0, return_code_);
  EXPECT_THAT(captured_stderr_, HasSubstr(expected_substring));
}

void CommandLineInterfaceTester::ExpectWarningSubstring(
    absl::string_view expected_substring) {
  EXPECT_EQ(0, return_code_);
  EXPECT_THAT(captured_stderr_, HasSubstr(expected_substring));
}

#if defined(_WIN32) && !defined(__CYGWIN__)
bool CommandLineInterfaceTester::HasAlternateErrorSubstring(
    const std::string& expected_substring) {
  EXPECT_NE(0, return_code_);
  return captured_stderr_.find(expected_substring) != std::string::npos;
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
  EXPECT_THAT(captured_stderr_, HasSubstr(expected_substring));
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
