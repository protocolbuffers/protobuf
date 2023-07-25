"""Macros that implement bootstrapping for the upb code generator."""

load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
)
load(
    "//cmake:build_defs.bzl",
    "staleness_test",
)

_stages = ["_stage0", "_stage1", ""]
_protoc = "@com_google_protobuf//:protoc"
_upbc_base = "//upbc:protoc-gen-upb"

# begin:google_only
# _is_google3 = True
# _extra_proto_path = ""
# end:google_only

# begin:github_only
_is_google3 = False
_extra_proto_path = "-Iexternal/com_google_protobuf/src "
# end:github_only

def _upbc(stage):
    return _upbc_base + _stages[stage]

def bootstrap_cc_library(name, visibility, deps, bootstrap_deps, **kwargs):
    for stage in _stages:
        stage_visibility = visibility if stage == "" else ["//upbc:__pkg__"]
        native.cc_library(
            name = name + stage,
            deps = deps + [dep + stage for dep in bootstrap_deps],
            visibility = stage_visibility,
            **kwargs
        )

def bootstrap_cc_binary(name, deps, bootstrap_deps, **kwargs):
    for stage in _stages:
        native.cc_binary(
            name = name + stage,
            deps = deps + [dep + stage for dep in bootstrap_deps],
            **kwargs
        )

def _generated_srcs_for_suffix(prefix, srcs, suffix):
    return [prefix + "/" + src[:-len(".proto")] + suffix for src in srcs]

def _generated_srcs(prefix, srcs):
    return _generated_srcs_for_suffix(prefix, srcs, ".upb.h") + _generated_srcs_for_suffix(prefix, srcs, ".upb.c")

def _stage0_proto_staleness_test(name, base_dir, src_files, src_rules, strip_prefix):
    native.genrule(
        name = name + "_generate_bootstrap",
        srcs = src_rules,
        outs = _generated_srcs("bootstrap_generated_sources/" + base_dir + "stage0", src_files),
        tools = [_protoc, _upbc(0)],
        cmd =
            "$(location " + _protoc + ") " +
            "-I$(GENDIR)/" + strip_prefix + " " + _extra_proto_path +
            "--plugin=protoc-gen-upb=$(location " + _upbc(0) + ") " +
            "--upb_out=bootstrap_upb:$(@D)/bootstrap_generated_sources/" + base_dir + "stage0 " +
            " ".join(src_files),
    )

    staleness_test(
        name = name + "_staleness_test",
        outs = _generated_srcs(base_dir + "stage0", src_files),
        generated_pattern = "bootstrap_generated_sources/%s",
        target_files = native.glob([base_dir + "stage0/**"]),
        # To avoid skew problems for descriptor.proto/pluging.proto between
        # GitHub repos.  It's not critical that the checked-in protos are up to
        # date for every change, they just needs to be complete enough to have
        # everything needed by the code generator itself.
        tags = ["manual"],
    )

def bootstrap_upb_proto_library(
        name,
        base_dir,
        google3_src_files,
        google3_src_rules,
        oss_src_files,
        oss_src_rules,
        oss_strip_prefix,
        proto_lib_deps,
        visibility,
        deps = [],
        **kwargs):
    """A version of upb_proto_library() that is augmented to allow for bootstrapping the compiler.

    Args:
        name: Name of this rule.  This name will resolve to a upb_proto_library().
        base_dir: The directory that all generated files should be placed under.
        google3_src_files: Google3 filenames of .proto files that should be built by this rule.
          The names should be relative to the depot base.
        google3_src_rules: Target names of the Blaze rules that will provide these filenames.
        oss_src_files: OSS filenames of .proto files that should be built by this rule.
        oss_src_rules: Target names of the Bazel rules that will provide these filenames.
        oss_strip_prefix: Prefix that should be stripped from OSS file names.
        proto_lib_deps: proto_library() rules that we will use to build the protos when we are
          not bootstrapping.
        visibility: Visibility list for the final upb_proto_library() rule.  Bootstrapping rules
          will always be hidden, and will not honor the visibility parameter passed here.
        deps: other bootstrap_upb_proto_library() rules that this one depends on.
        **kwargs: Other arguments that will be passed through to cc_library(), genrule(), and
          upb_proto_library().
    """
    _stage0_proto_staleness_test(name, base_dir, oss_src_files, oss_src_rules, oss_strip_prefix)

    # stage0 uses checked-in protos.
    native.cc_library(
        name = name + "_stage0",
        srcs = _generated_srcs_for_suffix(base_dir + "stage0", oss_src_files, ".upb.c"),
        hdrs = _generated_srcs_for_suffix(base_dir + "stage0", oss_src_files, ".upb.h"),
        includes = [base_dir + "stage0"],
        visibility = ["//upbc:__pkg__"],
        # This macro signals to the runtime that it must use OSS APIs for descriptor.proto/plugin.proto.
        defines = ["UPB_BOOTSTRAP_STAGE0"],
        deps = [
            "//:generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
            "//:mini_table",
        ] + [dep + "_stage0" for dep in deps],
        **kwargs
    )

    src_files = google3_src_files if _is_google3 else oss_src_files
    src_rules = google3_src_rules if _is_google3 else oss_src_rules

    # Generate stage1 protos using stage0 compiler.
    native.genrule(
        name = "gen_" + name + "_stage1",
        srcs = src_rules,
        outs = _generated_srcs(base_dir + "stage1", src_files),
        cmd = "$(location " + _protoc + ") " +
              "--plugin=protoc-gen-upb=$(location " + _upbc(0) + ") " + _extra_proto_path +
              "--upb_out=$(@D)/" + base_dir + "stage1 " +
              " ".join(src_files),
        visibility = ["//upbc:__pkg__"],
        tools = [
            _protoc,
            _upbc(0),
        ],
        **kwargs
    )

    native.cc_library(
        name = name + "_stage1",
        srcs = _generated_srcs_for_suffix(base_dir + "stage1", src_files, ".upb.c"),
        hdrs = _generated_srcs_for_suffix(base_dir + "stage1", src_files, ".upb.h"),
        includes = [base_dir + "stage1"],
        visibility = ["//upbc:__pkg__"],
        deps = [
            "//:generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ] + [dep + "_stage1" for dep in deps],
        **kwargs
    )

    # The final protos are generated via normal upb_proto_library().
    upb_proto_library(
        name = name,
        deps = proto_lib_deps,
        visibility = visibility,
        **kwargs
    )
