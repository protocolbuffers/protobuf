"""Starlark definitions for converting proto2 to proto3.

PLEASE DO NOT DEPEND ON THE CONTENTS OF THIS FILE, IT IS UNSTABLE.
"""

load("//:protobuf.bzl", "internal_php_proto_library")

def proto3_from_proto2_data(
    name,
    srcs,
    **kwargs):
    """Transforms proto2 binary data into a proto3-compatible format,

    Args:
      name: the name of the target representing the generated proto files.
      srcs: the source binary protobuf data files.
      **kwargs: standard arguments to forward on
    """
    outs = []
    out_files = []
    src_files = []
    for src in srcs:
        outs.append("proto3/" + src)
        out_files.append("$(RULEDIR)/proto3/" + src)
        src_files.append("$(rootpath %s)" % src);

    native.genrule(
        name = name + "_genrule",
        srcs = srcs,
        exec_tools = [
            "//benchmarks/util:proto3_data_stripper",
        ],
        outs = outs,
        cmd = "$(execpath //benchmarks/util:proto3_data_stripper) %s %s" % (
            " ".join(src_files), " ".join(out_files)),
    )

    native.filegroup(
        name = name,
        srcs = outs,
        **kwargs,
    )

def _proto3_from_proto2_library(
        name,
        srcs,
        **kwargs):
    """Create a proto3 library from a proto2 source.

    Args:
      name: the name of the target representing the generated proto files.
      srcs: the source proto2 files.  Note: these must be raw sources.
      **kwargs: standard arguments to forward on
    """
    outs = []
    src_files = []
    for src in srcs:
        outs.append(src + "3")
        src_files.append("$(rootpath %s)" % src);

    native.genrule(
        name = name,
        srcs = srcs,
        exec_tools = [
            "//:protoc",
            "//benchmarks/util:protoc-gen-proto2_to_proto3",
        ],
        outs = outs,
        cmd = """
            $(execpath //:protoc) \
                --plugin=$(execpath //benchmarks/util:protoc-gen-proto2_to_proto3) \
                --proto_path=. \
                --proto_path=$(GENDIR) \
                --proto2_to_proto3_out=$(GENDIR) \
                %s
        """ % (" ".join(src_files)),
        **kwargs,
    )

def php_proto3_from_proto2_library(
        name,
        src,
        outs = [],
        **kwargs):
    """Create a proto3 php library from a proto2 source.

    Args:
      name: the name of the target representing the generated proto files.
      src: the source proto2 file.
      outs: the expected php outputs.
      **kwargs: standard arguments to forward on
    """
    _proto3_from_proto2_library(
        name = name + "_genrule",
        srcs = [src],
    )

    internal_php_proto_library(
        name = name,
        srcs = [name + "_genrule"],
        outs = outs,
        **kwargs,
    )
