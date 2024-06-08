"""Provides a rule to generate a conformance test for a given version of the gencode."""

load("@bazel_skylib//rules:build_test.bzl", "build_test")

def java_runtime_conformance(name, gencode_version, tags = []):
    """Generates a conformance test for the given version of the runtime.

    For example, runtime_conformance("3.19.4") will generate a build test named "conformance_v3.19.4"
    that will fail if the runtime at that version fails to compile the unittest proto.

    Args:
        name: The name of the test.
        gencode_version: The version of the runtime to test.
        tags: Any tags to apply to the generated test.
    """

    if gencode_version == "main":
        protoc = "//:protoc"
    else:
        minor = gencode_version[gencode_version.find(".") + 1:]
        protoc = Label("@com_google_protobuf_v%s//:protoc" % minor)

    gencode = [
        "v%s/protobuf_unittest/UnittestProto.java" % gencode_version,
        "v%s/com/google/protobuf/test/UnittestImport.java" % gencode_version,
        "v%s/com/google/protobuf/test/UnittestImportPublic.java" % gencode_version,
    ]
    native.genrule(
        name = "unittest_proto_gencode_v" + gencode_version,
        srcs = [
            "//src/google/protobuf:unittest_proto_srcs",
        ],
        outs = gencode,
        cmd = "$(location %s) " % protoc +
              "$(locations //src/google/protobuf:unittest_proto_srcs) " +
              " -Isrc/ --java_out=$(@D)/v%s" % gencode_version,
        tools = [protoc],
    )

    conformance_name = "conformance_v" + gencode_version
    conformance_lib_name = conformance_name + "_lib"
    native.java_library(
        name = conformance_lib_name,
        srcs = gencode,
        deps = ["//java/core"],
        tags = tags,
    )

    build_test(
        name = name,
        targets = [":" + conformance_lib_name],
        tags = tags,
    )
