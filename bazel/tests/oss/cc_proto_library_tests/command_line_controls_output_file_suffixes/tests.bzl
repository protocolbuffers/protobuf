"""Tests that command-line flags control cc_proto_library output file suffixes."""

load("@rules_testing//lib:truth.bzl", "matching")
load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "get_cc_info_artifacts")

def _test_command_line_controls_output_file_suffixes(env, target):
    artifacts = get_cc_info_artifacts(target)

    env.expect.that_collection(artifacts.direct_files_in_package).transform(
        filter = matching.any(
            matching.file_path_matches("*.pb.h"),
            matching.file_path_matches("*.pb.cc"),
            matching.file_path_matches("*.pb.cc.meta"),
            matching.file_path_matches("*.proto.h"),
        ),
    ).contains_exactly_predicates([
        matching.file_path_matches("/foo.pb.cc"),
        matching.file_path_matches("/foo.pb.h"),
        matching.file_path_matches("/foo.pb.cc.meta"),
        matching.file_path_matches("/foo.proto.h"),
    ])

    # Library extensions vary by platform in OSS CI (Linux, macOS, Windows).
    env.expect.that_collection(
        artifacts.static_libs_in_package + artifacts.pic_static_libs_in_package,
    ).contains_predicate(
        matching.any(
            matching.file_path_matches("/libfoo_proto.a"),
            matching.file_path_matches("/libfoo_proto.pic.a"),
            matching.file_path_matches("/foo_proto.lib"),
        ),
    )

    # Shared libs may be _solib symlinks whose names escape '_' as '_U'
    # (e.g. ..._Slibfoo_Uproto.so), so match "foo*proto" rather than "foo_proto".
    if any([f.basename.endswith(".ifso") for f in artifacts.shared_libs_in_package]):
        env.expect.that_collection(artifacts.shared_libs_in_package).contains_at_least_predicates([
            matching.file_path_matches("*foo*proto*.ifso"),
            matching.file_path_matches("*foo*proto*.so"),
        ])
    elif artifacts.shared_libs_in_package:
        env.expect.that_collection(artifacts.shared_libs_in_package).contains_predicate(
            matching.any(
                matching.file_path_matches("*foo*proto*.so"),
                matching.file_path_matches("*foo*proto*.dylib"),
                matching.file_path_matches("*foo*proto*.dll"),
            ),
        )

TESTS = {
    ":foo_cc_proto": [_test_command_line_controls_output_file_suffixes],
}
