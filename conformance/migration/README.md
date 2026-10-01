# Conformance request golden (google3-only)

go/modernizing-conformance-tests

[TOC]

This package is **google3-only** (it is excluded from the GitHub export in
`//third_party/protobuf/copybara/cpp.copy.bara.sky`). It holds tooling that pins
the set of requests the gtest-based conformance suites send to a testee. It
guarded the migration of the protobuf conformance tests from the legacy
`ConformanceTestSuite` classes to the gtest-based framework (now complete) and
keeps guarding later refactorings of the tests the same way.

## What the goldens are

The goldens record **every request the conformance runner sends to a testee**,
in a fingerprinted form. They are produced by running the runner
(`//third_party/protobuf/conformance:conformance_test_runner`) with
`--record_requests` against `:skipping_testee`, a testee that answers every
request with `ConformanceResponse{skipped: "recording"}`.

Because the testee never parses or serializes anything, the goldens depend
*only* on which requests the runner emits, never on any implementation's
behaviour or on any failure list.

The runner is invoked four times (2 edition settings × 2 modes), always with
`--enforce_recommended` and empty failure lists:

*   `conformance_requests.golden` (the **main golden**, a superset) is the
    sorted union of two runs with `--maximum_edition 2023`:
    1.  in normal mode (every suite except the performance tests), and
    2.  with `--performance` (the opt-in performance tests only).
*   `conformance_requests_default_edition.golden` is the sorted union of the
    same two runs **without** `--maximum_edition`, i.e. with the runner's
    default of proto2/proto3 only. It pins the **edition gating**: a migrated
    test that is gated on the wrong edition would still appear in the main
    golden but would be added to or dropped from this one.

Each pair of recordings is concatenated and sorted with `LC_ALL=C sort`, so the
goldens are independent of test execution order.

### Line format

```
<test_name> <request_size_in_bytes> <crc32c_of_canonical_request_in_hex>
```

for example

```
Required.Proto3.ProtobufInput.TimestampProtoNegativeNanos.JsonOutput 73 1dc3c4a7
```

The size is that of the serialized `ConformanceRequest`; the hash is the CRC32C
of its *canonical* rendering, one `name: value` line per field in field-number
order, rather than of the wire bytes, because serialization order is not
canonical: non-opt google3 builds serialize some messages in descending field
order (go/ooo-serialization), which would make the recording depend on the build
configuration. The rendering is produced by parsing the request and printing
every field the recorder knows about (`CanonicalizeRequest()` in
`recording_test_runner.cc`); the recorder aborts on unknown fields, so a field
added to `ConformanceRequest` cannot slip past the hash unnoticed. Any change to
a request's payload, message type, output format, or test category changes the
line. CRC32C is a **drift detector**, not a collision-resistant hash; together
with the test name and the request size a collision that hides a real change is
moot. Every test name is unique across suites and modes: the runner rejects
duplicate test names within a suite, and the normal and `--performance` runs
execute disjoint sets of tests; `request_golden_test` asserts this for both
goldens.

## Why they must not change when tests are refactored

The migration moved tests between files and frameworks commit by commit. The
invariant that made this safe, and that still applies to any refactoring of the
tests: **at every commit, the set of requests sent to testees is byte-identical
to what was sent before.** If a moved or rewritten test sends exactly the same
request, the goldens do not change and there is nothing to review. If a golden
changes, the change altered behaviour (added, dropped, renamed, modified, or
re-gated a test case) and that must be understood and fixed, or explicitly
justified.

`request_golden_test` fails whenever the recorded requests differ from either
checked-in golden.

### The naming contract

The goldens pin the `test_name` strings that are passed to
`ConformanceTestRunner::RunTest`. For the goldens to keep guarding the tests,
the gtest-based framework must therefore

*   route every request through a `ConformanceTestRunner`, so that
    `--record_requests` sees it, and
*   produce **identical** test names for identical requests.

A test that bypasses the runner, or that renames a case, shows up as a dropped
and/or added line even if the bytes on the wire are unchanged.

## What this does NOT check

The goldens pin the **request set only**. A migrated test can send a
byte-identical request and still assert differently, and none of the following
is visible in the goldens:

