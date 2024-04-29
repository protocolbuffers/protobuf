# Life of an Edition

**Author:** [@mcy](https://github.com/mcy)

How to use Protobuf Editions to construct a large-scale change that modifies the
semantics of Protobuf in some way.

## Overview

This document describes how to use the Protobuf Editions mechanism (both
editions, themselves, and [features](protobuf-editions-design-features.md)) for
designing migrations and large-scale changes intended to solve a particular kind
of defect in the language.

This document describes:

*   How features are added to the language.
*   How editions are defined and "proclaimed".
*   How to build different kinds of large-scale changes.
*   Tooling in `protoc` to support large-scale changes.
*   An OSS strategy.

## Defining Features

There are two kinds of features:

*   Global features, which are the fields of `proto.Features`. In this document,
    we refer to them as `features.<name>`, e.g. `features.enum`.
*   Language-scoped features, which are defined in a typed extension field for
    that language. In this document, we refer to them as
    `features.(<lang>).name`, e.g. `features.(proto.cpp).legacy_string`.

Global features require a `descriptor.h` change, and are relatively heavy
weight, since defining one will also require providing helpers in `Descriptor`
wrapper classes to avoid the need for users to resolve inheritance. Because they
are not specific to a language, they need to be carefully, visibility
documented.

Language-scoped features require only a change in a backend's feature extension,
which has a smaller blast radius (except in C++ and Java). Often these are
relevant only for codegen and do not require reflective introspection.

Adding a feature is never a breaking change.

### Feature Lifetime

In general, features should have an *original default* and a *desired default*:
features are intended to gradually flip from one value to another throughout the
ecosystem as migrations progress. This is not always true, but this means most
features will be bools or enums.

Any migration that introduces a feature should plan to eventually deprecate and
remove that feature from both our internal codebase and open source, generally
with a multi-year horizon. Features are *transient*.

Removing a feature is a breaking change, but it does not need to be tied to an
edition. Feature removal in OSS must thus be batched into a breaking release.
Deletion of a feature should generally be announced to OSS a year in advance.

### Do's and Don'ts

Here are some things that we could use features for, very broadly:

*   Changing the generated API of any syntax production (name, behavior,
    signature, whether it is generated at all). E.g.
    `features.(proto.cpp).legacy_string`.
*   Changing the serialization encoding of a field (so long as it does not break
    readers). E.g., `features.packed`, eventually `features.group_encoding`.
*   Changing the deserialization semantics of a field. E.g., `features.enum`,
    `features.utf8`.

Although almost any semantic change can be feature-controlled, some things would
be a bit tricky to use a feature for:

*   Changing syntax. If we introduce a new syntax production, gating it doesn't
    do people much good and is just noise. We should avoid changing how things
    are spelled. In Protobuf's history, it has been incredibly rare that we have
    needed to do this.
*   Shape of a descriptor. Features should generally not cause fields, message,
    or enum descriptors to appear or disappear.
*   Names and field numbers. Features should not change the names or field
    numbers of syntax entities as seen in a descriptor. This is separate from
    using features to change generated API names.
*   Changing the wire encoding in an incompatible way. Using features to change
    the wire format has some long horizons and caveats described below.

## Proclaiming an Edition

An *edition* is a set of default values for all features that `protoc`'s
frontend, and its backends, understand. Edition numbers are announced by
protobuf-team, but not necessarily defined by us. `protoc` only defines the
edition defaults for global features, and each backend defines the edition
defaults for its features.

### Total Ordering of Editions

The `FileDescriptorProto.edition` field is a string, so that we can avoid nasty
surprises around needing to mint multiple editions per year: even if we mint
`edition = "2022";`, we can mint `edition = "2022.1";` in a pinch.

However, protobuf-team does not define editions, it only proclaims them.
Third-party backends are responsible for changing defaults across editions. To
minimize the amount of synchronization, we introduce a *total order* on
editions.

This means that a backend can pick the default not by looking at the edition,
but by asking "is this proto older than this edition, where I introduced this
default?"

The total order is thus: the edition string is split on `'.'`. Each component is
then ordered by `a.len < b.len && a < b`. This ensures that `9 < 10`, for
example.

By convention, we will make the edition be either the year, like `2022`, or the
year followed by a revision, like `2022.1`. Thus, we have the following total
ordering on editions:

```
2022 < 2022.0 < 2022.1 < ... < 2022.9 < 2022.10 < ... < 2023 < ... < 2024 < ...
```

(**Note:** The above edition ordering is updated in
[Edition Naming](edition-naming.md).)

Thus, if an imaginary Haskell backend defines a feature
`feature.(haskell).more_monads`, which becomes true in 2023, the backend can ask
`file.EditionIsLaterThan("2023")`. If it becomes false in 2023.1, a future
version would ask `file.EditionIsBetween("2023", "2023.1")`.

This means that backends only need to change when they make a change to
defaults. However, backends cannot add things to editions willy-nilly. A backend
can only start observing an edition after protobuf-team proclaims the next
edition number, and may not use edition numbers we do not proclaim.

### Proclamation

"Proclamation" is done via a two-step process: first, we announce an upcoming
edition some months ahead of time to OSS, and give an approximate date on which
we plan to release a non-breaking version that causes protoc to accept the new
edition. Around the time of that release, backends should make a release adding
support for that edition, if they want to change a default. It is a faux-pas,
but ultimately has no enforcement mechanism, for the meaning of an edition to
change long (> 1 month) after it has been released.

We promise to proclaim an edition once per calendar year, even if first-party
backends will not use it. In the event of an emergency (whatever that means), we
can proclaim a `Y.1`, `Y.2`, and so on. Because of the total order, only
backends that desperately need a new edition need to pay attention to the
announcement. As we gain experience, we should define guidelines for third
parties to request an unscheduled edition bump, but for the time being we will
deal with things case-by-case.

We may want to have a canonical way for finding out what the latest edition is.
It should be included in large print on our landing page, and `protoc
--latest-edition` should print the newest edition known to `protoc`. The intent
is for tooling that wants to generate `.proto` templates externally can choose
to use the latest edition for new messages.

## Large-scale Change Templates

The following are sketches of large-scale change designs for feature changes we
would like to execute, presented as example use-cases.

### Large-scale Changes with No Functional Changes: Edition Zero

We need to get the ecosystem into the `"editions"` syntax. This migration is
probably unique because we are not changing any behavior, just the spelling of a
bunch of things.

We also need to track down and upgrade (by hand) any code that is using the
value of `syntax`. This will likely be a manual large-scale change performed
either by Busy Beavers or a handful of protobuf-team members furnished with
appropriate stimulants (coffee, diet mountain dew, etc). Once we have migrated
95% of callers of `syntax`, we will mark all accessors of that field in various
languages as deprecated.

Because the value of `syntax` becomes unreliable at this point, this will be a
breaking change.

Next, we will introduce the features defined in
[Edition Zero Features](edition-zero-features.md). We will then implement
tooling that can take a `proto2` or `proto3` file and add `edition = "2023";`
and `option features.* = ...;` as appropriate, so that each file retains its
original behavior.

This second large-scale change can be fully automated, and does not require
breaking changes.

### Large-scale Changes with Features Only: Immolation of `required`

We can use features to move fields off of `features.presence = LEGACY_REQUIRED`
(the edition’s spelling of `required`) and onto `features.presence =
EXPLICIT_PRESENCE`.

To do this, we introduce a new value for `features.presence`,
`ALWAYS_SERIALIZE`, which behaves like `EXPLICIT_PRESENCE`, but, if the has-bit
is not set, the default is serialized. (This is sort of like a cross between
`required` and `proto3` no-label.)

It is always safe to turn a proto from `LEGACY_REQUIRED` to `ALWAYS_SERIALIZE`,
because `required` is a constraint on initialization checking, i.e., that the
value was present. This means the only requirement is that old readers not
break, which is accomplished by always providing *a* value. Because `required`
fields don't set the value anyways, this is not a behavioral change, but it now
permits writers to veer off of actually setting the value.

After an appropriate build horizon, we can assume that all readers are tolerant
of a potentially missing value (even though no writer would actually be omitting
it). At this point we can migrate from `ALWAYS_SERIALIZE` to
`EXPLICIT_PRESENCE`. If a reader does not see a record for the field, attempting
to access it will produce the default value; it is not likely that callers are
actually checking for presence of `required` fields, even though that is
technically a thing you can do.

Once all required fields have gone through both steps, `LEGACY_REQUIRED` and
`ALWAYS_SERIALIZE` can be removed as variants (breaking change).

### Large-scale Changes with Editions: absl::string_view Accessors

In C++, a `string` or `bytes` typed field has accessors that produce `const
std::string&`s. The missed optimizations of doing this are well-understood, so
we won't rehash that discussion.

We would like to migrate all of them to return `absl::string_view`, a-la
`ctype = STRING_PIECE`.

To do this, we introduce `features.(proto.cpp).legacy_string`[^1], a boolean
feature by default true. When false on a field of appropriate type, it does the
needful and causes accessors to become representationally opaque.

The feature can be set at file or field scope; tooling (see below) can be used
to minimize the diff impact of these changes. Changing a field may also require
changing code that was previously assuming they could write `std::string x =
proto.string_field();`. This has the usual "unspooling string" migration
caveats.

Once we have applied 95% of internal changes, we will upgrade the C++ backend at
the next edition to default `legacy_string` to false in the new edition. Tooling
(again, below) can be used to automatically delete explicit settings of the
feature throughout our internal codebase, as a second large-scale change. This
can happen in parallel to closing the loop on the last 5% of required internal
changes.

Once we have eliminated all the legacy accessors, we will remove the feature
(breaking change).

### Large-scale Changes with Wire Format Break: Group-Encoded Messages

It turns out that encoding and decoding groups (end-marker-delimited
submessages) is cheaper than handling length-delimited messages. There are
likely CPU and RAM savings in switching messages to use the group encoding.
Unfortunately, that would be a wire-breaking change, causing old readers to be
unable to parse new messages.

We can do what we did for `packed`. First, we modify parsers to accept message
fields that are encoded as either groups or messages (i.e., `TYPE_MESSAGE` and
`TYPE_GROUP` become synonyms in the deserializer). We will let this soak for
three years[^2] and bide our time.

After those three years, we can begin a large-scale change to add
`features.group_encoded` to message fields throughout our internal codebase
(note that groups don't actually exist in editions; they are just messages with
`features.group_encoded`). Because of our long waiting period, it is (hopefully)
unlikely that old readers will be caught by surprise.

Once we are 95% done, we will upgrade protoc to set `features.group_encoded` to
true by default in new editions. Tooling can be used to clean up features as
before.

We will probably never completely eliminate length-delimited messages, so this
is a rare case where the feature lives on forever.

## Large-scale Change Tooling

We will need a few different tools for minimizing migration toil, all of which
will be released in OSS. These are:

*   The features GC. Running `protoc --gc-features foo.proto` on a file in
    editions mode will compute the minimal (or a heuristically minimal, if this
    proves expensive) set of features to set on things, given the edition
    specified in the file. This will produce a Protochangifier `ProtoChangeSpec`
    that describes how to clean up the file.

*   The editions "adopter". Running `protoc --upgrade-edition -I... file.proto`
    figure out how to update `file.proto` from `proto2` or `proto3` to the
    latest edition, adding features as necessary. It will emit this information
    as a `ProtoChangeSpec`, implicitly running features GC.

*   The editions "upgrader". Running `protoc --upgrade-edition` as above on a
    file that is already in editions mode will bump it up to the latest edition
    known to `protoc` and add features as necessary. Again, this emits a
    features GC'd `ProtoChangeSpec`.

This is by no means all the tooling we need, but it will simplify the work of
robots and beavers, along with any bespoke, internal-codebase-specific tooling
we build.

## The OSS Story

We need to export our large-scale changes into open source to have any hope of
editions not splitting the ecosystem. It is impossible to do this the way we do
large-scale changes in our internal codebase, where we have global approvals and
a finite but nonzero supply of bureaucratic sticks to motivate reluctant users.

In OSS, we have neither of these things. The only stick we have is breaking
changes, and the only carrots we can offer are new features. There is no "global
approval" or "TAP" for OSS.

Our strategy must be a mixture of:

*   Convincing users this is a good thing that will help us make Protobuf easier
    to use, cheaper to deploy, and faster in production.
*   Gently steering users to the new edition in new Protobuf definitions,
    through protoc diagnostics (when an old edition is going or has gone out of
    date) and developer tooling (editor integration, new-file-boilerplate
    templates).
*   Convincing third-party backend vendors (such as Apple, for Swift) that they
    can leverage editions to fix mistakes. We should go out of our way to design
    attractive migrations for them to execute.
*   Providing Google-class tooling for migrations. This includes the large-scale
    change tooling above, and, where possible, specialized tooling. When it is
    not possible to provide tooling, we should provide detailed migration guides
    that highlight the benefits.
*   Being clear that we have a breaking changes policy and that we will
    regularly remove old features after a pre-announced horizon, locking new
    improvements behind completing migrations. This is a risky proposition,
    because users may react by digging in their heels. Comms planning is
    critical.

The common theme is comms and making it clear that these are improvements
everyone can benefit from, and that there is no "I" in "ecosystem": using
Protobuf, just like using Abseil, means accepting upgrades as a fact of life,
not something to be avoided.

We should lean in on lessons learned by Go (see: their `go fix` tool) and Rust
(see: their `rustfix` tool); Rust in particular has an editions/epoch mechanism
like we do; they also have feature gates, but those are not the same concept as
*our* features. We should also lean on the Carbon team's public messaging about
upgrading being a fact of life, to provide a unified Google front on the matter
from the view of observers.

### Prior Art: Rust Editions

The design of [Protobuf Editions](what-are-protobuf-editions.md) is directly
inspired by Rust's own
[edition system](https://doc.rust-lang.org/edition-guide/editions/index.html)[^3].

Rust defines and ships a new edition every three years, and focuses on changes
to the surface language that do not inhibit interop: crates of different
editions can always be linked together, and "edition" is a parallel ratchet to
the language/compiler version.

For example, keywords (like `async`) have been introduced using editions.
Editions have also been used to change the semantics of the borrow checker to
allow new programs, and to change name resolution rules to be more intuitive.
For Rust, an edition may require changes to existing code to be able to compile
again, but *only* at the point that the crate opts into the new edition, to
obtain some benefit from doing so.

Unlike Protobuf, Rust commits to supporting *all* past editions in perpetuity:
there is no ratcheting forward of the whole ecosystem. However, Rust does ship
with `rustfix` (runnable on Cargo projects via `cargo fix`), a tool that can
upgrade crates to a new edition. Edition changes are *required* to come with a
migration plan to enable `rustfix`.

Crates therefore have limited pressure to upgrade to the latest edition. It
provides better features, but because there is no EOL horizon, crates tend to
stay on old editions to support old compilers. For users, this is a great story,
and allows old code to work indefinitely. However, there is a maintenance burden
on the compiler that old editions and new language features (mostly) work
correctly together.

In Rust, macros present a challenge: rich support for interpreted, declarative
macros and compiled, fully procedural macros, mean that macros written for older
editions may not work well in crates written on newer editions, or vice versa.
There are mitigations for this in the compiler, but such fixes cannot be
perfect, so this is a source of difficulties in getting total conversion.
Protobuf does not have macros, but it does have rich descriptors that mirror
input files, and this is a potential source of problems to watch out for.

Overall, Rust's migration story is poor: they have accepted they need to support
old editions indefinitely, but only produce an edition every three years.
Protobuf plans to be much more aggressive, and we should study where Rust's
leniency to old versions is unavoidable and where it is an explicit design
choice.

## Notes

[^1]: `ctype` has baggage and I am going to ignore it for the purposes of
    discussion. The feature is spelled `legacy_string` because adding string
    view accessors is not likely the only thing to do, given we probably want
    to change the mutators as well.
[^2]: The correct size of the horizon is arbitrary, due to the "budget phones in
    India" problem. Realistically we would need to pick one, start the
    migration, and halt it if we encounter problems. It is quite difficult to
    do better than "hope" as our strategy, but `packed` is an existence proof
    that this is not insurmountable, merely very expensive.
[^3]: Rust also has feature gates, used mostly so that people may start trying
    out experimental unstable features. These are largely orthogonal to
    editions, and tied to compiler versions. Rust's feature gates generally do
    not change the semantics of existing programs, they just cause new
    programs to be valid. When a feature is "stabilized", the feature flag is
    removed. Feature flags do not participate in Rust's stability promises.
