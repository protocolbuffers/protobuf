An example that demonstrates how to use the `protobuf` and `protobuf_codegen`
crates together.

The source of the latest version of this example can be read
[here](https://docs.rs/crate/protobuf-example/latest/source/).

# How to get a compatible version of protoc

The protoc binary that you use to generate code needs to have a version that
exactly matches the version of the protobuf crate you are using. More
specifically, if you are using Rust protobuf `x.y.z` then you need to use protoc
`y.z`. See [here](https://protobuf.dev/support/version-support/) for more
details on our versioning scheme.

The easiest way to get ahold of protoc is to download a prebuilt binary from the
matching release [here](https://github.com/protocolbuffers/protobuf/releases).
Just make sure protoc is on your `$PATH` when you run `cargo`.
