# More efficient Protobuf C# serialization/deserialization API

## Background
Currently serialization of protobuf C# messages is done (much like java) through instances of CodedInputStream and CodedOutputStream.
These APIs work well, but there is some inherent overhead due to the fact that they are basically `byte[]` based. The way .NET gRPC uses these types involves allocating an array the size of the message, which for larger messages means allocations to the [large object heap](https://docs.microsoft.com/en-us/dotnet/standard/garbage-collection/large-object-heap#loh-performance-implications).
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
- ensure there are microbenchmark that allow comparing the two implementations
- as support for the new serialization/deserialization APIs requires generating code that is not compatible with some older .NET frameworks, figure out a schema under which messages with "new serialization API support" and without it can coexist (without causing user friction).
- the effort to maintain the Protobuf C# project should remain approximately the same (should not be negatively affected by introduction of new serialization/deserialization APIs).

Non-goals
- over-optimize the initial version of new serializer/deserializer (we should rather focus on enabling future improvements)
- remove or deprecate the existing serialization/deserialization API

## The Proposal

Key elements:
- Add `CodedInputReader` and `CodedOutputWriter` types (both are `ref structs` and thus can hold an instance of `Span`). These two types implement the low-level wire format semantics in terms of the `Span` type (as opposed to `byte[]` used by CodedInputStream and CodedOutputStream).
- Introduce `IBufferMessage` interface which inherits from `IMessage` and marks protobuf messages that support the new serialization/deserialization API.
- Introduce a new codegen option `--use_buffer_serialization` to enable/disable generating code required by CodedInputReader/CodedInputWriter (the `MergeFrom(ref CodedInputReader)` and `WriteTo(ref CodedOutputWriter output)` methods). This option will default to disabled. gRPC frameworks will likely provide the ability to explicitly control it, e.g. `<Protobuf Include="..\Shared\benchmark_service.proto" Serialization="Buffer" />`, and/or detect the version of .NET being targeted and automatically enable the option.
- Generated code that uses the new IBufferMessage/CodedInputReader/CodedOutputWriter types will be surrounded by `#if !PROTOBUF_DISABLE_BUFFER_SERIALIZATION` defines. That will allow potentially incompatible generated code to be excluded by a project to support multi-targeting scenarios.

A prototype implementation is https://github.com/protocolbuffers/protobuf/pull/5888

## Efficiency improvements

`MergeFrom(ref CodedInputReader)` allows parsing protobuf messages from a `ReadOnlySequence<byte>`, which offers these benefits over the existing `MergeFrom(ref CodedInputStream)` and/or `MergeFrom(byte[] input)`.

- The input buffer does not have to be continguous, but can be formed from several segment/slices. That's a format that naturally matches the format in which the data are received from the network card (= we can parse directly from the memory slices we received from network)

- Allows parsing directly from both managed and native memory buffer in a safe manner with a single API (this is beneficial for Grpc.Core implementation that uses a native layer, so we don't have to copy buffers into managed memory first).

- In grpc-dotnet, the requests can be exposed as `ReadOnlySequence<byte>` in a very efficient manner.

- Buffer segments can be reused once read so that there are no extra memory allocations.

- Access to segments of read only sequence via `ReadOnlySpan<>` is heavily optimized in recent .NET Core releases.

`WriteTo(ref CodedOutputWriter output)` allows serializing protobuf messages to `IBufferWriter<byte>` which has these benefits:

- The output doesn't have to be written to a single continguous chunk of memory, but into smaller buffer segments, which
avoids allocating large blocks of memory, resulting in slow LOH GC collections.

- Allows serializing to both managed and native memory in a safe manner with the same API.

- Buffer segments can be reused once read so that there are no extra memory allocations.

- Allows avoiding any additional copies of data when serializing (in both Grpc.Core and grpc-dotnet). 

## Platform support

- the new serialization API will be supported for projects that support at least `netstandard2.0` and allow use of C# 7.2. On newer .NET frameworks, there might be (and likely will be) further performance benefits.

- the new serialization API will be supported on `net45`, but it's mostly for convenience and to minimize the differences between different builds of the library. `netstandard2.0` and higher are the primary targets.

- the new serialization API won't be supported on netstandard1.0 (as it doesn't support the `System.Memory` package which contains several required types). Users will still be able to use the original serialization APIs as always. 

## Backwards compatibility

API changes to the runtime library are purely additive.

By default (without `--use_buffer_serialization` options), the generated code will stay the same and won't depend on any of the newly introduced types, so existing users that don't opt-in won't be affected by the new APIs at all.

When `--use_buffer_serialization` is used when generating the code, the code requiring the newly introduces types will
be protected by `PROTOBUF_DISABLE_BUFFER_SERIALIZATION` if-define, so that it can be disabled (needs to be manually set in a project) when needed.

## Concern: Code duplication and maintainability

While the logical structure of the (de)serialization code for both old and new approaches is very similar, due to the different nature of the object holding the parsing state (`class` vs `ref struct`), the parsing primitives need to be defined twice (e.g. ReadRawVarint64 exist in two copies, one under CodedInputStream and one user CodedInputReader) and that adds some extra overhead when:

- optimizing the (de)serialization code (two locations need to be updated)
- fixing bugs (some bug might need to be fixed in two locations)
- writing tests (in some cases, tests need to be duplicated, because behavior with both Stream/Buffer serialization need to be tested)

## Integration with generated gRPC stubs

To use the new (de)serialization API from gRPC, gRPC C# will need to modify the codegen plugin to generate 
grpc stubs that utilize the new API.

TODO: add the design for changes to grpc_csharp_plugin codegen plugin.

TODO: what is going to be the flag to select the new serialization API in gRPC stubs?

TODO: add example snippet of the generated marshaller code (current state: https://github.com/grpc/grpc/blob/b07d7f14857d09f05b58ccd0e5f15a6992618f3a/src/csharp/Grpc.Examples/MathGrpc.cs#L30)

TODO: is there a way to autodetect that IBufferMessage serialization API is available and use it automatically (and fallback if it's not available?)

## Other considerations

TODO: design for Grpc.Tools and whether the span-based parsing will be enabled by default

TODO: how will the old generated code coexist with the span-enabled generated code? how will it work across library boundaries (e.g. a client library contains code generated without the support for fast parsing and we want to use that client library).

## References

This proposal is based on previous work / discussions:
- https://github.com/grpc/grpc-dotnet/issues/30
- https://github.com/protocolbuffers/protobuf/issues/3431
- https://github.com/protocolbuffers/protobuf/pull/4988
- https://github.com/jtattermusch/protobuf/pull/8
