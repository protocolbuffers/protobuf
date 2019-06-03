
# μpb - a small protobuf implementation in C

|Platform|Build Status|
|--------|------------|
|macOS|[![Build Status](https://storage.googleapis.com/upb-kokoro-results/status-badge/macos.png)](https://fusion.corp.google.com/projectanalysis/summary/KOKORO/prod%3Aupb%2Fmacos%2Fcontinuous)|
|ubuntu|[![Build Status](https://storage.googleapis.com/upb-kokoro-results/status-badge/ubuntu.png)](https://fusion.corp.google.com/projectanalysis/summary/KOKORO/prod%3Aupb%2Fubuntu%2Fcontinuous)|

μpb (often written 'upb') is a small protobuf implementation written in C.

upb generates a C API for creating, parsing, and serializing messages
as declared in `.proto` files.  upb is heavily arena-based: all
messages always live in an arena (note: the arena can live in stack or
static memory if desired).  Here is a simple example:

```c
#include "conformance/conformance.upb.h"

void foo(const char* data, size_t size) {
  upb_arena *arena;

  /* Generated message type. */
  conformance_ConformanceRequest *request;
  conformance_ConformanceResponse *response;

  arena = upb_arena_new();
  request = conformance_ConformanceRequest_parse(data, size, arena);
  response = conformance_ConformanceResponse_new(arena);

  switch (conformance_ConformanceRequest_payload_case(request)) {
    case conformance_ConformanceRequest_payload_protobuf_payload: {
      upb_strview payload = conformance_ConformanceRequest_protobuf_payload(request);
      // ...
      break;
    }

    case conformance_ConformanceRequest_payload_NOT_SET:
      fprintf(stderr, "conformance_upb: Request didn't have payload.\n");
      break;

    default: {
      static const char msg[] = "Unsupported input format.";
      conformance_ConformanceResponse_set_skipped(
          response, upb_strview_make(msg, sizeof(msg)));
      break;
    }
  }

  /* Frees all messages on the arena. */
  upb_arena_free(arena);
}
```

API and ABI are both subject to change!  Please do not distribute
as a shared library for this reason (for now at least).

## Using upb in your project

Currently only Bazel is supported (CMake support is partial and incomplete
but full CMake support is an eventual goal).

To use upb in your Bazel project, first add upb to your `WORKSPACE` file,
either as a `git_repository()` or as a `new_local_repository()` with a
Git Submodule.  (For an example, see `examples/bazel/ in this repo).

```python
# Add this to your WORKSPACE file.
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "upb",
    remote = "https://github.com/protocolbuffers/upb.git",
    commit = "d16bf99ac4658793748cda3251226059892b3b7b",
)

load("@upb//bazel:workspace_deps.bzl", "upb_deps")

upb_deps()
```

Then in your BUILD file you can add `upb_proto_library()` rules that
generate code for a corresponding `proto_library()` rule.  For
example:

```python
# Add this to your BUILD file.
load("@upb//bazel:upb_proto_library.bzl", "upb_proto_library")

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
)

upb_proto_library(
    name = "foo_upbproto",
    deps = [":foo_proto"],
)

cc_binary(
    name = "test_binary",
    srcs = ["test_binary.c"],
    deps = [":foo_upbproto"],
)
```

Then in your `.c` file you can #include the generated header:

```c
#include "foo.upb.h"

/* Insert code that uses generated types. */
```

## Old "handlers" interfaces

This library contains several semi-deprecated interfaces (see BUILD
file for more info about which interfaces are deprecated).  These
deprecated interfaces are still used in some significant projects,
such as the Ruby and PHP C bindings for protobuf in the [main protobuf
repo](https://github.com/protocolbuffers/protobuf).  The goal is to
migrate the Ruby/PHP bindings to use the newer, simpler interfaces
instead.  Please do not use the old interfaces in new code.

## Lua bindings

This repo has some Lua bindings for the core library.  These are
experimental and very incomplete.  These are currently included in
order to validate that the C API is suitable for wrapping.  As the
project matures these Lua bindings may become publicly available.

## Contact

Author: Josh Haberman ([jhaberman@gmail.com](mailto:jhaberman@gmail.com),
[haberman@google.com](mailto:haberman@google.com))
