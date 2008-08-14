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
// emulates google3/testing/base/public/googletest.h

#ifndef GOOGLE_PROTOBUF_GOOGLETEST_H__
#define GOOGLE_PROTOBUF_GOOGLETEST_H__

#include <vector>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {

// When running unittests, get the directory containing the source code.
string TestSourceDir();

// When running unittests, get a directory where temporary files may be
// placed.
string TestTempDir();

// Capture all text written to stdout or stderr.
void CaptureTestStdout();
void CaptureTestStderr();

// Stop capturing stdout or stderr and return the text captured.
string GetCapturedTestStdout();
string GetCapturedTestStderr();

// For use with ScopedMemoryLog::GetMessages().  Inside Google the LogLevel
// constants don't have the LOGLEVEL_ prefix, so the code that used
// ScopedMemoryLog refers to LOGLEVEL_ERROR as just ERROR.
static const LogLevel ERROR = LOGLEVEL_ERROR;

// Receives copies of all LOG(ERROR) messages while in scope.  Sample usage:
//   {
//     ScopedMemoryLog log;  // constructor registers object as a log sink
//     SomeRoutineThatMayLogMessages();
//     const vector<string>& warnings = log.GetMessages(ERROR);
//   }  // destructor unregisters object as a log sink
// This is a dummy implementation which covers only what is used by protocol
// buffer unit tests.
class ScopedMemoryLog {
 public:
  ScopedMemoryLog();
  virtual ~ScopedMemoryLog();

  // Fetches all messages logged.  The internal version of this class
  // would only fetch messages at the given security level, but the protobuf
  // open source version ignores the argument since we always pass ERROR
  // anyway.
  const vector<string>& GetMessages(LogLevel dummy) const;

 private:
  vector<string> messages_;
  LogHandler* old_handler_;

  static void HandleLog(LogLevel level, const char* filename, int line,
                        const string& message);

  static ScopedMemoryLog* active_log_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ScopedMemoryLog);
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_GOOGLETEST_H__
