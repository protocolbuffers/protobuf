
workspace(name = "upb")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

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

git_repository(
    name = "com_google_protobuf",
    commit = "25feb59620627b673df76813dfd66e3f565765e7",
    #sha256 = "d7a221b3d4fb4f05b7473795ccea9e05dab3b8721f6286a95fffbffc2d926f8b",
    remote = "https://github.com/haberman/protobuf.git",
    #tag = "conformance-build-tag",
)

git_repository(
    name = "absl",
    commit = "070f6e47b33a2909d039e620c873204f78809492",
    remote = "https://github.com/abseil/abseil-cpp.git",
)

http_archive(
    name = "ragel",
    sha256 = "5f156edb65d20b856d638dd9ee2dfb43285914d9aa2b6ec779dac0270cd56c3f",
    build_file = "//:ragel.BUILD",
    strip_prefix = "ragel-6.10",
    urls = ["http://www.colm.net/files/ragel/ragel-6.10.tar.gz"],
)

# Used by protobuf.
http_archive(
    name = "bazel_skylib",
    sha256 = "bbccf674aa441c266df9894182d80de104cabd19be98be002f6d478aaa31574d",
    strip_prefix = "bazel-skylib-2169ae1c374aab4a09aa90e65efe1a3aad4e279b",
    urls = ["https://github.com/bazelbuild/bazel-skylib/archive/2169ae1c374aab4a09aa90e65efe1a3aad4e279b.tar.gz"],
)
