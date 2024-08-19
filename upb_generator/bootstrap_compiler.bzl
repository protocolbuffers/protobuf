"""Macros that implement bootstrapping for the upb code generator."""

# begin:github_only
load(
    "//bazel:upb_minitable_proto_library.bzl",
    "upb_minitable_proto_library",
)
# end:github_only

load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
)
load(
    "//upb/cmake:build_defs.bzl",
    "staleness_test",
)

_stages = ["_stage0", "_stage1", ""]
_protoc = "//:protoc"
_upbc_base = "//upb_generator:protoc-gen-"

# begin:google_only
# _is_google3 = True
# _extra_proto_path = ""
# end:google_only

# begin:github_only
_is_google3 = False
_extra_proto_path = "-I$$(dirname $(location @com_google_protobuf//:descriptor_proto_srcs))/../.. "
# end:github_only

def _upbc(generator, stage):
    return _upbc_base + generator + _stages[stage]

def bootstrap_cc_library(name, visibility, deps = [], bootstrap_deps = [], **kwargs):
    for stage in _stages:
        stage_visibility = visibility if stage == "" else ["//upb_generator:__pkg__"]
        native.cc_library(
            name = name + stage,
            deps = deps + [dep + stage for dep in bootstrap_deps],
            visibility = stage_visibility,
            **kwargs
        )

def bootstrap_cc_binary(name, deps = [], bootstrap_deps = [], **kwargs):
    for stage in _stages:
        native.cc_binary(
            name = name + stage,
            deps = deps + [dep + stage for dep in bootstrap_deps],
            **kwargs
        )

def _generated_file(proto, stage, generator, suffix):
    stripped = proto[:-len(".proto")]
    return "{}/{}.{}.{}".format(stage, stripped, generator, suffix)

def _generated_files(protos, stage, generator, suffix):
    return [_generated_file(proto, stage, generator, suffix) for proto in protos]

def _generated_hdrs_and_srcs(protos, stage, generator):
    ret = _generated_files(protos, stage, generator, "h")
    if generator != "upb" or stage == "stage0":
        ret += _generated_files(protos, stage, generator, "c")
    return ret

def _stage0_proto_staleness_test(name, src_files, src_rules, strip_prefix):
    native.genrule(
        name = name + "_generate_bootstrap",
        srcs = src_rules,
        outs = ["bootstrap_generated_sources/" + f for f in _generated_hdrs_and_srcs(src_files, "stage0", "upb")],
        tools = [_protoc, _upbc("upb", 0)],
        cmd =
            "$(location " + _protoc + ") " +
            "-I$(GENDIR)/" + strip_prefix + " " + _extra_proto_path +
            "--plugin=protoc-gen-upb=$(location " + _upbc("upb", 0) + ") " +
            "--upb_out=bootstrap_stage=0:$(@D)/bootstrap_generated_sources/stage0 " +
            " ".join(src_files),
    )

    staleness_test(
        name = name + "_stage0_staleness_test",
        outs = _generated_hdrs_and_srcs(src_files, "stage0", "upb"),
        generated_pattern = "bootstrap_generated_sources/%s",
        target_files = native.glob(["stage0/**"]),
        # To avoid skew problems for descriptor.proto/pluging.proto between
        # GitHub repos.  It's not critical that the checked-in protos are up to
        # date for every change, they just needs to be complete enough to have
        # everything needed by the code generator itself.
        tags = ["manual"],
    )

def _generate_stage1_proto(name, src_files, src_rules, generator, kwargs):
    native.genrule(
        name = "gen_{}_{}_stage1".format(name, generator),
        srcs = src_rules,
        outs = _generated_hdrs_and_srcs(src_files, "stage1", generator),
        cmd = "$(location " + _protoc + ") " +
              "--plugin=protoc-gen-" + generator +
              "=$(location " + _upbc(generator, 0) + ") " + _extra_proto_path +
              "--" + generator + "_out=bootstrap_stage=1:$(RULEDIR)/stage1 " +
              " ".join(src_files),
        visibility = ["//upb_generator:__pkg__"],
        tools = [
            _protoc,
            _upbc(generator, 0),
        ],
        **kwargs
    )

