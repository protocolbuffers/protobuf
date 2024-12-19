An example that demonstrates how to use the `protobuf` and `protobuf_codegen`
crates together.

# How to get a compatible version of protoc

Usage of this crate currently requires protoc to be built from
source, as it relies on changes that have not been included in the newest protoc
release yet.

A future stable release will be compatible with the officially released protoc
binaries.

You can build a compatible protoc from source as follows:

```
git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf
git checkout rust-prerelease-{VERSION}
cmake . -Dprotobuf_FORCE_FETCH_DEPENDENCIES=ON
cmake --build . --parallel 12"
```
