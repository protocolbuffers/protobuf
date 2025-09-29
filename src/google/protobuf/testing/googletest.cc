// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
// emulates google3/testing/base/public/googletest.cc

#include "google/protobuf/testing/googletest.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "google/protobuf/io/io_win32.h"
#include "google/protobuf/testing/file.h"
#include <gtest/gtest.h>

#ifdef _MSC_VER
// #include <direct.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace google {
namespace protobuf {

#ifdef _WIN32
// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
// them like we do below.
using google::protobuf::io::win32::close;
using google::protobuf::io::win32::dup2;
using google::protobuf::io::win32::dup;
using google::protobuf::io::win32::mkdir;
using google::protobuf::io::win32::open;
#endif

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0     // If this isn't defined, the platform doesn't need it.
#endif
#endif

std::string TestSourceDir() {
#ifndef GOOGLE_THIRD_PARTY_PROTOBUF
#ifdef GOOGLE_PROTOBUF_TEST_SOURCE_PATH
  return GOOGLE_PROTOBUF_TEST_SOURCE_PATH;
#else
#ifndef _MSC_VER
  // automake sets the "srcdir" environment variable.
  char* result = getenv("srcdir");
  if (result != nullptr) {
    return result;
  }
#endif  // _MSC_VER

  // Look for the "src" directory.
  std::string prefix = ".";

  // Keep looking further up the directory tree until we find
  // src/.../descriptor.cc. It is important to look for a particular file,
  // keeping in mind that with Bazel builds the directory structure under
  // bazel-bin/ looks similar to the main directory tree in the Git repo.
  while (!File::Exists(
      absl::StrCat(prefix, "/src/google/protobuf/descriptor.cc"))) {
    if (!File::Exists(prefix)) {
      ABSL_LOG(FATAL)
          << "Could not find protobuf source code.  Please run tests from "
             "somewhere within the protobuf source package.";
    }
    absl::StrAppend(&prefix, "/..");
  }
  absl::StrAppend(&prefix, "/src");
  return prefix;
#endif  // GOOGLE_PROTOBUF_TEST_SOURCE_PATH
#else
  return "third_party/protobuf/src";
#endif  // GOOGLE_THIRD_PARTY_PROTOBUF
}

namespace {

std::string GetTemporaryDirectoryName() {
#ifdef _WIN32
  DWORD pid = GetCurrentProcessId();
#else
  int pid = getpid();
#endif
  std::string result = absl::StrCat(testing::TempDir(), "protobuf_tempdir.", pid);
#ifdef _WIN32
  // The Win32 API accepts forward slashes as a path delimiter as long as the
  // path doesn't use the "\\?\" prefix.
  // Let's avoid confusion and use only forward slashes.
  result = absl::StrReplaceAll(result, {{"\\", "/"}});
#endif  // _WIN32
  return result;
}

// Creates a temporary directory on demand and deletes it when the process
// quits.
class TempDirDeleter {
 public:
  TempDirDeleter() {}
  ~TempDirDeleter() {
    if (!name_.empty()) {
      File::DeleteRecursively(name_, nullptr, nullptr);
    }
  }

  std::string GetTempDir() {
    if (name_.empty()) {
      name_ = GetTemporaryDirectoryName();
      File::DeleteRecursively(name_, nullptr, nullptr);
      ABSL_CHECK(mkdir(name_.c_str(), 0777) == 0) << strerror(errno);

      // Stick a file in the directory that tells people what this is, in case
      // we abort and don't get a chance to delete it.
      File::WriteStringToFileOrDie(
          "", absl::StrCat(name_, "/TEMP_DIR_FOR_PROTOBUF_TESTS"));
    }
    return name_;
  }

 private:
  std::string name_;
};

TempDirDeleter temp_dir_deleter_;

}  // namespace

std::string TestTempDir() { return temp_dir_deleter_.GetTempDir(); }

// TODO:  Share duplicated code below.  Too busy/lazy for now.

static std::string stdout_capture_filename_;
static std::string stderr_capture_filename_;
static int original_stdout_ = -1;
static int original_stderr_ = -1;

void CaptureTestStdout() {
  ABSL_CHECK_EQ(original_stdout_, -1) << "Already capturing.";

  stdout_capture_filename_ = absl::StrCat(TestTempDir(), "/captured_stdout");

  int fd = open(stdout_capture_filename_.c_str(),
                O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0777);
  ABSL_CHECK(fd >= 0) << "open: " << strerror(errno);

  original_stdout_ = dup(1);
  close(1);
  dup2(fd, 1);
  close(fd);
}

void CaptureTestStderr() {
  ABSL_CHECK_EQ(original_stderr_, -1) << "Already capturing.";

  stderr_capture_filename_ = absl::StrCat(TestTempDir(), "/captured_stderr");

  int fd = open(stderr_capture_filename_.c_str(),
                O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0777);
  ABSL_CHECK(fd >= 0) << "open: " << strerror(errno);

  original_stderr_ = dup(2);
  close(2);
  dup2(fd, 2);
  close(fd);
}

std::string GetCapturedTestStdout() {
  ABSL_CHECK_NE(original_stdout_, -1) << "Not capturing.";

  close(1);
  dup2(original_stdout_, 1);
  close(original_stdout_);
  original_stdout_ = -1;

  std::string result;
  File::ReadFileToStringOrDie(stdout_capture_filename_, &result);

  remove(stdout_capture_filename_.c_str());

  return result;
}

std::string GetCapturedTestStderr() {
  ABSL_CHECK_NE(original_stderr_, -1) << "Not capturing.";

  close(2);
  dup2(original_stderr_, 2);
  close(original_stderr_);
  original_stderr_ = -1;

  std::string result;
  File::ReadFileToStringOrDie(stderr_capture_filename_, &result);

  remove(stderr_capture_filename_.c_str());

  return result;
}

namespace {

// Force shutdown at process exit so that we can test for memory leaks.  To
// actually check for leaks, I suggest using the heap checker included with
// google-perftools.  Set it to "draconian" mode to ensure that every last
// call to malloc() has a corresponding free().
struct ForceShutdown {
  ~ForceShutdown() {
    ShutdownProtobufLibrary();
    // Test to shutdown the library twice, which should succeed.
    ShutdownProtobufLibrary();
  }
} force_shutdown;

}  // namespace

}  // namespace protobuf
}  // namespace google
