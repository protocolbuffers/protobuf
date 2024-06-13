# Editions Tooling

**Authors:** [@mcy](https://github.com/mcy)

**Approved:** 2022-08-09

## Overview

[Protobuf Editions](../editions/what-are-protobuf-editions.md) aims to introduce
new semantics for Protobuf, but with a major emphasis on mechanical, incremental
upgradability, to avoid the two systems problem of proto2 and proto3. The first
edition (likely "2023") will introduce *converged semantics* for Protobuf that
permit everything that proto2 and proto3 permitted: any non-editions file can
become an editions file with minimal human intervention.

We plan to achieve this with a strong tooling story. These tools are intended to
fully automate major steps in editions-related upgrade operations, for both large-scale changes
and open source software strategic reasons. In particular:

*   Non-automated large-scale change work in the editions space can be constrained to fixing
    uses of generated code and flipping features on specific fields (or other
    declarations).
*   We can give our external users the most painless migration possible, which
    consists of "run this tool and commit the results".

This document describes the detailed design of the tools we need. This document
presupposes *Protochangifier Backend Design Doc* (not available externally) integrated into protoc as a prerequisite, so we
can ship the tooling as part of protoc. Because the tooling must know the full
definition of an edition to work (see below), it seems to more-or-less place a
hard requirement of being linked to protoc.

There are three tools we will build.

1.  The "features janitor". This is a mode of `protoc` which consumes a `.proto`
    file and produces a `ProtoChangeSpec` that describes how to add and remove
    features such that the resulting janitor'ed file has fewer explicit
    features, but is not semantically different.
2.  The "editions adopter". This is another mode of `protoc`, which produces a
    `ProtoChangeSpec` that describes how to bring a `proto2` or `proto3` file
    into editions mode, starting at a specific edition.
3.  The "editions upgrader". This is a generalization of the adopter, which
    takes an editions file and produces a `ProtoChangeSpec` that brings it into
    a newer edition.

These tools will fundamentally speak `ProtoChangeSpec`, but we should also
provide in-place versions, since those will likely be more useful to OSS users
that just want to run the tool atomically on their entire project.

## The Janitor

The features janitor is intended to be used as part of migrations to
periodically clean up any messes made by flipping lots of features.
Conceptually, it turns this proto file

```
edition = "2023";
message Foo {
  optional string a = 1 [features.(pb.cpp).string_type = VIEW];
  optional string b = 2 [features.(pb.cpp).string_type = VIEW];
  optional string c = 3 [features.(pb.cpp).string_type = VIEW];
  optional string d = 4 [features.(pb.cpp).string_type = VIEW];
  optional string e = 5 [features.(pb.cpp).string_type = VIEW];
}
message Bar {
  optional string a = 1 [features.(pb.cpp).string_type = VIEW];
  optional string b = 2;
  optional string c = 3;
  optional string d = 4;
  optional string e = 5;
}
```

into this one:

```
edition = "2023";
message Foo {
  option features.(pb.cpp).string_type = VIEW;
  optional string a = 1;
  optional string b = 2;
  optional string c = 3;
  optional string d = 4;
  optional string e = 5;
}
message Bar {
  optional string a = 1 [features.(pb.cpp).string_type = VIEW];
  optional string b = 2;
  optional string c = 3;
  optional string d = 4;
  optional string e = 5;
}
```

Specifically, the janitor tries to minimize the number of explicit features on
the Protobuf schema. Actually doing this minimally feels like it's nonlinear, so
we should invent a heuristic. A sketch of what this could look like:

1.  Each feature that can appear explicitly on an AST node is either *critical*
    for that node or only for grouping. For example, `string_type` is critical
    for fields but not for messages.
2.  Propagate features explicitly to every node, including edition defaults.
3.  For each feature `f`, for each node `n` that `f` is non-critical for that
    contains (recursively) nodes that it is critical for (in DFS order):
    1.  Set `f` for `n` to the value for `f` that the plurality of its direct
        children have, and remove the explicit `f` from those. If tied, choose
        the edition default if it is among the plurals, or else choose randomly.
4.  Once repeated up to the root, delete all explicit features that are
    reachable from the root without crossing another explicit feature that isn't
    the edition default. I.e., those features which are implied by the edition
    defaults.

It is easy to construct cases where this is not optimal, but that is not
important. This merely exists to make files prettier while keeping them
equivalent. It is easy to see that, by construction, this algorithm satisfies
the "semantic no-op" requirement.

## The Adopter and the Updater

The adopter is merely a special case of the updater where `proto2` and `proto3`
are viewed as editions (in the sense that an edition is a set of defaults), so
we will only describe the updater.

To update one edition ("old") to another ("new", although not necessarily a
newer edition):

1.  Features that are not already explicitly set at the top level are set to the
    default given by "old"; they are only set on the outermost scope that does
    not have an explicit feature. For example, for file-level features, this
    means making all features explicit at the file level. For message-level
    features that are not file-level, this means placing an explicit feature on
    all top-level messages. This is a no-op, because `edition = "old";` implies
    this.
2.  The file's edition is set from "old" to "new". Because every feature that
    could be explicit is explicit, this is a no-op.
3.  Feature janitor runs. This explicitly propagates all features (all of which
    are set explicitly at the top level), and then cleans them up with respect
    to the "new" edition; note that feature janitor gives preference to editions
    defaults. This is a no-op, because feature janitor is a no-op.

## UX Concerns

Bundling the editions tooling with `protoc` ensures that it is easy to find. The
following will be the pattern for all Protochangifier tooling bundled into
`protoc`:

*   There is a flag `--change_spec=changespec.pb` which will cause protoc to
    apply a changespec to the passed-in `.proto` file, e.g. `protoc
    --change_spec=spec.pb --change_out=foo-changed.proto foo.proto`. This writes
    the change to `foo-changed.proto`. This may be the same file as `foo.proto`
    for in-place updates; it may be left out to have the change printed to
    stdout. This is the core entry-point for Protochanfigier.
*   There is a flag `--my_analysis` for the given analysis, e.g. `--janitor`.
    This flag can have an optional argument: if set, it will output the change
    spec to that path, e.g. `--janitor=spec.pb`. If it is not passed in, the
    change is applied in place without the need to use `protoc --change_spec`.

Alternatively, we could provide these as standalone tools. However, it seems
useful from a distribution perspective and user education perspective to say
"this is just part of the compiler". We expect to produce new migration tooling
with Protochangifier on an ongoing basis, so teaching users that every analysis
looks the same is important. Compare `rustfix`, the tool that Rust uses for
things like upgrading editions. Although it is a separate binary, it is
accessible through `cargo fix`, and in a lot of ways `cargo` is the user-facing
interface to Rust; having it be part of the "swiss army knife" helps put it in
front of users.
