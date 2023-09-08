# Edition Zero: JSON Handling

**Author:** [@mkruskal-google](https://github.com/mkruskal-google)

**Approved:** 2023-05-10

## Background

Today, proto3 fully validates JSON mappings for uniqueness during parsing, while
proto2 takes a best-effort approach and allows cases that don't have a 1:1
mapping. This is laid out in more detail by *JSON Field Name Conflicts* (not
available externally). While we had hoped to unify these before
[Protobuf editions](what-are-protobuf-editions.md) launched, we ended up blocked
by some internal use-cases. This issue is now blocking the editions launch,
since we can't represent this behavior with the current set of
[Edition Zero features](edition-zero-features.md).

## Overview

Today, by default, we transform each field name to a CamelCase name that will
always be valid, but not necessarily unique in JSON. We also support a
`json_name` field option to override this for JSON parsing/serialization. This
allows conflicts to potentially arise where many proto fields map to the same
JSON field. Our JSON handling has the following behaviors:

*   All proto messages can be serialized to JSON
    *   Conflicting mappings will produce JSON with duplicate keys
*   All proto messages can be parsed from JSON
    *   Conflicting mappings lead to undefined behavior. While the behavior is
        deterministic in all of the cases we've encountered, it's inconsistent
        across runtimes and unexpected.
*   The Protobuf compiler will fail to parse any proto3 files if any JSON
    conflicts are detected by default
    *   Disabled by `deprecated_legacy_json_field_conflicts` option
*   Proto2 files will only fail to parse if both of the conflicts fields have
    `json_name` set
    *   We will still warn for default json mapping conflicts if
        `deprecated_legacy_json_field_conflicts` isn't set

The goal here is to unify these behaviors into a future-facing feature as part
of edition zero.

## Recommendation

We recommend adding a new `json_format` feature as part of
[Edition Zero features](edition-zero-features.md). The doc will be updated to
reflect the following details.

JSON format can have three possible states:

*   `ALLOW` - By default, fields will be fully validated during proto parsing.
    Any conflicting JSON mappings will trigger protoc errors, guaranteeing
    uniqueness. This will be consistent with the current proto3 behavior. No
    runtime changes are needed, since we allow JSON parsing/serialization.
*   `DISALLOW` - Alternatively, we will ban JSON encoding and disable all
    validation related to JSON mappings. All runtimes will fail to parse or
    serialize any messages to/from JSON when this feature is set on the
    top-level messages. This is a new mode which provides an alternative to
    `LEGACY_BEST_EFFORT` that doesn't involve any schema changes.
*   `LEGACY_BEST_EFFORT` - Fields will be validated for correctness, but not for
    uniqueness. Any conflicting JSON mappings will trigger protoc warnings, but
    no errors. This will be consistent with the current proto2 behavior, or
    proto3 where `deprecated_legacy_json_field_conflicts` is set. Since this is
    undefined behavior we want to get rid of, a parallel effort will attempt to
    remove this later. No runtime changes are needed, since we allow JSON
    parsing/serialization.

Long-term, we want JSON support to be specified at the proto level. For the
migration from proto2/proto3, we will just migrate everything to `ALLOW` and
`LEGACY_BEST_EFFORT` depending on the `syntax` and the value of
`deprecated_legacy_json_field_conflicts`.

We will additionally ban any `ALLOW` message from containing a `DISALLOW` type
anywhere in its tree (including extensions, which will fail to compile).
Attempting to add this will result in a compiler error. This has the following
benefits:

*   The implementation is a lot simpler, since most of the work is done in
    protoc and parsers only need to check the top level message
*   Runtime failures aren't dependent on the contents of the message being
    serialized/parsed
*   Avoids messy blurring of ownership. If a bug occurs because a `DISALLOW`
    field is sometimes set, is the owner of the child type required to change it
    to `ALLOW`? Or is the owner of the parent type responsible because they
    added the dependency?

    `LEGACY_BEST_EFFORT` will continue to allow serialization/parsing of types
    with `DISALLOW` set.

This feature will target messages and enums, but we will also provide it at the
file level for convenience.

Example use-cases for `DISALLOW`:

*   https://github.com/protocolbuffers/protobuf/issues/12525
*   Some projects generate proto descriptors at runtime and uses underscores to
    disambiguate field names. They never use JSON format with these protos, but
    currently have to work around our conflict checks

## Alternatives

### Dual State

Instead of a tri-state feature, we could have a simple allow/disallow feature
for JSON format.

#### Pros

*   Simpler conceptually

#### Cons

*   We would end up blocked by many of the protos that we were unable to migrate
    as part of *JSON Field Name Conflicts* (not available externally). While
    some of them could be migrated to `DISALLOW`, others are actually
    **depending** on our current behavior under JSON mapping conflicts (as a
    hack around some limitations in JSON customization).

### Default to DISALLOW

Instead of defaulting to `ALLOW`, we could default to `DISALLOW`.

#### Pros

The majority of internal Google protos are used for binary/text encoding and
don't care about JSON, so this would:

*   Be less noisy for teams who forget to explicitly set `DISALLOW` and may have
    fields with conflicting JSON mappings
*   Decrease our support surface

#### Cons

*   We would need to figure out where `DISALLOW` can be added

### Do Nothing

#### Pros

*   Short-term it saves some trouble and keeps edition zero simpler

#### Cons

*   We'll eventually hit the same issues we hit in *JSON Field Name Conflicts*
    (not available externally)
*   The current proto2/proto3 behaviors are mutually exclusive. There's nothing
    we can migrate to in today's edition zero that won't risk breaking one of
    them.
