"""Load dependencies needed to compile the protobuf library as a 3rd-party consumer."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

PROTOBUF_MAVEN_ARTIFACTS = [
    "com.google.caliper:caliper:1.0-beta-3",
    "com.google.code.findbugs:jsr305:3.0.2",
    "com.google.code.gson:gson:2.8.9",
    "com.google.errorprone:error_prone_annotations:2.5.1",
    "com.google.j2objc:j2objc-annotations:1.3",
    "com.google.guava:guava:31.1-jre",
    "com.google.guava:guava-testlib:31.1-jre",
    "com.google.truth:truth:1.1.2",
    "junit:junit:4.13.2",
    "org.mockito:mockito-core:4.3.1",
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
                "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.2.1/bazel-skylib-1.2.1.tar.gz",
                "https://github.com/bazelbuild/bazel-skylib/releases/download/1.2.1/bazel-skylib-1.2.1.tar.gz",
            ],
            sha256 = "f7be3474d42aae265405a592bb7da8e171919d74c16f082a5457840f06054728",
        )

    if not native.existing_rule("com_google_absl"):
        # Abseil LTS from November 2021
        _github_archive(
            name = "com_google_absl",
            repo = "https://github.com/abseil/abseil-cpp",
            commit = "273292d1cfc0a94a65082ee350509af1d113344d",
            sha256 = "6764f226bd6e2d8ab9fe2f3cab5f45fb1a4a15c04b58b87ba7fa87456054f98b",
        )

    if not native.existing_rule("zlib"):
        http_archive(
            name = "zlib",
            build_file = "@com_google_protobuf//:third_party/zlib.BUILD",
            sha256 = "d8688496ea40fb61787500e863cc63c9afcbc524468cedeb478068924eb54932",
            strip_prefix = "zlib-1.2.12",
            urls = ["https://github.com/madler/zlib/archive/v1.2.12.tar.gz"],
        )

    if not native.existing_rule("jsoncpp"):
        http_archive(
            name = "jsoncpp",
            build_file = "@com_google_protobuf//:third_party/jsoncpp.BUILD",
            sha256 = "e34a628a8142643b976c7233ef381457efad79468c67cb1ae0b83a33d7493999",
            strip_prefix = "jsoncpp-1.9.4",
            urls = ["https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.4.tar.gz"],
        )

    if not native.existing_rule("utf8_range"):
        _github_archive(
            name = "utf8_range",
            repo = "https://github.com/protocolbuffers/utf8_range",
            commit = "77f787ec84cce7c4c7059a614e3147eac23d511e",
            sha256 = "e9ece5beebe0949cb4a4768b7fc9ca85b64fec41b3175b0c898bdd3f848c4ed1",
        )

    if not native.existing_rule("rules_cc"):
        _github_archive(
            name = "rules_cc",
            repo = "https://github.com/bazelbuild/rules_cc",
            commit = "818289e5613731ae410efb54218a4077fb9dbb03",
            sha256 = "0adbd6f567291ad526e82c765e15aed33cea5e256eeba129f1501142c2c56610",
        )

    if not native.existing_rule("rules_java"):
        _github_archive(
            name = "rules_java",
            repo = "https://github.com/bazelbuild/rules_java",
            commit = "981f06c3d2bd10225e85209904090eb7b5fb26bd",
            sha256 = "7979ece89e82546b0dcd1dff7538c34b5a6ebc9148971106f0e3705444f00665",
        )

    if not native.existing_rule("rules_proto"):
        _github_archive(
            name = "rules_proto",
            repo = "https://github.com/bazelbuild/rules_proto",
            commit = "f7a30f6f80006b591fa7c437fe5a951eb10bcbcf",
            sha256 = "a4382f78723af788f0bc19fd4c8411f44ffe0a72723670a34692ffad56ada3ac",
        )

    if not native.existing_rule("rules_python"):
        http_archive(
            name = "rules_python",
            sha256 = "9fcf91dbcc31fde6d1edb15f117246d912c33c36f44cf681976bd886538deba6",
            strip_prefix = "rules_python-0.8.0",
            url = "https://github.com/bazelbuild/rules_python/archive/refs/tags/0.8.0.tar.gz",
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

    if not native.existing_rule("io_bazel_rules_kotlin"):
        http_archive(
            name = "io_bazel_rules_kotlin",
            urls = ["https://github.com/bazelbuild/rules_kotlin/releases/download/v1.7.0-RC-1/rules_kotlin_release.tgz"],
            sha256 = "68b910730026921814d3a504ccbe9adaac9938983d940e626523e6e4ecfb0355",
        )

    if not native.existing_rule("upb"):
        _github_archive(
            name = "upb",
            repo = "https://github.com/protocolbuffers/upb",
            commit = "32c6e9baab03d584b85390fdba789118f20613fc",
            #sha256 = "4c82bff4f790dbb5a11ec40b1fac44e7c95d9a63fd215a13aaf44cb27b10ac27",
        )
