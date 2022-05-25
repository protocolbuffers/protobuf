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
`pkg_filegroup` rules, `proto_library` rules, `cc_*` rules (including
`cc_test`), `cc_dist_library` rules (see below), and `test_suite` rules.

Unlike most other Bazel rules, transitive dependencies are not included in the
output. For example, if a `proto_library` is listed, only its `srcs` will be
included in the output, but not those of any transitively-nominated
`proto_library`s. (As an exception, `test_suite` rules are expanded recursively
to find `cc_test` targets.)

When a `proto_library` is used for file lists, the generated file will include
the proto sources, as well as the expected generated C++ files.

## C++ runtime libraries

The `cc_dist_library` rule creates composite libraries from several other
`cc_library`-like targets. Bazel builds typically use a "fine-grained" library
model, where each `cc_library` produces its own library artifacts, without
transitive dependencies. The `cc_dist_library` rule combines several other
libraries together, creating a single library that may be suitable for
distribution.

Unlike most other Bazel C++ rules, `cc_dist_library` does not consider
transitive dependencies. This means that the resulting artifacts will contain
exactly the objects listed in `deps`.

When a `cc_dist_library` is used to generate build system file lists (see
above), it exposes the `srcs` of each of the inputs. In the generated output,
`hdrs` and `textual_hdrs` are listed separately. Files in `srcs` are also
filtered, so that actual sources are emitted as `srcs`, and everything else as
`internal_hdrs`.

Note that `cc_dist_library` uses Bazel's internal C++ build logic, but does not
treat tests specially. Because of this, it may not be possible to build a
`cc_test` target at the same time as a `cc_dist_library` that includes it.
