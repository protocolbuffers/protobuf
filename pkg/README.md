# Protobuf packaging

This directory contains Bazel rules for building packaging and distribution
artifacts.

*Everything in this directory should be considered internal and subject to
change.*

## Protocol compiler binary packaging

The protocol compiler is used in binary form in various places. There are rules
which package it, along with commonly used `.proto` files, for distribution.

## Source distribution packaging

Protobuf releases include source distributions, sliced by target language (C++,
Java, etc.). There are rules in this package to define those source archives.
These depend upon `pkg_files` rules elsewhere in the repo to get the contents.

The source distribution files should include the outputs from `autogen.sh`, but
this isn't something we can reliably do from Bazel. To produce fully functioning
source distributions, run `autogen.sh` before building the archives (this
populates the necessary files directly into the source tree).

## Build system file lists

The `gen_automake_file_lists` and `gen_cmake_file_lists` rules generate files
containing lists of files that can be included into definitions for other build
systems.

The files can come from files or `filegroup` rules, `pkg_file` or
`pkg_filegroup` rules, `proto_library` rules, or `cc_dist_library` rules (see
below).

When a `proto_library` is used for file lists, the output will include the proto
sources, as well as the expected generated C++ files.

## C++ runtime libraries

The `cc_dist_library` rule creates composite libraries from several other
`cc_library`-like targets. Bazel uses a "fine-grained" library model, where each
`cc_library` produces its own library artifacts, without transitive
dependencies. The `cc_dist_library` rule combines several other libraries
together, creating a single library that may be suitable for distribution.

Unlike most other Bazel C++ rules, `cc_dist_library` does not consider
transitive dependencies. This means that the resulting artifacts will contain
exactly the objects listed in `deps`. (There is one exception, explained below.)

When a `cc_dist_library` is used to generate build system file lists, the `hdrs`
and `textual_hdrs` are listed separately in the generated output. Files in
`srcs` are filtered, so that actual sources are emitted as `srcs`, and
everything else as `internal_hdrs`.

Tests can be passed to `cc_dist_library`, either as `cc_test` rules, or through
`test_suite` rules. As a special-case exception, `test_suite` rules are expanded
recursively. This allows using rules defined like `test_suite(name = "tests")`,
which easily group all tests in a package.
