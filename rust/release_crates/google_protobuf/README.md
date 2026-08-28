The runtime of the official Google Rust Protobuf implementation.

This is currently a beta release: the API is subject to change, and there may be
some rough edges, including missing documentation and features.

An example for how to use this crate can be found in the
[protobuf_example crate](http://crates.io/crates/protobuf_example)

# Ownership and implementation change

This crate is officially supported by the Protobuf team at Google.

This is a completely new implementation with a different API, as well as a
fundamentally different approach than prior versions of the `protobuf` crate. It
focuses on delivering a high-quality Rust API which is backed by either a pure C
implementation (upb) or the Protobuf C++ implementation. This choice was made
for performance, feature parity, development velocity, and security reasons.

It is not planned for the pure Rust lineage to be actively developed going
forward. While it is not expected to receive significant further development, as
a stable and high quality pure Rust implementation, many open source projects
may reasonably continue to stay on the `protobuf` crate.

Development was previously done under the `protobuf` crate by a community
project by [stepancheg](https://github.com/stepancheg) who generously donated
the crate name to Google. V4.36.0 of the `protobuf` crate corresponds to V0.36.0
of this crate.

# How to get a compatible version of protoc

The protoc binary that you use to generate code needs to have a version that
exactly matches the version of the protobuf crate you are using. More
specifically, if you are using Rust protobuf `x.y.z` then you need to use protoc
`y.z`. See [here](https://protobuf.dev/support/version-support/) for more
details on our versioning scheme.

The easiest way to get ahold of protoc is to download a prebuilt binary from the
matching release [here](https://github.com/protocolbuffers/protobuf/releases).
Just make sure protoc is on your `$PATH` when you run `cargo`.
