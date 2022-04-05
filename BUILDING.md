# Building Protocol Buffers

These are instructions for building the open source release of protobuf from source.

Given the right tools, this project can be built on Linux, Mac OS X, or Windows though Linux is by far the most commonly used and best tested platform.

## Prerequisites

Likely incomplete

* git
* JDK 8 or JDK 11.
* bazel
* clang

This project is built with bazel. Individual language builds assume that protoc is built and
installed first. 


## Detailed instructions

1. `$ git clone git@github.com:protocolbuffers/protobuf.git`. You can also clone the repo over https.
1. `$ cd protobuf`
1. `$ git clean -xfd .` (Important for cleaning up after previous builds.)
1. `$ bazel build //...`

Now run the tests:


1. `$ bazel test //...`



## Building subprojects

After the initial build, it's not always necessary to rebuild everything after a change.
For instance, you can build and test changes to the Java runtime without
rebuilding protoc as long as you've built it once and the build artifacts are still available. 
