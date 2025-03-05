load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

PROTOC_BUILDFILE = Label("//bazel/private:BUILD.protoc")
PREBUILT_BINARIES = {
    "com_google_protobuf_protoc_linux_aarch64": struct(
        integrity = "sha256-4LunQ/5RCqGL67mH9RHxwQrhxTMRkx0w5SWa2Fb7ZJk=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v30.0-rc2/protoc-30.0-rc-2-linux-aarch_64.zip",
    ),
    "com_google_protobuf_protoc_linux_ppc": struct(
        integrity = "sha256-hQiy28lPQ5dk7NsQoCGR0rFxTxduJTEAacTw4XPNavc=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v30.0-rc2/protoc-30.0-rc-2-linux-ppcle_64.zip",
    ),
    "com_google_protobuf_protoc_linux_s390_64": struct(
        integrity = "sha256-hYiuH9Pq+rMyil9G4DM7oXg74gmemUBurnZlA+BCleY=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v30.0-rc2/protoc-30.0-rc-2-linux-s390_64.zip",
    ),
    "com_google_protobuf_protoc_linux_x86_32": struct(
        integrity = "sha256-tN2rehmln+AsPXOgUv+VhTxKIvIhe59r0nybRFCaESg=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v30.0-rc2/protoc-30.0-rc-2-linux-x86_32.zip",
    ),
    "com_google_protobuf_protoc_linux_x86_64": struct(
        integrity = "sha256-qbS3bTy5Kom3AePAI0q28KSx0J5gqQJM4OZYgcKPDYA=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v30.0-rc2/protoc-30.0-rc-2-linux-x86_64.zip",
    ),
    "com_google_protobuf_protoc_macos_aarch64": struct(
        integrity = "sha256-7koCkd5xJT16s7AtwBMoAwzFk+JSMTFzCr8rGUfUG6A=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v30.0-rc2/protoc-30.0-rc-2-osx-aarch_64.zip",
    ),
    "com_google_protobuf_protoc_macos_x86_64": struct(
        integrity = "sha256-EbIMudCMgek/In6fM2ZEgUfkHPfBVXo0/M2ReKHD8Sw=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v30.0-rc2/protoc-30.0-rc-2-osx-x86_64.zip",
    ),
    "com_google_protobuf_protoc_windows_x86_32": struct(
        build_file = Label("//bazel/private:BUILD.windows_protoc"),
        integrity = "sha256-/WbDVbfzjvfMvkdjrVWX9ucaFaB7tMZJYmLkc3riiEg=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v30.0-rc2/protoc-30.0-rc-2-win32.zip",
    ),
    "com_google_protobuf_protoc_windows_x86_64": struct(
        build_file = ":BUILD.windows_protoc",
        integrity = "sha256-4HRmaW+0ZEUBTSF5LlGr4Jvnr45uZyY38i9N1iI3CGU=",
        url = "https://github.com/protocolbuffers/protobuf/releases/download/v30.0-rc2/protoc-30.0-rc-2-win64.zip",
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
