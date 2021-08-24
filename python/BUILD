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
    "@system_python//:build_defs.bzl",
    "EXT_SUFFIX",
)

cc_binary(
    name = "message",
    srcs = [
        "descriptor.c",
        "descriptor.h",
        "descriptor_pool.c",
        "descriptor_pool.h",
        "protobuf.c",
        "protobuf.h",
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
        "//:upb",
        "@system_python//:python_headers",
    ],
)

# Copy the extension into the location recognized by Python.
genrule(
    name = "message_ext",
    srcs = [":message"],
    outs = ["google/protobuf/pyext/_message" + EXT_SUFFIX],
    cmd = "cp $< $@",
)

py_test(
    name = "minimal_test",
    srcs = ["minimal_test.py"],
    data = [":message_ext"],
    imports = ["."],
    legacy_create_init = False,
)
