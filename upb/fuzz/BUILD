load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")

licenses(["notice"])

cc_fuzz_test(
    name = "file_descriptor_parsenew_fuzzer",
    srcs = ["file_descriptor_parsenew_fuzzer.cc"],
    deps = [
        "//:descriptor_upb_proto",
        "//:reflection",
        "//:upb",
    ],
)
