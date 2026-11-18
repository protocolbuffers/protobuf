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

## C++ runtime binary distribution

The `cc_dist_library` rule creates composite libraries from several other
`cc_library` targets. Bazel uses a "fine-grained" library model, where each
`cc_library` produces its own library artifacts, without transitive
dependencies. The `cc_dist_library` rule combines several other libraries
together, creating a single library that may be suitable for distribution.
