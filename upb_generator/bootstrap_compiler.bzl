"""Macros that implement bootstrapping for the upb code generator."""

load(
    "//bazel:upb_minitable_proto_library.bzl",
    "upb_minitable_proto_library",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
)
load(
    "//upb/cmake:build_defs.bzl",
    "staleness_test",
)

_stages = ["_stage0", "_stage1", ""]

_protoc = "//:protoc_stage0"

_extra_proto_path = "-I$$(dirname $(location //:descriptor_proto_srcs))/../.. "

# This visibility is used automatically for anything used by the bootstrapping process.
_bootstrap_visibility = [
    # TODO: b/396430482 - Remove protoc from bootstrap visibility.
    "//src/google/protobuf/compiler:__pkg__",
    "//src/google/protobuf/compiler/rust:__pkg__",
    "//third_party/upb/github:__pkg__",
    "//upb_generator:__subpackages__",
    "//upb/reflection:__pkg__",
    "//upb:__pkg__",  # For the amalgamations.
    "//python/dist:__pkg__",  # For the Python source package.
    "//:__pkg__",  # For protoc
]

def _stage_visibility(stage, visibility):
    return visibility if stage == "" else _bootstrap_visibility

def _upbc(generator, stage):
    if generator == "upb":
        return "//upb_generator/c:protoc-gen-upb" + _stages[stage]
    else:
        return "//upb_generator/minitable:protoc-gen-upb_minitable" + _stages[stage]

def bootstrap_cc_library(name, visibility = [], deps = [], bootstrap_deps = [], **kwargs):
    """A version of cc_library() that is augmented to allow for bootstrapping the compiler.

    In addition to the normal cc_library() target, this rule will also generate _stage0 and _stage1
    targets that are used internally for bootstrapping, and will automatically have bootstrap
    visibility. However the final target will use the normal visibility, and will behave like a
    normal cc_library() target.

    Args:
        name: Name of this rule.  This name will resolve to a upb_proto_library().
        deps: Normal cc_library() deps.
        bootstrap_deps: Special bootstrap_upb_proto_library() or bootstrap_cc_library() deps.
        visibility: Visibility of the final target.
        **kwargs: Other arguments that will be passed through to cc_library().
          upb_proto_library().
    """
    for stage in _stages:
        native.cc_library(
            name = name + stage,
            deps = deps + [dep + stage for dep in bootstrap_deps],
            visibility = _stage_visibility(stage, visibility),
            **kwargs
        )

def bootstrap_cc_binary(name, visibility = [], deps = [], bootstrap_deps = [], **kwargs):
    """A version of cc_binary() that is augmented to allow for bootstrapping the compiler.

    In addition to the normal cc_binary() target, this rule will also generate _stage0 and _stage1
    targets that are used internally for bootstrapping, and will automatically have bootstrap
    visibility. However the final target will use the normal visibility, and will behave like a
    normal cc_binary() target.

    Args:
        name: Name of this rule.  This name will resolve to a upb_proto_library().
        deps: Normal cc_library() deps.
        bootstrap_deps: Special bootstrap_upb_proto_library() or bootstrap_cc_library() deps.
        visibility: Visibility of the final target.
        **kwargs: Other arguments that will be passed through to cc_binary().
          upb_proto_library().
    """
    for stage in _stages:
        native.cc_binary(
            name = name + stage,
            deps = deps + [dep + stage for dep in bootstrap_deps],
            visibility = _stage_visibility(stage, visibility),
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
        outs = ["bootstrap_generated_sources/" + f.replace("third_party", "3rd_party") for f in _generated_hdrs_and_srcs(src_files, "stage0", "upb")],
        tools = [_protoc, _upbc("upb", 0)],
        cmd =
            "$(location " + _protoc + ") " +
            "-I. -I$(GENDIR)/" + strip_prefix + " " + _extra_proto_path +
            "--plugin=protoc-gen-upb=$(location " + _upbc("upb", 0) + ") " +
            "--upb_out=bootstrap_stage=0:$(@D)/bootstrap_generated_sources/stage0 " +
            " ".join(src_files) +
            "; rm -rf $(@D)/bootstrap_generated_sources/stage0/3rd_party" +
            "; if [ -e $(@D)/bootstrap_generated_sources/stage0/third_party ]; then mv $(@D)/bootstrap_generated_sources/stage0/third_party $(@D)/bootstrap_generated_sources/stage0/3rd_party; fi",
    )

    staleness_test(
        name = name + "_stage0_staleness_test",
        outs = [f.replace("third_party", "3rd_party") for f in _generated_hdrs_and_srcs(src_files, "stage0", "upb")],
        generated_pattern = "bootstrap_generated_sources/%s",
        target_files = native.glob(["stage0/**"]),
        # To avoid skew problems for descriptor.proto/plugin.proto between
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
        visibility = _bootstrap_visibility,
        tools = [
            _protoc,
            _upbc(generator, 0),
        ],
        **kwargs
    )

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

