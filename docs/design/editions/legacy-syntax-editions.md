# Legacy Syntax Editions

**Author:** [@mkruskal-google](https://github.com/mkruskal-google)

**Approved:** 2023-09-08

Should proto2/proto3 be treated as editions?

## Background

[Edition Zero Features](edition-zero-features.md) lays out our plan for edition
2023, which will unify proto2 and proto3. Since early in the design process,
we've discussed the possibility of making proto2 and proto3 "special" editions,
but never laid out what exactly it would look like or determined if it was
necessary.

We recently redesigned editions to be represented as enums
([Edition Naming](edition-naming.md)), and also how edition defaults are
propagated to generators and runtimes
([Editions: Life of a FeatureSet](editions-life-of-a-featureset.md)). With these
changes, there could be an opportunity to special-case proto2 and proto3 in a
beneficial way.

## Problem Description

While the original plan was to keep editions and syntax orthogonal, that naively
means we'd be supporting two very different codebases. This has some serious
maintenance costs though, especially when it comes to test coverage. We could
expect to have sub-optimal test coverage of editions initially, which would
gradually become poor coverage of syntax later. Since we need to support both
syntax and editions long-term, this isn't ideal.

In the implementation of editions in C++, we decided to unify a lot of the
infrastructure to avoid this issue. We define global feature sets for proto2 and
proto3, and try to use those internally instead of checking syntax directly. By
pushing the syntax/editions branch earlier in the stack, it gives us a lot of
indirect test coverage for editions much earlier.

A separate issue is how Prototiller will support the conversion of syntax to
edition 2023. For features it knows about, we can hardcode defaults into the
transforms. However, third party feature owners will have no way of signaling
what the old proto2/proto3 behavior was, so Prototiller won't be able to provide
any transformations by default. They'd need to provide custom Prototiller
transforms hardcoding all of their features.

## Recommended Solution

We recommend adding two new special editions to our current set:

```
enum Edition {
  EDITION_UNKNOWN = 0;
  EDITION_PROTO2 = 998;
  EDITION_PROTO3 = 999;
  EDITION_2023 = 1000;
}
```

These will be treated the same as any other edition, except in our parser which
will reject `edition = "proto2"` and `edition = "proto3"` in proto files. The
real benefit here is that this allows features to specify what their
proto2/proto3 defaults are, making it easier for Prototiller to handle
migration. It also allows generators and runtimes to unify their internals more
completely, treating proto2/proto3 files exactly the same as editions.

### Serialized Descriptors

As we now know, there are a lot of serialized `descriptor.proto` descriptor sets
out there that need to continue working for O(months). In order to avoid
blocking edition zero for that long, we may need fallbacks in protoc for the
case where feature resolution *fails*. If the file is proto2/proto3, failure
should result in a fallback to the existing hardcoded defaults. We can remove
these later once we're willing to break stale `descriptor.proto` snapshots that
predate the changes in this doc.

### Bootstrapping

In order to get feature resolution running in proto2 and proto3, we need to be
able to support bootstrapped protos. For these builds, we can't use any
reflection without deadlocking, which means feature defaults can't be compiled
during runtime. We would have had to solve this problem anyway when it came time
to migrate these protos to editions, but this proposal forces our hand early.
Luckily, "Editions: Life of a FeatureSet" already set us up for this scenario,
and we have Blaze rules for embedding these defaults into code. For C++
specifically, this will need to be checked in alongside the other bootstrapped
protos. Other languages will be able to do this more dynamically via genrules.

### Feature Inference

While we can calculate defaults using the same logic as in editions, actually
inferring "features" from proto2/proto3 needs some custom code. For example:

*   The `required` keyword sets `LEGACY_REQUIRED` feature
*   The `optional` keyword in proto3 sets `EXPLICIT` presence
*   The `group` keyword implies `DELIMITED` encoding
*   The `enforce_utf8` options flips between `PACKED` and `EXPANDED` encoding

This logic needs to be written in code, and will need to be duplicated in every
language we support. Any language-specific feature transformations will also
need to be included in that language. To make this as portable as possible, we
will define functions like:

Each type of descriptor will have its own set of transformations that should be
applied to its features for legacy editions.

#### Pros

*   Makes it clearer that proto2/proto3 are "like" editions

*   Gives Prototiller a little more information in the transformation from
    proto2/proto3 to editions (not necessarily 2023)

*   Allows proto2/proto3 defaults to be specified in a single location

*   Makes unification of syntax/edition code easier to implement in runtimes

*   Allows cross-language proto2/proto3 testing with the conformance framework
    mentioned in "Editions: Life of a FeatureSet"

#### Cons

*   Adds special-case legacy editions, which may be somewhat confusing

*   We will need to port feature inference logic across all languages. This is
    arguably cheaper than maintaining branched proto2/proto3 code in all
    languages though

## Considered Alternatives

### Do Nothing

If we do nothing, there will be no built-in unification of syntax and editions.
Runtimes could choose any point to split the logic.

#### Pros

*   Requires no changes to editions code

#### Cons

*   Likely results in lower test coverage
*   May hide issues until we start rolling out edition 2023
*   Prototiller would have to hard-code proto2/proto3 defaults of features it
    knows, and couldn't even try to migrate runtimes it doesn't
