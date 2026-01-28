"Validate that the protoc binary is authentic and not spoofed by a malicious actor."

load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")
load(":tool_integrity.bzl", "RELEASE_VERSION")

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
        mnemonic = "ProtocAuthenticityCheck",
        outputs = [validation_output],
        tools = [proto_lang_toolchain_info.proto_compiler],
        command = """\
        {protoc} --version > {validation_output}
        grep -q -e "-dev$" {validation_output} && {{
          echo 'WARNING: Detected a development version of protoc.
          Development versions are not validated for authenticity.
          To ensure a secure build, please use a released version of protoc.'
          exit 0
        }}
        grep -q "^libprotoc {RELEASE_VERSION}" {validation_output} || {{
          echo '{severity}: protoc version does not match protobuf Bazel module; we do not support this.
          It is considered undefined behavior that is expected to break in the future even if it appears to work today.'
          echo '{suppression_note}'
          echo 'Expected: libprotoc {RELEASE_VERSION}'
          echo -n 'Actual:   '
          cat {validation_output}
          exit {mismatch_exit_code}
        }} >&2
        """.format(
            protoc = proto_lang_toolchain_info.proto_compiler.executable.path,
            validation_output = validation_output.path,
            RELEASE_VERSION = RELEASE_VERSION.removeprefix("v"),
            suppression_note = (
                "To suppress this error, run Bazel with --@com_google_protobuf//bazel/toolchains:allow_nonstandard_protoc" if ctx.attr.fail_on_mismatch else ""
            ),
            mismatch_exit_code = 1 if ctx.attr.fail_on_mismatch else 0,
            severity = "ERROR" if ctx.attr.fail_on_mismatch else "INFO",
        ),
    )
    return [OutputGroupInfo(_validation = depset([validation_output]))]

protoc_authenticity = rule(
    implementation = _protoc_authenticity_impl,
    fragments = ["proto"],
    attrs = {
        "fail_on_mismatch": attr.bool(
            default = True,
            doc = "If true, the build will fail when the protoc binary does not match the expected version.",
        ),
    } | toolchains.if_legacy_toolchain({
        "_proto_compiler": attr.label(
            cfg = "exec",
            executable = True,
            allow_files = True,
            default = "//src/google/protobuf/compiler:protoc_minimal",
        ),
    }),
    toolchains = toolchains.use_toolchain(toolchains.PROTO_TOOLCHAIN),
)
