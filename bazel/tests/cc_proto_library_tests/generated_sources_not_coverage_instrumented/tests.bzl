"""Tests that generated sources are not coverage instrumented."""

load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "get_cc_info_artifacts")

def _test_generated_sources_not_coverage_instrumented(env, target):
    artifacts = get_cc_info_artifacts(target)
    action = env.expect.that_target(target).action_generating(
        artifacts.linker_files_in_package[0].short_path,
    )
    action.argv().not_contains("-fprofile-arcs")
    action.argv().not_contains("-ftest-coverage")

TESTS = {
    ":foo_proto": [_test_generated_sources_not_coverage_instrumented],
}
