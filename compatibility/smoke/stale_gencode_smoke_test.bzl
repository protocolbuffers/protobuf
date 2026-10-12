load("@rules_java//java:defs.bzl", "java_test")

def stale_gencode_smoke_test(version, also_lite = False):
    """Defines smoke tests for checked in gencode from a previous protoc version.

    Args:
      version: The protoc version, matching a v<version> subdirectory.
      also_lite: If true, also defines a test against the lite gencode in the
        v<version>/lite subdirectory, linked against only the lite runtime.
    """
    java_test(
        name = "stale_gencode_smoke_test_v%s" % version,
        srcs = [
            "StaleGencodeFullSmokeTest.java",
            "StaleGencodeSmokeTest.java",
        ],
        test_class = "smoke.StaleGencodeFullSmokeTest",
        deps = [
            "//java/core",
            "//:protobuf_java",
            "//:protobuf_java_util",
            "//compatibility/smoke/v%s:checked_in_gencode" % version,
            "@protobuf_maven_dev//:com_google_truth_truth",
            "@protobuf_maven_dev//:junit_junit",
        ],
    )

    if also_lite:
        java_test(
            name = "stale_gencode_smoke_test_v%s_lite" % version,
            srcs = ["StaleGencodeSmokeTest.java"],
            test_class = "smoke.StaleGencodeSmokeTest",
            deps = [
                "//:protobuf_javalite",
                "//compatibility/smoke/v%s/lite:checked_in_gencode" % version,
                "@protobuf_maven_dev//:com_google_truth_truth",
                "@protobuf_maven_dev//:junit_junit",
            ],
        )
