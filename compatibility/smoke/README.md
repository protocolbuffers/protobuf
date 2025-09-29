# Legacy generated code compatibility smoke test

This directory contains a smoke unit tests that runs the core functionality of
generated code against the latest runtime.

Note that running protoc as a build step rather than checking in gencode is the
strongly recommended pattern, and any Bazel users should use `proto_library()`
rules to achieve that rather than checking in generated code.

This directory is only using checked in generated code as the mechanism to test
the compatibility with stale gencode created by an older protoc, which is
expected to be relevant in non-Bazel usecases where it may be difficult to run
protoc at build time.
