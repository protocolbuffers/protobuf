"""Load dependencies needed to compile the protobuf library as a 3rd-party consumer."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

PROTOBUF_MAVEN_ARTIFACTS = [
    "com.google.caliper:caliper:1.0-beta-3",
    "com.google.code.findbugs:jsr305:3.0.2",
    "com.google.code.gson:gson:2.8.9",
    "com.google.errorprone:error_prone_annotations:2.5.1",
    "com.google.j2objc:j2objc-annotations:2.8",
    "com.google.guava:guava:32.0.1-jre",
    "com.google.guava:guava-testlib:32.0.1-jre",
    "com.google.truth:truth:1.1.2",
    "junit:junit:4.13.2",
    "org.mockito:mockito-core:4.3.1",
    "biz.aQute.bnd:biz.aQute.bndlib:6.4.0",
    "info.picocli:picocli:4.6.3",
]

def _github_archive(repo, commit, **kwargs):
    repo_name = repo.split("/")[-1]
    http_archive(
        urls = [repo + "/archive/" + commit + ".zip"],
        strip_prefix = repo_name + "-" + commit,
        **kwargs
    )

def protobuf_deps():
    """Loads common dependencies needed to compile the protobuf library."""

    if not native.existing_rule("bazel_skylib"):
        http_archive(
            name = "bazel_skylib",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
                "https://github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
            ],
            sha256 = "74d544d96f4a5bb630d465ca8bbcfe231e3594e5aae57e1edbf17a6eb3ca2506",
        )

    if not native.existing_rule("com_google_absl"):
        _github_archive(
            name = "com_google_absl",
            repo = "https://github.com/abseil/abseil-cpp",
            commit = "29bf8085f3bf17b84d30e34b3d7ff8248fda404e",  # Abseil LTS 20230802
            sha256 = "f4871f2982e29496f4ddd598ccd5a87fea42f23c49b5e5eb459d57eab91df9d9",
        )

    if not native.existing_rule("zlib"):
        http_archive(
            name = "zlib",
            build_file = Label("//:third_party/zlib.BUILD"),
            sha256 = "d14c38e313afc35a9a8760dadf26042f51ea0f5d154b0630a31da0540107fb98",
            strip_prefix = "zlib-1.2.13",
            urls = [
                "https://github.com/madler/zlib/releases/download/v1.2.13/zlib-1.2.13.tar.xz",
                "https://zlib.net/zlib-1.2.13.tar.xz",
            ],
        )

    if not native.existing_rule("jsoncpp"):
        _github_archive(
            name = "jsoncpp",
            repo = "https://github.com/open-source-parsers/jsoncpp",
            commit = "9059f5cad030ba11d37818847443a53918c327b1",  # 1.9.4
            sha256 = "c0c583c7b53a53bcd1f7385f15439dcdf0314d550362379e2db9919a918d1996",
            build_file = Label("//:third_party/jsoncpp.BUILD"),
        )

    if not native.existing_rule("utf8_range"):
        _github_archive(
            name = "utf8_range",
            repo = "https://github.com/protocolbuffers/utf8_range",
            commit = "d863bc33e15cba6d873c878dcca9e6fe52b2f8cb",
            sha256 = "568988b5f7261ca181468dba38849fabf59dd9200fb2ed4b2823da187ef84d8c",
        )

    if not native.existing_rule("rules_cc"):
        _github_archive(
            name = "rules_cc",
            repo = "https://github.com/bazelbuild/rules_cc",
            commit = "818289e5613731ae410efb54218a4077fb9dbb03",
            sha256 = "0adbd6f567291ad526e82c765e15aed33cea5e256eeba129f1501142c2c56610",
        )

    if not native.existing_rule("rules_java"):
        http_archive(
            name = "rules_java",
            url = "https://github.com/bazelbuild/rules_java/releases/download/6.0.0/rules_java-6.0.0.tar.gz",
            sha256 = "469b7f3b580b4fcf8112f4d6d0d5a4ce8e1ad5e21fee67d8e8335d5f8b3debab",
        )

    if not native.existing_rule("rules_proto"):
        _github_archive(
            name = "rules_proto",
            repo = "https://github.com/bazelbuild/rules_proto",
            commit = "f7a30f6f80006b591fa7c437fe5a951eb10bcbcf",
            sha256 = "a4382f78723af788f0bc19fd4c8411f44ffe0a72723670a34692ffad56ada3ac",
        )

    if not native.existing_rule("rules_python"):
        _github_archive(
            name = "rules_python",
            repo = "https://github.com/bazelbuild/rules_python",
            commit = "02b521fce3c7b36b05813aa986d72777cc3ee328",  # 0.24.0
            sha256 = "f9e4f6acf82449324d56669bda4bdb28b48688ad2990d8b39fa5b93ed39c9ad1",
        )

    if not native.existing_rule("rules_ruby"):
        _github_archive(
            name = "rules_ruby",
            repo = "https://github.com/protocolbuffers/rules_ruby",
            commit = "b7f3e9756f3c45527be27bc38840d5a1ba690436",
            sha256 = "347927fd8de6132099fcdc58e8f7eab7bde4eb2fd424546b9cd4f1c6f8f8bad8",
        )

    if not native.existing_rule("rules_jvm_external"):
        _github_archive(
            name = "rules_jvm_external",
            repo = "https://github.com/bazelbuild/rules_jvm_external",
            commit = "906875b0d5eaaf61a8ca2c9c3835bde6f435d011",
            sha256 = "744bd7436f63af7e9872948773b8b106016dc164acb3960b4963f86754532ee7",
        )

    if not native.existing_rule("rules_pkg"):
        http_archive(
            name = "rules_pkg",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.7.0/rules_pkg-0.7.0.tar.gz",
                "https://github.com/bazelbuild/rules_pkg/releases/download/0.7.0/rules_pkg-0.7.0.tar.gz",
            ],
            sha256 = "8a298e832762eda1830597d64fe7db58178aa84cd5926d76d5b744d6558941c2",
        )

    if not native.existing_rule("build_bazel_rules_apple"):
        http_archive(
            name = "build_bazel_rules_apple",
            sha256 = "f94e6dddf74739ef5cb30f000e13a2a613f6ebfa5e63588305a71fce8a8a9911",
            url = "https://github.com/bazelbuild/rules_apple/releases/download/1.1.3/rules_apple.1.1.3.tar.gz",
        )

    if not native.existing_rule("io_bazel_rules_kotlin"):
        http_archive(
            name = "io_bazel_rules_kotlin",
            urls = ["https://github.com/bazelbuild/rules_kotlin/releases/download/v1.8.1/rules_kotlin_release.tgz"],
            sha256 = "a630cda9fdb4f56cf2dc20a4bf873765c41cf00e9379e8d59cd07b24730f4fde",
        )

    if not native.existing_rule("upb"):
        http_archive(
            name = "upb",
            url = "https://github.com/protocolbuffers/protobuf/archive/7242c3619c6db9843614b2c865681bf397261be8.zip",
            strip_prefix = "protobuf-7242c3619c6db9843614b2c865681bf397261be8/upb",
            sha256 = "0fc581f5e5caaf30c7119a73f2cff5d45424e4a4f23a52ebba73e3df031ad1c6",
        )