# begin:github_only
def _cmake_staleness_test(name, src_files, proto_lib_deps, **kwargs):
    upb_minitable_proto_library(
        name = name + "_minitable",
        deps = proto_lib_deps,
        **kwargs
    )

    # Copy the final gencode for staleness comparison
    files = _generated_hdrs_and_srcs(src_files, "cmake", "upb") + \
            _generated_hdrs_and_srcs(src_files, "cmake", "upb_minitable")
    genrule = 0
    for src in files:
        genrule += 1
        native.genrule(
            name = name + "_copy_gencode_%d" % genrule,
            outs = ["generated_sources/" + src],
            srcs = [name + "_upb_proto", name + "_minitable"],
            cmd = """
                mkdir -p $(@D)
                for src in $(SRCS); do
                    if [[ $$src == *%s ]]; then
                        cp -f $$src $(@D) || echo 'copy failed!'
                    fi
                done
            """ % src[src.rfind("/"):],
        )

    # Keep bazel gencode in sync with our checked-in sources needed for cmake builds.
    staleness_test(
        name = name + "_staleness_test",
        outs = files,
        generated_pattern = "generated_sources/%s",
        tags = ["manual"],
    )

# end:github_only

def bootstrap_upb_proto_library(
        name,
        bootstrap_hdr,
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
    _stage0_proto_staleness_test(name, oss_src_files, oss_src_rules, oss_strip_prefix)

    # stage0 uses checked-in protos, and has no MiniTable.
    native.cc_library(
        name = name + "_stage0",
        srcs = _generated_hdrs_and_srcs(oss_src_files, "stage0", "upb"),
        hdrs = [bootstrap_hdr],
        visibility = ["//upb_generator:__pkg__"],
        defines = ["UPB_BOOTSTRAP_STAGE=0"],
        deps = [
            "//upb:generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
            "//upb:mini_table",
        ] + [dep + "_stage0" for dep in deps],
        **kwargs
    )

    src_files = google3_src_files if _is_google3 else oss_src_files
    src_rules = google3_src_rules if _is_google3 else oss_src_rules

    # Generate stage1 protos (C API and MiniTables) using stage0 compiler.
    _generate_stage1_proto(name, src_files, src_rules, "upb", kwargs)
    _generate_stage1_proto(name, src_files, src_rules, "upb_minitable", kwargs)

    native.cc_library(
        name = name + "_minitable_stage1",
        srcs = _generated_files(src_files, "stage1", "upb_minitable", "c"),
        hdrs = _generated_files(src_files, "stage1", "upb_minitable", "h"),
        visibility = ["//upb_generator:__pkg__"],
        defines = ["UPB_BOOTSTRAP_STAGE=1"],
        deps = [
            "//upb:generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ] + [dep + "_minitable_stage1" for dep in deps],
        **kwargs
    )
    native.cc_library(
        name = name + "_stage1",
        srcs = _generated_files(src_files, "stage1", "upb", "h"),
        hdrs = [bootstrap_hdr],
        visibility = ["//upb_generator:__pkg__"],
        defines = ["UPB_BOOTSTRAP_STAGE=1"],
        deps = [
            "//upb:generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
            ":" + name + "_minitable_stage1",
        ] + [dep + "_minitable_stage1" for dep in deps],
        **kwargs
    )

    # The final protos are generated via normal upb_proto_library().
    upb_proto_library(
        name = name + "_upb_proto",
        deps = proto_lib_deps,
        **kwargs
    )
    native.cc_library(
        name = name,
        hdrs = [bootstrap_hdr],
        deps = [name + "_upb_proto"],
        visibility = visibility,
        **kwargs
    )

    # begin:github_only
    _cmake_staleness_test(name, src_files, proto_lib_deps, **kwargs)

# end:github_only
