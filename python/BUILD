# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
)
load(
    "//bazel:py_proto_library.bzl",
    "py_proto_library",
)
load(
    "@rules_python//python:packaging.bzl",
    "py_wheel",
)

cc_binary(
    name = "message",
    srcs = [
        "convert.c",
        "convert.h",
        "descriptor.c",
        "descriptor.h",
        "descriptor_containers.c",
        "descriptor_containers.h",
        "descriptor_pool.c",
        "descriptor_pool.h",
        "extension_dict.c",
        "extension_dict.h",
        "map.c",
        "map.h",
        "message.c",
        "message.h",
        "protobuf.c",
        "protobuf.h",
        "python.h",
        "repeated.c",
        "repeated.h",
    ],
    copts = UPB_DEFAULT_COPTS + [
        # The Python API requires patterns that are ISO C incompatible, like
        # casts between function pointers and object pointers.
        "-Wno-pedantic",
    ],
    # We use a linker script to hide all symbols except the entry point for
    # the module.
    linkopts = select({
        "@platforms//os:linux": ["-Wl,--version-script,$(location :version_script.lds)"],
        "@platforms//os:macos": ["-Wl,-exported_symbol", "-Wl,_PyInit__message"],
    }),
    linkshared = True,
    linkstatic = True,
    deps = [
        ":version_script.lds",
        "//:reflection",
        "//:textformat",
        "//:upb",
        "//:descriptor_upb_proto_reflection",
        "//upb/util:compare",
        "//upb/util:def_to_proto",
        "//upb/util:required_fields",
        "@system_python//:python_headers",
    ],
)

cc_binary(
    name = "api_implementation",
    srcs = [
        "api_implementation.c",
    ],
    # Enable once linker script is available.
    #copts = ["-fvisibility=hidden"],
    deps = ["@system_python//:python_headers"],
    linkshared = True,
    linkstatic = True,
)

# Copy the extensions into the location recognized by Python.
# .abi3.so indicates use of the limited API, and cross-version ABI compatibility.
EXT_SUFFIX = ".abi3.so"

genrule(
    name = "copy_message",
    srcs = [":message"],
    outs = ["google/protobuf/pyext/_message" + EXT_SUFFIX],
    cmd = "cp $< $@",
)

genrule(
    name = "copy_api_implementation",
    srcs = [":api_implementation"],
    outs = ["google/protobuf/internal/_api_implementation" + EXT_SUFFIX],
    cmd = "cp $< $@",
    visibility = ["//python:__subpackages__"],
)

filegroup(
    name = "extension_files",
    srcs = [
        "google/protobuf/pyext/_message" + EXT_SUFFIX,
        "google/protobuf/internal/_api_implementation" + EXT_SUFFIX,
    ],
)

py_library(
    name = "message_ext",
    data = [":extension_files"],
    imports = ["."],
    visibility = ["//python:__subpackages__"],
)

py_test(
    name = "minimal_test",
    srcs = [
        "minimal_test.py",
    ],
    deps = [
        "//python:message_ext",
        "@com_google_protobuf//:python_srcs",
        "@com_google_protobuf//:python_common_test_protos",
        "@com_google_protobuf//:python_specific_test_protos",
    ],
    imports = ["."],
    legacy_create_init = False,
)

py_proto_library(
    name = "well_known_proto_pb2",
    deps = [
        "@com_google_protobuf//:any_proto",
        "@com_google_protobuf//:api_proto",
        "@com_google_protobuf//:compiler_plugin_proto",
        "@com_google_protobuf//:descriptor_proto",
        "@com_google_protobuf//:duration_proto",
        "@com_google_protobuf//:empty_proto",
        "@com_google_protobuf//:field_mask_proto",
        "@com_google_protobuf//:source_context_proto",
        "@com_google_protobuf//:struct_proto",
        "@com_google_protobuf//:timestamp_proto",
        "@com_google_protobuf//:type_proto",
        "@com_google_protobuf//:wrappers_proto",
    ],
)

py_wheel(
    name = "binary_wheel",
    abi = "abi3",
    distribution = "protobuf",
    python_tag = "cp36",
    # TODO(https://github.com/protocolbuffers/upb/issues/502): we need to make
    # this a select() that is calculated from the platform we are actually
    # building on.
    platform = "manylinux2014_x86_64",
    version = "4.20.0",
    strip_path_prefixes = ["python/"],
    deps = [
        ":extension_files",
        ":well_known_proto_pb2",
        # TODO(https://github.com/protocolbuffers/upb/issues/503): currently
        # this includes the unit tests.  We should filter these out so we are
        # only distributing true source files.
        "@com_google_protobuf//:python_srcs",
    ],
)
