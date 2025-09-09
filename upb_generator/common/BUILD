# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

package(default_applicable_licenses = ["//:license"])

# Note: this library should not depend on upb reflection or C++ proto
# reflection. It is intended to be a lightweight target that can be used by
# other code generators without pulling in reflection of either library.
cc_library(
    name = "names",
    srcs = ["names.cc"],
    hdrs = ["names.h"],
    visibility = ["//upb_generator:__subpackages__"],
    deps = [
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_library(
    name = "cpp_to_upb_def",
    srcs = ["cpp_to_upb_def.cc"],
    hdrs = ["cpp_to_upb_def.h"],
    visibility = [
        "//src/google/protobuf/compiler/hpb:__subpackages__",
        "//third_party/kotlin/protobuf/generator/native:__subpackages__",
    ],
    deps = [
        "//src/google/protobuf",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:reflection",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
    ],
)
