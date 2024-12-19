# Editions: Group Migration Issues

**Authors**: [@mkruskal-google](https://github.com/mkruskal-google)

## Summary

Address some unexpected issues in delimited encoding in edition 2023 before its
OSS release.

## Background

Joshua Humphries reported some well-timed
[issues](https://github.com/protocolbuffers/protobuf/issues/16239) discovered
while experimenting with our early release of Edition 2023. He discovered that
our new message encoding feature piggybacked a bit too much on the old group
logic, and actually ended up being virtually useless in general.

None of our testing or migrations caught this because they were heavily focused
on *preserving* old behavior (which is the primary goal of edition 2023).
Delimited messages structured exactly like proto2 groups (e.g. message and field
in the same scope with matching names) continued to work exactly as before,
making it seem like everything was fine.

All of this is especially problematic in light of *Submessages: In Pursuit of a
More Perfect Encoding* (not available externally yet), which intends to migrate the
ecosystem to use delimited encoding everywhere. Releasing a semi-broken feature
as a migration tool to eliminate a deprecated syntax is one thing, but trying to
push the ecosystem to it is especially bad.

## Overview

The problems here stem from the fact that before edition 2023, the field and
type name of group fields was guaranteed to always be unique and intuitive.
Proto2 splits groups into a synthetic nested message with a type name equivalent
to the group specification (required to be capitalized), and a field name that's
fully lowercased. For example,

```
optional group MyGroup = 1 { ... }
```

would become:

```
message MyGroup { ... }
optional MyGroup mygroup = 1;
```

The casing here is very important, since the transformation is irreversible. We
can't recover the group name from the field name in general, only if the group
is a single word.

The problem under edition 2023 is that we've removed the generation of
synchronized synthetic messages from the language. Users now explicitly define
messages, and any message field can be marked `DELIMITED`. This means that
anyone assuming that the type and field name are synchronized could now be
broken.

### Codegen

While using the field name for generated APIs required less special-casing in
the generators, the field name ends up producing slightly-less-readable APIs for
multi-word camelcased groups. The result is that we see a fairly random-seeming
mix in different generators. Using protoc-explorer (not available externally),
we find the following:

<table>
  <tr>
   <td><strong>Language</strong>
   </td>
   <td><strong>Generated APIs</strong>
   </td>
   <td><strong>Example proto2 getter</strong>
   </td>
  </tr>
  <tr>
   <td>C++
   </td>
   <td>field
   </td>
   <td><code>MyGroup mygroup()</code>
   </td>
  </tr>
  <tr>
   <td>Java (all)
   </td>
   <td>message
   </td>
   <td><code>MyGroup getMyGroup()</code>
   </td>
  </tr>
  <tr>
   <td>Python
   </td>
   <td>field
   </td>
   <td><code>mygroup</code>
   </td>
  </tr>
  <tr>
   <td>Go (all)
   </td>
   <td>field
   </td>
   <td><code>GetMygroup() *Foo_MyGroup</code>
   </td>
  </tr>
  <tr>
   <td>Dart V1
   </td>
   <td>field/message*
   </td>
   <td><code>get mygroup</code>
   </td>
  </tr>
  <tr>
   <td>upb **
   </td>
   <td>field
   </td>
   <td><code>Foo_mygroup()</code>
   </td>
  </tr>
  <tr>
   <td>Objective-c
   </td>
   <td>message
   </td>
   <td><code>MyGroup* myGroup</code>
   </td>
  </tr>
  <tr>
   <td>Swift
   </td>
   <td>message
   </td>
   <td><code>MyGroup myGroup</code>
   </td>
  </tr>
  <tr>
   <td>C#
   </td>
   <td>field/message*
   </td>
   <td><code>MyGroup Mygroup</code>
   </td>
  </tr>
</table>

\* This codegen difference was [caught](cl/611144002) during the implementation
and intentionally "fixed" in Edition 2023. \
\*\* This includes all upb-based runtimes as well (e.g. Ruby, Rust, etc.) \
† Extensions use field

In the Dart V1 implementation, we decided to intentionally introduce a behavior
change on editions upgrades. It was determined that this only affected a handful
of protos in google3, and could probably be manually fixed as-needed. Java's
handling changes the story significantly, since over 50% of protos in google3
produce generated Java code. Objective-C is also noteworthy since we open-source
it, and Swift because it's widely used in OSS and we don't own it.

While the editions upgrade is still non-breaking, it means that the generated
APIs could have very surprising spellings and may not be unique. For example,
using the same type for two delimited fields in the same containing message will
create two sets of generated APIs with the same name in some languages!

### Text Format

Our "official"
[draft specification](https://protobuf.dev/reference/protobuf/textformat-spec/)
of text-format explicitly states that group messages are encoded by the *message
name*, rather than the lowercases field name. A group `MyGroup` will be
serialized as:

```
MyGroup {
  ...
}
```

In C++, we always serialize the message name and have special handling to only
accept the message name in parsing. We also have conformance tests locking down
the positive path here (i.e. using the message name round-trip). The negative
path (i.e. failing to accept the field name) doesn't have a conformance test,
but C++/Java/Python all agree and there's no known case that doesn't.

To make things even stranger, for *extensions* (group fields extending other
messages), we always use the field name for groups. So as far as group
extensions are concerned, there's no problem for editions.

There are a few problems with non-extension group fields in editions:

*   Refactoring the message name will change any text-format output
*   New delimited fields will have unexpected text-format output, that *could*
    conflict with other fields
*   Text parsers will expect the message name, which is surprising and could be
    impossible to specify uniquely

## Recommendation

Clearly the end-state we want is for the field name to be used in all generated
APIs, and for text-format serialization/parsing. The only questions are: how do
we get there and can/should we do it in time for the 2023 release in 27.0 next
month?

We propose a combination of the alternatives listed below.
[Smooth Extension](#smooth-extension) seems like the best short-term path
forward to unblock the delimited migration. It *mostly* solves the problem and
doesn't require any new features. The necessary changes for this approach have
already been prepared, along with new conformance tests to lock down the
behavior changes.

[Global Feature](#global-feature) is a good long-term mitigation for tech debt
we're leaving behind with *Smooth Extension*. Ultimately we would like to remove
any labeling of fields by their type, and editions provides a good mechanism to
do this. Alternatively, we could implement [aliases](#aliases) and use that to
unify this old behavior and avoid a new feature. Either of these options will be
the next step after the release of 2023, with aliases being preferred as long as
the timing works out.

If we hit any unexpected delays, Nerf Delimited Encoding in 2023 (not available
externally) is the quickest path forward to unblock the release of edition 2023.
It has a lot of downsides though, and will block any migration towards delimited
encoding until edition 2024 has started rolling out.

## Alternatives

### Smooth Extension {#smooth-extension}

Instead of trying to change the existing behavior, we could expand the current
spec to try to cover both proto2 and editions. We would define a "group-like"
concept, which applies to all fields which:

*   Have `DELIMITED` encoding
*   Have a type corresponding to a nested message directly under its containing
    message
*   Have a name corresponding to its lowercased type name.

Note that proto2 groups will *always* be "group-like."

For any group-like field we will use the old proto2 semantics, whatever they are
today. Otherwise, we will treat them as regular fields for both codegen and
text-format. This means that *most* new cases of delimited encoding will have
the desired behavior, while *all* old groups will continue to function. The main
exception here is that users will see the unexpected proto2 behavior if they
have message/field names that *happen* to match.

While the old behavior will result in some unexpected capitalization when it's
hit, it's mostly safe. Because of 2 and 3 (and the fact that we disallow
duplicate field names), we can guarantee that in both codegen and text encoding
there will never be any conflicting symbols. There can never be two delimited
fields of the same type using the old behavior, and no other messages or fields
will exist with either spelling.

Additionally, we will update the text parsers to accept **both** the old
message-based spelling and the new field-based spelling for group-like fields.
This will at least prevent parsing failures if users hit this unexpected change
in behavior.

#### Pros

*   Fully supports old proto2 behavior
*   Treats most new editions fields correctly
*   Doesn't allow for any of the problematic cases we see today
*   By updating the parsers to accept both, we have a migration path to change
    the "wire"-format
*   Decoupled from editions launch (since it's a non-breaking change w/o a
    feature)

#### Cons

*   Requires coordinated changes in every editions-compatible runtime (and many
    generators)
*   Keeps the old proto2 behavior around indefinitely, with no path to remove it
*   Plants surprising edge case for users if they happen to name their
    message/fields a certain way

### Global Feature {#global-feature}

The simplest answer here is to introduce a new global message feature
`legacy_group_handling` to control all the changes we'd like. This will only be
applicable to group-like fields (see
[Smooth Extension](?tab=t.0#heading=h.blnhard1tpyx)). With this feature enabled,
these fields will always use their message name for text-format. Each
non-conformant language could also use this feature to gate the codegen rules.

#### Pros

*   Simple boolean to gate all the behavior changes
*   Doesn't require adding language features to a bunch of languages that don't
    have them yet
*   Uses editions to ratchet down the bad behavior

#### Cons

*   It's a little late in the game to be introducing new features to 2023
    (go/edition-lifetimes)
*   Requires coordinated changes in every editions-compatible runtime (and many
    generators)
*   The migration story for users is unclear. Overriding the value of this
    feature is both a "wire"-breaking and API-breaking change they may not be
    able to do easily.
*   With the feature set, users will still see all of the problems we have today

### Feature Suite

An extension of [Global feature](?tab=t.0#heading=h.mvtf74vplkdg) would be to
split the codegen changes out into separate per-language features.

#### Pros

*   Simple booleans to gate all the distinct behavior changes
*   Uses editions to ratchet down the bad behavior
*   Better migration story for users, since it separates API and "wire" breaking
    changes

#### Cons

*   Requires a whole slew of new language features, which typically have a
    difficult first-time setup
*   Requires coordinated changes in every editions-compatible runtime (and many
    generators)
*   Increases the complexity of edition 2023 significantly
*   With the features set, users will still see all of the problems we have
    today

### Nerf Delimited Encoding in 2023

A quick fix to avoid releasing a bad feature would be to simply ban the case
where the message and field names don't match. Adding this validation to protoc
would cover the majority of cases, although we might want additional checks in
every language that supports dynamic messages.

This is a good fallback option if we can't implement anything better before 27.0
is released. It allows us to release editions in a reasonable state, where we
can fix these issues and release a more functional `DELIMITED` feature in 2024.

#### Pros

*   Unblocks editions rollout
*   Easy and safe to implement
*   Avoids rushed implementation of a proper fix
*   Avoids runtime issues with text format
*   Avoids unexpected build breakages post-editions (e.g. renaming the nested
    message)

#### Cons

*   We'd still be releasing a really bad feature. Instead of opening up new
    possibilities, it's just "like groups but worse"
*   We couldn't fix this in 2023 without potential version skew from third party
    plugins. We'd likely have to wait until edition 2024
*   Might requires coordinated changes in a lot of runtimes
*   Doesn't unblock our effort to roll out delimited

### Rename Fields in Editions

While it might be tempting to leverage the edition 2023 upgrade as a place we
can just rename the group field, that doesn't actually work (e.g. rename
`mygroup` to `my_group`). Because so many runtimes already use the *field name*
in generated APIs, they would break under this transformation.

#### Pros

*   Works really well for text-format and some languages

#### Cons

*   Turns 2023 upgrade into a breaking change for many languages

### Aliases {#aliases}

We've discussed aliases a lot mostly in the context of `Any`, but they would be
useful for any encoding scheme that locks down field/message names. If we had a
fully implemented alias system in place, it would be the perfect mitigation
here. Unfortunately, we don't yet and the timeline here is probably too tight to
implement one.

#### Pros

*   Fixes all of the problems mentioned above
*   Allows us to specify the old behavior using the proto language, which allows
    it to be handled by Prototiller

#### Cons

*   We want this to be a real fully thought-out feature, not a hack rushed into
    a tight timeline

### Do Nothing

Doing nothing doesn't actually break anyone, but it is embarrassing.

#### Pros

*   Easy to do

#### Cons

*   Releases a horrible feature full of foot-guns in our first edition
*   Doesn't unblock our effort to roll out delimited
