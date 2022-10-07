workspace(name = "upb")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//bazel:workspace_deps.bzl", "upb_deps")

upb_deps()
register_toolchains("@system_python//:python_toolchain")

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

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
    name = "com_google_googletest",
    sha256 = "81964fe578e9bd7c94dfdb09c8e4d6e6759e19967e397dbea48d1c10e45d0df2",
    strip_prefix = "googletest-release-1.12.1",
    urls = [
        "https://mirror.bazel.build/github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz",
        "https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz",
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
    urls = ["https://github.com/googleapis/googleapis/archive/refs/heads/master.zip"],
    build_file = "//benchmarks:BUILD.googleapis",
    strip_prefix = "googleapis-master",
    patch_cmds = ["find google -type f -name BUILD.bazel -delete"],
)

http_archive(
    name = "rules_fuzzing",
    sha256 = "23bb074064c6f488d12044934ab1b0631e8e6898d5cf2f6bde087adb01111573",
    strip_prefix = "rules_fuzzing-0.3.1",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.3.1.zip"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

load("@rules_python//python:pip.bzl", "pip_install")

pip_install(
    name="pip_deps",
    requirements = "@com_google_protobuf//python:requirements.txt"
)
