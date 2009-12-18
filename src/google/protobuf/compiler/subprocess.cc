// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <google/protobuf/compiler/subprocess.h>

#include <algorithm>
#include <errno.h>
#include <sys/wait.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/message.h>
#include <google/protobuf/stubs/substitute.h>

namespace google {
namespace protobuf {
namespace compiler {

Subprocess::Subprocess()
    : child_pid_(-1), child_stdin_(-1), child_stdout_(-1) {}

Subprocess::~Subprocess() {
  if (child_stdin_ != -1) {
    close(child_stdin_);
  }
  if (child_stdout_ != -1) {
    close(child_stdout_);
  }
}

void Subprocess::Start(const string& program, SearchMode search_mode) {
  // Note that we assume that there are no other threads, thus we don't have to
  // do crazy stuff like using socket pairs or avoiding libc locks.

  // [0] is read end, [1] is write end.
  int stdin_pipe[2];
  int stdout_pipe[2];

  pipe(stdin_pipe);
  pipe(stdout_pipe);

  char* argv[2] = { strdup(program.c_str()), NULL };

  child_pid_ = fork();
  if (child_pid_ == -1) {
    GOOGLE_LOG(FATAL) << "fork: " << strerror(errno);
  } else if (child_pid_ == 0) {
    // We are the child.
    dup2(stdin_pipe[0], STDIN_FILENO);
    dup2(stdout_pipe[1], STDOUT_FILENO);

    close(stdin_pipe[0]);
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);

    switch (search_mode) {
      case SEARCH_PATH:
        execvp(argv[0], argv);
        break;
      case EXACT_NAME:
        execv(argv[0], argv);
        break;
    }

    // Write directly to STDERR_FILENO to avoid stdio code paths that may do
    // stuff that is unsafe here.
    write(STDERR_FILENO, argv[0], strlen(argv[0]));
    const char* message = ": program not found or is not executable\n";
    write(STDERR_FILENO, message, strlen(message));

    // Must use _exit() rather than exit() to avoid flushing output buffers
    // that will also be flushed by the parent.
    _exit(1);
  } else {
    free(argv[0]);

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    child_stdin_ = stdin_pipe[1];
    child_stdout_ = stdout_pipe[0];
  }
}

bool Subprocess::Communicate(const Message& input, Message* output,
                             string* error) {

  GOOGLE_CHECK_NE(child_stdin_, -1) << "Must call Start() first.";

  // Make sure SIGPIPE is disabled so that if the child dies it doesn't kill us.
  sighandler_t old_pipe_handler = signal(SIGPIPE, SIG_IGN);

  string input_data = input.SerializeAsString();
  string output_data;

  int input_pos = 0;
  int max_fd = max(child_stdin_, child_stdout_);

  while (child_stdout_ != -1) {
    fd_set read_fds;
    fd_set write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    if (child_stdout_ != -1) {
      FD_SET(child_stdout_, &read_fds);
    }
    if (child_stdin_ != -1) {
      FD_SET(child_stdin_, &write_fds);
    }

    if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0) {
      if (errno == EINTR) {
        // Interrupted by signal.  Try again.
        continue;
      } else {
        GOOGLE_LOG(FATAL) << "select: " << strerror(errno);
      }
    }

    if (child_stdin_ != -1 && FD_ISSET(child_stdin_, &write_fds)) {
      int n = write(child_stdin_, input_data.data() + input_pos,
                                  input_data.size() - input_pos);
      if (n < 0) {
        // Child closed pipe.  Presumably it will report an error later.
        // Pretend we're done for now.
        input_pos = input_data.size();
      } else {
        input_pos += n;
      }

      if (input_pos == input_data.size()) {
        // We're done writing.  Close.
        close(child_stdin_);
        child_stdin_ = -1;
      }
    }

    if (child_stdout_ != -1 && FD_ISSET(child_stdout_, &read_fds)) {
      char buffer[4096];
      int n = read(child_stdout_, buffer, sizeof(buffer));

      if (n > 0) {
        output_data.append(buffer, n);
      } else {
        // We're done reading.  Close.
        close(child_stdout_);
        child_stdout_ = -1;
      }
    }
  }

  if (child_stdin_ != -1) {
    // Child did not finish reading input before it closed the output.
    // Presumably it exited with an error.
    close(child_stdin_);
    child_stdin_ = -1;
  }

  int status;
  while (waitpid(child_pid_, &status, 0) == -1) {
    if (errno != EINTR) {
      GOOGLE_LOG(FATAL) << "waitpid: " << strerror(errno);
    }
  }

  // Restore SIGPIPE handling.
  signal(SIGPIPE, old_pipe_handler);

  if (WIFEXITED(status)) {
    if (WEXITSTATUS(status) != 0) {
      int error_code = WEXITSTATUS(status);
      *error = strings::Substitute(
          "Plugin failed with status code $0.", error_code);
      return false;
    }
  } else if (WIFSIGNALED(status)) {
    int signal = WTERMSIG(status);
    *error = strings::Substitute(
        "Plugin killed by signal $0.", signal);
    return false;
  } else {
    *error = "Neither WEXITSTATUS nor WTERMSIG is true?";
    return false;
  }

  if (!output->ParseFromString(output_data)) {
    *error = "Plugin output is unparseable.";
    return false;
  }

  return true;
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
