"""Contains build macros for the Protobuf Perl package."""

load("@rules_cc//cc:cc_test.bzl", "cc_test")

def perl_proto_c_test(name, srcs, deps = [], copts = [], **kwargs):
    """Generates a cc_test target for a Perl Protobuf C-level integration test.

    Args:
        name: The name of the test target.
        srcs: The source files for the test.
        deps: Extra dependencies to append to the default test dependencies.
        copts: Extra compiler options to append to the default test copts.
        **kwargs: Extra arguments to forward to the underlying cc_test rule.
    """
    cc_test(
        name = name,
        srcs = srcs,
        features = [
            "-layering_check",
            "-use_header_modules",
        ],
        copts = [
            "-I.",
            "-Ithird_party/upb",
            "-DGOOGLE3",
            "-Dgoogle_protobuf_FileDescriptorSet=proto2_FileDescriptorSet",
            "-Dgoogle_protobuf_FileDescriptorSet_parse=proto2_FileDescriptorSet_parse",
            "-Dgoogle_protobuf_FileDescriptorProto=proto2_FileDescriptorProto",
            "-Dgoogle_protobuf_FileDescriptorProto_parse=proto2_FileDescriptorProto_parse",
            "-Dgoogle_protobuf_FileDescriptorSet_file=proto2_FileDescriptorSet_file",
            "-Dgoogle_protobuf_FileDescriptorProto_name=proto2_FileDescriptorProto_name",
        ] + copts,
        data = [
            ":test_descriptor_bin",
        ],
        deps = [
            ":libprotobufperl",
            ":upb_perl_test_lib",
            "//third_party/pcre2",
            "//upb/reflection",
        ] + deps,
        **kwargs
    )

def define_perl_proto_c_tests(tests, name = "perl_proto_c_tests"):
    """Instantiates a list of Perl Protobuf C-level integration tests.

    Args:
        tests: A list of dicts, where each dict contains:
            - target_name: The name of the test target.
            - srcs: List of source files.
            - deps: List of extra dependencies.
    """
    for test in tests:
        perl_proto_c_test(
            name = test["target_name"],
            srcs = test["srcs"],
            deps = test["deps"],
        )
