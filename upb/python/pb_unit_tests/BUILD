# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(":pyproto_test_wrapper.bzl", "pyproto_test_wrapper")
load("@pip_deps//:requirements.bzl", "requirement")

licenses(["notice"])

pyproto_test_wrapper(name = "descriptor_database_test")

pyproto_test_wrapper(name = "descriptor_pool_test")

pyproto_test_wrapper(name = "descriptor_test")

pyproto_test_wrapper(name = "generator_test")

pyproto_test_wrapper(name = "json_format_test")

pyproto_test_wrapper(name = "keywords_test")

pyproto_test_wrapper(name = "message_factory_test")

# This target has different dependencies and fails when using the wrapper
# TODO: Move this to using pyproto_test_wrapper
py_test(
    name = "numpy_test",
    srcs = ["numpy_test_wrapper.py"],
    main = "numpy_test_wrapper.py",
    deps = [
      requirement("numpy"),
      "//python/google/protobuf/internal/numpy:numpy_test",
      "//upb/python:_message",
    ],
    target_compatible_with = select({
      "@system_python//:supported": [],
      "//conditions:default": ["@platforms//:incompatible"],
    }),
)

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
        "//upb/python/dist:__pkg__",  # Scheuklappen: keep
    ],
)
