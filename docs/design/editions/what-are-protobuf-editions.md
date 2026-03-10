# What are Protobuf Editions?

**Authors**: [@mcy](https://github.com/mcy), [@fowles](https://github.com/fowles)

## Summary

This document is an introduction to the Protobuf Editions project, an ambitious
re-imagining of how we migrate Protobuf users into the future.

## Goal

Enable incremental evolution of Protobuf across the entire ecosystem **without**
introducing permanent forks in the Protobuf language.

## TL;DR

1.  We are replacing
    [`syntax`](https://protobuf.dev/reference/protobuf/proto3-spec/#syntax) `=
    ...` with `edition = ...`.
    *   We plan to produce a new "edition" on a roughly yearly basis.
    *   We plan to regularly deprecate and remove old editions after a wide
        horizon.
    *   This gradual churn is enabled by the
        [Protobuf Breaking Changes policy](https://protobuf.dev/news/2022-07-06/#library-breaking-change-policy).
2.  "Features" are a special kind of file/message/field/enum/etc option.
    *   Features control the individual codegen and runtime behavior of fields,
        messages, enums, etc.
    *   Features cannot introduce changes that would directly break existing
        binaries.
    *   We expect heavy churn of features in `.proto` files, so their design is
        optimized to minimize diffs to `.proto` files while permitting
        fine-grained control.
    *   Features are **usually** attached to the field/message/enum they apply
        to.
        *   Features can be specified at a higher-level entity, such as a file,
            to apply to all definitions inside of that entity. This is called
            **feature inheritance**.
        *   Inheritance is intended to allow us to factor frequently-occurring
            feature declarations, minimizing clutter during migrations.
3.  Editions change only the defaults of features and do not otherwise introduce
    new behavior.
    *   New behavior is fundamentally controlled by features (explicitly set or
        implicit from an edition).
    *   Editions allow us to ratchet the ecosystem forward.
        *   Editions can be incremented on a per `.proto` file basis; projects
            can upgrade incrementally.
4.  Messages with any permutation of features are always interoperable (they can
    import each other freely and use messages from each other).
    *   Editions do not split the ecosystem, and migration is largely automated.
    *   Directly inspired by
        [Rust editions](https://doc.rust-lang.org/edition-guide/editions/index.html).
    *   Carbon has a similar philosophy
5.  The `proto2`/`proto3` distinction is going away.
    *   Editions will support everything from both and allow mixed semantics
        even within the same message or field.
    *   Undesirable features will be LSC'd away, using the same template as any
        other feature/edition migration.

## Motivation

Arguably the biggest hard-earned lesson among Software Foundations is that
successful migrations are incremental. Most of our experience with these has
been for internal migrations. Externally, progress has often ossified because of
a lack of established evolution mechanisms. More recently large projects have
started planning incremental evolution into their structure. For example, Carbon
is heavily focused on evolution as a core precept, and Rust has built language
evolution via editions into its core design..

Protobuf is one of Google's oldest and most successful toolchain projects.
However, it was designed before we learned and internalized this lesson, making
modernization difficult and haphazard. We still have `required` and `group`,
`packed` is not everywhere, and string accessors in C++ still return `const
std::string&`. The last radical change to Protobuf (`syntax = "proto3";`) split
the ecosystem.

*Editions* and *features* are new language features that will allow us to
incrementally evolve Protobuf into the future. This will be done by introducing
a new `syntax`, hopefully the last syntax addition we will ever need.

This high-level document is intended as an introduction to Protobuf Editions for
engineers not familiar with the background and the set of tradeoffs that lead us
here. Low-level technical details are skipped in preference to describing the
kernel of our proposed design. This document reflects the approximate consensus
of protobuf-team members who have been developing Protobuf Editions, but please
beware: many open questions remain.

## What is a feature?

A *feature*, in the narrow context of Protobuf Editions, is an `option` on any
syntax entity of a `.proto` file that has the following properties:

*   It is a field or extension of a top-level option named `features`, which is
    present on every syntax entity (file, message, enum, field, etc). It can be
    of any type, but `bool` and `enum` are the most common.
*   If a syntax entity's lexical parent has a particular value for a feature,
    then the child has the same value, unless the feature has a new value
    specified on the child, explicitly. This is called **feature inheritance**,
    and applies recursively. Features can specify a new value at any of the
    points where a feature can be added.
*   It explicitly specifies what syntax entities it can be set on, similar to
    Java annotations (although this does not preclude inheritance to or through
    an entity that it *cannot* be set on).

Features allow us to control the behavior of `protoc`, its backends, and the
Protobuf runtimes at arbitrary granularity. This is critical for large-scale
changes: if a message has few usages, features can be changed at a bigger scope,
minimizing diff churn, but if it has heavy usage and the CL to migrate a single
field is large, cleanups can happen at the field level, as necessary.

Features won't change a message’s serialization formats (binary, text, or json)
in incompatible ways except for extreme circumstances that will always be
managed directly by protobuf-team. It is critical for migrations that any
behavioral change coming from a feature is the result of a textual change to a
`.proto` file (either an edition bump or a feature change).

`ctype` is an existing field option that looks exactly like a feature: it
controls the behavior of the codegen backend, although it does not have the nice
ratcheting properties of editions.

Because features can be extensions, language backends can specify
**language-scoped** features. For example, `[ctype = CORD]` could instead be
phrased as `[features.(pb.cpp).string_type = CORD]`. Codegen backends own the
definitions of their features.

## What is an Edition?

An *edition* is a collection of defaults for features understood by `protoc` and
its backends. Editions are year-numbered, although we have defined a breakout in
case we need multiple editions in a particular year.

Instead of writing `syntax = "...";`, a Protobuf Editions-enabled `.proto` file
begins with `edition = "2022";` or similar. `edition` implies `syntax =
"editions";`, and the `syntax` keyword itself becomes deprecated. This is to
ensure that old tools not owned by protobuf-team, which only work for old
Protobuf syntaxes, crash or fail quickly and noticeably, instead of wandering
into a descriptor that they cannot understand (we will attempt to migrate what
we can, of course).

`protoc` specifies which editions it understands, and will reject `.proto` files
"from the future", since it cannot meaningfully parse them. `protoc` backends,
which can specify their own set of language-scoped features, must advertise the
defaults for a particular edition that they understand (and reject editions that
they don't). Runtimes must be able to handle descriptors "from the future"; this
only means that upon encountering a descriptor with an edition or feature it
does not understand, there must be a reasonable fallback for the runtime's
behavior.

### What is an Edition used for?

Editions provide the fundamental increments for the lifecycle of a feature. At
this point it is important to reiterate that most features will be specific to
particular code generators. What follows is an example life cycle for a
theoretical feature–`features.(pb.cpp).opaque_repeated_fields`.

1.  Edition “2025” creates `features.(pb.cpp).opaque_repeated_fields` with a
    default value of `false`. This value is equivalent to the behavior from
    editions less than “2025”.

    a. The migration to edition “2025” across google will move very fast as it
    is a no-op.

2.  Migration begins for `features.(pb.cpp).opaque_repeated_fields` (each change
    in this migration will add `features.(pb.cpp).opaque_repeated_fields = true`
    and be paired with code changes required to C++ code). It is not anticipated
    that protos shared between repos will undergo field by field migrations like
    this as that would cause a large stream of breaking changes, see
    [Protobuf Editions for schema producers](protobuf-editions-for-schema-producers.md)
    for more details.

3.  Edition “2027” switches the default of
    `features.(pb.cpp).opaque_repeated_fields` to `true`.

    a. The migration to “2027” will remove explicit uses of
    `features.(pb.cpp).opaque_repeated_fields = true` and add explicit uses of
    `features.(pb.cpp).opaque_repeated_fields = false` where they were implicit
    before. As above, this migration will be a no-op, so it will move very fast.

    b. Externally, we will release tools and migration guides for OSS customers.
    The tools will not be fully turnkey, but should provide a strong starting
    point for user migrations.

4.  Migration continues for `features.(pb.cpp).opaque_repeated_fields` (each
    change in this migration will remove
    `features.(pb.cpp).opaque_repeated_fields = false` and be paired with code
    changes required to C++ code).

5.  At some point, usage will be officially roped off internally, and
    externally.

    a. Internally, `features.(pb.cpp).opaque_repeated_fields` usage will be
    blocked with allowlists while we remove the hardest to migrate case.

    b. Externally, `features.(pb.cpp).opaque_repeated_fields` will be marked
    deprecated in a public edition and removed in a later one. When a feature is
    removed, the code generators for that behavior and the runtime libraries
    that support it may also be removed. In this hypothetical, that might be
    deprecated in “2029” and removed in “2031”. Any release that removes support
    for a feature would be a major version bump.

The key point to note here is that any `.proto` file that does not use
deprecated features has a no-op upgrade from one edition to the next and we will
provide tools to effect that upgrade. Internal users will be migrated centrally
before a feature is deprecated. External users will have the full window of the
Google migration as well as the deprecation window to upgrade their own code.

It is also important to note that external users will not receive compiler
warnings until the feature is actually deprecated, so we provide a period of
deprecation to ensure that they have time to update their code before forcing
them to upgrade for an edition update.

Separately from feature evolution, `protoc` itself may remove support for old
editions entirely after a suitably long window (like 10 years).

## Edition Zero

The first edition of Protobuf Editions, the so-called "edition zero", will
effectively be a "`proto4`" that introduces the new syntax, and merges the
semantics of `proto2` and `proto3`. In editions mode, everything that was
possible in `proto2` and `proto3` will be possible, and the handful of
irreconcilable differences will be expressed as features.

For example, whether values not specified in an `enum` go into unknown fields vs
producing an enum value outside of the bounds of the specified values in the
`.proto` file (i.e., so-called closed and open enums) will be controlled by
`feature.enum = OPEN` or `feature.enum = CLOSED`.

Edition Zero should be viewed as the "completion" of the union of `proto2` and
`proto3`: it contains both syntaxes as subsets (although with different
spellings to disambiguate things) as well as new behavior that was previously
inexpressible but which is an obvious consequence of allowing everything from
both. For example, `proto3`-style non-optional singular fields could allow
non-zero defaults.

Edition Zero is designed in such a way that we can mechanically migrate an
arbitrary `.proto` file from either `proto2` or `proto3` with no behavioral
changes, by replacing `syntax` with `edition` and adding features in the
appropriate locations.

This will form the foundation of Protobuf Editions and the torrent of parallel
migrations that will follow.

## FAQ

### I only interact with protos by moving them around and editing schemata. How does this affect me?

This will manifest as a handful of new `option`s appearing at the top of your
files. Going forward, expect new `options` to appear and disappear from your
`.proto` files as LSCs march across the codebase. We intend to minimize
disruption, and you should be able to safely ignore them.

In general, you should not need to add `option`s yourself unless we say so in
documentation. We will try to make sure tooling recommends the latest edition
when creating new files.

### Are you taking away &lt;thing>?

Everything expressible today will remain so in Edition Zero. Some syntax will
change: we will have only one way of spelling a singular field (with `optional`
vs. the `proto3` behavior vs. `required` controlled by a feature), `group`s will
turn into sub message fields with a special encoding.

### I think &lt;thing> from proto{2,3} is bad. Why are you letting people use it in my files?

Long-term bifurcation of the language has resulted in significant damage to the.
ecosystem and engineers' mental model of Protobuf. There are features we think
are questionable, too, and we want to remove them. But we need to break some
eggs to make an omelet.

As stewards of the Protobuf language, we believe this is the best way to get rid
of features that were a good idea at the time, but which history has shown to
have had poor outcomes.

### I manipulate protos reflectively, or have some other complicated use-case

We plan to upgrade reflection to be feature-aware in a way that minimizes code
we need to change. We do not expect anyone to implement feature-inheritance
logic themselves; feature inheritance should be fully transparent to users,
behaving as if features had been placed explicitly everywhere. (Owners of code
generators should be the only ones that need to know how to correctly propagate
features.)

We will be partnering with use-cases that are known risks for migration, such as
storage providers, to minimize toil and disruption on all sides.

### I want to use features to fix a defect in Protobuf

Generally, the owner of the relevant component that ingests a particular feature
(`protoc` or the appropriate language backend) will own it. We will try to make
it as straightforward as we can to add a language-scoped feature, but it may
require some degree of coordination with us to get it into an edition.

Even if it's about one of protobuf-team's backends, we'd love to hear what you
think we can fix, within the constraints of editions.

### What's your OSS strategy?

We want to share a variant of this document with the OSS community. We plan to
publish migration guides and, where feasible, any migration tooling, such as the
`proto2`/`proto3` -> `edition` migrator.

As stated above, we want to minimize friction for non-protobuf-team-owned
backends, and this ties into helping third party code generators minimize their
pain.

### I like Protobuf as it is. Can I keep my old files?

Yes, but you get to keep both pieces. Failing to migrate off of old use-cases
and into newer versions that fix known defects is a risk for the entire
ecosystem: C++'s disastrous standardization process is a solemn warning of
failing to do so.

Trying to stay on `proto2` or `proto3` will eventually cease to be supported,
and old editions (e.g. 5 years) will also cease to be supported. Evolution is at
the heart of Protobuf, and we want to make it as easy as possible for users to
keep up with our progress towards a better Protobuf.

### What do you hope to use editions to change in the short/mid term?

An incomplete list of *ideas*, which should be taken as non-committal.

*   Eliminate `required` completely by making a particular field be optional but
    serialized unconditionally.
*   Make all uses of `string` require UTF-8 checking, and all uses that don't
    want/need it `bytes`, fulfilling the original `proto3` vision.
*   Make every `string` and `bytes` accessor in C++ return `absl::string_view`,
    unlocking performance optimizations.
*   Make all scalar `repeated` fields `packed`, improving throughput.
*   Make `enum` enumerators in C++ use `kName` instead of `NAME`.
*   Make `enum` declarations in C++ into scoped `enum class`.
*   Make `CTYPE` into a language-scoped feature.
*   Replace per-language, file-level options with language-scoped features.
*   Make reflection opt-in for some languages (C++).
