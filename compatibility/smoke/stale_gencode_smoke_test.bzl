load("@rules_java//java:defs.bzl", "java_test")

def stale_gencode_smoke_test(version):
    java_test(
        name = "stale_gencode_smoke_test_v%s" % version,
        srcs = ["StaleGencodeSmokeTest.java"],
        test_class = "smoke.StaleGencodeSmokeTest",
        deps = [
            "//java/core",
            "//:protobuf_java",
            "//:protobuf_java_util",
            "//compatibility/smoke/v%s:checked_in_gencode" % version,
            "@protobuf_maven_dev//:com_google_truth_truth",
            "@protobuf_maven_dev//:junit_junit",
        ],
    )
