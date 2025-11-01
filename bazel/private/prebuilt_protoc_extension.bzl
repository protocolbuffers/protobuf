"Module extensions for use under bzlmod"

load("//toolchain:platforms.bzl", "PROTOBUF_PLATFORMS")
load("//bazel/private:prebuilt_protoc_toolchain.bzl", "prebuilt_protoc_repo")

def create_all_toolchain_repos(name, version):
    for platform in PROTOBUF_PLATFORMS.keys():
        prebuilt_protoc_repo(
            # We must replace hyphen with underscore to workaround rules_python py_proto_library constraint
            name = ".".join([name, platform.replace("-", "_")]),
            platform = platform,
            version = version,
        )

protoc = module_extension(
    # TODO: replace version number here during release, maybe with git archive .gitattributes config
    implementation = lambda module_ctx: create_all_toolchain_repos("prebuilt_protoc", "v33.0")
)
