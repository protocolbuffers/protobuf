# Protobuf Java Compatibility Tests

This directory contains tests to ensure protobuf library is compatible with
previously released versions.

## Directory Layout

For each released protobuf version we are testing compatibility with, there
is a sub-directory with the following layout (take v2.5.0 as an example):

  * v2.5.0
    * test.sh
    * pom.xml
    * protos/ - unittest protos.
    * more_protos/ - unittest protos that import the ones in "protos".
    * tests/ - actual Java test classes.

The testing code is extracted from regular protobuf unittests by removing:

  * tests that access package private methods/classes.
  * tests that are known to be broken by an intended behavior change (e.g., we
    changed the parsing recursion limit from 64 to 100).
  * all lite runtime tests.

It's also divided into 3 submodule with tests depending on more_protos and
more_protos depending on protos. This way we can test scenarios where only part
of the dependency is upgraded to the new version.

## How to Run The Tests

We use a shell script to drive the test of different scenarios so the test
will only run on unix-like environments. The script expects a few command
line tools to be available on PATH: git, mvn, wget, grep, sed, java.

Before running the tests, make sure you have already built the protoc binary
following [the C++ installation instructions](../../src/README.md). The test
scripts will use the built binary located at ${protobuf}/src/protoc.

To start a test, simply run the test.sh script in each version directory. For
example:

    $ v2.5.0/test.sh

For each version, the test script will test:

  * only upgrading protos to the new version
  * only upgrading more_protos to the new version

and see whether everything builds/runs fine. Both source compatibility and
binary compatibility will be tested.
