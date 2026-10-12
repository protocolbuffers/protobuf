"""Tests for cc_proto_library with rule sources."""

load("@rules_testing//lib:truth.bzl", "matching")
load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "get_cc_info_artifacts")

def _test_rule_src_cc_proto_library(env, target):
    artifacts = get_cc_info_artifacts(target)

    # Should depend on C++ outputs (headers and sources).
    env.expect.that_collection(artifacts.compilation_context_headers_in_package).contains_predicate(
        matching.file_path_matches(target.label.package + "/rule_src.pb.h"),
    )

    env.expect.that_collection(artifacts.direct_files_in_package).contains_predicate(
        matching.file_path_matches(target.label.package + "/rule_src.pb.cc"),
    )

    # Should provide C++ static libraries (PIC or non-PIC depending on toolchain/platform).
    env.expect.that_collection(artifacts.static_libs_in_package + artifacts.pic_static_libs_in_package).contains_predicate(
        matching.any(
            matching.file_path_matches(target.label.package + "/librule_src_proto.a"),
            matching.file_path_matches(target.label.package + "/librule_src_proto.pic.a"),
            matching.file_path_matches(target.label.package + "/rule_src_proto.lib"),
        ),
    )

    # Should provide object files.
    env.expect.that_collection(artifacts.linker_files_in_package).contains_predicate(
        matching.any(
            matching.file_path_matches(target.label.package + "/_objs/rule_src_proto/rule_src.pb.o"),
            matching.file_path_matches(target.label.package + "/_objs/rule_src_proto/rule_src.pb.pic.o"),
            matching.file_path_matches(target.label.package + "/_objs/rule_src_proto/rule_src.pb.obj"),
        ),
    )

def _test_rule_src_cc_binary_linker_inputs(env, target):
    inputs = env.expect.that_target(target).action_named("CppLink").inputs()

    # Should depend on C++ static libraries or objects.
    inputs.contains_predicate(
        matching.any(
            matching.file_path_matches(target.label.package + "/librule_src_proto.a"),
            matching.file_path_matches(target.label.package + "/librule_src_proto.pic.a"),
            matching.file_path_matches(target.label.package + "/rule_src_proto.lib"),
            matching.file_path_matches(target.label.package + "/_objs/rule_src_proto/rule_src.pb"),
        ),
    )

TESTS = {
    ":rule_src_cc_proto": [_test_rule_src_cc_proto_library],
    ":cctest_rule_src": [_test_rule_src_cc_binary_linker_inputs],
}
