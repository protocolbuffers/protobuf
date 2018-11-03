
workspace(name = "upb")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "lua",
    build_file = "//:lua.BUILD",
    sha256 = "b9e2e4aad6789b3b63a056d442f7b39f0ecfca3ae0f1fc0ae4e9614401b69f4b",
    strip_prefix = "lua-5.2.4",
    urls = [
        "https://mirror.bazel.build/www.lua.org/ftp/lua-5.2.4.tar.gz",
        "https://www.lua.org/ftp/lua-5.2.4.tar.gz",
    ],
)

http_archive(
    name = "com_google_protobuf",
    sha256 = "d7a221b3d4fb4f05b7473795ccea9e05dab3b8721f6286a95fffbffc2d926f8b",
    strip_prefix = "protobuf-3.6.1",
    urls = [
        "https://github.com/protocolbuffers/protobuf/archive/v3.6.1.zip"
    ],
)

http_archive(
    name = "ragel",
    sha256 = "5f156edb65d20b856d638dd9ee2dfb43285914d9aa2b6ec779dac0270cd56c3f",
    build_file = "//:ragel.BUILD",
    strip_prefix = "ragel-6.10",
    urls = [
        "http://www.colm.net/files/ragel/ragel-6.10.tar.gz"
    ],
)
