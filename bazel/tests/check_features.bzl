load("@proto_bazel_features//:features.bzl", "bazel_features")

def check_features():
    print("analysis_tests_can_transition_on_experimental_incompatible_flags: %s" % bazel_features.rules.analysis_tests_can_transition_on_experimental_incompatible_flags)
