# Edition Lifetimes

Now that Edition Zero is complete, we need to re-evaluate what the lifetimes of
features and editions look like going forward.

## Background

The implementation of editions today was based largely on
[Protobuf Editions Design: Features](protobuf-editions-design-features.md) and
[Life of an Edition](life-of-an-edition.md) (among other less-relevant docs).
Specifically, the latter one takes a strong stance on the lifetimes of both
editions and features. Many of the ideas around editions have since been
simplified in [Edition Naming](edition-naming.md), where we opted for a stricter
naming scheme owned and defined by us. In the process of rolling out editions to
various protoc plugins and planning for edition 2024, it's become clear that we
may need to re-evaluate the feature lifetimes as well.

*Editions: Life of a Feature* (not available externally) is an alternate vision
to *Life of an Edition*, which tries to put tighter constraints on how features
and editions interact. It also predicted many of the problems we face now, and
proposes a possible solution.

## Overview

Today, features and editions are largely disconnected from each other. We have a
set of features that govern various behaviors, and they can all be used in any
edition. Each edition is simply a distinct set of defaults for **every**
feature. Users can override the value of any released feature in any edition.
Each generator and protoc itself all advertise a range of editions they support,
and will reject any protos encountered outside that range.

This system does have the nice consequence that behavior-preserving editions
upgrades can always be performed (with respect to the most current proto
language). It separates editions from breaking changes, and means that we only
need to worry about one versioning scheme (our OSS release).

While this all works fine in Edition 2023 where we only have a single edition,
it poses a number of problems going forward. There are three relevant events in
the lifetime of both editions and features, introduction, deprecation, and
removal. Deprecation is essentially just a soft removal to give users adequate
warning, and the same problems apply to both.

### Introducing an Edition

Because every generator plugin (and protoc) advertises its edition support
window, introducing a new Edition is well-handled today. We get to enjoy all the
same benefits we saw rolling out Edition 2023 in every subsequent edition (e.g.
we can make radical language changes gated on edition).

### Dropping an Edition

Dropping support for an edition doesn't really mean that much today. We *could*
do it simply by bumping up the minimum supported edition of a binary in a
breaking release. However, that would have no relation to our feature support,
and at best would allow us to clean up some parser code branching on editions.
This code would always be tied to language changes we made in the introduction
of a new edition, where we could finalize them with the removal of an edition.

### Introducing a Feature

Whenever we introduce a new feature, we need to make sure to specify its
defaults for **every** edition (protoc enforces that every edition has a known
set of defaults). It will also immediately be overridable in every edition. This
means that pre-existing binaries that have declared support for an old edition
may suddenly be presented with protos that override a feature they can't
possibly know about.

We faced this problem when introducing the new `string_type` feature as part of
*New String APIs* (not available externally). Our solution at the time was to
create some ad hoc validation that prohibited overriding this feature in Edition
2023 until we were ready to release it. This solution doesn't work in general
though, where in OSS we can have arbitrarily old binaries floating around (and
even in google3 we can easily have up to six-month-old binaries within the build
horizon). These old binaries wouldn't have that validation layer, and would
happily process Edition 2023 files with `string_type` overrides, despite not
knowing how to properly treat that feature.

### Dropping a Feature

On the other end of the spectrum, we need a way to deprecate and then remove
support for features. Given that we expect most features to remain for many
years, we haven't been forced to consider this situation too much. The current
plan of record though, is that we would do this by first marking the feature
field definition `deprecated`, and then remove it entirely in a breaking release
(and total burndown in google3).

The problem with this plan is that it creates a lot of complexity for users
trying to understand our support guarantees. They'll need to track the lifetime
of **every** feature they use, and also difficult-to-predict interactions
between different versions of protoc and its plugins. If we drop a global
feature in protoc, some plugins may still expect to see that feature and become
broken, while others may not care and still work.

## Recommendation

### Feature Lifetimes

We recommend adding four new field options to be used in feature specifications:
`edition_introduced`, `edition_deprecated`, `deprecation_warning`, and
`edition_removed`. This will allow every feature to specify the edition it was
introduced in, the edition it became deprecated in, when we expect to remove it
(deprecation warnings), and the edition it actually becomes removed in.

We will also add a new special edition `EDITION_LEGACY`, to act as a placeholder
for "infinite past". For editions earlier than `edition_introduced`, the default
assigned to `EDITION_LEGACY` will be assigned and should always signal the *noop
behavior that predated the feature*. Proto files will not be allowed to override
this feature without upgrading to a newer edition. Deprecated features can get
special treatment beyond the regular `deprecated` option, and a custom warning
signaling that they should be migrated off of. For editions later than
`edition_removed`, the last edition default will continue to stay in place, but
overrides will be disallowed in proto files.

For example, a hypothetical feature might look like:

