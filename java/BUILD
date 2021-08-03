test_suite(
    name = "tests",
    tests = [
        "//java/core:tests",
        "//java/lite:tests",
        "//java/util:tests",
    ],
)

filegroup(
    name = "release",
    srcs = [
        "//java/core:release", # contains lite.
        "//java/util:release",
    ]
)