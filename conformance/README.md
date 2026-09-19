Protocol Buffers - Google's data interchange format
===================================================

Copyright 2008 Google LLC.

This directory contains conformance tests for testing completeness and
correctness of Protocol Buffers implementations.  These tests are designed
to be easy to run against any Protocol Buffers implementation.

## How the tests work

The tests themselves are gtest suites written in C++, one suite per format plus
one for the slow tests:

*   `binary` (`binary_*_test.cc`): wire-format parsing and serialization,
*   `json` (`json_*_test.cc`): the ProtoJSON mapping,
*   `text` (`text_*_test.cc`): text format,
*   `performance` (`*_performance_test.cc`, `binary_recursion_limit_test.cc`):
    the tests that stress the testee with deeply nested or very large payloads
    (binary and text format; JSON has none); kept apart because they are slow
    and their expected failures are unrelated to a format's.

Each test builds a `ConformanceRequest` (see
[conformance.proto](https://github.com/protocolbuffers/protobuf/blob/main/conformance/conformance.proto)),
sends it to the implementation under test (the *testee*) and checks the
`ConformanceResponse`. The testee is normally a separate program written in the
language being tested, which reads requests from stdin and writes responses to
stdout; a C++ implementation can instead be linked into the test binary (see
"In-process testees" below).

Test names look like `Required.Proto3.TextFormatInput.StringFieldBadUTF8Octal`:
`<Level>` names the test's priority (see "Priorities" below): `Required` tests
must pass for an implementation to be conformant; failures of `Recommended`
tests fail the run only under `--enforce_recommended`, which
`conformance_test()` passes by default. The second component names the edition
or syntax of the test message; tests newer than `--maximum_edition` (default:
proto2 and proto3) are skipped.

Known failures are kept in *failure lists*: text files with one test name per
line, optionally followed by `#` and the failure message the test currently
produces, e.g.

```
Recommended.Proto3.JsonInput.FieldNameDuplicate  # Should have failed to parse, but didn't.
```

Any dot-separated component of a name may be `*`, matching every test with the
other components (a trailing `.*` thus covers a test's `.ProtobufOutput` and
`.JsonOutput` variants at once); a wildcard entry that covers more than about
twenty failures fails the run (`kMaximumWildcardExpansions` in
`test_manager.cc`). A listed test that fails with the listed message (the
message is compared as a prefix of the actual one, and an entry without a
message accepts any failure) is an expected failure; a listed test that passes,
an unlisted test that fails, a listed test that fails with a different message,
and a listed test that the testee skips all fail the run, so a failure list
never goes stale silently. A wildcard matches skipped tests too: a testee that
skips a whole syntax (say, proto2) must list its failures per syntax
(`Required.Proto3.X`, `Required.Editions_Proto3.X`) rather than as
`Required.*.X`.

## Running the tests with Bazel

`conformance.bzl` defines the `conformance_test()` macro, which runs every suite
against one testee. In the package that holds the testee:

```
load("//conformance:conformance.bzl", "conformance_test", "failure_lists")

failure_lists(name = "failure_lists")

conformance_test(
    name = "cpp",
    testee = ":conformance_cpp",
    failure_lists = ":failure_lists",
    maximum_edition = "2023",
)
```

This creates one `cc_test` per suite (`:cpp_binary_test`, `:cpp_json_test`,
`:cpp_text_test`, `:cpp_performance_test`) and a `:cpp` test suite running all
of them. Each test reads its failure list from
`failure_lists/<name>_<suite>.txt` in the same package
(`failure_lists/cpp_binary.txt`, ...); if the file doesn't exist the testee is
expected to pass every test of that suite. `suites` restricts the suites to run
(e.g. for a runtime without text format, or to leave the performance suite out),
and `enforce_recommended = False` tolerates failures of `Recommended` tests.

To update a failure list after a change, run the test with `--fix`, which
rewrites (or creates) the file to match the observed results, keeping comments
and any wildcard that still matches:

```
$ bazel run //conformance:cpp_binary_test -- --fix
```

(It has to be `bazel run`: under `bazel test` the file would be written inside
the sandbox. `--fix` is refused, and the run fails without rewriting the list,
when only some of the tests ran, e.g. with `--gtest_filter`.)

The tests are ordinary gtest binaries, so the usual flags apply, for example:

```
$ bazel test //conformance:cpp_json_test \
    --test_arg=--gtest_filter='*FieldName*' --test_output=errors
$ bazel run //conformance:cpp_json_test -- --gtest_list_tests
```

Their own flags (`--failure_list`, `--maximum_edition`, `--enforce_recommended`,
`--fix`, `--fix_output_file`, and `--testee_binary` / `--testee_args` for a
forked testee) are documented in `test_environment_flags.cc`;
`conformance_test()` sets them from its attributes.

### In-process testees

For a C++ implementation the testee can be a `cc_library` that defines
`MakeTesteeRunner()` (see `testee_runner.h`) and is linked into each test with
`conformance_test(in_process = True, ...)`. The suites then call the testee
directly, in the test's own process: no fork or pipe, and a crash or a log
message of the testee shows up in the test itself. `//conformance:cpp` runs the
C++ runtime this way, on top of `conformance_cpp_harness.h`, which
`conformance_cpp` (the stdin/stdout testee binary) shares.

## The conformance_test_runner command line

`conformance_test_runner` is a single binary that contains all four suites
(`--performance` runs the performance suite alone, its absence the other three)
and runs them against a testee program given on its command line, printing one
report per failure list with the `update_failure_list` commands that fix it. It
takes failure lists in the same format as the Bazel tests (`--failure_list` for
the binary and JSON suites, `--text_format_failure_list` for the text suite) and
`--help` lists its options. The languages that aren't built with Bazel, and the
`conformance_test()` macro in `defs.bzl` that the per-language `BUILD.bazel`
files still use, run the tests through it.

If you're not using Bazel, build it with CMake from the base directory:

    $ cmake . -Dprotobuf_BUILD_CONFORMANCE=ON && cmake --build .

Running the tests for C++
-------------------------

To run the tests against the C++ implementation, run:

```
$ bazel test //conformance:cpp
```

or, through `conformance_test_runner`:

    $ bazel test //src:conformance_test

Or alternatively with CMake:

    $ ctest -R conformance_cpp_test

Running the tests for other languages
-------------------------------------

All of the languages in the Protobuf source tree are set up to run conformance
tests using similar patterns.  You can either use Bazel to run the
`conformance_test` target defined in the language's root `BUILD.bazel` file,
or create an executable for a custom test and pass it to
`conformance_test_runner`.

Note: CMake can be used to build the conformance test runner, but not any of
the conformance test executables outside C++.  So if you aren't using Bazel
you'll need to create the executable you pass to `conformance_test_runner` via
some alternate build system.

While we plan to model all our supported languages more completely in Bazel,
today some of them are a bit tricky to run.  Below is a list of the commands
(and prerequisites) to run each language's conformance tests.

C#:

```
$ which dotnet || echo "You must have dotnet installed!"
$ bazel test //csharp:conformance_test \
    --action_env=DOTNET_CLI_TELEMETRY_OPTOUT=1 --test_env=DOTNET_CLI_HOME=~ \
    --action_env=DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
```

Java:

    $ bazel test //java/core:conformance_test //java/lite:conformance_test

Objective-C (Mac only):

```
$ bazel test //objectivec:conformance_test --macos_minimum_os=12.0
```

PHP:

```
$ bazel test //php:conformance_test
```

PHP (C):

```
$ which gcc     || echo "gcc is required!"
$ which libtool || echo "libtool is required!"
$ which make    || echo "make is required!"
$ which pear    || echo "pear is required! It might require a development version of PHP such as a php-dev package"
$ which pecl    || echo "pecl is required! It might require a development version of PHP such as a php-dev package"
$ which phpize  || echo "phpize is required! It might require a development version of PHP such as a php-dev package"
$ bazel test //php:conformance_test_c
```

Python:

```
$ bazel test //python:conformance_test
```

Python (C++):

```
$ bazel test //python:conformance_test_cpp --define=use_fast_cpp_protos=true
```

Ruby:

    $ [[ $(ruby --version) == "ruby"* ]] || echo "Select a C Ruby!"
    $ bazel test //ruby:conformance_test --define=ruby_platform=c \
        --action_env=PATH --action_env=GEM_PATH --action_env=GEM_HOME

Ruby (JRuby):

    $ [[ $(ruby --version) == "jruby"* ]] || echo "Switch to Java Ruby!"
    $ bazel test //ruby:conformance_test_jruby --define=ruby_platform=java \
        --action_env=PATH --action_env=GEM_PATH --action_env=GEM_HOME

Testing other Protocol Buffer implementations
---------------------------------------------

To run these tests against a new Protocol Buffers implementation, write a
program in your language that uses the protobuf implementation you want
to test.  This program should implement the testing protocol defined in
[conformance.proto](https://github.com/protocolbuffers/protobuf/blob/main/conformance/conformance.proto).
This is designed to be as easy as possible: the C++ version is only
150 lines and is a good example for what this program should look like
(see [conformance_cpp.cc](https://github.com/protocolbuffers/protobuf/blob/main/conformance/conformance_cpp.cc)).
The program only needs to be able to read from stdin and write to stdout.

Then either declare a `conformance_test()` for it as shown above (create the
`failure_lists/` directory, then run each test with `--fix` to write its first
list), or pass it to `conformance_test_runner`.

Portability
-----------

Note that the test runner currently does not work on Windows.  Patches
to fix this are welcome!  (But please get in touch first to settle on
a general implementation strategy).
