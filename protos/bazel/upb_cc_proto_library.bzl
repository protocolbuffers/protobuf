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

"""Public rules for using upb protos:
  - upb_cc_proto_library()
"""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("//bazel:upb_proto_library.bzl", "GeneratedSrcsInfo", "UpbWrappedCcInfo", "upb_proto_library_aspect")

# begin:google_only
# load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain", "use_cpp_toolchain")
#
# end:google_only
# begin:github_only
# Compatibility code for Bazel 4.x. Remove this when we drop support for Bazel 4.x.
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")

def use_cpp_toolchain():
    return ["@bazel_tools//tools/cpp:toolchain_type"]
# end:github_only

# Generic support code #########################################################

# begin:github_only
_is_google3 = False
# end:github_only

# begin:google_only
# _is_google3 = True
# end:google_only

def _get_real_short_path(file):
    # For some reason, files from other archives have short paths that look like:
    #   ../com_google_protobuf/google/protobuf/descriptor.proto
    short_path = file.short_path
    if short_path.startswith("../"):
        second_slash = short_path.index("/", 3)
        short_path = short_path[second_slash + 1:]

    # Sometimes it has another few prefixes like:
    #   _virtual_imports/any_proto/google/protobuf/any.proto
    #   benchmarks/_virtual_imports/100_msgs_proto/benchmarks/100_msgs.proto
    # We want just google/protobuf/any.proto.
    virtual_imports = "_virtual_imports/"
    if virtual_imports in short_path:
        short_path = short_path.split(virtual_imports)[1].split("/", 1)[1]
    return short_path

def _get_real_root(file):
    real_short_path = _get_real_short_path(file)
    return file.path[:-len(real_short_path) - 1]

def _generate_output_file(ctx, src, extension):
    real_short_path = _get_real_short_path(src)
    real_short_path = paths.relativize(real_short_path, ctx.label.package)
    output_filename = paths.replace_extension(real_short_path, extension)
    ret = ctx.actions.declare_file(output_filename)
    return ret

def _filter_none(elems):
    out = []
    for elem in elems:
        if elem:
            out.append(elem)
    return out

def _cc_library_func(ctx, name, hdrs, srcs, copts, dep_ccinfos):
    """Like cc_library(), but callable from rules.

    Args:
      ctx: Rule context.
      name: Unique name used to generate output files.
      hdrs: Public headers that can be #included from other rules.
      srcs: C/C++ source files.
      copts: Additional options for cc compilation.
      dep_ccinfos: CcInfo providers of dependencies we should build/link against.

    Returns:
      CcInfo provider for this compilation.
    """

    compilation_contexts = [info.compilation_context for info in dep_ccinfos]
    linking_contexts = [info.linking_context for info in dep_ccinfos]
    toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )

    blaze_only_args = {}

    if _is_google3:
        blaze_only_args["grep_includes"] = ctx.file._grep_includes

    (compilation_context, compilation_outputs) = cc_common.compile(
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        name = name,
        srcs = srcs,
        public_hdrs = hdrs,
        user_compile_flags = copts,
        compilation_contexts = compilation_contexts,
        **blaze_only_args
    )

    # buildifier: disable=unused-variable
    (linking_context, linking_outputs) = cc_common.create_linking_context_from_compilation_outputs(
        actions = ctx.actions,
        name = name,
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        compilation_outputs = compilation_outputs,
        linking_contexts = linking_contexts,
        **blaze_only_args
    )

    return CcInfo(
        compilation_context = compilation_context,
        linking_context = linking_context,
    )

# Dummy rule to expose select() copts to aspects  ##############################

UpbCcProtoLibraryCoptsInfo = provider(
    "Provides copts for upb cc proto targets",
    fields = {
        "copts": "copts for upb_cc_proto_library()",
    },
)

def upb_cc_proto_library_copts_impl(ctx):
    return UpbCcProtoLibraryCoptsInfo(copts = ctx.attr.copts)

upb_cc_proto_library_copts = rule(
    implementation = upb_cc_proto_library_copts_impl,
    attrs = {"copts": attr.string_list(default = [])},
)

_UpbCcWrappedCcInfo = provider("Provider for cc_info for protos", fields = ["cc_info"])
_WrappedCcGeneratedSrcsInfo = provider("Provider for generated sources", fields = ["srcs"])

def _compile_upb_cc_protos(ctx, generator, proto_info, proto_sources):
    if len(proto_sources) == 0:
        return GeneratedSrcsInfo(srcs = [], hdrs = [])

    tool = getattr(ctx.executable, "_gen_" + generator)
    srcs = [_generate_output_file(ctx, name, ".upb.proto.cc") for name in proto_sources]
    hdrs = [_generate_output_file(ctx, name, ".upb.proto.h") for name in proto_sources]
    hdrs += [_generate_output_file(ctx, name, ".upb.fwd.h") for name in proto_sources]
    transitive_sets = proto_info.transitive_descriptor_sets.to_list()

    args = ctx.actions.args()
    args.use_param_file(param_file_arg = "@%s")
    args.set_param_file_format("multiline")

    args.add("--" + generator + "_out=" + _get_real_root(srcs[0]))
    args.add("--plugin=protoc-gen-" + generator + "=" + tool.path)
    args.add("--descriptor_set_in=" + ctx.configuration.host_path_separator.join([f.path for f in transitive_sets]))
    args.add_all(proto_sources, map_each = _get_real_short_path)

    ctx.actions.run(
        inputs = depset(
            direct = [proto_info.direct_descriptor_set],
            transitive = [proto_info.transitive_descriptor_sets],
        ),
        tools = [tool],
        outputs = srcs + hdrs,
        executable = ctx.executable._protoc,
        arguments = [args],
        progress_message = "Generating upb cc protos for :" + ctx.label.name,
    )
    return GeneratedSrcsInfo(srcs = srcs, hdrs = hdrs)

