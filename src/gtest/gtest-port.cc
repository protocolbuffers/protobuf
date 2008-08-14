// Copyright 2008, Google Inc.
// All rights reserved.
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
//
// Author: wan@google.com (Zhanyong Wan)

#include <gtest/internal/gtest-port.h>

#include <limits.h>
#ifdef GTEST_HAS_DEATH_TEST
#include <regex.h>
#endif  // GTEST_HAS_DEATH_TEST
#include <stdlib.h>
#include <stdio.h>

#include <gtest/gtest-spi.h>
#include <gtest/gtest-message.h>
#include <gtest/internal/gtest-string.h>

namespace testing {
namespace internal {

#ifdef GTEST_HAS_DEATH_TEST

// Implements RE.  Currently only needed for death tests.

RE::~RE() {
  regfree(&regex_);
  free(const_cast<char*>(pattern_));
}

// Returns true iff str contains regular expression re.
bool RE::PartialMatch(const char* str, const RE& re) {
  if (!re.is_valid_) return false;

  regmatch_t match;
  return regexec(&re.regex_, str, 1, &match, 0) == 0;
}

// Initializes an RE from its string representation.
void RE::Init(const char* regex) {
  pattern_ = strdup(regex);
  is_valid_ = regcomp(&regex_, regex, REG_EXTENDED) == 0;
  EXPECT_TRUE(is_valid_)
      << "Regular expression \"" << regex
      << "\" is not a valid POSIX Extended regular expression.";
}

#endif  // GTEST_HAS_DEATH_TEST

// Logs a message at the given severity level.
void GTestLog(GTestLogSeverity severity, const char* file,
              int line, const char* msg) {
  const char* const marker =
      severity == GTEST_INFO ?    "[  INFO ]" :
      severity == GTEST_WARNING ? "[WARNING]" :
      severity == GTEST_ERROR ?   "[ ERROR ]" : "[ FATAL ]";
  fprintf(stderr, "\n%s %s:%d: %s\n", marker, file, line, msg);
  if (severity == GTEST_FATAL) {
    abort();
  }
}

#ifdef GTEST_HAS_DEATH_TEST

// Defines the stderr capturer.

class CapturedStderr {
 public:
  // The ctor redirects stderr to a temporary file.
  CapturedStderr() {
    uncaptured_fd_ = dup(STDERR_FILENO);

    char name_template[] = "captured_stderr.XXXXXX";
    const int captured_fd = mkstemp(name_template);
    filename_ = name_template;
    fflush(NULL);
    dup2(captured_fd, STDERR_FILENO);
    close(captured_fd);
  }

  ~CapturedStderr() {
    remove(filename_.c_str());
  }

  // Stops redirecting stderr.
  void StopCapture() {
    // Restores the original stream.
    fflush(NULL);
    dup2(uncaptured_fd_, STDERR_FILENO);
    close(uncaptured_fd_);
    uncaptured_fd_ = -1;
  }

  // Returns the name of the temporary file holding the stderr output.
  // GTEST_HAS_DEATH_TEST implies that we have ::std::string, so we
  // can use it here.
  ::std::string filename() const { return filename_; }

