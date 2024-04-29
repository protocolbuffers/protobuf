# How to use `protobuf_generate`

This document explains how to use the function `protobuf_generate` which is provided by protobuf's CMake module.

## Usage

In the same directory that called `find_package(protobuf CONFIG)` and any of its subdirectories, the CMake function `protobuf_generate` is made available by 
[`protobuf-generate.cmake`](../cmake/protobuf-generate.cmake). It can be used to automatically generate source files from `.proto` schema files at build time.

### Basic example

Let us see how `protobuf_generate` can be used to generate and compile the source files of a proto schema whenever an object target called `proto-objects` is built.

Given the following directory structure:

- `proto/helloworld/helloworld.proto`
- `CMakeLists.txt`

where `helloworld.proto` is a protobuf schema file and `CMakeLists.txt` contains:

```cmake
find_package(protobuf CONFIG REQUIRED)

add_library(proto-objects OBJECT "${CMAKE_CURRENT_LIST_DIR}/proto/helloworld/helloworld.proto")

target_link_libraries(proto-objects PUBLIC protobuf::libprotobuf)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

target_include_directories(proto-objects PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

protobuf_generate(
    TARGET proto-objects
    IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/proto"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")
```

Building the target `proto-objects` will generate the files:

- `${CMAKE_CURRENT_BINARY_DIR}/generated/helloworld/helloworld.pb.h`
- `${CMAKE_CURRENT_BINARY_DIR}/generated/helloworld/helloworld.pb.cc`

and (depending on the build system) output:

```shell
[build] [1/2] Running cpp protocol buffer compiler on /proto/helloworld/helloworld.proto
[build] [2/2] Building CXX object /build/generated/helloworld/helloworld.pb.cc.o
```

### gRPC example

`protobuf_generate` can also be customized to invoke plugins like gRPC's `grpc_cpp_plugin`. Given the same directory structure as in the [basic example](#basic-example) 
and let `CMakeLists.txt` contain:

```cmake
find_package(gRPC CONFIG REQUIRED)

add_library(proto-objects OBJECT "${CMAKE_CURRENT_LIST_DIR}/proto/helloworld/helloworld.proto")

target_link_libraries(proto-objects PUBLIC protobuf::libprotobuf gRPC::grpc++)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(PROTO_IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/proto")

target_include_directories(proto-objects PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

protobuf_generate(
    TARGET proto-objects
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")

protobuf_generate(
    TARGET proto-objects
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")
```

Then building `proto-objects` will generate and compile:

- `${CMAKE_CURRENT_BINARY_DIR}/generated/helloworld/helloworld.pb.h`
- `${CMAKE_CURRENT_BINARY_DIR}/generated/helloworld/helloworld.pb.cc`
- `${CMAKE_CURRENT_BINARY_DIR}/generated/helloworld/helloworld.grpc.pb.h`
- `${CMAKE_CURRENT_BINARY_DIR}/generated/helloworld/helloworld.grpc.pb.cc`

And `protoc` will automatically be re-run whenever the schema files change and `proto-objects` is built.

### Note on unity builds

Since protobuf's generated source files are unsuited for [jumbo/unity builds](https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html) it is recommended 
to exclude them from such builds which can be achieved by adjusting their properties:

```cmake
protobuf_generate(
    OUT_VAR PROTO_GENERATED_FILES
    ...)

set_source_files_properties(${PROTO_GENERATED_FILES} PROPERTIES SKIP_UNITY_BUILD_INCLUSION on)
```

## How it works

For each source file ending in `proto` of the argument provided to `TARGET` or each file provided through `PROTOS`, `protobuf_generate` will set up
a [add_custom_command](https://cmake.org/cmake/help/latest/command/add_custom_command.html) which depends on `protobuf::protoc` and the proto files. 
It declares the generated source files as `OUTPUT` which means that any target that depends on them will automatically cause the custom command to execute 
when it is brought up to date. The command itself is made up of the arguments for `protoc`, like the output directory, the schema files, the language to 
generate for, the plugins to use, etc.

## Reference

Arguments accepted by `protobuf_generate`.

Flag arguments:

- `APPEND_PATH` — A flag that causes the base path of all proto schema files to be added to `IMPORT_DIRS`.

Single-value arguments:

- `LANGUAGE` — A single value: cpp or python. Determines what kind of source files are being generated.
- `OUT_VAR` — Name of a CMake variable that will be filled with the paths to the generated source files.
- `EXPORT_MACRO` — Name of a macro that is applied to all generated Protobuf message classes and extern variables. It can, for example, be used to declare DLL exports.
- `PROTOC_OUT_DIR` — Output directory of generated source files. Defaults to `CMAKE_CURRENT_BINARY_DIR`.
- `PLUGIN` — An optional plugin executable. This could, for example, be the path to `grpc_cpp_plugin`.
- `PLUGIN_OPTIONS` — Additional options provided to the plugin, such as `generate_mock_code=true` for the gRPC cpp plugin.
- `DEPENDENCIES` — Arguments forwarded to the `DEPENDS` of the underlying `add_custom_command` invocation.
- `TARGET` — CMake target that will have the generated files added as sources.

Multi-value arguments:

- `PROTOS` — List of proto schema files. If omitted, then every source file ending in *proto* of `TARGET` will be used.
- `IMPORT_DIRS` — A common parent directory for the schema files. For example, if the schema file is `proto/helloworld/helloworld.proto` and the import directory `proto/` then the generated files are `${PROTOC_OUT_DIR}/helloworld/helloworld.pb.h` and `${PROTOC_OUT_DIR}/helloworld/helloworld.pb.cc`.
- `GENERATE_EXTENSIONS` — If LANGUAGE is omitted then this must be set to the extensions that protoc generates.
- `PROTOC_OPTIONS` — Additional arguments that are forwarded to protoc.