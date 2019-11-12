# More efficient Protobuf C# serialization/deserialization API

## Background
Currently serialization of protobuf C# messages is done (much like java) through instances of CodedInputStream and CodedOutputStream.
These APIs work well, but there is some inherent overhead due to the fact that they are basically `byte[]` based.
Recently a set of new types has been added to the .NET Framework (Span, ReadOnlySequence, IBufferWriter)
and these new types allow defining new serialization/deserialization APIs, ones that would be much more efficient especially from
the perspective of memory management.

Both gRPC C# implementations (Grpc.Core and grpc-dotnet) already contain functionality that allows exposing request/response payloads
in terms of the new .NET Span APIs in a way that requires no copying of binary payloads and basically allows deserialization directly
from the buffer segments that were received from the network (and serializing directly into the buffer that is going to be sent out
over the network).

## Objective
The goals for this effort:
- keep the existing CodedInputStream and CodedOutputStream (no plans to remove them in the future), serialization/deserialization speed of these APIs should stay unaffected.
- all the changes made need to be backward compatible
- introduce a set of new public APIs for serialization/deserialization that utilize the Span type
- allow future incremental performance optimizations and make them as easy as possible
- ensure the are microbenchmark that allow comparing the two implementations
- as support for the new serialization/deserialization APIs requires generating code that is not compatible with some older .NET frameworks, figure out a schema under which messages with "new serialization API support" and without it can coexist (without causing user friction).
- the effort to maintain the Protobuf C# project should remain approximately the same (should not be negatively affected by introduction of new serialization/deserialization APIs).

Non-goals
- over-optimize the initial version of new serializer/deserializer (we should rather focus on enabling future improvements)
- remove or deprecate the existing serialization/deserialization API

## The Proposal

Key elements:
- Add CodedInputReader and CodedOutputWriter types (both are `ref structs` and thus can hold an instance of `Span`). These two types implement the low-level wire format semantics in terms of the `Span` type (as opposed to `byte[]` used by CodedInputStream and CodedOutputStream).
- Introduce `IBufferMessage` interface (exact name TBD) which inherits from `IMessage` and marks protobuf messages that support the new serialization/deserialization API.
- Introduce a new codegen option (exact name TBD) to enable/disable generating code required by CodedInputReader/CodedInputWriter (the MergeFrom and WriteTo methods).

A prototype implementation is https://github.com/protocolbuffers/protobuf/pull/5888

## Platform support

- the new serialization API will be supported for projects that support at least `netstandard2.0` and allow use of C# 7.2. On newer .NET frameworks, there might be (and likely will be) further performance benefits.

## Backwards compatibility

## Code duplication

TODO: testing (how to test both APIs without twice the effort).

## Other considerations

TODO: design for codegen option
TODO: will the generated code contain an #ifdef to enable multi-targeted projects?

TODO: design for Grpc.Tools and whether the span-based parsing will be enabled by default

TODO: how will the old generated code coexist with the span-enabled generated code? how will it work across library boundaries (e.g. a client library contains code generated without the support for fast parsing and we want to use that client library).

## References

This proposal is based on previous work / discussions:
- https://github.com/grpc/grpc-dotnet/issues/30
- https://github.com/protocolbuffers/protobuf/issues/3431
- https://github.com/protocolbuffers/protobuf/pull/4988
- https://github.com/jtattermusch/protobuf/pull/8