# Editions: Feature Extension Layout

**Author:** [@mkruskal-google](https://github.com/mkruskal-google),
[@zhangskz](https://github.com/zhangskz)

**Approved:** 2023-08-23

## Background

"[What are Protobuf Editions](what-are-protobuf-editions.md)" lays out a plan
for allowing for more targeted features not owned by the protobuf team. It uses
extensions of the global features proto to implement this. One thing that was
left a bit ambiguous was *who* should own these extensions. Language, code
generator, and runtime implementations are all similar but not identical
distinctions.

"Editions Zero Feature: utf8_validation" (not available externally, though a
later version,
"[Editions Zero: utf8_validation Without Problematic Options](editions-zero-utf8_validation.md)"
is) is a recent plan to add a new set of generator features for utf8 validation.
While the sole feature we had originally created (`legacy_closed_enum` in Java
and C++) didn't have any ambiguity here, this one did. Specifically in Python,
the current behaviors across proto2/proto3 are distinct for all 3
implementations: pure python, Python/C++, Python/upb.

## Overview

In meetings, we've discussed various alternatives, captured below. The original
plan was to make feature extensions runtime implementation-specific (e.g. C++,
Java, Python, upb). There are some notable complications that came up though:

1.  **Polyglot** - it's not clear how upb or C++ runtimes should behave in
    multi-language situations. Which feature sets do they consider for runtime
    behaviors? *Note: this is already a serious issue today, where all proto2
    strings and many proto3 strings are completely unsafe across languages.*

2.  **Shared Implementations** - Runtimes like upb and C++ are used as backing
    implementations of multiple other languages (e.g. Python, Rust, Ruby, PHP).
    If we have a single set of `upb` or `cpp` features, migrating to those
    shared implementations would be more difficult (since there's no independent
    switches per-language). *Note: this is already the situation we're in today,
    where switching the runtime implementation can cause subtle and dangerous
    behavior changes.*

Given that we only have two behaviors, and one of them is unambiguous, it seems
reasonable to punt on this decision until we have more information. We may
encounter more edge cases that require feature extensions (and give us more
information) during the rollout of edition zero. We also have a lot of freedom
to re-model features in later editions, so keeping the initial implementation as
simple as possible seems best (i.e. Alternative 2).

## Alternatives

### Alternative 1: Runtime Implementation Features

Features would be per-runtime implementation as originally described in
"Editions Zero Feature: utf8_validation." For example, Protobuf Python users
would set different features depending on the backing implementation (e.g.
`features.(pb.cpp).<feature>`, `features.(pb.upb).<feature>`).

#### Pros

*   Most consistent with range of behaviors expressible pre-Editions

#### Cons

*   Implementation may / should not be obvious to users.
*   Lack of levers specifically for language / implementation combos. For
    example, there is no way to set Python-C++ behavior independently of C++
    behavior which may make migration harder from other Python implementations.

### Alternative 2: Generator Features

Features would be per-generator only (i.e. each protoc plugin would own one set
of features). This was the second decision we made in later discussions, and
while very similar to the above alternative, it's more inline with our goal of
making features primarily for codegen.

For example, all Python implementations would share the same set of features
(e.g. `features.(pb.python).<feature>`). However, certain features could be
targeted to specific implementations (e.g.
`features.(pb.python).upb_utf8_validation` would only be used by Python/upb).

#### Pros

*   Allows independent controls of shared implementations in different target
    languages (e.g. Python's upb feature won't affect PHP).

#### Cons

*   Possible complexity in upb to understand which language's features to
    respect. UPB is not currently aware of what language it is being used for.
*   Limits in-process sharing across languages with shared implementations (e.g.
    Python upb, PHP upb) in the case of conflicting behaviors.
    *   Additional checks may be needed.

### Alternative 3: Migrate to bytes

Since this whole discussion revolves around the utf8 validation feature, one
option would be to just remove it from edition zero. Instead of adding a new
toggle for UTF8 behavior, we could simply migrate everyone who doesn't enforce
utf8 today to `bytes`. This would likely need another new *codegen* feature for
generating byte getters/setters as strings, but that wouldn't have any of the
ambiguity we're seeing today.

Unfortunately, this doesn't seem feasible because of all the different behaviors
laid out in "Editions Zero Feature: utf8_validation." UTF8 validation isn't
really a binary on/off decision, and it can vary widely between languages. There
are many cases where UTF8 is validated in **some** languages but not others, and
there's also the C++ "hint" behavior that logs errors but allows invalid UTF8.

**Note:** This could still be partially done in a follow-up LSC by targeting
specific combinations of the new feature that disable validation in all relevant
languages.

#### Pros

*   Punts on the issue, we wouldn't need any upb features and C++ features would
    all be code-gen only
*   Simplifies the situation, avoids adding a very complicated feature in
    edition zero

#### Cons

*   Not really possible given the current complexity
*   There are O(10M) proto2 string fields that would be blindly changed to bytes

### Alternative 4: Nested Features

Another option is to allow for shared feature set messages. For example, upb
would define a feature message, but *not* make it an extension of the global
`FeatureSet`. Instead, languages with upb implementations would have a field of
this type to allow for finer-grained controls. C++ would both extend the global
`FeatureSet` and also be allowed as a field in other languages.

For example, python utf8 validation could be specified as:

We could have checks during feature validation that enforce that impossible
combinations aren't specified. For example, with our current implementation
`features.(pb.python).cpp` should always be identical to `features.(pb.cpp)`,
since we don't have any mechanism for distinguishing them.

#### Pros

*   Much more explicit than options 1 and 2

#### Cons

*   Maybe too explicit? Proto owners would be forced to duplicate a lot of
    features
