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
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:string_view",
    ],
)
