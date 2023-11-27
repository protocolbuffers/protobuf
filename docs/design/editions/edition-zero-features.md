# Edition Zero Features

**Authors:** [@mcy](https://github.com/mcy),
[@zhangskz](https://github.com/zhangskz),
[@mkruskal-google](https://github.com/mkruskal-google)

**Approved:** 2022-07-22

Feature flags, and their defaults, that we will introduce to define the
converged semantics of Edition Zero.

**NOTE:** This document is largely replaced by the topic,
[Feature Settings for Editions](https://protobuf.dev/editions/features) (to be
released soon).

## Overview

*Edition Zero Features* defines the "first edition" of the brave new world of
no-`syntax` Protobuf. This document defines the actual mechanics of the features
(in the narrow sense of editions) we need to implement in protoc, as well as the
chosen defaults.

This document will require careful review from various stakeholders, because it
is essentially defining a new Protobuf `syntax`, even if it isn't spelled that
way. In particular, we need to ensure that there is a way to rewrite existing
`proto2` and `proto3` files as `editions` files, and the behavior of "mixed
syntax" messages, without any nasty surprises.

Note that it is an explicit goal that it be possible to take an arbitrary
proto2/proto3 file and convert it to editions without semantic changes, via
appropriate application of features.

## Existing Non-Conformance

We must keep in mind that the status quo is messy. Many languages have some
areas where they currently diverge from the correct proto2/proto3 semantics. For
edition zero, we must preserve these idiosyncratic behaviors, because that is
the only way for a proto2/proto3 -> editions LSC to be a no-op.

For example, in this document we define a feature `features.enum =
{CLOSED,OPEN}`. But currently Go does not implement closed enum semantics for
`syntax=proto2` as it should. This behavior is out of conformance, but we must
preserve this out-of-conformance behavior for edition zero.

In other words, defining features and their semantics is in scope for edition
zero, but fixing code generators to perfectly match those semantics is
explicitly out-of-scope.

## Glossary

Because we need to speak of two proto syntaxes, `proto2` and `proto3`, that have
disagreeing terminology in some places, we'll define the following terms to aid
discussion. When a term appears in `code font`, it refers to the Protobuf
language keyword.

*   A **presence discipline** is a handling for the presence (or hasbit) of a
    field. Every field notionally has a hasbit: whether it has been explicitly
    set via the API or whether a record for it was present on deserialization.
    See
    [Application Note: Field Presence](https://protobuf.dev/programming-guides/field_presence)
    for more on this topic. The discipline specifies how this bit is surfaced to
    the user:
    *   **No presence** means that the API does not expose the hasbit. The
        default value for the field behaves somewhat like a special sentinel
        value, which is not serialized and not merged-from. The hasbit may still
        exist in the implementation (C++ accidentally leaks this via HasField,
        for example). Note that repeated fields sort-of behave like no presence
        fields.
    *   **Explicit presence** means that the API exposes the hasbit through a
        `has` method and a `Clear` method; default values are always serialized
        if the hasbit is set.
*   A **closed enum** is an enum where parsing requires validating that a parsed
    `int32` representing a field of this type matches one of the known set of
    valid values.
*   An **open enum** does not have this restriction, and is just an `int32`
    field with well-known values.

For the purposes of this document, we will use the syntax described in *Features
as Custom Options*, since it is the prevailing consensus among those working on
editions, and allows us to have enum-typed features. The exact names for the
features are a matter of bikeshedding.

## Proposed Converged Semantics

There are two kinds of syntax behaviors we need to capture: those that are
turned on by a keyword, like `required`, and those that are implicit, like open
enums. The differences between proto2 and proto3 today are:

*   Required. Proto2 has `required` but not `defaulted`; Proto3 has `defaulted`
    but not `required`. Proto3 also does not allow custom defaults on
    `defaulted` fields, and on message-typed fields, `defaulted` is a synonym
    for `optional`.
*   Groups. Proto2 has groups, proto3 does not.
*   Enums. In Proto2, enums are **closed**: messages that have an enum not in
    the known set are stored in the unknown field set. In Proto3, enums are
    **open**.
*   String validation. Proto2 is a bit wobbly on whether strings must be UTF-8
    when serialized; Proto3 enforces this (sometimes).
*   Extensions. Proto2 has extensions, while Proto3 does not (`Any` is the
    canonical workaround).

We propose defining the following features as part of edition zero:

### features.field_presence

This feature is enum-typed and controls the presence discipline of a singular
field:

*   `EXPLICIT` (default) - the field has *explicit presence* discipline. Any
    explicitly set value will be serialized onto the wire (even if it is the
    same as the default value).
*   `IMPLICIT` - the field has *no presence* discipline. The default value is
    not serialized onto the wire (even if it is explicitly set).
*   `LEGACY_REQUIRED` - the field is wire-required and API-optional. Setting
    this will require being in the `required` allowlist. Any explicitly set
    value will be serialized onto the wire (even if it is the same as the
    default value).

The syntax for singular fields is a much debated question. After discussing the
tradeoffs, we have chosen to *eliminate both the `optional` and `required`
keywords, making them parse errors*. Singular fields are spelled as in proto3
(no label), and will take on the presence discipline given by
`features.:presence`. Migration will require deleting every instance of
`optional` in proto files in google3, of which there are 385,236.

It is important to observe that proto2 users are much likelier to care about
presence than proto3 users, since the design of proto3 discourages thinking
about presence as an interesting feature of protos, so arguably introducing
proto2-style presence will not register on most users' mental radars. This is
difficult to prove concretely.

`IMPLICIT` fields behave much like proto3 implicit fields: they cannot have
custom defaults and are ignored on submessage fields. Also, if it is an
enum-typed field, that enum must be open (i.e., it is either defined in a
`syntax = proto3;` file or it specifies `option features.enum = OPEN;`
transitively).

We also make some semantic changes:

*   ~~`IMPLICIT``fields may have a custom default value, unlike in`proto3`.
    Whether an`IMPLICIT` field containing its default value is serialized
    becomes an implementation choice (implementations are encouraged to try to
    avoid serializing too much, though).~~
*   `has_optional_keyword()` and `has_presence()` now check for `EXPLICIT`, and
    are effectively synonyms.
*   `proto3_optional` is rejected as a parse error (use the feature instead).

Migrating from proto2/3 involves deleting all `optional`/`required` labels and
adding `IMPLICIT` and `LEGACY_REQUIURED` annotations where necessary.

#### Alternatives

*   For syntax:
    *   Require `optional`. This may confuse proto3 users who are used to
        `optional` not being a default they reach for. Will result in
        significant (trivial, but noisy) churn in proto3 files. The keyword is
        effectively line noise, since it does not indicate anything other than
        "this is a singular field".
    *   Invent a new label, like `singular`. This results in more churn but
        avoids breaking peoples' priors.
    *   Allow `optional` and no label to coexist in a file, which take on their
        original meanings unless overridden by `features.field_presence`. The
        fact that a top-level `features.field_presence = IMPLICIT` breaks the
        proto3 expectation that `optional` means `EXPLICIT` may be a source of
        confusion.
*   `proto:allow_required`, which must be present for `required` to not be a
    syntax error.
*   Allow `required`/`optional` and introduce `defaulted` as a real keyword. We
    will not have another easy chance to introduce such syntax (which we do,
    because `edition = ...` is a breaking change).
*   Reject custom defaults for `IMPLICIT` fields. This is technically not really
    needed for converged semantics, but trying to remove the Proto3-ness from
    `IMPLICIT` fields seems useful for consistency.

#### Future Work

In the future, we can introduce something like `features.always_serialize` or a
similar new enumerator (`ALWAYS_SERIALIZE`) to the `when_missing` enum, which
makes `EXPLICIT_PRESENCE` fields unconditionally serialized, allowing
`LEGACY_REQUIRED` fields to become `EXPLICIT_PRESENCE` in a future large-scale
change. The details of such a migration are out-of-scope for this document.

#### Migration Examples

Given the following files:

```
// foo.proto
syntax = "proto2"

message Foo {
  required int32 x = 1;
  optional int32 y = 2;
  repeated int32 z = 3;
}

// bar.proto
syntax = "proto3"

message Bar {
  int32 x = 1;
  optional int32 y = 2;
  repeated int32 z = 3;
}
```

post-editions, they will look like this:

```
// foo.proto
edition = "tbd"

message Foo {
  int32 x = 1 [features.presence = LEGACY_REQUIRED];
  int32 y = 2;
  repeated int32 z = 3;
}

// bar.proto
edition = "tbd"
option features.presence = NO_PRESENCE;

message Bar {
  int32 x = 1;
  int32 y = 2 [features.presence = EXPLICIT_PRESENCE];
  repeated int32 z = 3;
}
```

### features.enum_type

Enum types come in two distinct flavors: *closed* and *open*.

*   *closed* enums will store enum values that are out of range in the unknown
    field set.
*   *open* enums will parse out of range values into their fields directly.

    **NOTE:** Closed enums can cause confusion for parallel arrays (two repeated
    fields that expect to have index i refer to the same logical concept in both
    fields) because an unknown enum value from a parallel array will be placed
    in the unknown field set and the arrays will cease being parallel. Similarly
    parsing and serializing can change the order of a repeated closed enum by
    moving unknown values to the end.

    **NOTE:** Some runtimes (C++ and Java, in particular) currently do not use
    the declaration site of enums to determine whether an enum field is treated
    as open; rather, they use the syntax of the message the field is defined in,
    instead. To preserve this proto2 quirk until we can migrate users off of it,
    Java and C++ (and runtimes with the same quirk) will use the value of
    `features.enum` as set at the file level of messages (so, if a file sets
    `features.enum = CLOSED` at the file level, enum fields defined in it behave
    as if the enum was closed, regardless of declaration). IMPLICIT singular
    fields in Java and C++ ignore this and are always treated as open, because
    they used to only be possible to define in proto3 files, which can't use
    proto2 enums.

In proto2, `enum` values are closed and no requirements are placed upon the
first `enum` value. The first enum value will be used as the default value.

In proto3, `enum` values are open and the first `enum` value must be zero. The
first `enum` value is used as the default value, but that value is required to
be zero.

In edition zero, We will add a feature `features.enum_type = {CLOSED,OPEN}`. The
default will be `OPEN`. Upgraded proto2 files will explicitly set
`features.enum_type = CLOSED`. The requirement of having the first enum value be
zero will be dropped.

**NOTE:** Nominally this exposes a new state in the configuration space, OPEN
enums with a non-zero default. We decided that excluding this option simply
because it was previously inexpressible was a false economy.

#### Alternatives

*   We could add a property for requiring a zero first value for an enum. This
    feels needlessly complicated.
*   We could drop the ability to have `CLOSED` enums, but that is a semantic
    change.

#### Migration Examples

Given the following files:

```
// foo.proto
syntax = "proto2"

enum Foo {
  A = 2, B = 4, C = 6,
}

// bar.proto
syntax = "proto3"

enum Bar {
  A = 0, B = 1, C = 5,
}
```

post-editions, they will look like this:

```
// foo.proto
edition = "tbd"
option features.enum_type = CLOSED;

enum Foo {
  A = 2, B = 4, C = 6,
}

// bar.proto
edition = "tbd"

enum Bar {
  A = 0, B = 1, C = 5,
}
```

If we wanted to merge them into one file, it would look like this:

```
// foo.proto
edition = "tbd"

enum Foo {
  option features.enum_type = CLOSED;
  A = 2, B = 4, C = 6,
}


enum Bar {
  A = 0, B = 1, C = 5,
}
```

### features.repeated_field_encoding

In proto3, the `repeated_field_encoding` attribute defaults to `PACKED`. In
proto2, the `repeated_field_encoding` attribute defaults to `EXPANDED`. Users
explicitly enabled packed fields 12.3k times, but only explicitly disable it 200
times. Thus we can see a clear preference for `repeated_field_encoding = PACKED`
emerge. This data matches best practices. As such, the default value for
`features.repeated_field_encoding` will be `PACKED`.

The existing `[packed = …]` syntax will be made an alias for setting the feature
in edition zero. This alias will eventually be removed. Whether that removal
happens during the initial large-scale change to enable edition zero or as a
follow on will be decided at the time.

In the long term, we would like to remove explicit usages of
`features.repeated_field_encoding = EXPANDED`, but we would prefer to separate
that large-scale change from the landing of edition zero. So we will explicitly
set `features.repeated_field_encoding` to `EXPANDED` at the file level when we
migrate proto2 files to edition zero.

#### Alternatives

*   Force everyone to use packed fields. This is a semantic change, which we're
    trying to avoid in edition zero.
*   Don’t add `features.repeated_field_encoding` and instead specify `[packed =
    false]` when converting from proto2. This will be incredibly noisy,
    syntax-wise and diff-wise.

#### Migration Examples

Given the following files:

```
// foo.proto
syntax = "proto2"

message Foo {
  repeated int32 x = 1;
  repeated int32 y = 2 [packed = true];
  repeated int32 z = 3;
}

// bar.proto
syntax = "proto3"

message Foo {
  repeated int32 x = 1;
  repeated int32 y = 2 [packed = false];
  repeated int32 z = 3;
}
```

post-editions, they will look like this:

```
// foo.proto
edition = "tbd"
options features.repeated_field_encoding = EXPANDED;

message Foo {
  repeated int32 x = 1;
  repeated int32 y = 2 [packed = true];
  repeated int32 z = 3;
}


// bar.proto
edition = "tbd"

message Foo {
  repeated int32 x = 1;
  repeated int32 y = 2 [packed = false];
  repeated int32 z = 3;
}
```

Note that post migration, we have not changed `packed` to
`features.repeated_field_encoding = PACKED`, although we could choose to do so
if the diff cost is not monumental. We prefer to defer to an LSC after editions
are shipped, if possible.

### features.string_field_validation

**WARNING:** UTF8 validation is actually messier than originally believed. This
feature is being reconsidered in _Editions Zero Feature: utf8_validation_.

This feature is a tristate:

*   `MANDATORY` - this means that a runtime MUST verify UTF-8.
*   `HINT` - this means that a runtime may refuse to parse invalid UTF-8, but it
    can also simply skip the check for performance in some build modes.
*   `NONE` - this field behaves like a `bytes` field on the wire, but parsers
    may mangle the string in an unspecified way (for example, Java may insert
    spaces as replacement characters).

The default will be `MANDATORY`.

Long term, we would like to remove this feature and make all `string` fields
`MANDATORY`.

#### Alternatives

*   Drop the UTF-8 requirements completely. This seems like it will create more
    problems than it will solve (e.g., random things relying on validation need
    to be fixed) and it will be a lot of work. This is also counter to the
    vision of string being a UTF-8 type, and bytes being its unchecked sibling.
*   Make opt-in verification a hard requirement instead of a hint, so that users
    have a nice performance needle they can play with.

#### Future Work

In the infinite future, we would like to remove this feature and force all
`string` fields to be UTF-8 validated. To do this, we need to recognize that
what many callers want from their `string` fields is a `bytes` field with a
`string`-like API. To ease the transition, we would add per-codegen backend
features, like `java.bytes_as_string`, that give a `bytes` field a generated API
resembling that of a `string` field (with caveats about replacement characters
forced by the host language's string type).

The migration would take `HINT` or `SKIP` `string` fields and convert them into
`bytes` fields with the appropriate API modifiers, depending on which languages
use that proto; C++-only protos, for example, are a no-op.

There is an argument to be made for "I want a string type, and I explicitly want
replacement U+FFFD characters if I get something that isn't UTF-8." It is
unclear if this is something users want and we would need to investigate it
before making a decision.

### features.json_format

This feature is dual state in edition zero:

*   `ALLOW` - this means that a runtime must allow JSON parsing and
    serialization. Checks will be applied at the proto level to make sure that
    there is a well-defined mapping to JSON.
*   `LEGACY_BEST_EFFORT` - this means that a runtime will do the best it can to
    parse and serialize JSON. Certain protos will be allowed that can result in
    undefined behavior at runtime (e.g. many:1 or 1:many mappings).

The default will be `ALLOW`, which maps the to the current proto3 behavior.
`LEGACY_BEST_EFFORT` will be used for proto2 files that require it (e.g. they’ve
set `deprecated_legacy_json_field_conflicts`)

#### Alternatives

*   Keep the proto2 behavior - this will regress proto3 files by removing
    validation for JSON mappings, and lead to *more* undefined runtime behavior
*   Only use `ALLOW` - there are ~30 cases internally where protos have invalid
    JSON mappings and rely on unspecified (but luckily well defined) runtime
    behavior.

#### Future Work

Long term, we would like to either remove this feature entirely or add a
`DISALLOW` option instead of `LEGACY_BEST_EFFORT`. This will more strictly
enforce that protos without a valid JSON mapping *can’t* be serialized or parsed
to JSON. `DISALLOW` will be enforced at the proto-language level, where no
message marked `ALLOW` can contain any message/enum marked `DISALLOW` (e.g.
through extensions or fields)

#### Migration Examples

### Extensions are Always Allowed

Extensions may be used on all messages. This lifts a restriction from proto3.

Extensions do not play nicely with `TypeResolver`. This is actually fixable, but
probably only worth it if someone complains.

#### Alternatives

*   Add `features.allow_extensions`, default true. This feels unnecessary since
    uttering `extend` and `extensions` is required to use extensions in the
    first place.

### features.message_encoding

This feature defaults to `LENGTH_PREFIXED`. The `group` syntax does not exist
under editions. Instead, message-typed fields that have
`features.message_encoding = DELIMITED` set will be encoded as groups (wire type
3/4) rather than byte blobs (wire type 2). This reflects the existing API
(groups are funny message fields) and simplifies the parser.

A `proto2` group field will be converted into a nested message type of the same
name, and a singular submessage field that is `features.message_encoding =
DELIMITED` with the message type's name in snake_case.

This could be used in the future to switch new message fields to use group
encoding, which suggested previously as an efficiency direction.

#### Alternatives

*   Allow groups in `editions` with no changes. `group` syntax is deprecated, so
    we may as well take the opportunity to knock it out.
*   Add a sidecar allowlist like we do for `required`. This is mostly
    orthogonal.

#### Migration Examples

Given the following file

```
// foo.proto
syntax = "proto2"

message Foo {
  group Bar = 1 {
    optional int32 x = 1;
    repeated int32 y = 2;
  }
}
```

post-editions, it will look like this:

```
// foo.proto
edition = "tbd"

message Foo {
  message Bar {
    optional int32 x = 1;
    repeated int32 y = 2;
  }
  Bar bar = 1 [features.message_encoding = DELIMITED];
}
```

## Proposed Features Message

Putting together all of the above, we propose the following `Features` message,
including retention and target rules associated with fields.

```
message Features {
  enum FieldPresence {
    EXPLICIT = 0;
    IMPLICIT = 1;
    LEGACY_REQUIRED = 2;
  }
  optional FieldPresence field_presence = 1 [
      retention = RUNTIME,
      target = FILE,
      target = FIELD
  ];

  enum EnumType {
    OPEN = 0;
    CLOSED = 1;
  }
  optional EnumType enum = 2 [
      retention = RUNTIME,
      target = FILE,
      target = ENUM
  ];

  enum RepeatedFieldEncoding {
    PACKED = 0;
    UNPACKED = 1;
  }
  optional RepeatedFieldEncoding repeated_field_encoding = 3 [
      retention = RUNTIME,
      target = FILE,
      target = FIELD
  ];

  enum StringFieldValidation {
    MANDATORY = 0;
    HINT = 1;
    NONE = 2;
  }
  optional StringFieldValidation string_field_validation = 4 [
      retention = RUNTIME,
      target = FILE,
      target = FIELD
  ];

  enum MessageEncoding {
    LENGTH_PREFIXED = 0;
    DELIMITED = 1;
  }
  optional MessageEncoding message_encoding = 5 [
      retention = RUNTIME,
      target = FILE,
      target = FIELD
  ];

  extensions 1000;  // for features_cpp.proto
  extensions 1001;  // for features_java.proto
}
```
