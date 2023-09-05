# The Road to Edition Zero

**Author:** [@mcy](https://github.com/mcy)

**Approved:** 2022-06-16

## Overview

The very first use-case of [protobuf editions](what-are-protobuf-editions.md) is
[edition zero](edition-zero-converged-semantics.md): the first edition that
replaces the proto2/3 split. In all places that proto2/3 disagree on semantics,
we will pick a default and an opt-out `feature`.

This is effectively a breaking change for the proto language: what could
previously be inferred from syntax is now inferred from features. Proto
backends, as well as other users of schemas must be made edition-aware before
editions can be used at all.

This document describes a roadmap for *getting* to Edition Zero.

## 2022 Q2

### descriptor.proto + Compiler Changes

We need to add support for `edition = "...";`[^1] and `option features =
[...];`. The former requires a compiler change; the latter we get for free out
of changing `*Options` messages in `descriptor.proto`.

The compiler change will be as follows:

*   Recognize `edition` instead of `syntax` (it is an error for both or neither
    to be present).
*   If `edition` is present, fill in the new `edition` field of
    `FileDescriptorProto` with its contents, and fill in the `syntax` field with
    the string `"editions"`, as a poison pill against anyone relying on `syntax`
    being `proto2` or `proto3`.

#### AIs:

1.  Implement the edition keyword behind `--allow_editions` (false by default).
    This includes a `descriptor.proto` change, a`descriptor.h` change, a parser
    change, and a CLI change.
2.  Implement the `options features` syntax in whichever way we choose.
3.  Define criteria to flip `--allow_editions` to true by default (should
    include parsing `options features`).
4.  Audit and fix uses of the `syntax` field of `FileDescriptorProto` in C++,
    Java, and Python. All uses must be `switch` statements, not `== PROTO3`. See
    *FileDescriptor::syntaxAudit Report* (not available externally) and
    [C++ APIs for Edition Zero](cpp-apis-for-edition-zero.md)
    1.  Recruit 20%ers to help with the cleanup.

### Define all features needed for Edition Zero

We need a complete list of every feature that will need to be added for Edition
Zero. This work has been started in
[Edition Zero: Converged Semantics](edition-zero-converged-semantics.md) but it
needs to be wrapped up into something that can be used as a migration plan.

Any semantic changes that are currently covered by `syntax() == "proto3"` need
to be provided migration instructions that we can apply in codegen backends and
for other users that use schemas. These should be described in terms of
`syntax() == "editions"` and examining other fields in the descriptor proto.

#### AIs:

1.  Do a detailed first pass at *Edition Zero: Converged Semantics* to see
    what's missing.
2.  Assemble a slate of stakeholders to review the proposed converged semantics.

## 2022 Q3

### descriptor.h (et. al.) Changes + Migration

We need to deprecate `FileDescriptor::syntax()` and replace all uses of it with
fine-grained functions for querying information that callers were previously
using `syntax()` for. When `syntax` is `proto2/3`, it will use the value implied
by the syntax keyword; when set to `editions`, it will search for the
appropriate feature (and select a default based on the value of `edition`). We
will not expose a `FileDescriptor::edition()`.

We must also create documentation that lists out how to do this in terms of the
descriptor proto, for users that are not using the C++ API. Given most of the
important callers in C++, we will get significant mileage out of this work
alone.

## Notes

[^1]: This document presupposes we are using strings, not integers. Strings are
    useful because we are very unlikely to do just one edition per year; not
    being able to write `edition = 2022b;` seems like it's asking for trouble.
