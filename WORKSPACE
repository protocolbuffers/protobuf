workspace(name = "com_google_protobuf")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

local_repository(
    name = "com_google_protobuf_examples",
    path = "examples",
)

# Load common dependencies first to ensure we use the correct version
load("//:protobuf_deps.bzl", "PROTOBUF_MAVEN_ARTIFACTS", "protobuf_deps")
protobuf_deps()

# Bazel platform rules.
http_archive(
    name = "platforms",
    sha256 = "a879ea428c6d56ab0ec18224f976515948822451473a80d06c2e50af0bbe5121",
    strip_prefix = "platforms-da5541f26b7de1dc8e04c075c99df5351742a4a2",
    urls = ["https://github.com/bazelbuild/platforms/archive/da5541f26b7de1dc8e04c075c99df5351742a4a2.zip"],  # 2022-05-27
)

http_archive(
    name = "com_google_googletest",
    sha256 = "730215d76eace9dd49bf74ce044e8daa065d175f1ac891cc1d6bb184ef94e565",
    strip_prefix = "googletest-f53219cdcb7b084ef57414efea92ee5b71989558",
    urls = [
        "https://github.com/google/googletest/archive/f53219cdcb7b084ef57414efea92ee5b71989558.tar.gz" # 2023-03-16
    ],
)

load("@com_google_googletest//:googletest_deps.bzl", "googletest_deps")

googletest_deps()

load("@rules_jvm_external//:repositories.bzl", "rules_jvm_external_deps")

rules_jvm_external_deps()

load("@rules_jvm_external//:setup.bzl", "rules_jvm_external_setup")

rules_jvm_external_setup()

load("@rules_jvm_external//:defs.bzl", "maven_install")

maven_install(
    artifacts = PROTOBUF_MAVEN_ARTIFACTS,
    # For updating instructions, see:
    # https://github.com/bazelbuild/rules_jvm_external#updating-maven_installjson
    maven_install_json = "//:maven_install.json",
    repositories = [
        "https://repo1.maven.org/maven2",
        "https://repo.maven.apache.org/maven2",
    ],
)

load("@maven//:defs.bzl", "pinned_maven_install")

pinned_maven_install()

# For `cc_proto_blacklist_test` and `build_test`.
load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")
rules_pkg_dependencies()

load("@build_bazel_rules_apple//apple:repositories.bzl", "apple_rules_dependencies")
apple_rules_dependencies()

# For `kt_jvm_library`
load("@io_bazel_rules_kotlin//kotlin:repositories.bzl", "kotlin_repositories")
kotlin_repositories()

load("@io_bazel_rules_kotlin//kotlin:core.bzl", "kt_register_toolchains")
kt_register_toolchains()

load("@rules_ruby//ruby:defs.bzl", "ruby_runtime")
ruby_runtime("system_ruby")
register_toolchains("@system_ruby//:toolchain")

load("@system_ruby//:bundle.bzl", "ruby_bundle")
ruby_bundle(
    name = "protobuf_bundle",
    srcs = ["//ruby:google-protobuf.gemspec"],
    gemfile = "//ruby:Gemfile",
)

load("@upb//bazel:workspace_deps.bzl", "upb_deps")
upb_deps()

load("@upb//bazel:system_python.bzl", "system_python")
system_python(
    name = "system_python",
    minimum_python_version = "3.7",
)

load("@system_python//:pip.bzl", "pip_parse")
pip_parse(
    name="pip_deps",
    requirements = "@upb//python:requirements.txt",
    requirements_overrides = {
        "3.11": "@upb//python:requirements_311.txt",
    },
)

load("@pip_deps//:requirements.bzl", "install_deps")
install_deps()

load("@utf8_range//:workspace_deps.bzl", "utf8_range_deps")
utf8_range_deps()

http_archive(
    name = "rules_fuzzing",
    sha256 = "d9002dd3cd6437017f08593124fdd1b13b3473c7b929ceb0e60d317cb9346118",
    strip_prefix = "rules_fuzzing-0.3.2",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.3.2.zip"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")
rules_fuzzing_dependencies()

bind(
    name = "python_headers",
    actual = "@system_python//:python_headers",
)

http_archive(
    name = "rules_rust",
    sha256 = "d125fb75432dc3b20e9b5a19347b45ec607fabe75f98c6c4ba9badaab9c193ce",
    urls = ["https://github.com/bazelbuild/rules_rust/releases/download/0.17.0/rules_rust-v0.17.0.tar.gz"],
)

load("@rules_rust//rust:repositories.bzl", "rules_rust_dependencies", "rust_register_toolchains")
rules_rust_dependencies()
rust_register_toolchains(edition = "2021")