```
optional FeatureType do_something = 2 [
    retention = RETENTION_RUNTIME,
    targets = TARGET_TYPE_FIELD,
    targets = TARGET_TYPE_FILE,
    feature_support {
      edition_introduced = EDITION_2023,
      edition_deprecated = EDITION_2025,
      deprecation_warning = "Feature do_something will be removed in edition 2027",
      edition_removed = EDITION_2027,
    }
    edition_defaults = { edition: EDITION_LEGACY, value: "LEGACY" }
    edition_defaults = { edition: EDITION_2023, value: "INTERMEDIATE" }
    edition_defaults = { edition: EDITION_2024, value: "FUTURE" }
];
```

Before edition 2023, this feature would always get a default of `LEGACY`, and
proto files would be prohibited from overriding it. In edition 2023, the default
would change to `INTERMEDIATE` and users could override it to the old default or
the future behavior. In edition 2024 the default would change again to `FUTURE`,
and in edition 2025 any overrides of that would start emitting warnings. In
edition 2027 we would prohibit overriding this feature, and the behavior would
always be `FUTURE`.

### Edition Lifetimes

By tying feature lifetimes to specific editions, it gives editions a lot more
meaning. We will still limit this to breaking releases, but it means that
**all** of the editions-related breaking changes come from this process. When we
drop an edition, breaking changes will always come from the removal of
previously deprecated features. By regularly dropping support for editions, we
will be able to gradually clean up our codebase.

#### Edition Upgrades

A consequence of this design is that edition upgrades could now become
potentially breaking. Any proto files using deprecated features could be broken
by bumping its edition to one where the feature has been removed. Within
google3, we would need to completely burn down all deprecated uses before we can
remove the feature.

This is not a substantial change on our end from the existing situation though,
where we'd still need to remove all uses before removing it. The key difference
is that we have the *option* to allowlist some people to stay on an older
edition while still moving the rest of google3 forward. We would also be able to
continue testing removed features by allow-listing dedicated tests to stay on
old editions.

#### Garbage Collection

Another consequence of this is that we can't actually clean up feature-related
code until every edition before its `edition_removed` declaration has been
dropped. This ties feature support directly to edition support, especially in
OSS where we can't forcibly upgrade protos to the latest edition.

#### Predictability

The main win with this strategy is that it clarifies our guarantees and makes
our library more predictable. We can guarantee that a proto file at a specific
edition will not see any behavioral changes unless we:

1.  Make a breaking change outside the editions framework.
2.  Drop the edition the proto file uses.

We can also guarantee that as long as users stay away from deprecated features,
they will still be able to upgrade to the next edition without any changes.

### Implementation

Fortunately, this design would be **very** easy to implement right now. We
simply need to add the new field options and the new placeholder edition, and
then implement new validation in protoc. Because the two error conditions (using
a feature outside its existence window) and the warning (using a deprecated
feature) only trigger on *overridden* features, protoc already has all the
information it needs. Generator feature extensions must be imported to be
overridden, so the problem of protoc not knowing feature defaults doesn't come
into play at all.

If we wait until edition 2024 has been released, the situation would be a bit
more difficult to unravel. Any new features added in 2024 would be usable from
2023, so we'd have to either intentionally backport support or remove all of
those uses before enabling the validation layer. Therefore, the recommendation
is to implement this ASAP, before we start rolling out 2024.

#### Runtimes with Dynamic Messages

None of the generators where editions have already been rolled out require any
changes. We likely will want to add validation layers to runtimes that support
dynamic messages though, to make sure there are no invalid descriptors floating
around. Since they all have access to protoc's compiled defaults IR, we can pack
as much information in there as possible to minimize duplication. Specifically,
we will add two new `FeatureSet` fields to `FeatureSetEditionDefault` in
addition to the existing `features` field.

*   overridable_features - The default values that users **are** allowed to
    override in a given edition
*   fixed_features - The default values that users **are not** allowed to
    override in a given edition

We will keep the existing `features` field as a migration tool, to avoid
breaking plugins and runtimes that already use it to calculate defaults. We can
strip it from OSS prior to the 27.0 release though, and remove it once everyone
has been migrated.

In order to calculate the full defaults of any edition, each language will
simply need to merge the two `FeatureSet` objects. The advantage to splitting
them means that we can fairly easily implement validation checks in every
language that needs it for dynamic messages. The algorithm is as follows, for
some incoming unresolved `FeatureSet` user_features:

1.  Strip all unknown fields from user_features
2.  Strip all extensions from user_features that the runtime doesn't handle
3.  merged_features := user_features.Merge(overridable_defaults)
4.  assert merged_features == overridable_defaults

This will work as long as every feature is a scalar value (making merge a simple
override). We already ban oneof and repeated features, and we plan to ban
message features before the OSS release.

Note, that there is a slight gap here in that we perform no validation for
features owned by *other* languages. Dynamic messages in language A will naively
be allowed to specify whatever language B features they want. This isn't
optimal, but it is in line with our current situation where validation of
dynamic messages is substantially more permissive than descriptors processed by
protoc.

