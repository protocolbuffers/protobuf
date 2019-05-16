workspace(name = "upb")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//bazel:workspace_deps.bzl", "upb_deps")

upb_deps()

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
    name = "ragel",
    build_file = "//bazel:ragel.BUILD",
    sha256 = "5f156edb65d20b856d638dd9ee2dfb43285914d9aa2b6ec779dac0270cd56c3f",
    strip_prefix = "ragel-6.10",
    urls = ["http://www.colm.net/files/ragel/ragel-6.10.tar.gz"],
)
