"""Tests for basic cc_proto_library functionality."""

load("@rules_testing//lib:truth.bzl", "matching")
load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "get_cc_info_artifacts")

def _test_basic(env, target):
    artifacts = get_cc_info_artifacts(target)

    env.expect.that_collection(artifacts.direct_files_in_package).transform(
        filter = matching.any(
            matching.file_path_matches("*.pb.h"),
            matching.file_path_matches("*.pb.cc"),
        ),
    ).contains_exactly_predicates([
        matching.file_path_matches("/foo.pb.h"),
        matching.file_path_matches("/foo.pb.cc"),
    ])

    # Static library naming varies by toolchain/platform (non-PIC .a, PIC .pic.a on Google3 k8, .lib on Windows).
    env.expect.that_collection(
        artifacts.static_libs_in_package + artifacts.pic_static_libs_in_package,
    ).contains_predicate(
        matching.any(
            matching.file_path_matches("/libfoo_proto.a"),
            matching.file_path_matches("/libfoo_proto.pic.a"),
            matching.file_path_matches("/foo_proto.lib"),
        ),
    )

    # Shared/interface library naming varies across Google3 (which mangles '_' to '_U' in dynamic
    # library names) and OSS CI platforms (Linux .so/.ifso, macOS .dylib, Windows .dll).
    if any([f.basename.endswith(".ifso") for f in artifacts.shared_libs_in_package]):
        env.expect.that_collection(artifacts.shared_libs_in_package).contains_at_least_predicates([
            matching.any(
                matching.file_path_matches("*foo_proto*.ifso"),
                matching.file_path_matches("*foo_Uproto*.ifso"),
            ),
            matching.any(
                matching.file_path_matches("*foo_proto*.so"),
                matching.file_path_matches("*foo_Uproto*.so"),
            ),
        ])
    elif artifacts.shared_libs_in_package:
        env.expect.that_collection(artifacts.shared_libs_in_package).contains_predicate(
            matching.any(
                matching.file_path_matches("*foo_proto*.so"),
                matching.file_path_matches("*foo_Uproto*.so"),
                matching.file_path_matches("*foo_proto*.dylib"),
                matching.file_path_matches("*foo_proto*.dll"),
            ),
        )

TESTS = {
    ":foo_cc_proto": [_test_basic],
}
