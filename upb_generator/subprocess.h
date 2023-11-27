// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Shamelessly copied from the protobuf compiler's subprocess.h
// except this version passes strings instead of Messages.

#ifndef THIRD_PARTY_UPB_UPB_GENERATOR_H_
#define THIRD_PARTY_UPB_UPB_GENERATOR_H_

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  // right...
#endif
#include <windows.h>
#else  // _WIN32
#include <sys/types.h>
#include <unistd.h>
#endif  // !_WIN32
#include <string>

namespace upb {
namespace generator {

// Utility class for launching sub-processes.
class Subprocess {
 public:
  Subprocess();
  ~Subprocess();

  enum SearchMode {
    SEARCH_PATH,  // Use PATH environment variable.
    EXACT_NAME    // Program is an exact file name; don't use the PATH.
  };

  // Start the subprocess.  Currently we don't provide a way to specify
  // arguments as protoc plugins don't have any.
  void Start(const std::string& program, SearchMode search_mode);

  // Pipe the input message to the subprocess's stdin, then close the pipe.
  // Meanwhile, read from the subprocess's stdout and copy into *output.
  // All this is done carefully to avoid deadlocks.
  // Returns true if successful.  On any sort of error, returns false and sets
  // *error to a description of the problem.
  bool Communicate(const std::string& input_data, std::string* output_data,
                   std::string* error);

#ifdef _WIN32
  // Given an error code, returns a human-readable error message.  This is
  // defined here so that CommandLineInterface can share it.
  static std::string Win32ErrorMessage(DWORD error_code);
#endif

 private:
#ifdef _WIN32
  DWORD process_start_error_;
  HANDLE child_handle_;

  // The file handles for our end of the child's pipes.  We close each and
  // set it to NULL when no longer needed.
  HANDLE child_stdin_;
  HANDLE child_stdout_;

#else  // _WIN32
  pid_t child_pid_;

  // The file descriptors for our end of the child's pipes.  We close each and
  // set it to -1 when no longer needed.
  int child_stdin_;
  int child_stdout_;

#endif  // !_WIN32
};

}  // namespace generator
}  // namespace upb

#endif  // THIRD_PARTY_UPB_UPB_GENERATOR_H_
