# Minimum Required Edition

**Author:** [@mcy](https://github.com/mcy)

**Approved:** 2022-11-15

A versioning mechanism for descriptors that ensures old runtimes do not load
descriptors that are "too new."

## Background

Suppose we decide to add a novel definition like

```
const int32 MY_CONSTANT = 42;
```

to the Protobuf language. This would entail a descriptor change to track the
values of constants, but they would not be loaded properly by older runtimes.
This document describes an addition to `descriptor.proto` that prevents this
version mismatch issue.

[Protobuf Editions](what-are-protobuf-editions.md) intends to add the concept of
an edition to Protobuf, which will be an approximately-annually incrementing
value. Because of their annual nature, and because runtimes need to be updated
to handle new features they implement regardless, we can use them as a poison
pill for old runtimes that try to load descriptors that are "too new."

## Overview

We propose adding a new field to `FileDescriptorProto`:

```
optional string minimum_required_edition = ...;
```

This field would exist alongside the `edition` field, and would have the
following semantics:

Every Protobuf runtime implementation must specify the newest edition whose
constructs it can handle (at a particular rev of that implementation). If that
edition is less than `minimum_required_edition`, loading the descriptor must
fail.

"Less than" is defined per the edition total order given in
[Life of an Edition](life-of-an-edition.md). To restate it, it is the following
algorithm:

```
def edition_less_than(a, b):
  parts_a = a.split(".")
  parts_b = b.split(".")
  for i in range(0, min(len(parts_a), len(parts_b))):
    if int(parts_a[i]) < int(parts_b[i]): return True
  return len(a) < len(b)
```

`protoc` should keep track of which constructions require which minimum edition.
For example, if constants are introduced in edition 2025, but they are not
present in a file, `protoc` should not require that runtimes understand
constants by picking a lower edition, like 2023 (assuming no other construct
requires a higher edition).

In particular, the following changes should keep the minimum edition constant,
with all other things unchanged:

*   An upgrade of the proto compiler.
*   Upgrading the specified edition of a file via Prototiller.

### Bootstrapping Concerns

"Epochs for `descriptor.proto`" (not available externally) describes a potential
issue with bootstrapping. It is not the case here: minimum edition is only
incremented once a particular file uses a new feature. Since `descriptor.proto`
and other schemas used by `protoc` and the backends would not use new features
immediately, introducing a new feature does not immediately stop the compiler
from being able to compile itself.

### Concerns for Schema Producers

Schema producers should consider changes to their schemas that increase the
minimum required edition to be breaking changes, since it will stop compiled
descriptors from being loaded at runtime.

## Recommendation

We recommend adding the aforementioned minimum required edition field, along
with the semantics it entails. This logic should be implemented entirely in the
protoc frontend.

## Alternatives

### Use a Non-Editions Version Number

Rather than using the editions value, use some other version number. This number
would be incremented rarely (3-5 year horizon). This is the approach proposed
in "Epochs for `descriptor.proto`."

#### Pros

*   Does not re-use the editions value for a semantically-different meaning; the
    edition remains being "just" a key into a table of features defaults.

#### Cons

*   Introduces another version number to Protobuf that increments at its own
    cadence.
*   Could potentially be confused with the edition value, even though they serve
    distinct purposes.

### Minimum Required Edition Should Not Be Minimal

The proto compiler should not guarantee that the minimum required edition is as
small as it could possibly be.

#### Pros

*   Reduces implementation burden.

#### Cons

*   This introduces situations where an upgrade of the proto compiler, or an
    innocuous change to a schema, can lead the the minimum required edition
    being incremented. This is a problem for schema producers.

### Do Nothing

#### Pros

*   Reduces churn in runtimes, since they do not need to implement new handling
    for new *editions* (as contrasted to just *features)* regularly.
*   Avoids a situation where old software cannot load new descriptors at
    runtime.

#### Cons

*   Commits us to never changing the descriptor wire format in
    backwards-incompatible ways, which has far-reaching effects on evolution.
    These consequences are discussed in "Epochs for `descriptor.proto`."
