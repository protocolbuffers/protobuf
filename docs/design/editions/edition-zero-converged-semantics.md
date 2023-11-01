# Edition Zero: Converged Semantics

**Authors:** [@perezd](https://github.com/perezd),
[@haberman](https://github.com/haberman)

**Approved:** 2021-10-07

## Background

The Protobuf Team has been exploring potential facilities for introducing
breaking API and semantic changes. This document is an attempt to make use of
these facilities to unify proto semantics from this point forward, while giving
customers the ability to more-granularly manage their project's specific needs.

## Objective

We want to reduce complications of API semantics that are coarsely managed
through the syntax keyword, and instead default to converged proto2/proto3 when
opting into editions. Where needed, customers will be able to opt out of
specific semantics that are incompatible with their existing usages, at a
fine-grained level, using the new capabilities provided by editions + features.

### Why Now?

As we introduce new facilities for managing breaking changes, we have an
additional opportunity to cutover and rectify a long-standing vision of
converging proto 2/3 semantics as a natural extension of this.

Doing this in lockstep with the introduction of editions provides the protobuf
team with a few valuable outcomes:

*   Editions provides us with more granular specification of intent than the
    existing coarse knob of "proto2" or "proto3." By opting into our first
    edition, customers are upgrading to what we've referred to in the past as
    "converged semantics," and if needed can reversibly downgrade back to proto2
    or proto3 semantics respectively by opt-ing out of the specific features
    that are incompatible with their existing needs.

*   The protobuf team can avoid the n^2 complexity of considering how an
    edition/feature will interplay with an explicit syntax designation of
    "proto2" vs "proto3" for all impacted runtimes. This allows us to transition
    our thinking/support model to be explicitly feature-centric.

*   The introduction of editions will almost certainly cause a major version
    bump and gives us ample justification to make breaking changes as we
    transition to this granular specification.

## Introduction of the `edition` keyword to proto IDL

The `edition` keyword is used to define which semantic version a particular file
and all of its contents will adhere to as a baseline. Whenever a proto file
declares an `edition` keyword, it automatically defaults to converged proto2/3
semantics.

An edition's value is represented as a string, encoded by convention as a year.

## Introduction of `features` option to `descriptor.proto`

This option will be uniformly defined as a repeated set of strings which can be
used to encode the ability to opt-out of a specific feature (eg:
`"-string_view"`), or to potentially opt-in to a future/experimental feature
(eg: `"string_view"`). The `features` option will be added to `descriptor.proto`
for the following descriptor options:

*   File
*   Message
*   Field
*   Enum
*   Enum Value
*   Oneof
*   Service
*   Method
*   Stream (internal repositories only)

Features are only respected when used in conjunction with the `edition` keyword.
They are not validated for correctness to ensure they are forward/backward
compatible with releases.

Features may be declared at any descriptor level, however, a feature definition
may influence descendant types at the discretion of the protobuf team. (e.g., a
file-level feature opt-out could impact all fields within the file, if it was
desired).

## A taxonomy of features

Features can be broken down into two main categories: language-specific and
semantic.

### Language-specific features

Language-specific features pertain to the generated API for a given language.
Referring to the protobuf breaking changes backlog we can see some examples:

*   (C++) Changing string fields to return `string_view`.
*   (Java) Removing the confusing `Enum#valueOf(int)` API.
*   (Java) Rename oneof enums to do appropriate camel casing.

Language-specific features have no meaning for any other language: they can be
ignored entirely. They are, in essence, a private (tunneled) interface between
protobuf IDL and the respective code generator. Each language's code generator
can independently decide what the "base" set of features is for any given
edition. Each language defines the migration path between editions
independently.

### Semantic Features

Semantic features define behavior changes that apply to the protobuf data model,
independent of language. These can also have API implications, but their meaning
goes deeper than just a surface-level API. Some examples of semantic features:

*   Open enums (enums are placed directly into the field instead of the
    `UnknownFieldSet`).
*   Packed (whether repeated fields are packed on the wire)

Semantic features have significantly broader scope, since they must be respected
across languages, and each language must implement the semantic correctly. This
also implies that every language must either (1) know the canonical set of "base
features" for each edition, or (2) that the set of "default" features for the
edition must be resolved in protoc itself and propagated explicitly into the
descriptor.

## Rev'ing the protobuf IDL vs. descriptor.proto

Changing `descriptor.proto` to reflect editions is a much more intrusive change
than changing just the protobuf IDL. The protobuf IDL is parsed and resolved in
protoc, and we have only a single implementation of that parser. Any change that
can be resolved in the parser alone is relatively unintrusive (though there are
build horizon issues since GCL parses protos in prod).

Rev'ing `descriptor.proto` is a far more intrusive change that affects many
downstream systems. Many systems access descriptors through either a descriptor
API (for example, `google::protobuf::Descriptor` in C++) or by directly accessing a proto
from `descriptor.proto` (eg. `google.protobuf.DescriptorProto`). Any changes
here need to be managed much more delicately.

## Deprecation of the `syntax` keyword from proto IDL

The `syntax` keyword shall no longer be required/observed when an `edition`
keyword is present, as it is now considered redundant. If `edition` and `syntax`
are both present, `edition` takes precedence and `syntax` is ignored.

## Migrating from `proto2` and `proto3` to Editions + Features

Today's usage of syntax opaquely bundles a collection of implied feature flags
that are set based on the presence of `proto2` or `proto3`. This is often a
source of confusion for customers (eg: what am I gaining by moving to proto3?
What am I losing?).

By deciding that editions/features exist in a state of proto2/3 convergence,
this enables customers to decide for themselves what features are important to
their usage of protos.

Migrating existing users of proto2 and proto3 to editions w/converged semantics
would mean we'd need to execute a large-scale change to make their
implicit/implied behavior explicit. Here are examples of implied behavior.
today:

<table>
  <tr>
   <td><strong>Feature</strong>
   </td>
   <td><strong><code>proto2</code> implied behavior</strong>
   </td>
   <td><strong><code>proto3</code> implied behavior</strong>
   </td>
  </tr>
  <tr>
   <td>packed_repeated_primitives
   </td>
   <td>🚫
   </td>
   <td>✅
   </td>
  </tr>
  <tr>
   <td>extensions
   </td>
   <td>✅
   </td>
   <td>🚫
   </td>
  </tr>
  <tr>
   <td>required
   </td>
   <td>✅
   </td>
   <td>🚫
   </td>
  </tr>
  <tr>
   <td>groups
   </td>
   <td>✅
   </td>
   <td>🚫
   </td>
  </tr>
  <tr>
   <td>cpp_string_view
   </td>
   <td>🚫
   </td>
   <td>🚫
   </td>
  </tr>
  <tr>
   <td>java_enum_no_value_of
   </td>
   <td>🚫
   </td>
   <td>🚫
   </td>
  </tr>
  <tr>
   <td>open_enums
   </td>
   <td>🚫
   </td>
   <td>✅
   </td>
  </tr>
  <tr>
   <td>MORE STUFF ...
   </td>
   <td>
   </td>
   <td>
   </td>
  </tr>
</table>

### Managing Complexity of `features` for Large Deployments

A separate concept has been established to help mitigate the complexity of
editions and progressive feature rollouts and synchronizations for larger proto
projects.

This facility could be used to migrate existing usages of the `syntax` keyword
to use Editions + Features across google3, for example.

## Prior Work

*   proto{2,3} Convergence Vision (not available externally)

*   Epochs for descriptor.proto (not available externally)

*   Rust editions: https://doc.rust-lang.org/edition-guide/editions/index.html
