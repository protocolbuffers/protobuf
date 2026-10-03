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

    # Proto2 extendable messages generated before 3.25.8 (release version 25.8) are not source
    # compatible with the 4.x runtime due to a generic typing conflict. We skip versions with
    # release major version < 25 (which covers 3.x and 21.x).
    if int(version.split(".")[0]) >= 25:
        java_test(
            name = "stale_gencode_proto2_smoke_test_v%s" % version,
            srcs = ["Proto2StaleGencodeSmokeTest.java"],
            test_class = "smoke.Proto2StaleGencodeSmokeTest",
            deps = [
                "//java/core",
                "//:protobuf_java",
                "//:protobuf_java_util",
                "//compatibility/smoke/v%s:checked_in_gencode" % version,
                "@protobuf_maven_dev//:com_google_truth_truth",
                "@protobuf_maven_dev//:junit_junit",
            ],
        )