def _upb_cc_proto_rule_impl(ctx):
    if len(ctx.attr.deps) != 1:
        fail("only one deps dependency allowed.")
    dep = ctx.attr.deps[0]

    if _WrappedCcGeneratedSrcsInfo in dep:
        srcs = dep[_WrappedCcGeneratedSrcsInfo].srcs
    else:
        fail("proto_library rule must generate _WrappedCcGeneratedSrcsInfo (aspect should have " +
             "handled this).")

    if _UpbCcWrappedCcInfo in dep:
        cc_info = dep[_UpbCcWrappedCcInfo].cc_info
    elif UpbWrappedCcInfo in dep:
        cc_info = dep[UpbWrappedCcInfo].cc_info
    else:
        fail("proto_library rule must generate UpbWrappedCcInfo or " +
             "_UpbCcWrappedCcInfo (aspect should have handled this).")

    lib = cc_info.linking_context.linker_inputs.to_list()[0].libraries[0]
    files = _filter_none([
        lib.static_library,
        lib.pic_static_library,
        lib.dynamic_library,
    ])
    return [
        DefaultInfo(files = depset(files + srcs.hdrs + srcs.srcs)),
        srcs,
        cc_info,
    ]

def _upb_cc_proto_aspect_impl(target, ctx, generator, cc_provider, file_provider):
    proto_info = target[ProtoInfo]
    files = _compile_upb_cc_protos(ctx, generator, proto_info, proto_info.direct_sources)
    deps = ctx.rule.attr.deps + getattr(ctx.attr, "_" + generator)
    dep_ccinfos = [dep[CcInfo] for dep in deps if CcInfo in dep]
    dep_ccinfos += [dep[UpbWrappedCcInfo].cc_info for dep in deps if UpbWrappedCcInfo in dep]
    dep_ccinfos += [dep[_UpbCcWrappedCcInfo].cc_info for dep in deps if _UpbCcWrappedCcInfo in dep]
    if UpbWrappedCcInfo not in target:
        fail("Target should have UpbWrappedCcInfo provider")
    dep_ccinfos.append(target[UpbWrappedCcInfo].cc_info)
    cc_info = _cc_library_func(
        ctx = ctx,
        name = ctx.rule.attr.name + "." + generator,
        hdrs = files.hdrs,
        srcs = files.srcs,
        copts = ctx.attr._ccopts[UpbCcProtoLibraryCoptsInfo].copts,
        dep_ccinfos = dep_ccinfos,
    )
    return [cc_provider(cc_info = cc_info), file_provider(srcs = files)]

def _upb_cc_proto_library_aspect_impl(target, ctx):
    return _upb_cc_proto_aspect_impl(target, ctx, "upbprotos", _UpbCcWrappedCcInfo, _WrappedCcGeneratedSrcsInfo)

def _maybe_add(d):
    if _is_google3:
        d["_grep_includes"] = attr.label(
            allow_single_file = True,
            cfg = "exec",
            default = "@bazel_tools//tools/cpp:grep-includes",
        )
    return d

_upb_cc_proto_library_aspect = aspect(
    attrs = _maybe_add({
        "_ccopts": attr.label(
            default = "//protos:upb_cc_proto_library_copts__for_generated_code_only_do_not_use",
        ),
        "_gen_upbprotos": attr.label(
            executable = True,
            cfg = "exec",
            default = "//protos_generator:protoc-gen-upb-protos",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "exec",
            default = "@com_google_protobuf//:protoc",
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
        "_upbprotos": attr.label_list(
            default = [
                # TODO: Add dependencies for cc runtime (absl/string etc..)
                "//:generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
                "//protos:generated_protos_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
                "@com_google_absl//absl/strings",
                "@com_google_absl//absl/status:statusor",
                "//protos",
            ],
        ),
    }),
    implementation = _upb_cc_proto_library_aspect_impl,
    provides = [
        _UpbCcWrappedCcInfo,
        _WrappedCcGeneratedSrcsInfo,
    ],
    required_aspect_providers = [
        UpbWrappedCcInfo,
    ],
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = use_cpp_toolchain(),
    incompatible_use_toolchain_transition = True,
)

upb_cc_proto_library = rule(
    output_to_genfiles = True,
    implementation = _upb_cc_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [
                upb_proto_library_aspect,
                _upb_cc_proto_library_aspect,
            ],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
        "_ccopts": attr.label(
            default = "//protos:upb_cc_proto_library_copts__for_generated_code_only_do_not_use",
        ),
    },
)
