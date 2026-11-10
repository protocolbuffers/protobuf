# Agent Instructions for Protobuf

Protobuf is core infrastructure: it is used by a huge number of projects, both
at Google and across the open source ecosystem. The quality and correctness bar
here is **much higher** than in most projects. A change that would be fine
elsewhere may still be rejected here.

See [CONTRIBUTING.md](CONTRIBUTING.md) for the full contribution process.

## Quality Bar

-   **Correctness first**: Every change must be obviously correct to a reviewer.
    Think through edge cases, undefined behavior, memory safety, lifetimes,
    concurrency, and unusual inputs (malformed wire data, deep recursion, very
    large messages, unknown fields, extensions).
-   **Code health is a requirement, not a nice-to-have**: A change that adds
    complexity, special cases, duplication, or harder-to-understand code will be
    rejected even if it is correct and benchmarks well. Prefer the simplest
    design that works, and match the surrounding code's patterns.
-   **Small, focused changes**: One logical change per pull request
    ([Small PRs](https://google.github.io/eng-practices/review/developer/small-cls.html)).
    Split refactors from behavior changes. The pull request description must
    explain *what* changes and *why*
    ([Writing good PR descriptions](https://google.github.io/eng-practices/review/developer/cl-descriptions.html)).
-   **Meaningful tests**: Behavior changes need tests that would fail if the
    change were broken, including edge cases and error paths. Tests that only
    exercise the happy path are not enough.

## Compatibility

-   **Breaking changes are extremely hard to make.** Protobuf has a huge number
    of users. Assume every public API, observable behavior, and generated-code
    shape is depended on by someone ([Hyrum's Law](https://xkcd.com/1172/)).
-   Do not change public APIs, generated code, wire format, text format, or JSON
    behavior without explicit agreement from the Protobuf maintainers. Open an
    issue to discuss the change before writing code.
-   Do not make anything public "for convenience." New public API needs design
    review by the Protobuf maintainers.

## Performance Changes

-   **Good benchmark numbers alone are not enough to accept a change.** If the
    code health cost is too high, the change will be rejected.
-   **Microbenchmarks often do not reflect real-world wins.** They exaggerate
    effects from hot caches, branch predictor training, and inlining that don't
    happen in real workloads. A microbenchmark win can be neutral or a
    regression in production.
-   Performance pull requests must say what was measured, how, and why the
    result is expected to carry over to real workloads (e.g. production profiles
    or realistic load tests). Report both wins and regressions.
-   Weigh any win against the added complexity, code size, and maintenance cost.

## Bug Reports and Bug Fixes

Protobuf has a very large surface area, and there are certainly bugs in rarely
used corners of it. With AI tools, finding a possible bug now costs much less
than carefully reviewing a fix for it, and that review time is what limits how
many fixes we can accept. Google already runs its own automated bug finding
against Protobuf.

We welcome bug reports and fixes that come from real problems. Please don't
search the codebase for bugs just to find something to fix. Every bug report or
bug-fix pull request must include:

-   **A reproduction**: Minimal steps, code, or input that triggers the bug
    using the public API, and the observed versus expected behavior.
-   **Why it matters**: An explanation of how the steps to reproduce the issue
    come up in a real use case, such as a project that hit the bug or a
    realistic way untrusted input reaches the affected code under the threat
    model in [SECURITY.md](SECURITY.md).

If the issue does not affect any real use and was found only by fuzzing, static
analysis, or code reading, don't file a bug or send a pull request. The
maintainers will politely decline the report or fix. Report security
vulnerabilities privately as described in [SECURITY.md](SECURITY.md), not in
public issues or pull requests.

## Author Responsibility

Code review is a shared responsibility between the author and the reviewer; it
is not only the reviewer's job. The author is fully responsible for every line
of code they send, no matter how it was written. Before asking for review, the
author must make sure the change is:

-   Correct.
-   Appropriately tested.
-   Understandable by others.
-   Maintainable.
-   Considered for its effects on performance, reliability, and security.

AI-generated code must be verified as rigorously as hand-written code. The
author must be able to answer reviewers' questions about the change. Don't send
unverified code and expect reviewers to figure out whether it works.

If a pull request is a work in progress, say so clearly in the title or
description, and ask for early feedback on specific questions instead of a full
review.

## Required Self-Review Before Sending

Before a pull request is opened (or marked ready for review), the agent **must**
review the full change with this prompt and address what it finds:

> Review this frankly and brutally honest. What are the flaws of the change that
> could be done better?

Fix the problems this finds, or explain in the pull request description why you
left them. Do not send a pull request that has not had this review.

Pull requests that don't meet this bar will be sent back without a line-by-line
review (see [Sending Pull Requests Back](#sending-pull-requests-back)).

## Reviewing Protobuf Pull Requests

When helping review a pull request, apply the same bar:

-   Review the change frankly and brutally honestly, using the prompt above. Do
    not soften findings or approve because the change "works."
-   Flag code health regressions, compatibility risks, missing or weak tests,
    and performance claims backed only by microbenchmarks, even when the
    benchmark numbers look good.
-   If the pull request has systemic problems, recommend sending it back as
    described below. Comment on the code, not the author, and don't guess
    whether the code was AI-generated.

### Sending Pull Requests Back

Reviewers don't need to finish a line-by-line review of a pull request that
isn't ready for one. Common signs that a pull request should be sent back:

-   **No self-review**: Obvious logic errors, leftover debugging code, untested
    code, or repeated mistakes the author would have caught by reading their own
    diff.
-   **Widespread quality problems**: Overly verbose code, unnecessary
    complexity, ignored language idioms or style guides, or lifetime,
    memory-safety, or concurrency hazards.
-   **Missing or ineffective tests**: Changed logic without tests, or tests that
    only check mocks, skip important edge cases, or would still pass if the code
    were broken.
-   **Too large or unclear**: The change is too big to review as one unit, or
    the description doesn't explain what changed and why.
-   **Unmotivated bug fix**: The fix has no reproduction, or no explanation of
    how the bug affects a real use case (see
    [Bug Reports and Bug Fixes](#bug-reports-and-bug-fixes)).

When this happens, stop commenting on individual issues. Instead, leave one
polite top-level comment that states which part of the bar is missing,
optionally points to one or two examples, and says what the author needs to do
before a full review. Focus on the code and the technical standard, not on the
author or on how the code was written.

Passing CI is necessary but not enough: automated checks can't tell whether the
design fits, the code is maintainable, subtle concurrency or lifetime bugs are
absent, or the tests would catch a regression. Correctness risks, new
complexity, and missing tests must be fixed before merging, not deferred to a
follow-up.

When an author marks review comments as addressed, check that the underlying
problem was fixed everywhere it appears, not just on the line where it was
flagged.