def bootstrap_upb_proto_library(
        name,
        bootstrap_hdr,
        src_files,
        src_rules,
        proto_lib_deps,
        deps = [],
        strip_prefix = "",
        **kwargs):
    """A version of upb_proto_library() that is augmented to allow for bootstrapping the compiler.

    Note that this rule is only intended to be used by bootstrap_cc_library() targets. End users
    should use the normal upb_proto_library() targets. As a result, we don't have a visibility
    parameter: all targets will automatically have bootstrap visibility.

    Args:
        name: Name of this rule.  This name will resolve to a upb_proto_library().
        bootstrap_hdr: The forwarding header that exposes the generated code, taking into account
          the current stage.
        src_files: Filenames of .proto files that should be built by this rule.
        src_rules: Target names of the Blaze/Bazel rules that will provide these filenames.
        proto_lib_deps: proto_library() rules that we will use to build the protos when we are
          not bootstrapping.
        deps: other bootstrap_upb_proto_library() rules that this one depends on.
        strip_prefix: Prefix that should be stripped from file names.
        **kwargs: Other arguments that will be passed through to cc_library(), genrule(), and
          upb_proto_library().
    """
    _stage0_proto_staleness_test(name, src_files, src_rules, strip_prefix)

    # stage0 uses checked-in protos, and has no MiniTable.
    native.cc_library(
        name = name + "_stage0",
        srcs = _generated_hdrs_and_srcs(src_files, "stage0", "upb"),
        hdrs = [bootstrap_hdr],
        visibility = _bootstrap_visibility,
        defines = ["UPB_BOOTSTRAP_STAGE=0"],
        deps = [
            "//upb:generated_code_support",
            "//upb:mini_table",
        ] + [dep + "_stage0" for dep in deps],
        **kwargs
    )

    # Generate stage1 protos (C API and MiniTables) using stage0 compiler.
    _generate_stage1_proto(name, src_files, src_rules, "upb", kwargs)
    _generate_stage1_proto(name, src_files, src_rules, "upb_minitable", kwargs)

    native.cc_library(
        name = name + "_minitable_stage1",
        srcs = _generated_files(src_files, "stage1", "upb_minitable", "c"),
        hdrs = _generated_files(src_files, "stage1", "upb_minitable", "h"),
        visibility = _bootstrap_visibility,
        defines = ["UPB_BOOTSTRAP_STAGE=1"],
        deps = [
            "//upb:generated_code_support",
        ] + [dep + "_minitable_stage1" for dep in deps],
        **kwargs
    )
    native.cc_library(
        name = name + "_stage1",
        srcs = _generated_files(src_files, "stage1", "upb", "h"),
        hdrs = [bootstrap_hdr],
        visibility = _bootstrap_visibility,
        defines = ["UPB_BOOTSTRAP_STAGE=1"],
        deps = [
            "//upb:generated_code_support",
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
        visibility = _bootstrap_visibility,
        **kwargs
    )

    _cmake_staleness_test(name, src_files, proto_lib_deps, **kwargs)
