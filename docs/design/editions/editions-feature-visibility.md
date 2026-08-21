# Editions Feature Visibility

**Authors:** [@mkruskal-google](https://github.com/mkruskal-google)

**Approved:** 2023-09-08

## Background

While [Editions: Life of a FeatureSet](editions-life-of-a-featureset.md) handles
how we propagate features *to* runtimes, what's left under-specified is how the
runtimes should expose features to their users. *Exposing Editions Feature Sets*
(not available externally) was an initial attempt to cover both these topics
(specifically the C++ API section), but much of it has been redesigned since.
This is a much more targeted document laying out how features should be treated
by runtimes.

## Problem Description

There are two main concerns from a runtime's perspective:

1.  **Direct access to resolved features protos** - While runtime decisions
    *should* be made based on the data in these protos, their struct-like nature
    makes them very rigid. Once users start to depend on the proto API, it makes
    it very difficult for us to do internal refactoring. These protos are also
    naturally structured based on how feature *specification* is done in proto
    files, rather than the actual behaviors they represent. This makes it
    difficult to guarantee that complex relationships between features and other
    conditions are being uniformly handled.

2.  **Accidental use of unresolved features** - Unresolved features represent a
    clear foot-gun for users, that could also cause issues for us. Since they
    share the same type as resolved features, it's not always easy to tell the
    two apart. If runtime decisions are made using unresolved features, it's
    very plausible that everything will work as expected in a given edition by
    coincidence. However, when the proto's edition is bumped, it will very
    likely break this code unexpectedly.

Some concrete examples to help illustrate these concerns:

*   **Remodeling features** - We've bounced back and forth on how UTF8
    validation should be modeled as a feature. None of the proposals resulted in
    any functional changes, since edition zero preserves all proto2/proto3
    behavior, the question was just about what features should be used to
    control them. While the `.proto` file large-scale change to bump them to the
    next edition containing these changes is unavoidable, we'd like to avoid
    having to update any code simultaneously. If everyone is directly inspecting
    the `utf8_validation` feature, we would need to do both.

*   **Incomplete features** - Looking at a feature like `packed`, it's really
    more of a contextual *suggestion* than a strict rule. If it's set at the
    file level, **all** fields will have the feature even though only packable
    ones will actually respect it. Giving users direct access to this feature
    would be problematic, because they would *also* need to check if it's
    packable before making decisions based on it. Field presence is an even more
    complicated example, where the logic we want people making runtime decisions
    based on is distinct from what's specified in the proto file.

*   **Optimizations** - One of the major considerations in *Exposing Editions
    Feature Sets* (not available externally) was whether or not it would be
    possible to reduce the cost of editions later. Every descriptor is going to
    contain two separate features protos, and it's likely this will end up
    getting expensive as we roll out edition zero. We could decide to optimize
    this by storing them as a custom class with a much more compact memory
    layout. This is similar to other optimizations we've done to descriptor
    classes, where we have the freedom to *because* we don't generally expose
    them as protos.

*   **Bumpy Edition Large-scale Change** - The proto team is going to be
    responsible for rolling out the next edition to internal Google repositories
    every year (at least 80% of it per our churn policy). We *expect* that
    people are only making decisions based on resolved features, and therefore
    that Prototiller transformations are behavior-preserving (despite changing
    the unresolved features). If people have easy access to unresolved features
    though, we can expect a lot of Hyrum's law issues slowing down these
    large-scale changes.

## Recommended Solution

We recommend a conservative approach of hiding all `FeatureSet` protos from
public APIs whenever possible. This means that there should be no public
`features()` getter, and that features should be stripped from any descriptor
options. All `options()` getters should have an unset features field. Instead,
helper methods should be provided on the relevant descriptors to encapsulate the
behaviors users care about. This has already been done for edition zero features
(e.g. `has_presence`, `requires_utf8_validation`, etc), and we should continue
this model.

The one notable place where we *can't* completely hide features is in
reflection. Most of our runtimes provide APIs for converting descriptors back to
their original state at runtime (e.g. `CopyTo` and `DebugString` in C++). In
order to give a faithful representation of the original proto file in these
cases, we should include the **unresolved** features here. Given how inefficient
these methods are and how hard the resulting protos are to work with, we expect
misuse of these unresolved features to be rare.

**Note:** While we may need to adjust this approach in the future, this is the
one that gives us the most flexibility to do so. Adding a new API when we have
solid use-cases for it is easy to do. Removing an existing one when we decide we
don't want it has proven to be very difficult.

### Enforcement

While we make the recommendation above, ultimately this decision should be up to
the runtime owners. Outside of Google we can't enforce it, and the cost would be
a worse experience for *their* users (not the entire protobuf ecosystem). Inside
of Google, we should be more diligent about this, since the cost mostly falls on
us.

### μpb

One notable standout here is μpb, which is a runtime *implementation*, but not a
full runtime. Since μpb only provides APIs to the wrapping runtime in a target
language, it's free to expose features anywhere it wants. The wrapping language
should be responsible for stripping them out where appropriate.

#### Pros

*   Prevents any direct access to resolved feature protos

    *   Gives us freedom to do internal refactoring
    *   Allows us to encapsulate more complex relationships
    *   Users don't have to distinguish between resolved/unresolved features

*   Limits access to unresolved features

    *   Accidental usage of these is less likely (especially considering the
        above)

*   This should be easy to loosen in the future if we find a real use-case for
    `features()` getters.

*   More inline with our descriptor APIs, which wrap descriptor protos but
    aren't strictly 1:1 with them. Options are more an exception here, mostly
    due to the need to expose extensions.

#### Cons

*   There's no precedent for modifying `options()` like this. Up until now it
    represented a faithful clone of what was specified in the proto file.

*   Deciding to loosen this in the future would be a bit awkward for
    `options()`. If we stop stripping it, people will suddenly start seeing a
    new field and Hyrum's law might result in breakages.

*   Requires duplicating high-level feature behaviors across every language. For
    example, `has_presence` will need to be implemented identically in every
    language. We will likely need some kind of conformance test to make sure
    these all agree.

## Considered Alternatives

### Expose Features

This is the simplest implementation, and was the initial approach taken in
prototypes. We would just have public `features()` getters in our descriptor
APIs, and keep the unresolved features in `options()`.

#### Pros

*   Very easy to implement

#### Cons

*   Doesn't solve any of the problems laid out above

*   Difficult to reverse later

### Hide Features in Generated Options

This is a tweak of the recommended solution where we add a hack to the generated
options messages. Instead of just stripping the features out and leaving an
empty field, we could give the `features` fields "package-scoped" visibility
(e.g. access tokens in C++). We would still strip them, but nobody outside of
our runtimes could even access them to see that they're empty. This eliminates
the Hyrum's law concern above.

#### Pros

*   Resolves one of the cons in the recommended approach.

#### Cons

*   We'd have to do this separately for each runtime, meaning specific hacks in
    *every* code generator

*   No clear benefit. This only helps **if** we decide to expose features and
    **if** a bunch of people start depending on the fact that `features` are
    always empty.

### ClangTidy warning Options Features

Similar to the above alternative, but leverages ClangTidy to warn users against
checking `options().features()`.

#### Pros

*   Resolves one of the cons in the recommended approach.

#### Cons

*   Doesn't work in every language

*   Doesn't work in OSS
