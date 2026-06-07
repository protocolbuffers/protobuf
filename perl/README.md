# Protobuf - High-performance Perl Protocol Buffers using upb

Protobuf is a high-performance Protocol Buffers implementation for Perl, built
upon the efficient `upb` C library. It provides a memory-safe, thread-isolated
environment with advanced features like thread-local arena caching,
SIMD-accelerated conversions, and zero-copy IPC support.

## Features

-   **Extreme Performance:** Utilizes the `upb` C library and VPP-inspired
    patterns.
-   **Memory Safety:** Implements 16-byte canary guards for all arena-allocated
    blocks.
-   **Thread Isolation:** Interpreter-local state via a per-interpreter
    registry.
-   **Advanced Containers:** Direct-to-hash projection for maps and descriptors.
-   **Well-Known Types:** Full support for Any, Duration, Timestamp, Struct,
    etc.
-   **JSON & Text Format:** High-speed serialization and parsing.

## Compatibility & Roadmap

This library is designed to become the official, unified, and modern Protocol
Buffers implementation for the Perl ecosystem.

Our long-term goal is to provide a **complete, drop-in replacement** for the two
most widely used community distributions:

*   **`Google::ProtocolBuffers::Dynamic`** (the high-performance C++ XS accessor
    layer).
*   **`Google::ProtocolBuffers`** (the pure-Perl runtime).

By implementing the exact APIs and behaviors offered by these distributions, we
aim to allow existing Perl codebases to seamlessly migrate to this officially
supported, memory-safe, and thread-isolated `upb`-based engine with zero code
modifications.

## Prerequisites

This Perl module requires Bazel to build the `protoc-gen-perl-pb` plugin. Ensure
you have Bazel installed. The necessary C++ Protobuf libraries used by the
plugin are typically fetched and built by Bazel as part of the main project's
workspace setup.

Additionally, you'll need a C++ compiler (like g++), Make, and the Perl
development headers.

To install Perl module dependencies, you can use `cpanm`:

```bash
# Install cpanminus and Carton
cpanm App::cpanminus Carton

# From the perl/ directory
carton install
```

This will install dependencies listed in the `cpanfile`.

## Building the Plugin

The `protoc-gen-perl-pb` plugin is built automatically via Bazel when you run
`make` in the `perl/` directory, as configured in `Makefile.PL`.

## Installation

To build and test the Perl module:

```bash
# From the perl/ directory
perl Makefile.PL
make -j$(nproc)
make test
sudo make install
```

To run a full clean test cycle, including rebuilding the plugin:

```bash
# From the perl/ directory
# Clean up previous Bazel run for the plugin
bazel --output_base=/usr/local/google/home/cjac/.gemini/tmp/protobuf/bazel_output_base clean --expunge
# Clean up MakeMaker build
make clean
# Regenerate Makefile and build everything including the plugin, then test
perl Makefile.PL && make -j$(nproc) && make test
```

**Note:** We use a specific Bazel `output_base` to ensure cache consistency
between different shells. If you run Bazel commands manually in this workspace,
you should use the same output base: `alias bazel='bazel
--output_base=/usr/local/google/home/cjac/.gemini/tmp/protobuf/bazel_output_base'`

## Code Generation

To translate a `.proto` schema file into Perl classes, you use the standard
Protocol Buffers compiler `protoc` in combination with the custom
`protoc-gen-perl-pb` plugin.

### 1. Build the Plugin

First, ensure that the plugin executable has been built in the `perl` directory
by running:

```bash
make protoc-gen-perl-pb
```

This generates the executable file `protoc-gen-perl-pb` in the root of the
`perl` directory.

> [!NOTE] The file `bin/protoc-gen-perl-pb` tracked in the repository is a
> skeletal mock script. The actual compiled C++ plugin binary will be created in
> the root of the `perl` directory.

### 2. Generate Perl Classes

Run `protoc` specifying the path to our plugin and the output directory:

```bash
protoc --plugin=protoc-gen-perl-pb=./protoc-gen-perl-pb \
       --perl-pb_out=embed_descriptors=true:./lib \
       --proto_path=./proto \
       ./proto/my_service.proto
```

Flag Details:

*   `--plugin=protoc-gen-perl-pb=./protoc-gen-perl-pb` points the compiler to
    the plugin executable
*   `--perl-pb_out=embed_descriptors=true:./lib` specifies the target directory
    for the generated `.pm` files. The `embed_descriptors=true` parameter is
    required to embed the compiled schema metadata in the generated modules,
    enabling runtime loading without `.proto` file dependencies
*   `--proto_path=./proto` defines the search path for imported `.proto` schemas
*   `./proto/my_service.proto` is the target schema file to compile

### 3. Load the Generated Module

The plugin generates Perl modules matching the package namespace. If
`my_service.proto` defines `package google.cloud.test.v1;`, it generates
`Google/Cloud/Test/V1/Service.pm` under the target directory.

You can then load it in your Perl code by adding the target directory to `@INC`:

```perl
use lib 'lib';
use Google::Cloud::Test::V1::Service;

my $req = Google::Cloud::Test::V1::Service::HelloRequest->new(
    name => 'World',
);
```

## Usage

```perl
use Protobuf::DescriptorPool;

my $pool = Protobuf::DescriptorPool->generated_pool();
$pool->add_serialized_file_descriptor_set($data);

my $msg = My::Generated::Message->new();
$msg->set_value(42);
my $wire = $msg->serialize();
```

## License

This software is licensed under the Apache License 2.0.
