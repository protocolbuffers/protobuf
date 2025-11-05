"Module extensions for use under bzlmod"

load("@bazel_skylib//lib:modules.bzl", "modules")
load("//toolchain:platforms.bzl", "PROTOBUF_PLATFORMS")
load("//bazel/private:prebuilt_protoc_toolchain.bzl", "prebuilt_protoc_repo")

def create_all_toolchain_repos(name = "prebuilt_protoc"):
    for platform in PROTOBUF_PLATFORMS.keys():
        prebuilt_protoc_repo(
            # We must replace hyphen with underscore to workaround rules_python py_proto_library constraint
            name = ".".join([name, platform.replace("-", "_")]),
            platform = platform,
        )

protoc = modules.as_extension(create_all_toolchain_repos)
