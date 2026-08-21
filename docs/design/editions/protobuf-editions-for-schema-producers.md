### Protobuf Editions for Schema Producers

**Author:** [@fowles](https://github.com/fowles)

Explains the expected use of and interaction with editions for schema providers
and their customers.

## Background

The [Protobuf Editions](what-are-protobuf-editions.md) project uses "editions"
to allow Protobuf to safely evolve over time. This is primarily accomplished
through ["features"](protobuf-editions-design-features.md). The first edition
(colloquially known as "Edition Zero") will use features to unify proto2 and
proto3 ([Edition Zero Features](edition-zero-features.md)). This document will
use definitions from [Protobuf Editions: Rollout](protobuf-editions-rollout.md)
but focus primarily on the use case of **Schema Producers** and **Schema
Consumers**:

> **Schema Producers** are teams that produce `.proto` files for the consumption
> of other teams.

As a reminder, features will generally not change the wire format of messages
and thus changing the edition for a `.proto` will not change the wire format of
message.

## Initial Release

There will be a large period of time during which `protoc` is able to consume
`proto3`, `proto2`, and editions files. Once all of the supported `protoc`
releases handle editions, schema producers should upgrade their published
`.proto` files to edition zero. The protobuf team will provide a tool that
upgrades `proto2` and `proto3` files to edition zero in a fully compatible way.

### Edition Zero Features

In order to unify proto2 and proto3, "Edition Zero" is taking an opinionated
stance on which choices are good and bad, by choosing "good" defaults and
requiring explicit requests for the "bad" semantics
([Edition Zero Features](edition-zero-features.md)). Schema producers that are
simply upgrading existing `.proto` files should publish these files as produced
by the upgrade tool. This will ensure wire compatibility. Newly published
`.proto` files should use the default values from this first edition.

## Steady State Flow

For the most part, editions should not disturb the general pattern for schema
producers. Any schema producer should already specify what versions of protobuf
they support and should not support versions of protobuf that are themselves
unsupported. Schema producers should generally publish all of their `.proto`
files with a consistent edition for the simplicity of their users. When updating
the edition for their `.proto` files, producers should target an edition
supported by all of the versions of protobuf in their support matrix. **A good
rule of thumb is to target the newest edition supported by the oldest release of
protobuf in the support matrix.**

### Best Practices

#### Publish `.proto` files

Schema producers should publish `.proto` files and not generated sources. This
is already the case and editions do not change it. Publishing generated sources
can lead to mismatches between the compiler and runtime environment used.
Protobuf does not support mixed generation/runtime configurations and sometimes
security patches require updating both.

#### Minimize use of features

Codegenerator specific [features](protobuf-editions-design-features.md) (like
`features.(pb.cpp).string_field_type`) should only be applied within the context
of a single code base. **Consumers** of published schemas may wish to add
generator specific features (either by hand or with an automated `.proto`
refactoring tool), but **producers** should not force that onto users.

## Client Usage Patterns

Consumers’ usage is heavily constrained by their build system. Language agnostic
build systems, like Bazel, can run `protoc` as one of the build steps. Language
specific build systems, like Maven or Go, make running `protoc` more difficult
and so consumers often avoid it. Languages like Python that traditionally lack a
build system are more extreme.

### Running `protoc` Directly

Because language-specific features will not change the wire format of messages,
clients will be able to update to newer editions or specify specific features
appropriate to their environment while still connecting to external endpoints.

In particular, protobuf will provide two distinct mechanisms for supporting
these users. First, we will provide tools for automating updates to `.proto`
files in a safe way. These tools will apply semantic patches to `.proto` files
that they can then commit into source control. Second, we will provide
primitives in `protoc` to compile a `.proto` file and a semantic patch as a set
of inputs so that users never have to materialize the modified `.proto` file.
Protobuf team will investigate adding support for semantic patches when it
addresses Bazel rules.

In the long term, we want a Bazel rule (and possibly similar for other build
systems) that seamlessly packages changes like:

```proto
proto_library(
    name = "cloud_spanner_proto",
    modifications = ["cloud_spanner.change_spec"],
    deps = ["@com_google_cloud//spanner"],
)
```

### Publishing Generated Libraries

As a reminder, publishing generated code is not a good idea. It frequently runs
afoul of runtime/generation time mismatches and is an active source of confusion
where users are unable to reason about what version they are on.

Teams determined to do this anyway should adhere to the following best
practices.

*   Publish generated libraries using semver.
*   Publish both the generated code and the protobuf runtime.
*   Pin the protobuf runtime library to the exact version used to generate the
    library.
*   When upgrading the `protoc` generator for a major, minor, or micro release,
    increment the corresponding number in the published library’s version.
*   When updating the edition of the `.proto` file, increment the major number
    of the published library’s version.

It is worth note that only the last bullet point is new, everything else is a
restatement of current best practice.
