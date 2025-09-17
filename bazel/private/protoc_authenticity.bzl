"Checks that the protoc binary is authentic and not spoofed by a malicious actor"
load("//bazel/common:proto_common.bzl", "proto_common")
load("toolchain_helpers.bzl", "toolchains")

def _protoc_authenticity_impl(ctx):
    if proto_common.INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION:
        toolchain = ctx.toolchains[toolchains.PROTO_TOOLCHAIN]
        if not toolchain:
            fail("Protocol compiler toolchain could not be resolved.")
        proto_lang_toolchain_info = toolchain.proto
    else:
        proto_lang_toolchain_info = proto_common.ProtoLangToolchainInfo(
            out_replacement_format_flag = "--descriptor_set_out=%s",
            output_files = "single",
            mnemonic = "GenProtoDescriptorSet",
            progress_message = "Generating Descriptor Set proto_library %{label}",
            proto_compiler = ctx.executable._proto_compiler,
            protoc_opts = ctx.fragments.proto.experimental_protoc_opts,
            plugin = None,
        )
    validation_output = ctx.actions.declare_file("validation_output.txt")
    
    ctx.actions.run_shell(
        outputs = [validation_output],
        tools = [proto_lang_toolchain_info.proto_compiler],
        command = proto_lang_toolchain_info.proto_compiler.path + " --version ; echo 'protoc came from an untrusted source, we do not support this. To suppress this warning run with --norun_validations'; false".format(),
    )
    return [OutputGroupInfo(_validation = depset([validation_output]))]

protoc_authenticity = rule(
    implementation = _protoc_authenticity_impl,
    fragments = ["proto"],
    attrs = toolchains.if_legacy_toolchain({
        "_proto_compiler": attr.label(
            cfg = "exec",
            executable = True,
            allow_files = True,
            default = "//src/google/protobuf/compiler:protoc_minimal",
        ),
    }),
    toolchains = toolchains.use_toolchain(toolchains.PROTO_TOOLCHAIN),
)