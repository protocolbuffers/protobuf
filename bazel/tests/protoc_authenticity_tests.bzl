"""Tests for protoc authenticity validation actions."""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching")

def protoc_authenticity_test_suite(name):
    test_suite(
        name = name,
        tests = [
            _test_authenticity_action_uses_generated_script,
        ],
    )

def _test_authenticity_action_uses_generated_script(name):
    analysis_test(
        name = name,
        target = "//bazel/private/oss/toolchains/prebuilt:authenticity_validation",
        impl = _test_authenticity_action_uses_generated_script_impl,
    )

def _test_authenticity_action_uses_generated_script_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "bazel/private/oss/toolchains/prebuilt/validation_output.txt",
    )
    action.mnemonic().equals("ProtocAuthenticityCheck")
    action.argv().contains_predicate(
        matching.str_matches("*authenticity_validation_authenticity_check.sh"),
    )

    script_action = env.expect.that_target(target).action_generating(
        "bazel/private/oss/toolchains/prebuilt/authenticity_validation_authenticity_check.sh",
    )
    script_action.content().contains("grep -q")
