"Module extensions for use under bzlmod"

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("//bazel/toolchains:prebuilt_toolchains.bzl", "prebuilt_toolchains_repo", "PROTOC_PLATFORMS")
load("//bazel/private:prebuilt_protoc_toolchain.bzl", "prebuilt_protoc_repo")

DEFAULT_REPOSITORY = "prebuilt_protoc_hub"

def create_all_toolchain_repos(name, version):
    for platform in PROTOC_PLATFORMS.keys():
        prebuilt_protoc_repo(
            # We must replace hyphen with underscore to workaround rules_python py_proto_library constraint
            name = ".".join([name, platform.replace("-", "_")]),
            platform = platform,
            version = version,
        )
    # name will be mangled by bzlmod into apparent name, so pass an extra copy that is preserved
    prebuilt_toolchains_repo(name = name, user_repository_name = name)

def _proto_extension_impl(module_ctx):
    registrations = {}
    root_name = None
    for mod in module_ctx.modules:
        for toolchain in mod.tags.prebuilt_toolchain:
            if toolchain.name != DEFAULT_REPOSITORY and not mod.is_root:
                fail("""\
                Only the root module may override the default name for the toolchain.
                This prevents conflicting registrations in the global namespace of external repos.
                """)

            # Ensure the root wins in case of differences
            if mod.is_root:
                create_all_toolchain_repos(toolchain.name, toolchain.version)
                root_name = toolchain.name
            elif toolchain.name not in registrations.keys():
                registrations[toolchain.name] = toolchain
    for name, toolchain in registrations.items():
        if name != root_name:
            create_all_toolchain_repos(name, toolchain.version)

    if bazel_features.external_deps.extension_metadata_has_reproducible:
        return module_ctx.extension_metadata(reproducible = True)
    else:
        return None

protoc = module_extension(
    implementation = _proto_extension_impl,
    tag_classes = {
        # buildifier: disable=unsorted-dict-items
        "prebuilt_toolchain": tag_class(attrs = {
            "name": attr.string(
                doc = """\
                Base name for generated repositories, allowing more than one toolchain to be registered.
                Overriding the default is only permitted in the root module.
                """,
                default = DEFAULT_REPOSITORY,
            ),
            "version": attr.string(doc = "A tag of protocolbuffers/protobuf repository."),
        }),
    },
)
