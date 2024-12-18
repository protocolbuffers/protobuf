load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

PROTOC_BUILDFILE = Label("//bazel/private:BUILD.protoc")
PREBUILT_BINARIES = {
    "com_google_protobuf_protoc_linux_aarch64": struct(
        integrity = "sha256-uP1ZdpJhmKfE6lxutL94lZ1frtJ7/GGCVMqhBD93BEU=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.1/protoc-29.1-linux-aarch_64.zip",
    ),
    "com_google_protobuf_protoc_linux_ppc": struct(
        integrity = "sha256-B1vWZq1B60BKkjv6+pCkr6IHSiyaqLnHfERF5hbo75s=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.1/protoc-29.1-linux-ppcle_64.zip",
    ),
    "com_google_protobuf_protoc_linux_s390_64": struct(
        integrity = "sha256-J5fNVlyn/7/ALaVsygE7/0+X5oMkizuZ3PfUuxf27GE=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.1/protoc-29.1-linux-s390_64.zip",
    ),
    "com_google_protobuf_protoc_linux_x86_32": struct(
        integrity = "sha256-nd/EAbEqC4dHHg4POg0MKpByRj8EFDUUnpSfiiCCu+s=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.1/protoc-29.1-linux-x86_32.zip",
    ),
    "com_google_protobuf_protoc_linux_x86_64": struct(
        integrity = "sha256-AMg/6XIthelsgblBsp8Xp0SzO0zmbg8YAJ/Yk33iLGA=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.1/protoc-29.1-linux-x86_64.zip",
    ),
    "com_google_protobuf_protoc_macos_aarch64": struct(
        integrity = "sha256-2wK0uG3k1MztPqmTQ0faKNyV5/OIY//EzjzCYoMCjaY=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.1/protoc-29.1-osx-aarch_64.zip",
    ),
    "com_google_protobuf_protoc_macos_x86_64": struct(
        integrity = "sha256-2wK0uG3k1MztPqmTQ0faKNyV5/OIY//EzjzCYoMCjaY=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.1/protoc-29.1-osx-x86_64.zip",
    ),
    "com_google_protobuf_protoc_windows_x86_32": struct(
        build_file = Label("//bazel/private:BUILD.windows_protoc"),
        integrity = "sha256-fqSCJYV//BIkWIwzXCsa+deKGK+dV8BSjMoxk+M26c4=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.1/protoc-29.1-win32.zip",
    ),
    "com_google_protobuf_protoc_windows_x86_64": struct(
        build_file = ":BUILD.windows_protoc",
        integrity = "sha256-EQXg+mRFnwsa9e5NWHfauGTy4Q2K6wRhjytpxsOm7QM=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v29.1/protoc-29.1-win64.zip",
    ),
}

def prebuilt_binaries():
    # Prebuilt binaries
    for name in PREBUILT_BINARIES:
        if not native.existing_rule(name):
            http_archive(
                name = name,
                build_file = getattr(PREBUILT_BINARIES[name], "build_file", PROTOC_BUILDFILE),
                integrity = PREBUILT_BINARIES[name].integrity,
                url = PREBUILT_BINARIES[name].url,
            )

prebuilt_binaries_ext = module_extension(lambda ctx: prebuilt_binaries())
