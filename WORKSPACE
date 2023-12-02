workspace(name = "com_google_protobuf")

# An explicit self-reference to work around changes in Bazel 7.0
# See https://github.com/bazelbuild/bazel/issues/19973#issuecomment-1787814450
# buildifier: disable=duplicated-name
local_repository(name = "com_google_protobuf", path = ".")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

local_repository(
    name = "com_google_protobuf_examples",
    path = "examples",
)

# Load common dependencies first to ensure we use the correct version
load("//:protobuf_deps.bzl", "PROTOBUF_MAVEN_ARTIFACTS", "protobuf_deps")

protobuf_deps()

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()

# Bazel platform rules.
http_archive(
    name = "platforms",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/platforms/releases/download/0.0.7/platforms-0.0.7.tar.gz",
        "https://github.com/bazelbuild/platforms/releases/download/0.0.7/platforms-0.0.7.tar.gz",
    ],
    sha256 = "3a561c99e7bdbe9173aa653fd579fe849f1d8d67395780ab4770b1f381431d51",
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

http_archive(
    name = "rules_ruby",
    urls = [
      "https://github.com/protocolbuffers/rules_ruby/archive/b7f3e9756f3c45527be27bc38840d5a1ba690436.zip"
    ],
    strip_prefix = "rules_ruby-b7f3e9756f3c45527be27bc38840d5a1ba690436",
    sha256 = "347927fd8de6132099fcdc58e8f7eab7bde4eb2fd424546b9cd4f1c6f8f8bad8",
)

load("@rules_ruby//ruby:defs.bzl", "ruby_runtime")

ruby_runtime("system_ruby")

register_toolchains("@system_ruby//:toolchain")

# Uncomment pairs of ruby_runtime() + register_toolchain() calls below to enable
# local JRuby testing. Do not submit the changes (due to impact on test duration
# for non JRuby builds due to downloading JRuby SDKs).
#ruby_runtime("jruby-9.2")
#
#register_toolchains("@jruby-9.2//:toolchain")
#
#ruby_runtime("jruby-9.3")
#
#register_toolchains("@jruby-9.3//:toolchain")

load("@system_ruby//:bundle.bzl", "ruby_bundle")

ruby_bundle(
    name = "protobuf_bundle",
    srcs = ["//ruby:google-protobuf.gemspec"],
    gemfile = "//ruby:Gemfile",
)

http_archive(
    name = "lua",
    build_file = "//bazel:lua.BUILD",
    sha256 = "b9e2e4aad6789b3b63a056d442f7b39f0ecfca3ae0f1fc0ae4e9614401b69f4b",
    strip_prefix = "lua-5.2.4",
    urls = [
        "https://mirror.bazel.build/www.lua.org/ftp/lua-5.2.4.tar.gz",
        "https://www.lua.org/ftp/lua-5.2.4.tar.gz",
    ],
)

http_archive(
    name = "com_github_google_benchmark",
    urls = ["https://github.com/google/benchmark/archive/0baacde3618ca617da95375e0af13ce1baadea47.zip"],
    strip_prefix = "benchmark-0baacde3618ca617da95375e0af13ce1baadea47",
    sha256 = "62e2f2e6d8a744d67e4bbc212fcfd06647080de4253c97ad5c6749e09faf2cb0",
)

http_archive(
    name = "com_google_googleapis",
    urls = ["https://github.com/googleapis/googleapis/archive/30ed2662a85403cbdeb9ea38df1e414a2a276b83.zip"],
    strip_prefix = "googleapis-30ed2662a85403cbdeb9ea38df1e414a2a276b83",
    sha256 = "4dfc28101127d22abd6f0f6308d915d490c4594c0cfcf7643769c446d6763a46",
    build_file = "//benchmarks:BUILD.googleapis",
    patch_cmds = ["find google -type f -name BUILD.bazel -delete"],
)

load("//bazel:system_python.bzl", "system_python")

system_python(
    name = "system_python",
    minimum_python_version = "3.7",
)

load("@system_python//:pip.bzl", "pip_parse")

pip_parse(
    name = "pip_deps",
    requirements = "//python:requirements.txt",
)

load("@pip_deps//:requirements.bzl", "install_deps")

install_deps()

http_archive(
    name = "rules_fuzzing",
    sha256 = "ff52ef4845ab00e95d29c02a9e32e9eff4e0a4c9c8a6bcf8407a2f19eb3f9190",
    strip_prefix = "rules_fuzzing-0.4.1",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/releases/download/v0.4.1/rules_fuzzing-0.4.1.zip"],
    patches = ["//third_party:rules_fuzzing.patch"],
    patch_args = ["-p1"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

load("@fuzzing_py_deps//:requirements.bzl", fuzzing_py_deps_install_deps = "install_deps")

fuzzing_py_deps_install_deps()

bind(
    name = "python_headers",
    actual = "@system_python//:python_headers",
)

http_archive(
    name = "rules_rust",
    sha256 = "9ecd0f2144f0a24e6bc71ebcc50a1ee5128cedeceb32187004532c9710cb2334",
    urls = ["https://github.com/bazelbuild/rules_rust/releases/download/0.29.1/rules_rust-v0.29.1.tar.gz"],
)

load("@rules_rust//rust:repositories.bzl", "rules_rust_dependencies", "rust_register_toolchains")

rules_rust_dependencies()

rust_register_toolchains(edition = "2021")
load("@rules_rust//crate_universe:defs.bzl", "crate", "crates_repository")
# to repin, invoke `CARGO_BAZEL_REPIN=1 bazel sync --only=crate_index`
crates_repository(
    name = "crate_index",
    cargo_lockfile = "//:Cargo.lock",
    lockfile = "//:Cargo.bazel.lock",
    packages = {
        "googletest": crate.spec(
            version = ">0.0.0",
        ),
        "paste": crate.spec(
          version = ">=1",
        ),
    },
)

load("@crate_index//:defs.bzl", "crate_repositories")
crate_repositories()
