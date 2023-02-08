Protocol Buffers - Google's data interchange format
===================================================

Copyright 2008 Google Inc.

This directory contains conformance tests for testing completeness and
correctness of Protocol Buffers implementations.  These tests are designed
to be easy to run against any Protocol Buffers implementation.

This directory contains the tester process `conformance-test`, which
contains all of the tests themselves.  Then separate programs written
in whatever language you want to test communicate with the tester
program over a pipe.

If you're not using Bazel to run these tests, make sure you build the C++
tester code beforehand, e.g. from the base directory:

    $ cmake . -Dprotobuf_BUILD_CONFORMANCE=ON && cmake --build .

This will produce a `conformance_test_runner` binary that can be used to run
conformance tests on any executable.  Pass it `--help` for more information.

Running the tests for C++
-------------------------

To run the tests against the C++ implementation, run:

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

Java:

    $ bazel test //java/core:conformance_test //java/lite:conformance_test

Python:

    $ bazel test //python:conformance_test

Python C++:

    $ bazel test //python:conformance_test_cpp --define=use_fast_cpp_protos=true

C#:

    $ `which dotnet || echo "You must have dotnet installed!"
    $ `bazel test //csharp:conformance_test \
        --action_env=DOTNET_CLI_TELEMETRY_OPTOUT=1 --test_env=DOTNET_CLI_HOME=~ \
        --action_env=DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1

Objective-C (Mac only):

    $ `bazel test //objectivec:conformance_test --macos_minimum_os=10.9

Ruby:

    $ [[ $(ruby --version) == "ruby"* ]] || echo "Select a C Ruby!"
    $ bazel test //ruby:conformance_test --define=ruby_platform=c \
        --action_env=PATH --action_env=GEM_PATH --action_env=GEM_HOME

JRuby:

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

Portability
-----------

Note that the test runner currently does not work on Windows.  Patches
to fix this are welcome!  (But please get in touch first to settle on
a general implementation strategy).
