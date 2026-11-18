# Edition Naming

**Author:** [@mkruskal-google](https://github.com/mkruskal-google)

**Approved:** 2023-08-25

## Background

One of the things [Life of an Edition](life-of-an-edition.md) lays out is a very
loose scheme for naming editions. It defines an ordering rule and "." delimiter,
but otherwise imposes no constraints on the edition name. By *convention*, we
decided to use the year followed by an optional revision number.

One of the decisions in
[Editions: Life of a FeatureSet](editions-life-of-a-featureset.md) was that
feature resolution would, at least partially, need to be duplicated across every
supported language.

## Problem Description

As discussed in *Life of a FeatureSet*, the minimal operations we need to
duplicate are edition comparison and proto merging. While edition comparison
isn't *very* complicated today, it has *a lot* of edge cases we may miss due to
the loose constraints on edition names. It would also be really nice if we could
reduce it down to a lexicographical string comparison, which can be easily done
in any language.

There's also a very real Hyrum's law risk when we permit an infinite number of
edition names that we never expect to exist in practice. For example, `2023.a`
is currently valid, and its relative ordering to `2023.10` (for example) is not
obvious. Notably, our initial editions tests used an edition `very-cool`, which
doesn't seem like something we need to support. Ideally, our edition names would
be as simple as possible, with enforceable and documented constraints.

There has been a lot of confusion when looking at the interaction between
editions and releases. Specifically, the fact that editions *look* like calver
numbers has led to us calling revisions "patch editions", which suggests that
they're bug fixes to an earlier one. This was not the original intention though,
which to summarize was:

*   **Editions are strictly time-ordered**. Revisions were simply a mechanism to
    release more than one edition per calendar year, but new editions can't be
    inserted into earlier slots.

*   **New editions can be added at any time**. As long as they're ordered
    *after* the pre-existing ones, this is a non-breaking change and can be done
    in a patch release.

*   **New features can be added at any time without changing the edition**.
    Since they're a non-breaking change by definition, this can be done in a
    patch release.

*   **Features can only be deleted in a breaking release**. The editions model
    doesn't support the deletion of features, which is always a breaking change.
    This will only be done in a major version bump of protobuf.

*   **Feature defaults can only be changed in a new edition**. Once the default
    is chosen for a feature, we can only change it by releasing a new edition
    with an updated default. This is a non-breaking change though, and can be
    done in patch releases.

## Recommended Solution

We want the following properties out of editions:

*   Discrete allowable values controlled by us
*   Easily comparable
*   Cross-language support
*   Relatively small set (<100 for the next century)
*   Slow growth (~once per year)

The simplest solution here is to just make an `Edition` enum for specifying the
edition. We will continue to use strings in proto files, but the parser will
quickly convert them and all code from then on will treat them as enums. This
way, we will have a centralized cross-language list of all valid editions:

```
enum Edition {
  EDITION_UNKNOWN = 0;
  EDITION_2023 = 1;
  EDITION_2024 = 2;
  // ...
}
```

We will document that these are *intended* to be comparable numerically for
finding the time ordering of editions. Proto files will specify the edition in
exactly the same way as in the original decision:

```
edition = "2023";
```

Ideally, this would be an open enum to avoid ever having the edition thrown into
the unknown field set. However, since it needs to exist in `descriptor.proto`,
we won't be able to make it open until the end of our edition zero migration.
Until then, we can take advantage of the fact that `syntax` gets set to
`editions` by the parser when an edition is found. In this case, an unset
edition should be treated as an error. Once we migrate to an open enum we can
replace this with a simpler validity check.

Additionally, we've decided to punt on the question of revisions and just not
allow them for now. If we ever need more than that it means we've made a huge
mistake, and we can bikeshed on naming at that time (e.g. `EDITION_2023_OOPS`).
We will plan to have exactly one edition every year.

#### Pros

*   Edition comparison becomes **even simpler**, as it's just an integer
    comparison

    *   Feature resolution becomes trivial in every language

*   Automatic rejection of unknown editions.

    *   This includes both future and dropped editions, and also unknown
        revisions
    *   Other solutions would need custom logic in protoc to enforce these

*   Doesn't look like calver, avoiding that confusion

*   Lack of revisions simplifies our documentation and makes editions easier to
    understand/maintain

#### Cons

*   Might prove to be a challenge when we go to migrate `descriptor.proto` to
    editions

*   Might be a bit tricky to implement in the parser (but Prototiller does this
    just fine)

## Considered Alternatives

### Edition Enums

Instead of using a string, editions could be represented as an enum. This would
give us a lot of enforcement for free and be maximally constraining. For
example:

```
enum Edition {
  E2023  = 1;
  E2023A = 2;
  E2024  = 3;
}
```

#### Pros

*   Edition comparison becomes **even simpler**, as it's just an integer
    comparison

*   Arbitrary number of revisions within a year

*   Automatic rejection of unknown editions (with closed enums).

    *   This includes both future and dropped editions, and also unknown
        revisions
    *   Other solutions would need custom logic in protoc to enforce these

*   Doesn't look like calver, avoiding that confusion

#### Cons

*   Edition specification in every proto file becomes arguably less readable.
    `edition = "2023.1"` is easier to read than `edition = E2023A`.

*   Might require parser changes to get `descriptor.proto` onto editions

*   This is a pretty big change and will require documentation/comms updates

*   Plugin owners can't pre-release upcoming features

    *   Do we even want to allow this? It could help coordinating editions
        releases

#### Neutral

*   Edition must be strictly time-ordered. We can't go back and add a revision
    to an older revision, but the original plan didn't really allow for this
    anyway.

*   Edition ordering is not directly linked to the name *at all*. We'd have to
    write a reflection test that enforces that they're strictly increasing.

### Truncated Revisions

The simplest solution that's consistent with the original plan would be to limit
ourselves to no more than 9 intermediate editions per year. This means editions
would either be a simple year (e.g. `2023`), or have a single digit revision
number (e.g. `2024.3`). The revision number would be constrained to `(0,9]` and
the year would have to be an integer `>=2023`.

With these constraints in place, edition ordering becomes a simple
lexicographical string ordering.

#### Pros

*   Tightly constrains edition names and avoids unexpected edge cases
*   Edition comparison code is easily duplicated (basic string comparison)
*   Removes ambiguity around revision 0 (e.g. `2023.0` isn't allowed)

#### Cons

*   Limits us to no more than 10 editions per year. This *seems* reasonable
    today, and since protoc is the one enforcing this we can always revisit it
    later. As long as the ordering stays lexicographical we can expand it
    arbitrarily.

### Fixed length editions

This is similar to the recommended solution, but instead of allowing year
editions we would always start with a `.0` release. This means that Edition Zero
would become `2023.0`. This has the same pros/cons as the solution above, with
the addition of:

#### Pros

*   Makes revisions less jarring. Users won't be confused by `2023.1` if they're
    used to `2023.0`.

#### Cons

*   We've already published comms and documentation calling the first edition
    `2023`. We'd have to go update those and communicate the change.
*   We have no real use-case for revisions yet, and this adds complexity to the
    "typical" case.

### Edition Messages

Instead of using a string, editions could be represented as a message. This
would allow us to model the constraints better. For example:

```
message Edition {
  uint32 year = 1;
  uint32 revision = 2;
}
```

#### Pros

*   The schema itself would automatically enforce a lot of our constraints
*   Arbitrary number of revisions would be allowed
*   Can't accidentally use string comparison

#### Cons

*   Custom comparison code would still be needed in every language (marginally
    more expensive than string comparison)
*   We would either need to convert the edition string into this message during
    parsing or radically change the edition specification

### Do Nothing

#### Pros

*   Easier today, since we don't have to do anything

#### Cons

*   Harder tomorrow when we have to duplicate this comparison code into every
    language
*   Leaves us open to Hyrum's law and unexpected abuse
