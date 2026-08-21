# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""This file implements rust_proto_library (rule implementations)."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_rust//rust:defs.bzl", "rust_common")
load("@rules_rust//rust:rust_common.bzl", "CrateInfo", "DepInfo")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load(
    "//rust/bazel:aspects.bzl",
    "RustProtoInfo",
    "label_to_crate_name",
    "proto_rust_toolchain_label",
    "rust_cc_proto_library_aspect",
    "rust_upb_proto_library_aspect",
)

ProtoCrateNamesInfo = provider(
    doc = """A provider that contains both names a Protobuf crate has throughout the build.""",
    fields = {
        "crate_name": "The name of rust_proto_library.",
        "old_crate_name": "The name of the proto_library.",
    },
)

def _user_visible_label(ctx):
    label = str(ctx.label)
    label = label.removesuffix("_cpp_rust_proto")
    label = label.removesuffix("_upb_rust_proto")
    return label + "_rust_proto"

def _rust_proto_library_impl(ctx):
    if not ctx.label.name.endswith("_rust_proto"):
        fail(
            "{}: Name of rust_proto_library target should end with `_rust_proto`."
                .format(_user_visible_label(ctx)),
        )
    deps = ctx.attr.deps
    if not deps:
        fail(
            "{}: Exactly 1 dependency in `deps` attribute expected, none were provided."
                .format(_user_visible_label(ctx)),
        )
    if len(deps) > 1:
        fail(
            "{}: Exactly 1 dependency in `deps` attribute expected, too many were provided."
                .format(_user_visible_label(ctx)),
        )

    dep = deps[0]
    rust_proto_info = dep[RustProtoInfo]

    proto_common.check_collocated(
        ctx.label,
        dep[ProtoInfo],
        ctx.attr._proto_lang_toolchain[proto_common.ProtoLangToolchainInfo],
    )

    if len(rust_proto_info.dep_variant_infos) != 1:
        fail(
            "{}: rust_proto_library does not support src-less proto_library targets."
                .format(_user_visible_label(ctx)),
        )
    dep_variant_info = rust_proto_info.dep_variant_infos[0]
    crate_info = dep_variant_info.crate_info

    # Change the crate name from the name of the proto_library to the name of the rust_proto_library.
    #
    # When the aspect visits proto_libraries, it doesn't know and cannot deduce the name of the
    # rust_proto_library (although the name of rust_proto_libraries is consistently ending with
    # _rust_proto, we can't rely on all proto_libraries to have a name consistently ending with
    # _proto), therefore we have to modify it after the fact here.
    #
    # Since Starlark providers are frozen once they leave the _impl function that defines them,
    # we have to create a shallow copy.
    toolchain = ctx.toolchains["@rules_rust//rust:toolchain_type"]
    fields = {field: getattr(crate_info, field) for field in dir(crate_info)}

    # Construct a label and compute the crate name.
    # The label's package and workspace root are only relevant when 1P crate renaming is enabled.
    # The current implementation of crate renaming supports only monorepos which
    # means that it will only rename when label.workspace_root is empty.
    label_str = _user_visible_label(ctx)
    label = Label(label_str)
    fields["name"] = label_to_crate_name(ctx, label, toolchain)

    # These two fields present on the dir(crate_info) but break on some versions of Bazel when
    # passed back in to crate_info. Strip them for now.
    fields.pop("to_json", None)
    fields.pop("to_proto", None)

    crate_info_with_rust_proto_name = rust_common.crate_info(**fields)

    return [
        ProtoCrateNamesInfo(
            crate_name = crate_info_with_rust_proto_name.name,
            old_crate_name = crate_info.name,
        ),
        crate_info_with_rust_proto_name,
        dep_variant_info.dep_info,
        dep_variant_info.cc_info,
        DefaultInfo(files = dep_variant_info.crate_info.srcs),
    ]

def _make_rust_proto_library(is_upb):
    return rule(
        implementation = _rust_proto_library_impl,
        attrs = {
            "deps": attr.label_list(
                mandatory = True,
                providers = [ProtoInfo],
                aspects = [rust_upb_proto_library_aspect if is_upb else rust_cc_proto_library_aspect],
            ),
            "_proto_lang_toolchain": attr.label(
                default = Label(proto_rust_toolchain_label(is_upb)),
            ),
        },
        toolchains = [
            "@rules_rust//rust:toolchain_type",
        ],
        provides = [ProtoCrateNamesInfo, CrateInfo, DepInfo, CcInfo],
    )

rust_upb_proto_library = _make_rust_proto_library(is_upb = True)
rust_cc_proto_library = _make_rust_proto_library(is_upb = False)
