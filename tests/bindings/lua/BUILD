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
    "//upb/bindings/lua:lua_proto_library.bzl",
    "lua_proto_library",
)
load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
)

licenses(["notice"])

cc_test(
    name = "test_lua",
    srcs = ["main.c"],
    copts = UPB_DEFAULT_COPTS,
    data = [
        "test_upb.lua",
        ":descriptor_proto_lua",
        ":empty_proto_lua",
        ":test_messages_proto2_proto_lua",
        ":test_messages_proto3_proto_lua",
        ":test_proto_lua",
        "//:third_party/lunit/console.lua",
        "//:third_party/lunit/lunit.lua",
        "//upb/bindings/lua:upb.lua",
        "@com_google_protobuf//:conformance_proto",
        "@com_google_protobuf//:descriptor_proto",
    ],
    linkstatic = 1,
    deps = [
        "//upb/bindings/lua:lupb",
        "@lua//:liblua",
    ],
)

proto_library(
    name = "test_proto",
    testonly = 1,
    srcs = ["test.proto"],
    deps = ["@com_google_protobuf//:timestamp_proto"],
)

lua_proto_library(
    name = "test_proto_lua",
    testonly = 1,
    deps = [":test_proto"],
)

lua_proto_library(
    name = "descriptor_proto_lua",
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

lua_proto_library(
    name = "empty_proto_lua",
    deps = ["@com_google_protobuf//:empty_proto"],
)

lua_proto_library(
    name = "test_messages_proto3_proto_lua",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto3_proto"],
)

lua_proto_library(
    name = "test_messages_proto2_proto_lua",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto2_proto"],
)
