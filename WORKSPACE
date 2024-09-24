workspace(name = "com_google_protobuf")

# An explicit self-reference to work around changes in Bazel 7.0
# See https://github.com/bazelbuild/bazel/issues/19973#issuecomment-1787814450
# buildifier: disable=duplicated-name
local_repository(name = "com_google_protobuf", path = ".")

# Second self-reference that makes it possible to load proto rules from @protobuf.
# buildifier: disable=duplicated-name
local_repository(name = "protobuf", path = ".")

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

load("@rules_python//python/pip_install:repositories.bzl", "pip_install_dependencies")

pip_install_dependencies()

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
    sha256 = "7315acb6bf10e99f332c8a43f00d5fbb1ee6ca48c52f6b936991b216c586aaad",
    strip_prefix = "googletest-1.15.0",
    urls = [
        "https://github.com/google/googletest/releases/download/v1.15.0/googletest-1.15.0.tar.gz" # 2024-07-15
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

load("@build_bazel_apple_support//lib:repositories.bzl", "apple_support_dependencies")

apple_support_dependencies()

load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies")

rules_cc_dependencies()

# For `kt_jvm_library`
load("@rules_kotlin//kotlin:repositories.bzl", "kotlin_repositories")

kotlin_repositories()

load("@rules_kotlin//kotlin:core.bzl", "kt_register_toolchains")

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
    build_file = "//python/dist:lua.BUILD",
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
    urls = ["https://github.com/googleapis/googleapis/archive/d81d0b9e6993d6ab425dff4d7c3d05fb2e59fa57.zip"],
    strip_prefix = "googleapis-d81d0b9e6993d6ab425dff4d7c3d05fb2e59fa57",
    sha256 = "d986023c3d8d2e1b161e9361366669cac9fb97c2a07e656c2548aca389248bb4",
    build_file = "//benchmarks:BUILD.googleapis",
    patch_cmds = ["find google -type f -name BUILD.bazel -delete"],
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
    sha256 = "77206c54b71f4dd5335123a6ff2a8ea688eca5378d34b4838114dff71652cf26",
    strip_prefix = "rules_fuzzing-0.5.1",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/releases/download/v0.5.1/rules_fuzzing-0.5.1.zip"],
    patches = ["//third_party:rules_fuzzing.patch"],
    patch_args = ["-p1"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

load("@fuzzing_py_deps//:requirements.bzl", fuzzing_py_deps_install_deps = "install_deps")

fuzzing_py_deps_install_deps()

http_archive(
    name = "rules_rust",
    integrity = "sha256-F8U7+AC5MvMtPKGdLLnorVM84cDXKfDRgwd7/dq3rUY=",
    urls = ["https://github.com/bazelbuild/rules_rust/releases/download/0.46.0/rules_rust-v0.46.0.tar.gz"],
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
            git = "https://github.com/google/googletest-rust",
            rev = "b407f3b5774defb8917d714bfb7af485e117d621",
        ),
        "paste": crate.spec(
          version = ">=1",
        ),
    },
)

load("@crate_index//:defs.bzl", "crate_repositories")
crate_repositories()

# For testing runtime against old gencode from a previous major version.
http_archive(
    name = "com_google_protobuf_v25.0",
    strip_prefix = "protobuf-25.0",
    url = "https://github.com/protocolbuffers/protobuf/releases/download/v25.0/protobuf-25.0.tar.gz",
)

# Needed as a dependency of @com_google_protobuf_v25.0
load("@com_google_protobuf_v25.0//:protobuf_deps.bzl", protobuf_v25_deps="protobuf_deps")
protobuf_v25_deps()

# Needed for testing only
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
http_archive(
    name = "rules_testing",
    sha256 = "02c62574631876a4e3b02a1820cb51167bb9cdcdea2381b2fa9d9b8b11c407c4",
    strip_prefix = "rules_testing-0.6.0",
    url = "https://github.com/bazelbuild/rules_testing/releases/download/v0.6.0/rules_testing-v0.6.0.tar.gz",
)

# For checking breaking changes to well-known types from the previous release version.
load("//:protobuf_version.bzl", "PROTOBUF_PREVIOUS_RELEASE")

http_archive(
    name = "com_google_protobuf_previous_release",
    strip_prefix = "protobuf-" + PROTOBUF_PREVIOUS_RELEASE,
    url = "https://github.com/protocolbuffers/protobuf/releases/download/v{0}/protobuf-{0}.tar.gz".format(PROTOBUF_PREVIOUS_RELEASE),
)

http_archive(
    name = "rules_buf",
    integrity = "sha256-Hr64Q/CaYr0E3ptAjEOgdZd1yc+cBjp7OG1wzuf3DIs=",
    strip_prefix = "rules_buf-0.3.0",
    urls = [
        "https://github.com/bufbuild/rules_buf/archive/refs/tags/v0.3.0.zip",
    ],
)

load("@rules_buf//buf:repositories.bzl", "rules_buf_dependencies", "rules_buf_toolchains")

rules_buf_dependencies()

rules_buf_toolchains(version = "v1.32.1")
