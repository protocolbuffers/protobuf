workspace(name = "com_google_protobuf")

# An explicit self-reference to work around changes in Bazel 7.0
# See https://github.com/bazelbuild/bazel/issues/19973#issuecomment-1787814450
# buildifier: disable=duplicated-name
local_repository(
    name = "com_google_protobuf",
    path = ".",
)

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Load common dependencies first to ensure we use the correct version
load("//:protobuf_deps.bzl", "PROTOBUF_MAVEN_ARTIFACTS", "protobuf_deps")

protobuf_deps()

load("@rules_java//java:rules_java_deps.bzl", "rules_java_dependencies")

rules_java_dependencies()

load("@rules_java//java:repositories.bzl", "rules_java_toolchains")

rules_java_toolchains()

load("@bazel_features//:deps.bzl", "bazel_features_deps")

bazel_features_deps()

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()

# Bazel platform rules.
http_archive(
    name = "platforms",
    sha256 = "218efe8ee736d26a3572663b374a253c012b716d8af0c07e842e82f238a0a7ee",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/platforms/releases/download/0.0.10/platforms-0.0.10.tar.gz",
        "https://github.com/bazelbuild/platforms/releases/download/0.0.10/platforms-0.0.10.tar.gz",
    ],
)

http_archive(
    name = "googletest",
    sha256 = "7315acb6bf10e99f332c8a43f00d5fbb1ee6ca48c52f6b936991b216c586aaad",
    strip_prefix = "googletest-1.15.0",
    urls = [
        "https://github.com/google/googletest/releases/download/v1.15.0/googletest-1.15.0.tar.gz",  # 2024-07-15
    ],
)

load("@googletest//:googletest_deps.bzl", "googletest_deps")

googletest_deps()

load("@rules_jvm_external//:repositories.bzl", "rules_jvm_external_deps")

rules_jvm_external_deps()

load("@rules_jvm_external//:setup.bzl", "rules_jvm_external_setup")

rules_jvm_external_setup()

load("@rules_jvm_external//:defs.bzl", "maven_install")

