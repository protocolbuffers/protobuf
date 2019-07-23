"""Load dependencies needed to compile the protobuf library as a 3rd-party consumer."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def protobuf_deps():
    """Loads common dependencies needed to compile the protobuf library."""

    if not native.existing_rule("zlib"):
        http_archive(
            name = "zlib",
            build_file = "@com_google_protobuf//:third_party/zlib.BUILD",
            sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
            strip_prefix = "zlib-1.2.11",
            urls = ["https://zlib.net/zlib-1.2.11.tar.gz"],
        )

    if not native.existing_rule("six"):
        http_archive(
            name = "six",
            build_file = "@//:six.BUILD",
            sha256 = "105f8d68616f8248e24bf0e9372ef04d3cc10104f1980f54d57b2ce73a5ad56a",
            urls = ["https://pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz#md5=34eed507548117b2ab523ab14b2f8b55"],
        )

    if not native.existing_rule("rules_cc"):
        http_archive(
            name = "rules_cc",
            sha256 = "29daf0159f0cf552fcff60b49d8bcd4f08f08506d2da6e41b07058ec50cfeaec",
            strip_prefix = "rules_cc-b7fe9697c0c76ab2fd431a891dbb9a6a32ed7c3e",
            urls = ["https://github.com/bazelbuild/rules_cc/archive/b7fe9697c0c76ab2fd431a891dbb9a6a32ed7c3e.tar.gz"],
        )

    if not native.existing_rule("rules_java"):
        http_archive(
            name = "rules_java",
            sha256 = "f5a3e477e579231fca27bf202bb0e8fbe4fc6339d63b38ccb87c2760b533d1c3",
            strip_prefix = "rules_java-981f06c3d2bd10225e85209904090eb7b5fb26bd",
            urls = ["https://github.com/bazelbuild/rules_java/archive/981f06c3d2bd10225e85209904090eb7b5fb26bd.tar.gz"],
        )

    if not native.existing_rule("rules_proto"):
        http_archive(
            name = "rules_proto",
            sha256 = "88b0a90433866b44bb4450d4c30bc5738b8c4f9c9ba14e9661deb123f56a833d",
            strip_prefix = "rules_proto-b0cc14be5da05168b01db282fe93bdf17aa2b9f4",
            urls = ["https://github.com/bazelbuild/rules_proto/archive/b0cc14be5da05168b01db282fe93bdf17aa2b9f4.tar.gz"],
        )
