# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(":pyproto_test_wrapper.bzl", "pyproto_test_wrapper")

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

pyproto_test_wrapper(name = "descriptor_pool_test")

pyproto_test_wrapper(name = "descriptor_test")

pyproto_test_wrapper(name = "generator_test")

pyproto_test_wrapper(name = "message_factory_test")

pyproto_test_wrapper(name = "proto_builder_test")

pyproto_test_wrapper(name = "message_test")

pyproto_test_wrapper(name = "reflection_test")

filegroup(
    name = "test_files",
    srcs = glob(["*.py"]),
    visibility = [
        "//python/dist:__pkg__",  # Scheuklappen: keep
    ],
)
