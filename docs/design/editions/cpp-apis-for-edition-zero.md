# C++ APIs for Edition Zero

**Author:** [@mcy](https://github.com/mcy)

**Approved:** 2022-06-27

## Overview

As recorded in *FileDescriptor::syntaxAudit Report* (not available externally),
there are significant uses of `FileDescriptor::syntax()` in internal Google
repositories that Edition Zero will break; for example, existing usage checks
`syntax() == PROTO3` to determine if enums are open.

Because we expect to make everything that callers are querying be controlled by
individual edition-gated features, the cleanest way forward is to enrich the
`Descriptor` types with focused APIs that serve current usages, followed by a
migration to them.

Tier 1 proposal:

## Proposed API

This proposal adds the following code to `descriptor.h:`

```
class FileDescriptor {
  // Copies package, syntax, edition, dependencies, and file-level options.
  void CopyHeadingTo(FileDescriptorProto*) const;
};
class FieldDescriptor {
  // Returns whether this field has a proto3-like zero default value.
  bool has_zero_default_value() const;
  // Returns whether this is a string field that enforces UTF-8 in the codec.
  bool enforces_utf8() const;
};
class EnumDescriptor {
  // Returns whether this enum is a proto2-style closed enum.
  bool is_closed() const;
};
```

This covers everything not already covered by existing accessors.
`CopyHeadingTo` is intended to simplify the common, easy-to-get-wrong pattern of
copying the heading of a proto file before doing custom manipulation of
descriptors within.

Additionally, we propose generating `unknown_fields()` and
`mutable_unknown_fields()` for all protos unconditionally.

## Migration

In addition to the above functions, we'll use `FieldDescriptor::has_presence()`
and `FieldDescriptor::is_packed()` to migrate callers performing comparisons to
`syntax()`. Users can migrate to the new functions by:

1.  Searching for calls to `syntax()`.
2.  Identifying what proto2/proto3 distinction is being picked up on, among the
    following:
    1.  UTF-8 verification on parse (use new API).
    2.  Closed/open enum (use new API).
    3.  Packed-ness (use existing API instead of guessing from `syntax`).
    4.  Whether fields have hasbits (use `has_presence()`).
3.  Migrate to the new API.

The volume of such changes in google3 is small enough that it's probably easiest
to create a giant CL that fixes every instance of a particular misuse and then
hand it to Rosie for splitting up.

Once all the "easy" usages are migrated, we will mark `syntax()` as
`ABSL_DEPRECATED`. `syntax()` will return a new special value,
`Syntax::EDITIONS`, intended to explicitly break caller expectations about what
this function returns. Virtually all cases not covered by this proposal are
using `syntax()` to reject one of the two existing syntaxes, or produce an error
when encountering an unknown `Syntax` value, so they will continue to work as
expected (i.e. fail on `editions` protos).

A number of existing tools are hostile to proto2 or proto3. Once the migration
plan is approved, we should reach out to them individually to coordinate either
updating or deprecating their tool.
