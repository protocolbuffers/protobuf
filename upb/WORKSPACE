workspace(name = "upb")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//bazel:workspace_deps.bzl", "upb_deps")

upb_deps()

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

load("@utf8_range//:workspace_deps.bzl", "utf8_range_deps")
utf8_range_deps()

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
    urls = ["https://github.com/googleapis/googleapis/archive/refs/heads/master.zip"],
    build_file = "//benchmarks:BUILD.googleapis",
    strip_prefix = "googleapis-master",
    patch_cmds = ["find google -type f -name BUILD.bazel -delete"],
)

http_archive(
    name = "com_google_absl",
    sha256 = "e7fdfe0bed87702a22c5b73b6b5fe08bedd25f17d617e52df6061b0f47d480b0",
    strip_prefix = "abseil-cpp-e6044634dd7caec2d79a13aecc9e765023768757",
    urls = [
        "https://github.com/abseil/abseil-cpp/archive/e6044634dd7caec2d79a13aecc9e765023768757.tar.gz"
    ],
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

load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")

rules_pkg_dependencies()

load("//bazel:system_python.bzl", "system_python")
system_python(
    name = "system_python",
    minimum_python_version = "3.7",
)

load("@system_python//:register.bzl", "register_system_python")
register_system_python()

load("@system_python//:pip.bzl", "pip_parse")

pip_parse(
    name="pip_deps",
    requirements = "//python:requirements.txt",
    requirements_overrides = {
        "3.11": "//python:requirements_311.txt",
    },
)

load("@pip_deps//:requirements.bzl", "install_deps")
install_deps()
