# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Declares the crates rendered by //rust/crates_vendor:crates_vendor."""

load("//rust/crates_vendor/crates:crates.bzl", "crate_repositories")

def _crates_impl(module_ctx):
    crate_repositories()

    # Only the hub is meant to be used directly, the per-crate repositories
    # are implementation details.
    return module_ctx.extension_metadata(
        root_module_direct_deps = ["crate_index"],
        root_module_direct_dev_deps = [],
        reproducible = True,
    )

crates = module_extension(implementation = _crates_impl)
