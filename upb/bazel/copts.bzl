# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Common copts for building upb."""

visibility([
    "//lua/...",
    "//python/...",
    "//upb/...",
    "//upb_generator/...",
])

_DEFAULT_CPPOPTS = []
_DEFAULT_COPTS = [
    # this is a compile error in C++ clang and GNU C, but not clang C by default
    "-Werror=incompatible-pointer-types",
    # GCC does not seem to support the no_sanitize attribute in some places
    # where we use it.
    "-Wno-error=attributes",
]

_DEFAULT_CPPOPTS.extend([
    "-Wextra",
    # "-Wshorten-64-to-32",  # not in GCC (and my Kokoro images doesn't have Clang)
    "-Wno-unused-parameter",
    "-Wno-long-long",
])
_DEFAULT_COPTS.extend([
    "-std=c99",
    "-Wall",
    "-Wstrict-prototypes",
    # GCC (at least) emits spurious warnings for this that cannot be fixed
    # without introducing redundant initialization (with runtime cost):
    #   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80635
    #"-Wno-maybe-uninitialized",
])

UPB_DEFAULT_CPPOPTS = select({
    "//upb:windows": [],
    "//conditions:default": _DEFAULT_CPPOPTS,
}) + select({
    "//upb:fasttable_enabled_setting": ["-DUPB_ENABLE_FASTTABLE"],
    "//conditions:default": [],
}) + select({
    "//conditions:default": [],
})

UPB_DEFAULT_COPTS = select({
    "//upb:windows": [],
    "//conditions:default": _DEFAULT_COPTS,
}) + select({
    "//upb:fasttable_enabled_setting": ["-DUPB_ENABLE_FASTTABLE"],
    "//conditions:default": [],
}) + select({
    "//conditions:default": [],
})