*   the expected output (the payload the testee is supposed to produce),
*   whether a parse failure or a parse success is expected,
*   `require_same_wire_format` and similar per-test options, and
*   how a test's outcome is matched against the failure lists.

Those must be reviewed by other means (for example by running the migrated
suites against real testees with their existing failure lists).

## Failure lists

The goldens don't look at failure lists (see above), so the second invariant of
every CL that moves or rewrites a test is checked by hand: **every existing
failure list entry must still match after the move.** Two things about how
entries match:

*   An entry matches a test by its **name** (wildcards allowed) **and** by the
    text after `#`, which must be a **prefix of the actual failure message**
    (`TestManager::ReportFailure()`; the legacy runner did the same). An entry
    whose message no longer matches makes the failure *unexpected*, and the test
    fails.
*   The gtest matchers keep the legacy messages byte-identical wherever the
    legacy runner had one (`matchers.h` lists them), so a test migrated onto the
    equivalent matcher kept its lines untouched. A test moved onto a *different*
    matcher changes its message. Known cases from the JSON suite's migration:
    *   the legacy `RunValidJsonTestOrParseFailure()` message `Should have
        failed to parse or matched expected output but did not.` becomes
        `AnyOf(IsParseError(), ParsedPayload(...))`'s, i.e. `Should have failed
        to parse, but didn't., and Output was not equivalent to reference
        message: <differences>` (gMock's `AnyOf()` wording, truncated to 128
        characters like every list message);
    *   the legacy validators' `JSON payload validation failed.` becomes
        `JsonPayload(<matcher>)`'s, i.e. the matcher's own explanation (for
        `AllOf(HasJsonMember("a"), ...)` that is gMock's `which doesn't match
        (has member "a"), and ...`) or, when the matcher has none, `Expect: JSON
        payload <description>, but got: "<json>"`; their `Expected JSON payload
        but got type N` becomes the framework's generic `Failed to parse input
        or produce output.` / `Test was asked for JSON output but provided ...
        instead.`.

Every CL that changes the matcher of a test that is listed for some language
must therefore:

1.  update the message part of those list lines in the same CL (keep the name
    and any wildcard exactly as it was);
2.  derive the new message deterministically: run the language's google3
    conformance target where one exists (`:cpp`, `:cpp_using_json_format`,
    `:java`, `:python{,_cpp,_upb}`, the ESF converter's
    `//net/proto2/util/converter/internal/conformance:esf`, ...) and copy the
    message it reports; otherwise reason it out from the legacy failure mode
    (e.g. the legacy "or parse failure" message means the testee returned a
    non-equivalent protobuf payload, so the new message starts with
    `IsParseError()`'s explanation) and use a prefix that is safe whatever the
    exact differences are, saying so in the description;
3.  prefer descriptive named matchers (`MATCHER_P`, e.g. `HasJsonMember()` in
    `json_test_util.h`) over `Truly()` lambdas, so that the fallback `Expect:
    JSON payload <description>, but got: ...` message is meaningful;
4.  list every changed list line in the CL description.

## Running the test

```shell
blaze test //third_party/protobuf/conformance/migration:request_golden_test
```

It takes a few seconds. On failure the log contains a unified diff of each
golden against the actual recording; the actual recordings are also attached as
undeclared test outputs (`conformance_requests.actual` and
`conformance_requests_default_edition.actual`).

## Regenerating the goldens

Only do this when a change to the recorded requests is **intentional** (for
example a conformance test case is deliberately added or its input changed), and
say so in the CL description:

```shell
blaze run //third_party/protobuf/conformance/migration:regenerate_golden
```

This rewrites both goldens in your workspace and reports whether each changed.
The test and the regeneration script share `record_requests_lib.sh`, so they can
never disagree about how requests are recorded.

## Notes

*   With the skipping testee and empty failure lists the runner exits `0` in all
    four modes (all tests are reported as skipped; nothing is unmatched or
    unexpectedly failing), so the helper treats a non-zero exit as an error.
*   `skipping_testee` depends on nothing but `absl::string_view`; it embeds the
    fixed framed response and implements the 4-byte little-endian
    length-prefixed protocol described in `conformance_test_runner.cc`.
