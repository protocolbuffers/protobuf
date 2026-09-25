// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Hermetic tests for ForkPipeRunner's process management.  The testee is
// /bin/sh running small scripts that stand in for a well-behaved, a stubborn,
// a crashing, a signal-killed and a hung conformance testee.  The pipe protocol
// is a 4-byte little-endian length prefix followed by the payload in each
// direction, so `cat` is a testee that echoes every request back verbatim, and
// a 3-byte request is exactly 7 bytes on the wire.

// TODO: b/418427266 - add Windows coverage; these tests drive the testee with
// /bin/sh.
#ifndef _WIN32

#include "conformance/fork_pipe_runner.h"

#include <sys/types.h>
#include <sys/wait.h>

#include <cerrno>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "conformance/conformance.pb.h"

namespace google {
namespace protobuf {
namespace {

using ::testing::HasSubstr;
using ::testing::Not;

constexpr char kShell[] = "/bin/sh";

// True iff this process has no child processes left, live or zombie.  The
// test process has no children other than the ones ForkPipeRunner spawns.
bool NoChildRemains() {
  return waitpid(-1, nullptr, WNOHANG) == -1 && errno == ECHILD;
}

// A runner for /bin/sh running `script`.  Tests of the SIGKILL fallback and of
// the read-timeout path pass short timeouts so as not to wait out the defaults.
std::unique_ptr<ForkPipeRunner> MakeRunner(const std::string& script,
                                           ForkPipeRunnerOptions options = {}) {
  const std::vector<std::string> args = {"-c", script};
  return std::make_unique<ForkPipeRunner>(kShell, args, options);
}

ForkPipeRunnerOptions WithShutdownGracePeriod(absl::Duration grace_period) {
  ForkPipeRunnerOptions options;
  options.shutdown_grace_period = grace_period;
  return options;
}

ForkPipeRunnerOptions WithReadTimeout(absl::Duration read_timeout) {
  ForkPipeRunnerOptions options;
  options.read_timeout = read_timeout;
  return options;
}

conformance::ConformanceResponse ParseResponse(const std::string& serialized) {
  conformance::ConformanceResponse response;
  EXPECT_TRUE(response.ParseFromString(serialized));
  return response;
}

// Destroys `runner` and returns how long that took.
absl::Duration TimeDestruction(std::unique_ptr<ForkPipeRunner> runner) {
  const absl::Time start = absl::Now();
  runner.reset();
  return absl::Now() - start;
}

TEST(ForkPipeRunnerTest, CooperativeTesteeIsReaped) {
  // `cat` echoes the length-prefixed request straight back, and exits on EOF
  // like a real testee does.
  auto runner = MakeRunner("exec cat");
  EXPECT_EQ(runner->RunTest("t", "abc"), "abc");

  // A cooperative testee must not be made to wait for the grace period.
  EXPECT_LT(TimeDestruction(std::move(runner)), absl::Seconds(5));
  EXPECT_TRUE(NoChildRemains());
}

TEST(ForkPipeRunnerTest, UnusedRunnerSpawnsNothing) {
  // The testee is only spawned by the first RunTest(), so a runner that is
  // never used has nothing to shut down.
  auto runner = MakeRunner("exec cat");
  EXPECT_LT(TimeDestruction(std::move(runner)), absl::Seconds(1));
  EXPECT_TRUE(NoChildRemains());
}

TEST(ForkPipeRunnerTest, EmptyRequestAndResponseRoundTrip) {
  // A zero-length payload is just a zero length prefix in each direction.
  auto runner = MakeRunner("exec cat");
  EXPECT_EQ(runner->RunTest("t", ""), "");
  EXPECT_EQ(runner->RunTest("t", "abc"), "abc");
}

TEST(ForkPipeRunnerTest, PayloadsLargerThanThePipeBufferRoundTrip) {
  // A pipe holds 64 KiB on Linux.  Like a real testee, the script reads the
  // whole request before it writes anything, so neither side deadlocks on a
  // full pipe (`cat` would).  The response is 100000 bytes, announced by its
  // little-endian length prefix, so the runner has to assemble it from
  // several reads.
  constexpr size_t kSize = 100000;
  auto runner = MakeRunner(
      "head -c 100004 >/dev/null; printf '\\240\\206\\001\\000'; "
      "head -c 100000 /dev/zero");
  const std::string response = runner->RunTest("t", std::string(kSize, 'x'));
  EXPECT_EQ(response.size(), kSize);
  EXPECT_EQ(response.find_first_not_of('\0'), std::string::npos);
}

TEST(ForkPipeRunnerTest, PipesAreNotInheritedByAnotherTestee) {
  // The second testee is forked while the first one's pipes are open.  Were
  // it to inherit the write end of the first testee's stdin, the first `cat`
  // would not see EOF when its runner shuts down and would have to be killed
  // after the grace period.
  constexpr absl::Duration kGrace = absl::Seconds(2);
  auto first = MakeRunner("exec cat", WithShutdownGracePeriod(kGrace));
  EXPECT_EQ(first->RunTest("t", "abc"), "abc");
  auto second = MakeRunner("exec cat");
  EXPECT_EQ(second->RunTest("t", "def"), "def");

  EXPECT_LT(TimeDestruction(std::move(first)), kGrace);
  EXPECT_EQ(second->RunTest("t", "ghi"), "ghi");
  second.reset();
  EXPECT_TRUE(NoChildRemains());
}

TEST(ForkPipeRunnerTest, StubbornTesteeIsKilled) {
  // Answers one request, then ignores EOF on stdin and lives forever.
  constexpr absl::Duration kGrace = absl::Milliseconds(100);
  auto runner = MakeRunner("trap '' PIPE; cat; while :; do sleep 1; done",
                           WithShutdownGracePeriod(kGrace));
  EXPECT_EQ(runner->RunTest("t", "abc"), "abc");

  // The destructor waits out the grace period, then SIGKILLs the testee and
  // reaps it promptly.
  const absl::Duration elapsed = TimeDestruction(std::move(runner));
  EXPECT_GE(elapsed, kGrace);
  EXPECT_LT(elapsed, kGrace + absl::Seconds(5));
  EXPECT_TRUE(NoChildRemains());
}

TEST(ForkPipeRunnerTest, CrashedTesteeIsReapedAndRespawned) {
  // Consumes the 7-byte request, then exits with status 3 without answering.
  auto runner = MakeRunner("head -c 7 >/dev/null; exit 3");

  conformance::ConformanceResponse response =
      ParseResponse(runner->RunTest("t", "abc"));
  EXPECT_THAT(response.runtime_error(), HasSubstr("exited with status=3"));
  EXPECT_FALSE(response.has_timeout_error());
  EXPECT_TRUE(NoChildRemains()) << "crash path must reap the testee";

  // The next call respawns the (same) testee, which fails the same way.
  response = ParseResponse(runner->RunTest("t2", "abc"));
  EXPECT_THAT(response.runtime_error(), HasSubstr("exited with status=3"));

  // The crash path already shut the testee down, so the destructor has
  // nothing to wait for.
  EXPECT_LT(TimeDestruction(std::move(runner)), absl::Seconds(1));
  EXPECT_TRUE(NoChildRemains());
}

TEST(ForkPipeRunnerTest, SignaledTesteeIsReported) {
  // Consumes the request (so the runner's write cannot race the death of the
  // reader and raise SIGPIPE here), then kills itself.
  auto runner = MakeRunner("head -c 7 >/dev/null; kill -9 $$");

  conformance::ConformanceResponse response =
      ParseResponse(runner->RunTest("t", "abc"));
  EXPECT_THAT(response.runtime_error(), HasSubstr("killed by signal 9"));
  EXPECT_FALSE(response.has_timeout_error());

  runner.reset();
  EXPECT_TRUE(NoChildRemains());
}

TEST(ForkPipeRunnerTest, HungTesteeTimesOutAndIsKilled) {
  // Consumes the request, then hangs without answering.  It ignores SIGQUIT
  // (the ignore disposition survives the exec) and prints nothing in response
  // to it, which is the case in which the diagnostic read after the timeout
  // must be bounded, and it holds its stdout open so that read never sees EOF.
  constexpr absl::Duration kReadTimeout = absl::Milliseconds(200);
  auto runner =
      MakeRunner("trap '' QUIT; head -c 7 >/dev/null; exec sleep 1000",
                 WithReadTimeout(kReadTimeout));

  const absl::Time start = absl::Now();
  conformance::ConformanceResponse response =
      ParseResponse(runner->RunTest("t", "abc"));
  const absl::Duration elapsed = absl::Now() - start;

  EXPECT_THAT(response.timeout_error(), HasSubstr("child timed out"));
  EXPECT_THAT(response.timeout_error(), HasSubstr("killed by runner"));
  EXPECT_FALSE(response.has_runtime_error());
  EXPECT_GE(elapsed, kReadTimeout);
  // Read timeout, plus the bounded SIGQUIT diagnostic read, plus the grace
  // period before SIGKILL; anything much longer means something blocked.
  EXPECT_LT(elapsed, absl::Seconds(15));
  EXPECT_TRUE(NoChildRemains()) << "timeout path must reap the testee";

  runner.reset();
  EXPECT_TRUE(NoChildRemains());
}

TEST(ForkPipeRunnerTest, HungTesteeKilledBySigquitIsReportedAsSuch) {
  // Hangs like the one above, but with SIGQUIT's default disposition, so the
  // SIGQUIT the runner sends on timeout kills it.  The failure must say so
  // rather than claim the runner killed it: a testee that has failed gets a
  // moment to finish dying so that its own fate is what gets reported.
  constexpr absl::Duration kReadTimeout = absl::Milliseconds(200);
  auto runner = MakeRunner("head -c 7 >/dev/null; exec sleep 1000",
                           WithReadTimeout(kReadTimeout));

  conformance::ConformanceResponse response =
      ParseResponse(runner->RunTest("t", "abc"));
  EXPECT_THAT(response.timeout_error(), HasSubstr("child timed out"));
  EXPECT_THAT(response.timeout_error(), HasSubstr("killed by signal 3"));
  EXPECT_TRUE(NoChildRemains());
}

TEST(ForkPipeRunnerTest, SigquitOutputIsBounded) {
  // Answers SIGQUIT by flooding its stdout forever (a JVM's thread dump is
  // the benign version of this).  The runner logs a bounded amount of it and
  // moves on to shutting the testee down, and closing its end of the pipe is
  // what ends the flood: `yes` dies of SIGPIPE, or of EPIPE if the test
  // process happens to ignore SIGPIPE (which the testee inherits), but either
  // way on its own, before the runner has to kill it.
  constexpr absl::Duration kReadTimeout = absl::Milliseconds(200);
  auto runner = MakeRunner(
      "head -c 7 >/dev/null; trap 'exec yes' QUIT; while :; do sleep 1; done",
      WithReadTimeout(kReadTimeout));

  const absl::Time start = absl::Now();
  conformance::ConformanceResponse response =
      ParseResponse(runner->RunTest("t", "abc"));
  const absl::Duration elapsed = absl::Now() - start;

  EXPECT_THAT(response.timeout_error(), HasSubstr("child timed out"));
  EXPECT_THAT(response.timeout_error(), Not(HasSubstr("killed by runner")));
  // Read timeout, up to a second for the shell to get to the trap, the
  // bounded diagnostic read and the grace period; an unbounded read would
  // never return.
  EXPECT_LT(elapsed, absl::Seconds(10));
  EXPECT_TRUE(NoChildRemains());
}

}  // namespace
}  // namespace protobuf
}  // namespace google

#endif  // !_WIN32
