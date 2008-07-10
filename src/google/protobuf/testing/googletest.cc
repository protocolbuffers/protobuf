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
// emulates google3/testing/base/public/googletest.cc

#include <google/protobuf/testing/googletest.h>
#include <google/protobuf/testing/file.h>
#include <google/protobuf/stubs/strutil.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <fcntl.h>

namespace google {
namespace protobuf {

#ifdef _WIN32
#define mkdir(name, mode) mkdir(name)
#endif

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0     // If this isn't defined, the platform doesn't need it.
#endif
#endif

string TestSourceDir() {
#ifdef _MSC_VER
  // Look for the "src" directory.
  string prefix = ".";

  while (!File::Exists(prefix + "/src/google/protobuf")) {
    if (!File::Exists(prefix)) {
      GOOGLE_LOG(FATAL)
        << "Could not find protobuf source code.  Please run tests from "
           "somewhere within the protobuf source package.";
    }
    prefix += "/..";
  }
  return prefix + "/src";
#else
  // automake sets the "srcdir" environment variable.
  char* result = getenv("srcdir");
  if (result == NULL) {
    // Otherwise, the test must be run from the source directory.
    return ".";
  } else {
    return result;
  }
#endif
}

namespace {

string GetTemporaryDirectoryName() {
  // tmpnam() is generally not considered safe but we're only using it for
  // testing.  We cannot use tmpfile() or mkstemp() since we're creating a
  // directory.
  string result = tmpnam(NULL);
#ifdef _WIN32
  // On Win32, tmpnam() returns a file prefixed with '\', but which is supposed
  // to be used in the current working directory.  WTF?
  if (HasPrefixString(result, "\\")) {
    result.erase(0, 1);
  }
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
      File::DeleteRecursively(name_, NULL, NULL);
    }
  }

  string GetTempDir() {
    if (name_.empty()) {
      name_ = GetTemporaryDirectoryName();
      GOOGLE_CHECK(mkdir(name_.c_str(), 0777) == 0) << strerror(errno);

      // Stick a file in the directory that tells people what this is, in case
      // we abort and don't get a chance to delete it.
      File::WriteStringToFileOrDie("", name_ + "/TEMP_DIR_FOR_PROTOBUF_TESTS");
    }
    return name_;
  }

 private:
  string name_;
};

TempDirDeleter temp_dir_deleter_;

}  // namespace

string TestTempDir() {
  return temp_dir_deleter_.GetTempDir();
}

static string stderr_capture_filename_;
static int original_stderr_ = -1;

void CaptureTestStderr() {
  GOOGLE_CHECK_EQ(original_stderr_, -1) << "Already capturing.";

  stderr_capture_filename_ = TestTempDir() + "/captured_stderr";

  int fd = open(stderr_capture_filename_.c_str(),
                O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0777);
  GOOGLE_CHECK(fd >= 0) << "open: " << strerror(errno);

  original_stderr_ = dup(2);
  close(2);
  dup2(fd, 2);
  close(fd);
}

string GetCapturedTestStderr() {
  GOOGLE_CHECK_NE(original_stderr_, -1) << "Not capturing.";

  close(2);
  dup2(original_stderr_, 2);
  original_stderr_ = -1;

  string result;
  File::ReadFileToStringOrDie(stderr_capture_filename_, &result);

  remove(stderr_capture_filename_.c_str());

  return result;
}

ScopedMemoryLog* ScopedMemoryLog::active_log_ = NULL;

ScopedMemoryLog::ScopedMemoryLog() {
  GOOGLE_CHECK(active_log_ == NULL);
  active_log_ = this;
  old_handler_ = SetLogHandler(&HandleLog);
}

ScopedMemoryLog::~ScopedMemoryLog() {
  SetLogHandler(old_handler_);
  active_log_ = NULL;
}

const vector<string>& ScopedMemoryLog::GetMessages(LogLevel dummy) const {
  GOOGLE_CHECK_EQ(dummy, ERROR);
  return messages_;
}

void ScopedMemoryLog::HandleLog(LogLevel level, const char* filename,
                                int line, const string& message) {
  GOOGLE_CHECK(active_log_ != NULL);
  if (level == ERROR) {
    active_log_->messages_.push_back(message);
  }
}

}  // namespace protobuf
}  // namespace google
