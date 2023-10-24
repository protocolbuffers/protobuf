"""Load dependencies needed to compile the protobuf library as a 3rd-party consumer."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//bazel:python_downloads.bzl", "python_nuget_package", "python_source_archive")

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
            commit = "fb3621f4f897824c0dbe0615fa94543df6192f30",  # Abseil LTS 20230802.1
            sha256 = "aa768256d0567f626334fcbe722f564c40b281518fc8423e2708a308e5f983ea",
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
        http_archive(
            name = "rules_proto",
            sha256 = "dc3fb206a2cb3441b485eb1e423165b231235a1ea9b031b4433cf7bc1fa460dd",
            strip_prefix = "rules_proto-5.3.0-21.7",
            urls = [
                "https://github.com/bazelbuild/rules_proto/archive/refs/tags/5.3.0-21.7.tar.gz",
            ],
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

    # Python Downloads
    python_source_archive(
        name = "python-3.8.0",
        sha256 = "f1069ad3cae8e7ec467aa98a6565a62a48ef196cb8f1455a245a08db5e1792df",
    )
    python_nuget_package(
        name = "nuget_python_i686_3.8.0",
        sha256 = "87a6481f5eef30b42ac12c93f06f73bd0b8692f26313b76a6615d1641c4e7bca",
    )
    python_nuget_package(
        name = "nuget_python_x86-64_3.8.0",
        sha256 = "96c61321ce90dd053c8a04f305a5f6cc6d91350b862db34440e4a4f069b708a0",
    )
    python_nuget_package(
        name = "nuget_python_i686_3.9.0",
        sha256 = "229abecbe49dc08fe5709e0b31e70edfb3b88f23335ebfc2904c44f940fd59b6",
    )
    python_nuget_package(
        name = "nuget_python_x86-64_3.9.0",
        sha256 = "6af58a733e7dfbfcdd50d55788134393d6ffe7ab8270effbf724bdb786558832",
    )
    python_nuget_package(
        name = "nuget_python_i686_3.10.0",
        sha256 = "e115e102eb90ce160ab0ef7506b750a8d7ecc385bde0a496f02a54337a8bc333",
    )
    python_nuget_package(
        name = "nuget_python_x86-64_3.10.0",
        sha256 = "4474c83c25625d93e772e926f95f4cd398a0abbb52793625fa30f39af3d2cc00",
    )
