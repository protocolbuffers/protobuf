# Prototiller Project Roadmap

**Author:** [@perezd](https://github.com/perezd)

**Approved:** 2022-10-07

## Overview

This roadmap is an outline of the steps to deliver
[Prototiller, Plan of Record](prototiller-plan-of-record.md).

The Protobuf Editions Working Group has required that a sufficiently powerful
tool must be developed which can rewrite Protobuf source code (.proto) files to
ease the migration for internal *and* external users of Protobuf to the new
[Protobuf Editions](../editions/what-are-protobuf-editions.md) vision. This tool
should be lightweight and easy to engage[^1] with, similar to Bazel's
[buildozer](https://github.com/bazelbuild/buildtools/tree/master/buildozer)
tool.

## 2022 Q4

**THEME:** Project and infrastructure turn up to empower critical de-risking.

### Update ANTLR4 in third_party

Our use of Go as a language would benefit from the latest viable release of
ANTLR4 since major changes occurred in this space. Will be collaborating with
the owner of ANTLR4 for this work.

Key Steps:

1.  Import latest
    1.  Setup copybara transforms for import rewrites
    2.  Ensure TGP is green

### Define internal and GitHub project repositories and establish bi-directional syncing capabilities

Goal is to have an open source project that is relatively healthy and ready to
go even though an open source version won't be needed as urgently as internally.

Key Steps:

1.  Establish internal repo @ //third_party/prototiller
2.  Establish GitHub repo @
    [protocolbuffers/prototiller](https://github.com/protocolbuffers/prototiller)
3.  Setup *Copybara as a Service* to sync internal project repository + GitHub
    bi-directionally
    1.  Transform blaze centric imports
    2.  Define a manual GitHub-local `go.mod` files for analogous building w/ Go
        tool.
    3.  Make transforms reversible
    4.  Refactor code to use packages which are OSS friendly
4.  Setup test automation to run on all PRs in GitHub using GitHub Actions.

    1.  GitHub action to install Go
    2.  GitHub action to install Protoc?
    3.  GitHub action to build Antlr4 off dev branch?

        Currently this is being handled manually as the latest release hasn't
        been cut yet. This is a work around right now, but I believe having the
        ability to use the dev branch should be something supported. Example:
        Dart provides this functionality by allowing users to use the stable or
        dev channel.

5.  Setup test automation to run on all CLs in our internal project repository
    using TAP.

    1.  Setup Project Blueprint file + TAP project

### Revise and Port the moneta Prototiller grammar to //third_party/prototiller

This version of the grammar should be reviewed and ported over with the
intention of being open sourced along with the rest of the Prototiller codebase.

Key Steps:

1.  Prototype multi-language support in grammar: at least Java and Go. Should be
    adaptable to other languages in the future.
2.  Prototype testing approach in Go: how to adapt from Java JUnit tests so that
    test cases are common to Go and Java. Should be adaptable to other languages
    in the future.
3.  Move prototiller protos from moneta/common/annotation/prototiller/parser to
    new location
4.  Move grammar files to new location
5.  Update Java interpreter unit tests to mostly use files (input, expectations,
    settings) rather than bespoke custom tests
    1.  Comment Interpreter Tests
6.  Share starter CL for lexer/parser tests with Theo

### Establish stub layout of a conventional Go-based CLI tool

The CLI tool frontend is the exclusive interface for end users. The code layout
must be compatible with the standard Go build system (externally) and Blaze
(internally). This implementation will support the design of the CLI UX.

Code tree structure will align with this guidance:
https://github.com/golang-standards/project-layout

Key Steps:

1.  Setup filesystem layout to match Go conventions for a CLI tool using
    [Cobra](http://Cobra.dev)
2.  Setup Blaze BUILD files for internal project repository
    1.  Establish ANTLR generator using build rules and runtime dependencies
3.  Setup Go module for prototiller project in GitHub
    1.  Establish ANTLR generator using //go:generate (or equiv functionality)
        and runtime dependencies
    2.  Run protoc on metadata .proto files to generate Go bindings
4.  Establish Makefile as the build controller

## 2023 Q1

**THEME:** Execute feature development in earnest.

First functionality:

1.  Lexer dump (useful for debugging)
2.  Parse tree dump (useful for debugging)
3.  Formatting (should produce output as close to existing protofmt tool as
    possible – basically a no-op read/write of the .proto file)

### Port Base Functionality to Go

Base functionality means all the code to read and rewrite a proto file ported
from Java to Go. This does not include any transforms except those that are
unavoidable artifacts of the rewriting process.

ETA: By end of Q1 (How do we speed this up?)

1.  Port Interpreter Stage to Go
    1.  Port comment interpreter
        1.  Comment interpreter testcases in third_party/prototiller
        2.  Java tests converted to use new file-based test cases
        3.  Port Comment Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for comment interpreter passing for Go implementation
    2.  Port OptionPreprocessor to Go
        1.  OptionPreProcessor testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port OptionPreProcessor logic to Go
        4.  Get Go readability reviews on Cls
        5.  All tests for OptionPreprocessor passing for Go implementation
    3.  Port Syntax Interpreter
        1.  Syntax interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Syntax Interpreter logic to Go
        4.  Get Go readability reviews on CLs (landed without readability
            review)
        5.  All tests for syntax interpreter passing for Go implementation
    4.  Port package interpreter
        1.  Package interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Package Interpreter logic to Go
        4.  Get Go readability reviews on CLs (landed without readability
            review)
        5.  All tests for Package interpreter passing for Go implementation
    5.  Import Interpreter
        1.  Import interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Import Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for Import interpreter passing for Go implementation
    6.  Enum Value Interpreter
        1.  Enum Value interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Enum Value Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for Enum Value interpreter passing for Go implementation
    7.  Enum Interpreter
        1.  Enum interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Enum Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for Enum interpreter passing for Go implementation
    8.  Field Interpreter
        1.  Field interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Field Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for Field interpreter passing for Go implementation
        6.  Handle Map Fields, add test cases
    9.  Oneof Interpreter
        1.  Oneof interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Oneof Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for Oneof interpreter passing for Go implementation
    10. Extend Interpreter
        1.  Extend interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Extend Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for Extend interpreter passing for Go implementation
    11. Message Interpreter
        1.  Message interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Message Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for Message interpreter passing for Go implementation
    12. Group Interpreter
        1.  Group interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Group Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for Group interpreter passing for Go implementation
    13. Method Interpreter
        1.  Method interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Method Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for Method interpreter passing for Go implementation
    14. Service Interpreter
        1.  Service interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Service Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for Service interpreter passing for Go implementation
    15. File Interpreter
        1.  File interpreter testcases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port File Interpreter logic to Go
        4.  Get Go readability reviews on CLs
        5.  All tests for File interpreter passing for Go implementation

## 2023 Q2

**THEME:** Port Writer stage

First Transforms

1.  Remove unused deps
2.  LSC create edits – Find the well-lit path for LSCs like this: tie in to go
    flume?
    1.  Possible able to use patcher
3.  Update Edition
4.  TBD

What will actually drive external customers to do this migration?

### Port Writer Stage to Go

1.  Port Writer Stage to Go
    1.  Port Integer Writer
        1.  Integer Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Integer writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Integer writer passing for Go implementation
    2.  Port String Writer
        1.  String Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port String writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for String writer passing for Go implementation
    3.  Port Comment Writer
        1.  Comment Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Comment writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Comment writer passing for Go implementation
    4.  Port Option Writer
        1.  Option Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Option writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Option writer passing for Go implementation
    5.  Port Enum Value Writer
        1.  Enum Value Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Enum Value writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Enum Value writer passing for Go implementation
    6.  Port Enum Writer
        1.  Enum Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Enum writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Enum writer passing for Go implementation
    7.  Port Field Writer
        1.  Field Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Field writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Field writer passing for Go implementation
    8.  Port Oneof Writer
        1.  Oneof Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Oneof writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Oneof writer passing for Go implementation
    9.  Port Extend Writer
        1.  Extend Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Extend writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Extend writer passing for Go implementation
    10. Port Message Writer
        1.  Message Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Message writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Message writer passing for Go implementation
    11. Port Group Writer
        1.  Group Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Group writer logic to Go
        4.  Get Go reliability reviews on CL (was just a method implementation
            in field writer, not worth readability involvement)
        5.  All tests for Group writer passing for Go implementation
    12. Port Method Writer
        1.  Method Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Method writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Method writer passing for Go implementation
    13. Port Service Writer
        1.  Service Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port Service writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for Service writer passing for Go implementation
    14. Port File Writer
        1.  File Writer test cases in third_party/prototiller
        2.  Java tests converted to use file-based test cases
        3.  Port File writer logic to Go
        4.  Get Go reliability reviews on CL
        5.  All tests for File writer passing for Go implementation

## 2023 Q3

### Add Debugging capabilities to Lexer/Parser

Provide a way for Prototiller to explicitly request a "lexer" and "parser"
output as top-level commands to make debugging easier.

1.  Lexer
2.  Parser
3.  Formatter

### Add Testing Improvements

1.  Options are currently failing because of Go's fuzzing and comparison for
    text proto
2.  Create test harness that allows diffing of FileDescriptors produced by
    prototiller interpreter and protoc for any given .proto file. We should
    gradually reduce diffs to 0.
    1.  Sketch out a design for what that looks like.

### Port Linker Stage to Go

1.  Publish a design document w.r.t what the linker stage is and how it will be
    ported/implemented in the new Go-based codebase.
2.  Port Linker Stage Tests to Go (linkage isn't needed until we do advanced
    transforms)

    1.  Update Java linker unit tests to mostly use files (input, expectations,
        settings) rather than bespoke custom tests
    2.  Put linker test files in new location
    3.  Get Go readability reviews on CLs
    4.  All tests for linker passing for Go implementation

    **Port linker logic to Go**

    Milestones:

3.  Resolving Type Names

4.  Restoring Type Names

5.  Interpreting Options

6.  Uninterpreting Options

### Implement minimal transforms to convert proto2/proto3 to "Edition Zero"

This is the minimum required to get to our 80% transition goal for 2023. Update
syntax and edition fields, add features to get backwards compatibility. Should
work as a general “edition upgrade” command.

### Design CLI UX for Prototiller

The CLI UX design will provide specifics on how end users can invoke the tool
and instruct its operation using our *Semantic Actions* spec (not available
externally) for all the necessary use cases defined thus far to support
Editions.

Key Steps:

1.  Write design doc
    1.  Revise semantic action plans to support editions, w/loose priority of
        need for editions as primary use case.
2.  Get review sign-off for Editions WG
3.  Publish and notify protobuf-team@ of Plan of Record

### Write a How-To/Cookbook Guide on how to use Prototiller

This will be helpful documentation for all customers on how to achieve common
tasks with the tool.

## 2024 Q1

**THEME:** Large-scale changes to get 80% of google3 proto2 and proto3 schema
files converted to Edition 0 by the end of this quarter.

This recognizes that Q4 is very difficult to make real progress in, so the bulk
of the 80% migration needs to be done by end of Q3. Q4 will be for leftovers,
cleanup, and corner-cases.

**THEME:** Open sourcing prototiller on GitHub. Launch and be generally
available to external *and* internal customers.

### Establish GitHub Action-powered Release Automation

The simple needs of building a Go binary can be modeled and implemented
holistically within GitHub, so we can take advantage of that. We can leverage
[existing actions](https://github.com/marketplace/actions/go-release-binaries)
for the actual procedure.

### Execute an release process launch for releasing tool to make GitHub repo public.

This is a necessary bureaucratic step to open source the project and make it
available to our external customers.

## Notes

[^1]: Engagement is defined as direct usage of the tool itself of course, but
    also contributions, enrichment to the capabilities of the tool, and
    packaging/distribution for OS/arch variants.