 private:
  int uncaptured_fd_;
  ::std::string filename_;
};

static CapturedStderr* g_captured_stderr = NULL;

// Returns the size (in bytes) of a file.
static size_t GetFileSize(FILE * file) {
  fseek(file, 0, SEEK_END);
  return static_cast<size_t>(ftell(file));
}

// Reads the entire content of a file as a string.
// GTEST_HAS_DEATH_TEST implies that we have ::std::string, so we can
// use it here.
static ::std::string ReadEntireFile(FILE * file) {
  const size_t file_size = GetFileSize(file);
  char* const buffer = new char[file_size];

  size_t bytes_last_read = 0;  // # of bytes read in the last fread()
  size_t bytes_read = 0;       // # of bytes read so far

  fseek(file, 0, SEEK_SET);

  // Keeps reading the file until we cannot read further or the
  // pre-determined file size is reached.
  do {
    bytes_last_read = fread(buffer+bytes_read, 1, file_size-bytes_read, file);
    bytes_read += bytes_last_read;
  } while (bytes_last_read > 0 && bytes_read < file_size);

  const ::std::string content(buffer, buffer+bytes_read);
  delete[] buffer;

  return content;
}

// Starts capturing stderr.
void CaptureStderr() {
  if (g_captured_stderr != NULL) {
    GTEST_LOG(FATAL, "Only one stderr capturer can exist at one time.");
  }
  g_captured_stderr = new CapturedStderr;
}

// Stops capturing stderr and returns the captured string.
// GTEST_HAS_DEATH_TEST implies that we have ::std::string, so we can
// use it here.
::std::string GetCapturedStderr() {
  g_captured_stderr->StopCapture();
  FILE* const file = fopen(g_captured_stderr->filename().c_str(), "r");
  const ::std::string content = ReadEntireFile(file);
  fclose(file);

  delete g_captured_stderr;
  g_captured_stderr = NULL;

  return content;
}

// A copy of all command line arguments.  Set by ParseGTestFlags().
::std::vector<String> g_argvs;

// Returns the command line as a vector of strings.
const ::std::vector<String>& GetArgvs() { return g_argvs; }

#endif  // GTEST_HAS_DEATH_TEST

// Returns the name of the environment variable corresponding to the
// given flag.  For example, FlagToEnvVar("foo") will return
// "GTEST_FOO" in the open-source version.
static String FlagToEnvVar(const char* flag) {
  const String full_flag = (Message() << GTEST_FLAG_PREFIX << flag).GetString();

  Message env_var;
  for (int i = 0; i != full_flag.GetLength(); i++) {
    env_var << static_cast<char>(toupper(full_flag.c_str()[i]));
  }

  return env_var.GetString();
}

// Reads and returns the Boolean environment variable corresponding to
// the given flag; if it's not set, returns default_value.
//
// The value is considered true iff it's not "0".
bool BoolFromGTestEnv(const char* flag, bool default_value) {
  const String env_var = FlagToEnvVar(flag);
  const char* const string_value = GetEnv(env_var.c_str());
  return string_value == NULL ?
      default_value : strcmp(string_value, "0") != 0;
}

// Parses 'str' for a 32-bit signed integer.  If successful, writes
// the result to *value and returns true; otherwise leaves *value
// unchanged and returns false.
bool ParseInt32(const Message& src_text, const char* str, Int32* value) {
  // Parses the environment variable as a decimal integer.
  char* end = NULL;
  const long long_value = strtol(str, &end, 10);  // NOLINT

  // Has strtol() consumed all characters in the string?
  if (*end != '\0') {
    // No - an invalid character was encountered.
    Message msg;
    msg << "WARNING: " << src_text
        << " is expected to be a 32-bit integer, but actually"
        << " has value \"" << str << "\".\n";
    printf("%s", msg.GetString().c_str());
    fflush(stdout);
    return false;
  }

  // Is the parsed value in the range of an Int32?
  const Int32 result = static_cast<Int32>(long_value);
  if (long_value == LONG_MAX || long_value == LONG_MIN ||
      // The parsed value overflows as a long.  (strtol() returns
      // LONG_MAX or LONG_MIN when the input overflows.)
      result != long_value
      // The parsed value overflows as an Int32.
      ) {
    Message msg;
    msg << "WARNING: " << src_text
        << " is expected to be a 32-bit integer, but actually"
        << " has value " << str << ", which overflows.\n";
    printf("%s", msg.GetString().c_str());
    fflush(stdout);
    return false;
  }

  *value = result;
  return true;
}

// Reads and returns a 32-bit integer stored in the environment
// variable corresponding to the given flag; if it isn't set or
// doesn't represent a valid 32-bit integer, returns default_value.
Int32 Int32FromGTestEnv(const char* flag, Int32 default_value) {
  const String env_var = FlagToEnvVar(flag);
  const char* const string_value = GetEnv(env_var.c_str());
  if (string_value == NULL) {
    // The environment variable is not set.
    return default_value;
  }

  Int32 result = default_value;
  if (!ParseInt32(Message() << "Environment variable " << env_var,
                  string_value, &result)) {
    printf("The default value %s is used.\n",
           (Message() << default_value).GetString().c_str());
    fflush(stdout);
    return default_value;
  }

  return result;
}

// Reads and returns the string environment variable corresponding to
// the given flag; if it's not set, returns default_value.
const char* StringFromGTestEnv(const char* flag, const char* default_value) {
  const String env_var = FlagToEnvVar(flag);
  const char* const value = GetEnv(env_var.c_str());
  return value == NULL ? default_value : value;
}

}  // namespace internal
}  // namespace testing