On the other hand, owners of language A will have the *option* of easily adding
validation for language B's features, without having to reimplement the
reflective inspection of imports that protoc does. This can be done by simply
adding those features to the compilation of the defaults IR, and then not
stripping those extensions during validation. This will have the effect of tying
the edition support window of A to that of B though, and A won't be able to
extend its maximum edition until B does (at least for dynamic messages). For
generators in a monorepo like Protobuf's this seems fine, but may not be
desirable elsewhere.

### Patching Old Editions

In [Edition Naming](edition-naming.md) we decided to drop the idea of "patch"
editions, because editions were always forward and backward compatible. We would
only ever need multiple editions in a year if somehow we managed to speed up the
rollout process and wanted faster turnaround. This changes those assumptions
though, since now editions are neither forward-compatible (new features don't
work in old editions) or backward-compatible (old features may not work in new
editions).

Hypothetically, if there were a bug in the editions layer itself we may require
a "patch" edition to safely roll out a fix. For example, imagine we discover
that our calculation of edition defaults is broken in edition 2023 and we had
accidentally released it. If we've already fixed the issue and released edition
2024 as well, we can't just create a `2023A` "patch" to fix the issue because
editions are represented as integers (and 2023 and 2024 are adjacent). We would
want to release some kind of fix for people still on edition 2023 though, so
that they can minimally upgrade before 2024 (which may be a breaking edition).

What we could do in this situation (if it ever arises) is introduce a new
integer field in `FileDescriptorProto` called `edition_patch`. It would take
some work to fit this into feature resolution and roll it out to every plugin,
but given that we've hidden the edition from most users
([Editions Feature Visibility](editions-feature-visibility.md)) it shouldn't be
too bad. As long as patches never introduce or remove features or change their
defaults, protoc and plugins can always use the latest patch they know about to
represent that edition.

### Documentation

As part of this change, we need to document all of this publicly for
plugin/runtime owners. We should create a new topic in
https://protobuf.dev/editions/ to cover all of this, along with other relevant
details they'd need to know.

## Alternatives

### Continue as usual

The only real alternative here is to make no change, which has all of the
problems listed in the overview of this topic.

#### Pros

*   Requires no effort short-term
*   Editions upgrades will **never** be breaking changes

#### Cons

*   Likely to cause problems as soon as edition 2024
*   Introducing new features is dangerous and unpredictable
*   Dropping features affects all editions simultaneously
*   The features supported in each edition can vary between protobuf releases
*   High cognitive overhead for our users. They'd need to track the progress of
    every feature individually across releases.

### Full Validation for Dynamic Messages

None of the generators where editions have already been rolled out require any
changes. We will need to add validation layers to runtimes that support dynamic
messages though, to make sure there are no invalid descriptors floating around.
Any runtime that supports dynamic messages should have reflection, and the same
reflection-based algorithm will need to be duplicated everywhere. For each
`FeatureSet` specified on a descriptor:

```
absl::Status Validate(Edition edition, Message& features) {
  std::vector<const FieldDescriptor*> fields;
  features.GetReflection()->ListFields(features, &fields);
  for (const FieldDescriptor* field : fields) {
    // Recurse into message extension.
    if (field->is_extension() &&
        field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      CollectLifetimeResults(
          edition, message.GetReflection()->GetMessage(message, field),
          results);
      continue;
    }

    // Skip fields that don't have feature support specified.
    if (!field->options().has_feature_support()) continue;

    // Check lifetime constrains
    const FieldOptions::FeatureSupport& support =
        field->options().feature_support();
    if (edition < support.edition_introduced()) {
      return absl::FailedPrecondition(absl::StrCat(
          "Feature ", field->full_name(), " wasn't introduced until edition ",
          support.edition_introduced()));
    }
    if (support.has_edition_removed() && edition >= support.edition_removed()) {
      return absl::FailedPrecondition(absl::StrCat(
          "Feature ", field->full_name(), " has been removed in edition ",
          support.edition_removed()));
    } else if (support.has_edition_deprecated() &&
               edition >= support.edition_deprecated()) {
      ABSL_LOG(WARNING) << absl::StrCat(
          "Feature ", field->full_name(), " has been deprecated in edition ",
          support.edition_deprecated(), ": ", support.deprecation_warning());
    }
  }
}
```

#### Pros

*   Prevents any feature lifetime violations for any language, in any language
*   Easier to understand
*   Less error-prone
*   Easy to test with fake features

#### Cons

*   Only works post-build, which requires a huge amount of code in every
    language to walk the descriptor tree applying these checks
*   Performance concerns, especially in upb
*   Duplicates protoc validation, even though most languages perform
    significantly looser checks on dynamic messages

#### Pros

*   Minimizes the amount of reflection needed

#### Cons

*   Can't validate extensions for languages we don't know about, since they're
    not built into the binary
*   Potential version skew between pool and runtime features
*   Requires reflection stripping unexpected fields
*   Difficult to understand the algorithm from the code
