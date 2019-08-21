load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# We're pinning to a commit because this project does not have a recent release.
# Nothing special about this commit, though.
http_archive(
    # TODO(yannic): This should be named `com_google_googletest`.
    # https://github.com/google/googletest/blob/c9ccac7cb7345901884aabf5d1a786cfa6e2f397/WORKSPACE#L1
    name = "googletest",
    sha256 = "eb46ff71810090f76d5ff69c27b64ddc5d3ad93d6665df7dbe9712401d13887e",
    strip_prefix = "googletest-d5e9e0c38f85363e90b0a3e95a9484fe896d38e5",
    urls = [
        "https://mirror.bazel.build/github.com/google/googletest/archive/d5e9e0c38f85363e90b0a3e95a9484fe896d38e5.tar.gz",
        "https://github.com/google/googletest/archive/d5e9e0c38f85363e90b0a3e95a9484fe896d38e5.tar.gz",
    ],
)

http_archive(
    name = "six_archive",
    build_file = "@//:six.BUILD",
    sha256 = "105f8d68616f8248e24bf0e9372ef04d3cc10104f1980f54d57b2ce73a5ad56a",
    urls = [
        "https://pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz#md5=34eed507548117b2ab523ab14b2f8b55",
    ],
)

http_archive(
    name = "rules_proto",
    sha256 = "57001a3b33ec690a175cdf0698243431ef27233017b9bed23f96d44b9c98242f",
    strip_prefix = "rules_proto-9cd4f8f1ede19d81c6d48910429fe96776e567b1",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/9cd4f8f1ede19d81c6d48910429fe96776e567b1.tar.gz",
        "https://github.com/bazelbuild/rules_proto/archive/9cd4f8f1ede19d81c6d48910429fe96776e567b1.tar.gz",
    ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()

bind(
    name = "python_headers",
    actual = "//util/python:python_headers",
)

bind(
    name = "gtest",
    actual = "@googletest//:gtest",
)

bind(
    name = "gtest_main",
    actual = "@googletest//:gtest_main",
)

bind(
    name = "six",
    actual = "@six_archive//:six",
)

maven_jar(
    name = "guava_maven",
    artifact = "com.google.guava:guava:18.0",
)

bind(
    name = "guava",
    actual = "@guava_maven//jar",
)

maven_jar(
    name = "gson_maven",
    artifact = "com.google.code.gson:gson:2.3",
)

bind(
    name = "gson",
    actual = "@gson_maven//jar",
)
