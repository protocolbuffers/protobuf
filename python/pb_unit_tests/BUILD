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

load(":pyproto_test_wrapper.bzl", "pyproto_test_wrapper")
# begin:github_only
load("@pip_deps//:requirements.bzl", "requirement")
# end:github_only

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
# end:google_only

licenses(["notice"])

pyproto_test_wrapper(name = "descriptor_database_test")

pyproto_test_wrapper(name = "descriptor_pool_test")

pyproto_test_wrapper(name = "descriptor_test")

# begin:github_only
pyproto_test_wrapper(name = "generator_test")
# end:github_only

pyproto_test_wrapper(name = "json_format_test")

pyproto_test_wrapper(name = "keywords_test")

pyproto_test_wrapper(name = "message_factory_test")

# begin:github_only
# This target has different dependencies and fails when using the wrapper
# TODO: Move this to using pyproto_test_wrapper
py_test(
    name = "numpy_test",
    srcs = ["numpy_test_wrapper.py"],
    main = "numpy_test_wrapper.py",
    deps = [
      requirement("numpy"),
      "@com_google_protobuf//python/google/protobuf/internal/numpy:numpy_test",
      "//python:_message",
    ],
    target_compatible_with = select({
      "@system_python//:supported": [],
      "//conditions:default": ["@platforms//:incompatible"],
    }),
)
# end:github_only

# begin:google_only
# pyproto_test_wrapper(name = "numpy_test")
# end:google_only

pyproto_test_wrapper(name = "proto_builder_test")

pyproto_test_wrapper(name = "service_reflection_test")

pyproto_test_wrapper(name = "symbol_database_test")

pyproto_test_wrapper(name = "text_encoding_test")

pyproto_test_wrapper(name = "message_test")

pyproto_test_wrapper(name = "reflection_test")

pyproto_test_wrapper(name = "text_format_test")

pyproto_test_wrapper(name = "unknown_fields_test")

pyproto_test_wrapper(name = "well_known_types_test")

pyproto_test_wrapper(name = "wire_format_test")

filegroup(
    name = "test_files",
    srcs = glob(["*.py"]),
    visibility = [
        "//python/dist:__pkg__",  # Scheuklappen: keep
    ],
)
