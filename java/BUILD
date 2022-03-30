test_suite(
    name = "tests",
    tests = [
        "//java/core:tests",
        "//java/kotlin:tests",
        "//java/kotlin-lite:tests",
        "//java/lite:tests",
        "//java/util:tests",
    ],
)

filegroup(
    name = "release",
    srcs = [
        "//java/core:release", # contains lite.
        "//java/kotlin:release",
        "//java/kotlin-lite:release",
        "//java/util:release",
    ]
)
