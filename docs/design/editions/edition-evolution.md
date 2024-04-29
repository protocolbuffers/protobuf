# Edition Evolution

**Author:** [@mcy](https://github.com/mcy)

**Approved:** 2022-07-06

## Overview

[Protobuf Editions](what-are-protobuf-editions.md) give us a mechanism for
pulling protobuf files into the future by upgrading their *edition*. Features
are flags associated with syntax items of a `.proto` file; they can be either
set explicitly, or they can be implied by the edition. Features can either
correspond to behavior in `protoc`'s frontend (e.g. `proto:closed_enums`), or to
a specific backend (`cpp:string_view`).

This document seeks to answer:

*   When do we create an edition?
*   How do backends inform protoc of their defaults?

## Proposal

### Total Ordering of Editions

**NOTE:** This topic is largely superseded by [Edition Naming](edition-naming.md).

The `FileDescriptorProto.edition` field is a string, so that we can avoid nasty
surprises around needing to mint multiple editions per year: even if we mint
`edition = "2022";`, we can mint `edition = "2022b";` in a pinch.

However, we might not own some third-party backends, and they might be unaware
of when we decide to mint editions, and might want to mint editions on their
own. Suppose that I maintain the Haskell protobuf backend, and I decide to add
the `haskell:more_monads` feature. How do I get this into an edition?

We propose defining a *total order* on editions. This means that a backend can
pick the default not by looking at the edition, but by asking "is this proto
older than this edition, where I introduced this default?"

The total order is thus: the edition string is split on `'.'`. Each component is
then ordered by `a.len &lt; b.len && a &lt; b`. This ensures that `9 &lt; 10`,
for example.

By convention, we will make the edition be either the year, like `2022`, or the
year followed by a revision, like `2022.1`. Thus, we have the following total
ordering on editions:

```
2022 < 2022.0 < 2022.1 < ... < 2022.9 < 2022.10 < ... < 2023 < ... < 2024 < ...
```

This means that backends (even though we don't particularly recommend it) can
change defaults as often as they like. Thus, if I decide that
`haskell:more_monads` becomes true in 2023, I simply ask
`file.EditionIsLaterThan("2023")`. If it becomes false in 2023.1, a future
backend can ask `file.EditionIsBetween("2023", "2023.1")`.

### Creating an Edition

In a sense, every edition already exists; it's just a matter of defining
features on it.

If the feature is a `proto:` feature, `protoc` intrinsically knows it, and it is
implemented in the frontend.

If the feature is a backend feature, the backend must be able to produce some
kind of proto like `message Edition { repeated Feature defaults = 1; }` that
describes what a specific edition must look like, based on less-than/is-between
predicates like those above. That information can be used by protoc to display
the full set of features it knows about given its backends. (The backend must,
of course, use this information to make relevant codegen decisions.)

### What about Editions "From the Future"?

Suppose that in version v5.0 of my Haskell backend I introduced
`haskell:more_monads`, and this has a runtime component to it; that is, the
feature must be present in the descriptor to be able to handle parsing the
message correctly.

However, suppose I have an older service running v4.2. It is compiled with a
proto that was already edition 2023 at build time (alternatively, it dynamically
loads a proto). My v5.0 client sends it an incompatible message from "the
future". Because a parse failure would be an unacceptable service degradation,
we have a couple of options:

*   Editions cannot introduce a feature that requires readers to accept new
    encodings.
    *   Similarly, editions cannot add restrictions that constrain past parsers.
*   Editions may introduce such features, but they must somehow fit into some
    kind of build horizon.

The former is a reasonable-sounding but ultimately unacceptable position, since
it means we cannot use editions if we wanted to, say, make it so that message
fields are encoded as groups rather than length-prefixed chunks. The alternative
is to define some kind of build horizon.