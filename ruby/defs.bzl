"""Definition of ruby rules for Bazel

These are in the //ruby subpackage to avoid leaking a dependency on the
protocolbuffers/rules_ruby fork to Bazel users who don't use ruby.
"""
load("@rules_ruby//ruby:defs.bzl", "ruby_library")
load("//:protobuf.bzl", "internal_only_proto_gen")

def internal_ruby_proto_library(
        name,
        srcs = [],
        deps = [],
        includes = ["."],
        default_runtime = "@com_google_protobuf//ruby:protobuf",
        protoc = "@com_google_protobuf//:protoc",
        testonly = None,
        visibility = ["//visibility:public"],
        **kwargs):
    """Bazel rule to create a Ruby protobuf library from proto source files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      name: the name of the ruby_proto_library.
      srcs: the .proto files to compile.
      deps: a list of dependency labels; must be a internal_ruby_proto_library.
      includes: a string indicating the include path of the .proto files.
      default_runtime: the RubyProtobuf runtime
      protoc: the label of the protocol compiler to generate the sources.
      testonly: common rule attribute (see:
          https://bazel.build/reference/be/common-definitions#common-attributes)
      visibility: the visibility of the generated files.
      **kwargs: other keyword arguments that are passed to ruby_library.

    """

    # Note: we need to run the protoc build twice to get separate targets for
    # the generated header and the source files.
    internal_only_proto_gen(
        name = name + "_genproto",
        srcs = srcs,
        deps = [s + "_genproto" for s in deps],
        langs = ["ruby"],
        includes = includes,
        protoc = protoc,
        testonly = testonly,
        visibility = visibility,
        tags = ["manual"],
    )

    deps = []
    if default_runtime:
        deps.append(default_runtime)
    ruby_library(
        name = name,
        srcs = [name + "_genproto"],
        deps = deps,
        testonly = testonly,
        visibility = visibility,
        includes = includes,
        **kwargs
    )
