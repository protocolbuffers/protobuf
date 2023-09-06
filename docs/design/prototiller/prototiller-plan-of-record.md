# Prototiller, Plan of Record

**Author:** pzd@

**Approved:** 2022-10-06

**NOTE:** This doc is written assuming that this plan is viable and works. This
is the set of things that are intended to de-risk as the proposed plan of record
for satisfying our inherent need to be able to transform Protobuf source code
files. Keep this in mind when reading this in the definitive tone it's written
in.

## Background

The Protobuf Editions Working Group has required that a sufficiently powerful
tool must be developed which can rewrite Protobuf source code (.proto) files to
ease the migration for internal *and* external users of Protobuf to the new
[Editions](what-are-protobuf-editions.md) vision. This tool should be
lightweight and easy to engage[^1] with, similar to Bazel's
[buildozer](https://github.com/bazelbuild/buildtools/tree/master/buildozer)
tool.

The top-line need to support external customers as equals customers has a
non-trivial influence on our recommendations.

## Recommendation

*   The Protobuf team will develop a new command-line tool and open source the
    project as Prototiller[^2] in a sibling repository under the protocolbuffers
    organization. The project will make use of internal publishing tooling from
    day 1 to ensure it's accessible and usable from GitHub and in Google.

*   Prototiller will explicitly promise zero source-code-level stability
    guarantees as a project, meaning our deliverable is a command-line tool and
    we will explicitly not support any public library APIs.

*   The frontend of this tool will support the *Semantic Actions specification*
    (not available externally) as defined from our initial tooling explorations.
    Prototiller will explicitly document stability guarantees for its change
    specifications.

*   Prototiller will use[^3] an existing ANTLR4[^4] -based grammar to generate a
    suitable and comprehensive parser/lexer for the tool to ingest Protobuf
    source files for the purposes of rewriting source code files based on change
    specification instructions.

*   Prototiller will be written in Go, and make use of the Blaze build system
    internally, ~~but will be idiomatically compatible with the standard Go
    build system externally for ease of use/integration with external customer
    expectations~~[^5]~~.~~ Externally, we'll be using Bazel for as a build
    system to wrangle the complexity of our dependencies.

## Alternatives Considered

### Why Go and not C++ or $LANG?

Go is a simple, performant, and capable enough environment to deliver something
high quality with wide native support and minimal development and maintenance
costs[^6]. Recently, we released
[Protoscope](https://github.com/protocolbuffers/protoscope) to open source as a
Go-based tool, and its simplicity is admirable and we'd intend to continue the
precedent. These sorts of command-line tool projects are a sweet spot for Go,
especially since we aren't hard pressed to squeeze every ounce of performance
out of this class of project.

We also considered Java (Prototiller v1 is written in this), but seemed also to
be not a good fit (even considering that it already exists!) for this class of
tool. The JVM is great when it stays online for a long time and gets "hot",
something that a command line tool typically isn't doing. Additionally, tools
based on the JVM are substantially harder to package for end user
consumption[^7].

### Why use a Parser Generator?

For this *particular* project, speed of delivering this capability trumps all
other things as our forcing priority. It also enables us to learn in a
non-trivial setting about this tool to help inform how we might evolve parsing
needs in other areas of Protobuf.

### What About our Existing C++ Parser?

Extending the capabilities and improving the performance of our existing C++
parser is an explicit non-goal of this project going forward. However, there are
many things we can learn from developing this tool using a new approach that may
inform evolution of the existing parser.

Our existing parser is narrowly designed to translate Protobuf source files into
our Descriptor IR. We've invested a bit already in making a more generalized
hand-written AST solution to make it practical in the past, but ultimately, it
was the working group's opinion that this is not a high value investment given
the costs and lack of ability to redirect efforts on other high-value projects.
This is an especially hard sell given the existence of tools like ANTLR to get
us something working quickly that has suitable performance nearly immediately.

### Why not Continue Protochangifier as Designed?

A combination of total cost and new opportunities. When we started
Protochangifer, we didn't have Jerry on staff who has developed a very similar
tool in Java for our internal repository. His work is battle hardened against
the many variations of proto files observed internally, and learning from his
hard-earned experience is a valuable opportunity we have available now.

Additionally, Protochangifier's implementation was predicated on the idea that
we were enhancing the capabilities of our existing C++ parser, which ultimately
is a cost we're choosing not to pay to deliver the functionality any longer.

Having said all that, a ton of great design came out of the project,
specifically the *Semantic Actions* (not available externally) work, so we
intend to develop and deliver that as specified as part of the Prototiller
project.

### Why not build off `buf` open source technology?

Buf is developing tooling in Go as well and there's a potential for us to
reuse/leverage some of their work in the future, but it's fairly
immature/incomplete at the moment. Jerry's IDL grammar work, for example, has
been battle-tested by nearly every proto file internally, whereas the spec
available at [protobuf.com](protobuf.com) has not. They also seem to be
[developing a Go-based replacement for protoc](https://github.com/bufbuild/protocompile)
which is worth watching, but still at this point too early in development to
comment on as a viable tool to leverage.

[^1]: Engagement is defined as direct usage of the tool itself of course, but
    also contributions, enrichment to the capabilities of the tool, and
    packaging/distribution for OS/arch variants.
[^2]: We've decided to reuse the name from the original project since it was
    never officially open sourced nor an officially supported Protobuf
    project, it can be thought of as a spiritual successor to the original
    project.
[^3]: We consider the use of ANTLR4 a "reversible decision" but fully intend to
    start development here, reserving the right to swap this out later based
    on empirical review of its deployment success.
[^4]: ANTLR4 is well supported by Blaze and Go build environments.
[^5]: While Bazel does have support for Go, it's by far the minority use case
    from the de facto Go build system/toolchain. Using Bazel doesn't offer any
    meaningful value to this project.
[^6]: We don't have to worry about OS/arch pairs nearly as much, build system
    pains (CMake/Bazel), etc. Go is a batteries-included toolchain that we can
    leverage for high velocity delivery.
[^7]: We considered using
    [Graal's Native Image capabilities](https://www.graalvm.org/22.1/reference-manual/native-image/)
    for native/static Java binaries which obliterates the JVM startup cost
    entirely, but this adds toolchain complexity/overhead for everyone.
