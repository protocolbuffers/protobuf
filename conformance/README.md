Protocol Buffers - Google's data interchange format
===================================================

[![Build Status](https://travis-ci.org/google/protobuf.svg?branch=master)](https://travis-ci.org/google/protobuf)

Copyright 2008 Google Inc.

This directory contains conformance tests for testing completeness and
correctness of Protocol Buffers implementations.  These tests are designed
to be easy to run against any Protocol Buffers implementation.

This directory contains the tester process `conformance-test`, which
contains all of the tests themselves.  Then separate programs written
in whatever language you want to test communicate with the tester
program over a pipe.

Before running any of these tests, make sure you run `make` in the base
directory to build `protoc`, since all the tests depend on it.

    $ make

Then to run the tests against the C++ implementation, run:

    $ cd conformance && make test_cpp

More tests and languages will be added soon!

Testing other Protocol Buffer implementations
---------------------------------------------

To run these tests against a
new Protocol Buffers implementation, write a testee program
in the target language that uses the target protobuf implementation.
The testee program should implement the protocol defined in
[conformance.proto](https://github.com/google/protobuf/blob/master/conformance/conformance.proto).
See [conformance-protobufcc](https://github.com/google/protobuf/blob/master/conformance/conformance-protobufcc.cc).
for an example of what this program should look like.  The program
only needs to be able to read from stdin and write to stdout.

Portability
-----------

Note that the test runner currently does not work on Windows.  Patches
to fix this are welcome!  (But please get in touch first to settle on
a general implementation strategy).
