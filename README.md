Protocol Buffers - Google's data interchange format
===================================================

[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/protocolbuffers/protobuf/badge)](https://securityscorecards.dev/viewer/?uri=github.com/protocolbuffers/protobuf)

Copyright 2023 Google LLC

Overview
--------

Protocol Buffers (a.k.a., protobuf) are Google's language-neutral,
platform-neutral, extensible mechanism for serializing structured data. You
can learn more about it in [protobuf's documentation](https://protobuf.dev).

This README file contains protobuf installation instructions. To install
protobuf, you need to install the protocol compiler (used to compile .proto
files) and the protobuf runtime for your chosen programming language.

Working With Protobuf Source Code
---------------------------------

Most users will find working from
[supported releases](https://github.com/protocolbuffers/protobuf/releases) to be
the easiest path.

If you choose to work from the head revision of the main branch your build will
occasionally be broken by source-incompatible changes and insufficiently-tested
(and therefore broken) behavior.

If you are using C++ or otherwise need to build protobuf from source as a part
of your project, you should pin to a release commit on a release branch.

This is because even release branches can experience some instability in between
release commits.

### Bazel with Bzlmod

Protobuf supports
[Bzlmod](https://bazel.build/external/module) with Bazel 7 +.
Users should specify a dependency on protobuf in their MODULE.bazel file as
follows.

```
bazel_dep(name = "protobuf", version = <VERSION>)
```

Users can optionally override the repo name, such as for compatibility with
WORKSPACE.

```
bazel_dep(name = "protobuf", version = <VERSION>, repo_name = "com_google_protobuf")
```

### Bazel with WORKSPACE

Users can also add the following to their legacy
[WORKSPACE](https://bazel.build/external/overview#workspace-system) file.

Note that the `protobuf_extra_deps.bzl` is added in the `v30.x` release.

```
http_archive(
    name = "com_google_protobuf",
    strip_prefix = "protobuf-VERSION",
    sha256 = ...,
    url = ...,
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

load("@com_google_protobuf//:protobuf_extra_deps.bzl", "protobuf_extra_deps")

protobuf_extra_deps();
```

Protobuf Compiler Installation
------------------------------

The protobuf compiler is written in C++. If you are using C++, please follow
the [C++ Installation Instructions](src/README.md) to install protoc along
with the C++ runtime.

For non-C++ users, the simplest way to install the protocol compiler is to
download a pre-built binary from our [GitHub release page](https://github.com/protocolbuffers/protobuf/releases).

In the downloads section of each release, you can find pre-built binaries in
zip packages: `protoc-$VERSION-$PLATFORM.zip`. It contains the protoc binary
as well as a set of standard `.proto` files distributed along with protobuf.

If you are looking for an old version that is not available in the release
page, check out the [Maven repository](https://repo1.maven.org/maven2/com/google/protobuf/protoc/).

These pre-built binaries are only provided for released versions. If you want
to use the github main version at HEAD, or you need to modify protobuf code,
or you are using C++, it's recommended to build your own protoc binary from
source.

If you would like to build protoc binary from source, see the [C++ Installation Instructions](src/README.md).

Protobuf Runtime Installation
-----------------------------

Protobuf supports several different programming languages. For each programming
language, you can find instructions in the corresponding source directory about
how to install protobuf runtime for that specific language:

| Language                             | Source                                                      |
|--------------------------------------|-------------------------------------------------------------|
| C++ (include C++ runtime and protoc) | [src](src)                                                  |
| Java                                 | [java](java)                                                |
| Python                               | [python](python)                                            |
| Objective-C                          | [objectivec](objectivec)                                    |
| C#                                   | [csharp](csharp)                                            |
| Ruby                                 | [ruby](ruby)                                                |
| Go                                   | [protocolbuffers/protobuf-go](https://github.com/protocolbuffers/protobuf-go)|
| PHP                                  | [php](php)                                                  |
| Dart                                 | [dart-lang/protobuf](https://github.com/dart-lang/protobuf) |
| JavaScript                           | [protocolbuffers/protobuf-javascript](https://github.com/protocolbuffers/protobuf-javascript)|

Quick Start
-----------

The best way to learn how to use protobuf is to follow the [tutorials in our
developer guide](https://protobuf.dev/getting-started).

If you want to learn from code examples, take a look at the examples in the
[examples](examples) directory.

Documentation
-------------

The complete documentation is available at the [Protocol Buffers doc site](https://protobuf.dev).

Support Policy
--------------

Read about our [version support policy](https://protobuf.dev/version-support/)
to stay current on support timeframes for the language libraries.

Developer Community
-------------------

To be alerted to upcoming changes in Protocol Buffers and connect with protobuf developers and users,
[join the Google Group](https://groups.google.com/g/protobuf).