maven_install(
    name = "maven",
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

maven_install(
    name = "protobuf_maven_dev",
    artifacts = [
        "com.google.caliper:caliper:1.0-beta-3",
        "com.google.guava:guava-testlib:32.0.1-jre",
        "com.google.testparameterinjector:test-parameter-injector:1.18",
        "com.google.truth:truth:1.1.2",
        "junit:junit:4.13.2",
        "org.mockito:mockito-core:4.3.1",
        "biz.aQute.bnd:biz.aQute.bndlib:6.4.0",
        "info.picocli:picocli:4.6.3",
    ],
    # For updating instructions, see:
    # https://github.com/bazelbuild/rules_jvm_external#updating-maven_installjson
    maven_install_json = "//:maven_dev_install.json",
    repositories = [
        "https://repo1.maven.org/maven2",
        "https://repo.maven.apache.org/maven2",
    ],
)

load("@protobuf_maven_dev//:defs.bzl", pinned_protobuf_maven_install = "pinned_maven_install")

pinned_protobuf_maven_install()

# For `cc_proto_blacklist_test` and `build_test`.
load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")

rules_pkg_dependencies()

load("@build_bazel_rules_apple//apple:repositories.bzl", "apple_rules_dependencies")

apple_rules_dependencies()

load("@build_bazel_apple_support//lib:repositories.bzl", "apple_support_dependencies")

apple_support_dependencies()

load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies", "rules_cc_toolchains")

rules_cc_dependencies()

rules_cc_toolchains()

# For `kt_jvm_library`
load("@rules_kotlin//kotlin:repositories.bzl", "kotlin_repositories")

kotlin_repositories()

load("@rules_kotlin//kotlin:core.bzl", "kt_register_toolchains")

kt_register_toolchains()

# Workaround for https://github.com/bazel-contrib/rules_ruby/issues/216
# Patch rules_ruby to disable automatic attempt to install bundler. When fixed,
# delete Disable_bundle_install.patch and remove the patch related attributes
http_archive(
    name = "rules_ruby",
    patch_args = ["-p1"],
    patches = [
        "@com_google_protobuf//:Disable_bundle_install.patch",
    ],
    sha256 = "005da20827bee6b33d8ece7dc9973da293d7f8bb2ca07beaac43c31acaadbd31",
    strip_prefix = "rules_ruby-0.17.3",
    url = "https://github.com/bazel-contrib/rules_ruby/releases/download/v0.17.3/rules_ruby-v0.17.3.tar.gz",
)

load("@rules_ruby//ruby:deps.bzl", "rb_bundle_fetch", "rb_register_toolchains")

rb_register_toolchains(
    version = "system",
)

rb_bundle_fetch(
    name = "protobuf_bundle",
    srcs = [
        "//ruby:google-protobuf.gemspec",
    ],
    gem_checksums = {
        "bigdecimal-3.1.9": "2ffc742031521ad69c2dfc815a98e426a230a3d22aeac1995826a75dabfad8cc",
        "bigdecimal-3.1.9-java": "dd9b8f7c870664cd9538a1325ce385ba57a6627969177258c4f0e661a7be4456",
        "ffi-1.17.1": "26f6b0dbd1101e6ffc09d3ca640b2a21840cc52731ad8a7ded9fb89e5fb0fc39",
        "ffi-1.17.1-java": "2546e11f9592e2b9b6de49eb96d2a378da47b0bb8469d5cbc9881a55c0d55da7",
        "ffi-compiler-1.3.2": "a94f3d81d12caf5c5d4ecf13980a70d0aeaa72268f3b9cc13358bcc6509184a0",
        "power_assert-2.0.5": "63b511b85bb8ea57336d25156864498644f5bbf028699ceda27949e0125bc323",
        "rake-13.2.1": "46cb38dae65d7d74b6020a4ac9d48afed8eb8149c040eccf0523bec91907059d",
        "rake-compiler-1.1.9": "51b5c95a1ff25cabaaf92e674a2bed847ab53d66302fc8843830df46ab1f51f5",
        "rake-compiler-dock-1.2.1": "3cc968d7ffc923c0e775b28d79a3389efb3d2b16ef52ed0298fbc97d347e5878",
        "test-unit-3.6.7": "c342bb9f7334ea84a361b43c20b063f405c0bf3c7dbe3ff38f61a91661d29221",
    },
    gemfile = "//ruby:Gemfile",
    gemfile_lock = "//ruby:Gemfile.lock",
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
    name = "google_benchmark",
    sha256 = "62e2f2e6d8a744d67e4bbc212fcfd06647080de4253c97ad5c6749e09faf2cb0",
    strip_prefix = "benchmark-0baacde3618ca617da95375e0af13ce1baadea47",
    urls = ["https://github.com/google/benchmark/archive/0baacde3618ca617da95375e0af13ce1baadea47.zip"],
)

http_archive(
    name = "googleapis",
    integrity = "sha256-PopiL25y4WYMFq6EavbasLPraW+98BcuV7nN1nUjeOc=",
    strip_prefix = "googleapis-fe8ba054ad4f7eca946c2d14a63c3f07c0b586a0",
    urls = ["https://github.com/googleapis/googleapis/archive/fe8ba054ad4f7eca946c2d14a63c3f07c0b586a0.zip"],
)

load("@googleapis//:repository_rules.bzl", "switched_rules_by_language")

switched_rules_by_language(
    name = "com_google_googleapis_imports",
    cc = True,
)

load("@system_python//:pip.bzl", "pip_parse")

pip_parse(
    name = "protobuf_pip_deps",
    requirements = "//python:requirements.txt",
)

load("@protobuf_pip_deps//:requirements.bzl", "install_deps")

install_deps()

http_archive(
    name = "com_google_absl_py",
    sha256 = "8a3d0830e4eb4f66c4fa907c06edf6ce1c719ced811a12e26d9d3162f8471758",
    strip_prefix = "abseil-py-2.1.0",
    urls = [
        "https://github.com/abseil/abseil-py/archive/refs/tags/v2.1.0.tar.gz",
    ],
)

http_archive(
    name = "rules_fuzzing",
    integrity = "sha256-CCdEIsQ4NBbfX5gpQ+QNWBQfdJwJAIu3gEQO7OaxE+Q=",
    strip_prefix = "rules_fuzzing-0.5.3",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.5.3.tar.gz"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

load("@fuzzing_py_deps//:requirements.bzl", fuzzing_py_deps_install_deps = "install_deps")

fuzzing_py_deps_install_deps()

http_archive(
    name = "rules_rust",
    integrity = "sha256-8TBqrAsli3kN8BrZq8arsN8LZUFsdLTvJ/Sqsph4CmQ=",
    urls = ["https://github.com/bazelbuild/rules_rust/releases/download/0.56.0/rules_rust-0.56.0.tar.gz"],
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
            rev = "0db12bb338b9e75884a24493273c6bf4ce0fa5f5",
        ),
        "paste": crate.spec(
            version = ">=1",
        ),
        "quote": crate.spec(
            version = ">=1",
        ),
        "syn": crate.spec(
            version = ">=2",
        ),
    },
)

load("@crate_index//:defs.bzl", "crate_repositories")

crate_repositories()

# For testing runtime against old gencode from a previous major version.
http_archive(
    name = "com_google_protobuf_v25",
    integrity = "sha256-e+7ZxRHWMs/3wirACU3Xcg5VAVMDnV2n4Fm8zrSIR0o=",
    patch_args = ["-p1"],
    patches = [
        # There are other patches, but they are only needed for bzlmod.
        "@com_google_protobuf//:patches/protobuf_v25/0005-Make-rules_ruby-a-dev-only-dependency.patch",
    ],
    strip_prefix = "protobuf-25.0",
    url = "https://github.com/protocolbuffers/protobuf/releases/download/v25.0/protobuf-25.0.tar.gz",
)

# Needed as a dependency of @com_google_protobuf_v25
load("@com_google_protobuf_v25//:protobuf_deps.bzl", protobuf_v25_deps = "protobuf_deps")

protobuf_v25_deps()

http_archive(
    name = "rules_testing",
    sha256 = "89feaf18d6e2fc07ed7e34510058fc8d48e45e6d2ff8a817a718e8c8e4bcda0e",
    strip_prefix = "rules_testing-0.8.0",
    url = "https://github.com/bazelbuild/rules_testing/releases/download/v0.8.0/rules_testing-v0.8.0.tar.gz",
)

# For checking breaking changes to well-known types from the previous release version.
http_archive(
    name = "com_google_protobuf_previous_release",
    integrity = "sha256-EKDVjzmhqQnpXgDougtbHcZNApl/dBFRlTorNln254w=",
    strip_prefix = "protobuf-29.0",
    urls = ["https://github.com/protocolbuffers/protobuf/releases/download/v29.0/protobuf-29.0.tar.gz"],
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
