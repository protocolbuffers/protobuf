// Copyright 2005, Google Inc.
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
//
// This file implements death tests.

#include <gtest/gtest-death-test.h>
#include <gtest/internal/gtest-port.h>

#include <errno.h>
#include <limits.h>
#include <stdarg.h>

#include <gtest/gtest-message.h>
#include <gtest/internal/gtest-string.h>

// Indicates that this translation unit is part of Google Test's
// implementation.  It must come before gtest-internal-inl.h is
// included, or there will be a compiler error.  This trick is to
// prevent a user from accidentally including gtest-internal-inl.h in
// his code.
#define GTEST_IMPLEMENTATION
#include "gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION

namespace testing {

// Constants.

// The default death test style.
static const char kDefaultDeathTestStyle[] = "fast";

GTEST_DEFINE_string(
    death_test_style,
    internal::StringFromGTestEnv("death_test_style", kDefaultDeathTestStyle),
    "Indicates how to run a death test in a forked child process: "
    "\"threadsafe\" (child process re-executes the test binary "
    "from the beginning, running only the specific death test) or "
    "\"fast\" (child process runs the death test immediately "
    "after forking).");

namespace internal {
GTEST_DEFINE_string(
    internal_run_death_test, "",
    "Indicates the file, line number, temporal index of "
    "the single death test to run, and a file descriptor to "
    "which a success code may be sent, all separated by "
    "colons.  This flag is specified if and only if the current "
    "process is a sub-process launched for running a thread-safe "
    "death test.  FOR INTERNAL USE ONLY.");
}  // namespace internal

#ifdef GTEST_HAS_DEATH_TEST

// ExitedWithCode constructor.
ExitedWithCode::ExitedWithCode(int exit_code) : exit_code_(exit_code) {
}

// ExitedWithCode function-call operator.
bool ExitedWithCode::operator()(int exit_status) const {
  return WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == exit_code_;
}

// KilledBySignal constructor.
KilledBySignal::KilledBySignal(int signum) : signum_(signum) {
}

// KilledBySignal function-call operator.
bool KilledBySignal::operator()(int exit_status) const {
  return WIFSIGNALED(exit_status) && WTERMSIG(exit_status) == signum_;
}

namespace internal {

// Utilities needed for death tests.

// Generates a textual description of a given exit code, in the format
// specified by wait(2).
static String ExitSummary(int exit_code) {
  Message m;
  if (WIFEXITED(exit_code)) {
    m << "Exited with exit status " << WEXITSTATUS(exit_code);
  } else if (WIFSIGNALED(exit_code)) {
    m << "Terminated by signal " << WTERMSIG(exit_code);
  }
#ifdef WCOREDUMP
  if (WCOREDUMP(exit_code)) {
    m << " (core dumped)";
  }
#endif
  return m.GetString();
}

// Returns true if exit_status describes a process that was terminated
// by a signal, or exited normally with a nonzero exit code.
bool ExitedUnsuccessfully(int exit_status) {
  return !ExitedWithCode(0)(exit_status);
}

// Generates a textual failure message when a death test finds more than
// one thread running, or cannot determine the number of threads, prior
// to executing the given statement.  It is the responsibility of the
// caller not to pass a thread_count of 1.
static String DeathTestThreadWarning(size_t thread_count) {
  Message msg;
  msg << "Death tests use fork(), which is unsafe particularly"
      << " in a threaded context. For this test, " << GTEST_NAME << " ";
  if (thread_count == 0)
    msg << "couldn't detect the number of threads.";
  else
    msg << "detected " << thread_count << " threads.";
  return msg.GetString();
}

// Static string containing a description of the outcome of the
// last death test.
static String last_death_test_message;

// Flag characters for reporting a death test that did not die.
static const char kDeathTestLived = 'L';
static const char kDeathTestReturned = 'R';
static const char kDeathTestInternalError = 'I';

// An enumeration describing all of the possible ways that a death test
// can conclude.  DIED means that the process died while executing the
// test code; LIVED means that process lived beyond the end of the test
// code; and RETURNED means that the test statement attempted a "return,"
// which is not allowed.  IN_PROGRESS means the test has not yet
// concluded.
enum DeathTestOutcome { IN_PROGRESS, DIED, LIVED, RETURNED };

// Routine for aborting the program which is safe to call from an
// exec-style death test child process, in which case the the error
// message is propagated back to the parent process.  Otherwise, the
// message is simply printed to stderr.  In either case, the program
// then exits with status 1.
void DeathTestAbort(const char* format, ...) {
  // This function may be called from a threadsafe-style death test
  // child process, which operates on a very small stack.  Use the
  // heap for any additional non-miniscule memory requirements.
  const InternalRunDeathTestFlag* const flag =
      GetUnitTestImpl()->internal_run_death_test_flag();
  va_list args;
  va_start(args, format);

  if (flag != NULL) {
    FILE* parent = fdopen(flag->status_fd, "w");
    fputc(kDeathTestInternalError, parent);
    vfprintf(parent, format, args);
    fclose(parent);
    va_end(args);
    _exit(1);
  } else {
    vfprintf(stderr, format, args);
    va_end(args);
    abort();
  }
}

// A replacement for CHECK that calls DeathTestAbort if the assertion
// fails.
#define GTEST_DEATH_TEST_CHECK(expression) \
  do { \
    if (!(expression)) { \
      DeathTestAbort("CHECK failed: File %s, line %d: %s", \
                     __FILE__, __LINE__, #expression); \
    } \
  } while (0)

// This macro is similar to GTEST_DEATH_TEST_CHECK, but it is meant for
// evaluating any system call that fulfills two conditions: it must return
// -1 on failure, and set errno to EINTR when it is interrupted and
// should be tried again.  The macro expands to a loop that repeatedly
// evaluates the expression as long as it evaluates to -1 and sets
// errno to EINTR.  If the expression evaluates to -1 but errno is
// something other than EINTR, DeathTestAbort is called.
#define GTEST_DEATH_TEST_CHECK_SYSCALL(expression) \
  do { \
    int retval; \
    do { \
      retval = (expression); \
    } while (retval == -1 && errno == EINTR); \
    if (retval == -1) { \
      DeathTestAbort("CHECK failed: File %s, line %d: %s != -1", \
                     __FILE__, __LINE__, #expression); \
    } \
  } while (0)

// Death test constructor.  Increments the running death test count
// for the current test.
DeathTest::DeathTest() {
  TestInfo* const info = GetUnitTestImpl()->current_test_info();
  if (info == NULL) {
    DeathTestAbort("Cannot run a death test outside of a TEST or "
                   "TEST_F construct");
  }
}

// Creates and returns a death test by dispatching to the current
// death test factory.
bool DeathTest::Create(const char* statement, const RE* regex,
                       const char* file, int line, DeathTest** test) {
  return GetUnitTestImpl()->death_test_factory()->Create(
      statement, regex, file, line, test);
}

const char* DeathTest::LastMessage() {
  return last_death_test_message.c_str();
}

// ForkingDeathTest provides implementations for most of the abstract
// methods of the DeathTest interface.  Only the AssumeRole method is
// left undefined.
class ForkingDeathTest : public DeathTest {
 public:
  ForkingDeathTest(const char* statement, const RE* regex);

  // All of these virtual functions are inherited from DeathTest.
  virtual int Wait();
  virtual bool Passed(bool status_ok);
  virtual void Abort(AbortReason reason);

 protected:
  void set_forked(bool forked) { forked_ = forked; }
  void set_child_pid(pid_t child_pid) { child_pid_ = child_pid; }
  void set_read_fd(int fd) { read_fd_ = fd; }
  void set_write_fd(int fd) { write_fd_ = fd; }

 private:
  // The textual content of the code this object is testing.
  const char* const statement_;
  // The regular expression which test output must match.
  const RE* const regex_;
  // True if the death test successfully forked.
  bool forked_;
  // PID of child process during death test; 0 in the child process itself.
  pid_t child_pid_;
  // File descriptors for communicating the death test's status byte.
  int read_fd_;   // Always -1 in the child process.
  int write_fd_;  // Always -1 in the parent process.
  // The exit status of the child process.
  int status_;
  // How the death test concluded.
  DeathTestOutcome outcome_;
};

// Constructs a ForkingDeathTest.
ForkingDeathTest::ForkingDeathTest(const char* statement, const RE* regex)
    : DeathTest(),
      statement_(statement),
      regex_(regex),
      forked_(false),
      child_pid_(-1),
      read_fd_(-1),
      write_fd_(-1),
      status_(-1),
      outcome_(IN_PROGRESS) {
}

// Reads an internal failure message from a file descriptor, then calls
// LOG(FATAL) with that message.  Called from a death test parent process
// to read a failure message from the death test child process.
static void FailFromInternalError(int fd) {
  Message error;
  char buffer[256];
  ssize_t num_read;

  do {
    while ((num_read = read(fd, buffer, 255)) > 0) {
      buffer[num_read] = '\0';
      error << buffer;
    }
  } while (num_read == -1 && errno == EINTR);

  // TODO(smcafee):  Maybe just FAIL the test instead?
  if (num_read == 0) {
    GTEST_LOG(FATAL, error);
  } else {
    GTEST_LOG(FATAL,
              Message() << "Error while reading death test internal: "
              << strerror(errno) << " [" << errno << "]");
  }
}

// Waits for the child in a death test to exit, returning its exit
// status, or 0 if no child process exists.  As a side effect, sets the
// outcome data member.
int ForkingDeathTest::Wait() {
  if (!forked_)
    return 0;

  // The read() here blocks until data is available (signifying the
  // failure of the death test) or until the pipe is closed (signifying
  // its success), so it's okay to call this in the parent before
  // the child process has exited.
  char flag;
  ssize_t bytes_read;

  do {
    bytes_read = read(read_fd_, &flag, 1);
  } while (bytes_read == -1 && errno == EINTR);

  if (bytes_read == 0) {
    outcome_ = DIED;
  } else if (bytes_read == 1) {
    switch (flag) {
      case kDeathTestReturned:
        outcome_ = RETURNED;
        break;
      case kDeathTestLived:
        outcome_ = LIVED;
        break;
      case kDeathTestInternalError:
        FailFromInternalError(read_fd_);  // Does not return.
        break;
      default:
        GTEST_LOG(FATAL,
                  Message() << "Death test child process reported unexpected "
                  << "status byte (" << static_cast<unsigned int>(flag)
                  << ")");
    }
  } else {
    GTEST_LOG(FATAL,
              Message() << "Read from death test child process failed: "
              << strerror(errno));
  }

  GTEST_DEATH_TEST_CHECK_SYSCALL(close(read_fd_));
  GTEST_DEATH_TEST_CHECK_SYSCALL(waitpid(child_pid_, &status_, 0));
  return status_;
}

// Assesses the success or failure of a death test, using both private
// members which have previously been set, and one argument:
//
// Private data members:
//   outcome:  an enumeration describing how the death test
//             concluded: DIED, LIVED, or RETURNED.  The death test fails
//             in the latter two cases
//   status:   the exit status of the child process, in the format
//             specified by wait(2)
//   regex:    a regular expression object to be applied to
//             the test's captured standard error output; the death test
//             fails if it does not match
//
// Argument:
//   status_ok: true if exit_status is acceptable in the context of
//              this particular death test, which fails if it is false
//
// Returns true iff all of the above conditions are met.  Otherwise, the
// first failing condition, in the order given above, is the one that is
// reported. Also sets the static variable last_death_test_message.
bool ForkingDeathTest::Passed(bool status_ok) {
  if (!forked_)
    return false;

#if GTEST_HAS_GLOBAL_STRING
  const ::string error_message = GetCapturedStderr();
#else
  const ::std::string error_message = GetCapturedStderr();
#endif  // GTEST_HAS_GLOBAL_STRING

  bool success = false;
  Message buffer;

  buffer << "Death test: " << statement_ << "\n";
  switch (outcome_) {
    case LIVED:
      buffer << "    Result: failed to die.\n"
             << " Error msg: " << error_message;
      break;
    case RETURNED:
      buffer << "    Result: illegal return in test statement.\n"
             << " Error msg: " << error_message;
      break;
    case DIED:
      if (status_ok) {
        if (RE::PartialMatch(error_message, *regex_)) {
          success = true;
        } else {
          buffer << "    Result: died but not with expected error.\n"
                 << "  Expected: " << regex_->pattern() << "\n"
                 << "Actual msg: " << error_message;
        }
      } else {
        buffer << "    Result: died but not with expected exit code:\n"
               << "            " << ExitSummary(status_) << "\n";
      }
      break;
    default:
      GTEST_LOG(FATAL,
                "DeathTest::Passed somehow called before conclusion of test");
  }

  last_death_test_message = buffer.GetString();
  return success;
}

// Signals that the death test code which should have exited, didn't.
// Should be called only in a death test child process.
// Writes a status byte to the child's status file desriptor, then
// calls _exit(1).
void ForkingDeathTest::Abort(AbortReason reason) {
  // The parent process considers the death test to be a failure if
  // it finds any data in our pipe.  So, here we write a single flag byte
  // to the pipe, then exit.
  const char flag =
      reason == TEST_DID_NOT_DIE ? kDeathTestLived : kDeathTestReturned;
  GTEST_DEATH_TEST_CHECK_SYSCALL(write(write_fd_, &flag, 1));
  GTEST_DEATH_TEST_CHECK_SYSCALL(close(write_fd_));
  _exit(1);  // Exits w/o any normal exit hooks (we were supposed to crash)
}

// A concrete death test class that forks, then immediately runs the test
// in the child process.
class NoExecDeathTest : public ForkingDeathTest {
 public:
  NoExecDeathTest(const char* statement, const RE* regex) :
      ForkingDeathTest(statement, regex) { }
  virtual TestRole AssumeRole();
};

// The AssumeRole process for a fork-and-run death test.  It implements a
// straightforward fork, with a simple pipe to transmit the status byte.
DeathTest::TestRole NoExecDeathTest::AssumeRole() {
  const size_t thread_count = GetThreadCount();
  if (thread_count != 1) {
    GTEST_LOG(WARNING, DeathTestThreadWarning(thread_count));
  }

  int pipe_fd[2];
  GTEST_DEATH_TEST_CHECK(pipe(pipe_fd) != -1);

  last_death_test_message = "";
  CaptureStderr();
  // When we fork the process below, the log file buffers are copied, but the
  // file descriptors are shared.  We flush all log files here so that closing
  // the file descriptors in the child process doesn't throw off the
  // synchronization between descriptors and buffers in the parent process.
  // This is as close to the fork as possible to avoid a race condition in case
  // there are multiple threads running before the death test, and another
  // thread writes to the log file.
  FlushInfoLog();

  const pid_t child_pid = fork();
  GTEST_DEATH_TEST_CHECK(child_pid != -1);
  set_child_pid(child_pid);
  if (child_pid == 0) {
    GTEST_DEATH_TEST_CHECK_SYSCALL(close(pipe_fd[0]));
    set_write_fd(pipe_fd[1]);
    // Redirects all logging to stderr in the child process to prevent
    // concurrent writes to the log files.  We capture stderr in the parent
    // process and append the child process' output to a log.
    LogToStderr();
    return EXECUTE_TEST;
  } else {
    GTEST_DEATH_TEST_CHECK_SYSCALL(close(pipe_fd[1]));
    set_read_fd(pipe_fd[0]);
    set_forked(true);
    return OVERSEE_TEST;
  }
}

// A concrete death test class that forks and re-executes the main
// program from the beginning, with command-line flags set that cause
// only this specific death test to be run.
class ExecDeathTest : public ForkingDeathTest {
 public:
  ExecDeathTest(const char* statement, const RE* regex,
                const char* file, int line) :
      ForkingDeathTest(statement, regex), file_(file), line_(line) { }
  virtual TestRole AssumeRole();
 private:
  // The name of the file in which the death test is located.
  const char* const file_;
  // The line number on which the death test is located.
  const int line_;
};

// Utility class for accumulating command-line arguments.
class Arguments {
 public:
  Arguments() {
    args_.push_back(NULL);
  }
  ~Arguments() {
    for (std::vector<char*>::iterator i = args_.begin();
         i + 1 != args_.end();
         ++i) {
      free(*i);
    }
  }
  void AddArgument(const char* argument) {
    args_.insert(args_.end() - 1, strdup(argument));
  }

  template <typename Str>
  void AddArguments(const ::std::vector<Str>& arguments) {
    for (typename ::std::vector<Str>::const_iterator i = arguments.begin();
         i != arguments.end();
         ++i) {
      args_.insert(args_.end() - 1, strdup(i->c_str()));
    }
  }
  char* const* Argv() {
    return &args_[0];
  }
 private:
  std::vector<char*> args_;
};

// A struct that encompasses the arguments to the child process of a
// threadsafe-style death test process.
struct ExecDeathTestArgs {
  char* const* argv;  // Command-line arguments for the child's call to exec
  int close_fd;       // File descriptor to close; the read end of a pipe
};

// The main function for a threadsafe-style death test child process.
static int ExecDeathTestChildMain(void* child_arg) {
  ExecDeathTestArgs* const args = static_cast<ExecDeathTestArgs*>(child_arg);
  GTEST_DEATH_TEST_CHECK_SYSCALL(close(args->close_fd));
  execve(args->argv[0], args->argv, environ);
  DeathTestAbort("execve failed: %s", strerror(errno));
  return EXIT_FAILURE;
}

// Two utility routines that together determine the direction the stack
// grows.
// This could be accomplished more elegantly by a single recursive
// function, but we want to guard against the unlikely possibility of
// a smart compiler optimizing the recursion away.
static bool StackLowerThanAddress(const void* ptr) {
  int dummy;
  return &dummy < ptr;
}

static bool StackGrowsDown() {
  int dummy;
  return StackLowerThanAddress(&dummy);
}

// A threadsafe implementation of fork(2) for threadsafe-style death tests
// that uses clone(2).  It dies with an error message if anything goes
// wrong.
static pid_t ExecDeathTestFork(char* const* argv, int close_fd) {
  static const bool stack_grows_down = StackGrowsDown();
  const size_t stack_size = getpagesize();
  void* const stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
                           MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  GTEST_DEATH_TEST_CHECK(stack != MAP_FAILED);
  void* const stack_top =
      static_cast<char*>(stack) + (stack_grows_down ? stack_size : 0);
  ExecDeathTestArgs args = { argv, close_fd };
  const pid_t child_pid = clone(&ExecDeathTestChildMain, stack_top,
                                SIGCHLD, &args);
  GTEST_DEATH_TEST_CHECK(child_pid != -1);
  GTEST_DEATH_TEST_CHECK(munmap(stack, stack_size) != -1);
  return child_pid;
}

// The AssumeRole process for a fork-and-exec death test.  It re-executes the
// main program from the beginning, setting the --gtest_filter
// and --gtest_internal_run_death_test flags to cause only the current
// death test to be re-run.
DeathTest::TestRole ExecDeathTest::AssumeRole() {
  const UnitTestImpl* const impl = GetUnitTestImpl();
  const InternalRunDeathTestFlag* const flag =
      impl->internal_run_death_test_flag();
  const TestInfo* const info = impl->current_test_info();
  const int death_test_index = info->result()->death_test_count();

  if (flag != NULL) {
    set_write_fd(flag->status_fd);
    return EXECUTE_TEST;
  }

  int pipe_fd[2];
  GTEST_DEATH_TEST_CHECK(pipe(pipe_fd) != -1);
  // Clear the close-on-exec flag on the write end of the pipe, lest
  // it be closed when the child process does an exec:
  GTEST_DEATH_TEST_CHECK(fcntl(pipe_fd[1], F_SETFD, 0) != -1);

  const String filter_flag =
      String::Format("--%s%s=%s.%s",
                     GTEST_FLAG_PREFIX, kFilterFlag,
                     info->test_case_name(), info->name());
  const String internal_flag =
      String::Format("--%s%s=%s:%d:%d:%d",
                     GTEST_FLAG_PREFIX, kInternalRunDeathTestFlag, file_, line_,
                     death_test_index, pipe_fd[1]);
  Arguments args;
  args.AddArguments(GetArgvs());
  args.AddArgument("--logtostderr");
  args.AddArgument(filter_flag.c_str());
  args.AddArgument(internal_flag.c_str());

  last_death_test_message = "";

  CaptureStderr();
  // See the comment in NoExecDeathTest::AssumeRole for why the next line
  // is necessary.
  FlushInfoLog();

  const pid_t child_pid = ExecDeathTestFork(args.Argv(), pipe_fd[0]);
  GTEST_DEATH_TEST_CHECK_SYSCALL(close(pipe_fd[1]));
  set_child_pid(child_pid);
  set_read_fd(pipe_fd[0]);
  set_forked(true);
  return OVERSEE_TEST;
}

// Creates a concrete DeathTest-derived class that depends on the
// --gtest_death_test_style flag, and sets the pointer pointed to
// by the "test" argument to its address.  If the test should be
// skipped, sets that pointer to NULL.  Returns true, unless the
// flag is set to an invalid value.
bool DefaultDeathTestFactory::Create(const char* statement, const RE* regex,
                                     const char* file, int line,
                                     DeathTest** test) {
  UnitTestImpl* const impl = GetUnitTestImpl();
  const InternalRunDeathTestFlag* const flag =
      impl->internal_run_death_test_flag();
  const int death_test_index = impl->current_test_info()
      ->increment_death_test_count();

  if (flag != NULL) {
    if (death_test_index > flag->index) {
      last_death_test_message = String::Format(
          "Death test count (%d) somehow exceeded expected maximum (%d)",
          death_test_index, flag->index);
      return false;
    }

    if (!(flag->file == file && flag->line == line &&
          flag->index == death_test_index)) {
      *test = NULL;
      return true;
    }
  }

  if (GTEST_FLAG(death_test_style) == "threadsafe") {
    *test = new ExecDeathTest(statement, regex, file, line);
  } else if (GTEST_FLAG(death_test_style) == "fast") {
    *test = new NoExecDeathTest(statement, regex);
  } else {
    last_death_test_message = String::Format(
        "Unknown death test style \"%s\" encountered",
        GTEST_FLAG(death_test_style).c_str());
    return false;
  }

  return true;
}

// Splits a given string on a given delimiter, populating a given
// vector with the fields.  GTEST_HAS_DEATH_TEST implies that we have
// ::std::string, so we can use it here.
static void SplitString(const ::std::string& str, char delimiter,
                        ::std::vector< ::std::string>* dest) {
  ::std::vector< ::std::string> parsed;
  ::std::string::size_type pos = 0;
  while (true) {
    const ::std::string::size_type colon = str.find(':', pos);
    if (colon == ::std::string::npos) {
      parsed.push_back(str.substr(pos));
      break;
    } else {
      parsed.push_back(str.substr(pos, colon - pos));
      pos = colon + 1;
    }
  }
  dest->swap(parsed);
}

// Attempts to parse a string into a positive integer.  Returns true
// if that is possible.  GTEST_HAS_DEATH_TEST implies that we have
// ::std::string, so we can use it here.
static bool ParsePositiveInt(const ::std::string& str, int* number) {
  // Fail fast if the given string does not begin with a digit;
  // this bypasses strtol's "optional leading whitespace and plus
  // or minus sign" semantics, which are undesirable here.
  if (str.empty() || !isdigit(str[0])) {
    return false;
  }
  char* endptr;
  const long parsed = strtol(str.c_str(), &endptr, 10);  // NOLINT
  if (*endptr == '\0' && parsed <= INT_MAX) {
    *number = static_cast<int>(parsed);
    return true;
  } else {
    return false;
  }
}

// Returns a newly created InternalRunDeathTestFlag object with fields
// initialized from the GTEST_FLAG(internal_run_death_test) flag if
// the flag is specified; otherwise returns NULL.
InternalRunDeathTestFlag* ParseInternalRunDeathTestFlag() {
  if (GTEST_FLAG(internal_run_death_test) == "") return NULL;

  InternalRunDeathTestFlag* const internal_run_death_test_flag =
      new InternalRunDeathTestFlag;
  // GTEST_HAS_DEATH_TEST implies that we have ::std::string, so we
  // can use it here.
  ::std::vector< ::std::string> fields;
  SplitString(GTEST_FLAG(internal_run_death_test).c_str(), ':', &fields);
  if (fields.size() != 4
      || !ParsePositiveInt(fields[1], &internal_run_death_test_flag->line)
      || !ParsePositiveInt(fields[2], &internal_run_death_test_flag->index)
      || !ParsePositiveInt(fields[3],
                           &internal_run_death_test_flag->status_fd)) {
    DeathTestAbort("Bad --gtest_internal_run_death_test flag: %s",
                   GTEST_FLAG(internal_run_death_test).c_str());
  }
  internal_run_death_test_flag->file = fields[0].c_str();
  return internal_run_death_test_flag;
}

}  // namespace internal

#endif  // GTEST_HAS_DEATH_TEST

}  // namespace testing
