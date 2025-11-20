"Validate that the protoc binary is authentic and not spoofed by a malicious actor."
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")
load("//bazel/private:prebuilt_tool_integrity.bzl", "RELEASE_VERSION")

def _protoc_authenticity_impl(ctx):
    # When this flag is disabled, then users have no way to replace the protoc binary with their own toolchain registration.
    # Therefore there's no validation to perform.
    if not proto_common.INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION:
        return [OutputGroupInfo(_validation = depset())]
    toolchain = ctx.toolchains[toolchains.PROTO_TOOLCHAIN]
    if not toolchain:
        fail("Protocol compiler toolchain could not be resolved.")
    proto_lang_toolchain_info = toolchain.proto
    validation_output = ctx.actions.declare_file("validation_output.txt")

    ctx.actions.run_shell(
        outputs = [validation_output],
        tools = [proto_lang_toolchain_info.proto_compiler],
        command = """\
        {protoc} --version > {validation_output}
        grep -q "^libprotoc {RELEASE_VERSION}" {validation_output} || {{
          echo 'ERROR: protoc version does not match protobuf Bazel module; we do not support this. To suppress this error, run Bazel with --norun_validations'
          echo 'Expected: libprotoc {RELEASE_VERSION}'
          echo -n 'Actual:   '
          cat {validation_output}
          exit 1
        }} >&2
        """.format(
            protoc = proto_lang_toolchain_info.proto_compiler.executable.path,
            validation_output = validation_output.path,
            RELEASE_VERSION = RELEASE_VERSION.removeprefix("v"),
        ),
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