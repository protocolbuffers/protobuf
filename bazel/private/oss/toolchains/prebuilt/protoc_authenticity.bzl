"Validate that the protoc binary is authentic and not spoofed by a malicious actor."

load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")
load(":tool_integrity.bzl", "RELEASE_VERSION")

_ProtocAuthenticityOsInfo = provider(
    doc = "Information about the protoc authenticity action's execution platform.",
    fields = ["is_windows"],
)

def _protoc_authenticity_is_windows_impl(ctx):
    return _ProtocAuthenticityOsInfo(
        is_windows = ctx.target_platform_has_constraint(
            ctx.attr._windows_constraint[platform_common.ConstraintValueInfo],
        ),
    )

protoc_authenticity_is_windows = rule(
    implementation = _protoc_authenticity_is_windows_impl,
    attrs = {
        "_windows_constraint": attr.label(default = "@platforms//os:windows"),
    },
)

def _protoc_authenticity_unix_script(
        protoc,
        validation_output,
        expected_version,
        severity,
        suppression_note,
        mismatch_exit_code):
    return """\
{protoc} --version > {validation_output}
grep -q -e "-dev$" {validation_output} && {{
  echo 'WARNING: Detected a development version of protoc.
  Development versions are not validated for authenticity.
  To ensure a secure build, please use a released version of protoc.'
  exit 0
}}
grep -q "^libprotoc {expected_version}" {validation_output} || {{
  echo '{severity}: protoc version does not match protobuf Bazel module; we do not support this.
  It is considered undefined behavior that is expected to break in the future even if it appears to work today.'
  echo '{suppression_note}'
  echo 'Expected: libprotoc {expected_version}'
  echo -n 'Actual:   '
  cat {validation_output}
  exit {mismatch_exit_code}
}} >&2
""".format(
        protoc = protoc,
        validation_output = validation_output,
        expected_version = expected_version,
        severity = severity,
        suppression_note = suppression_note,
        mismatch_exit_code = mismatch_exit_code,
    )

def _protoc_authenticity_windows_script(
        protoc,
        validation_output,
        expected_version,
        severity,
        suppression_note,
        mismatch_exit_code):
    mismatch_lines = [
        "  1>&2 echo {severity}: protoc version does not match protobuf Bazel module; we do not support this.".format(
            severity = severity,
        ),
        "  1>&2 echo It is considered undefined behavior that is expected to break in the future even if it appears to work today.",
    ]
    if suppression_note:
        mismatch_lines.append(
            "  1>&2 echo {suppression_note}".format(suppression_note = suppression_note),
        )
    mismatch_lines.extend([
        "  1>&2 echo Expected: libprotoc {expected_version}".format(expected_version = expected_version),
        "  1>&2 echo Actual:",
        '  type "{validation_output}" 1>&2'.format(validation_output = validation_output),
        "  exit /B {mismatch_exit_code}".format(mismatch_exit_code = mismatch_exit_code),
    ])

    return "\r\n".join([
        "@echo off",
        "setlocal",
        '"{protoc}" --version > "{validation_output}"'.format(
            protoc = protoc,
            validation_output = validation_output,
        ),
        'findstr /R /C:"-dev$" "{validation_output}" >NUL'.format(
            validation_output = validation_output,
        ),
        "if %ERRORLEVEL% EQU 0 (",
        "  echo WARNING: Detected a development version of protoc.",
        "  echo Development versions are not validated for authenticity.",
        "  echo To ensure a secure build, please use a released version of protoc.",
        "  exit /B 0",
        ")",
        'findstr /R /C:"^libprotoc {expected_version}" "{validation_output}" >NUL'.format(
            expected_version = expected_version,
            validation_output = validation_output,
        ),
        "if %ERRORLEVEL% NEQ 0 (",
    ] + mismatch_lines + [
        ")",
        "exit /B 0",
        "",
    ])

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

    expected_version = RELEASE_VERSION.removeprefix("v")
    suppression_note = ""
    if ctx.attr.fail_on_mismatch:
        suppression_note = (
            "To suppress this error, run Bazel with "
            "--@com_google_protobuf//bazel/toolchains:allow_nonstandard_protoc"
        )
    mismatch_exit_code = 1 if ctx.attr.fail_on_mismatch else 0
    severity = "ERROR" if ctx.attr.fail_on_mismatch else "INFO"

    is_windows = ctx.attr._exec_is_windows[_ProtocAuthenticityOsInfo].is_windows
    if is_windows:
        script = ctx.actions.declare_file(ctx.label.name + "_authenticity_check.bat")
        script_content = _protoc_authenticity_windows_script(
            protoc = proto_lang_toolchain_info.proto_compiler.executable.path.replace(
                "/",
                "\\",
            ),
            validation_output = validation_output.path.replace("/", "\\"),
            expected_version = expected_version,
            severity = severity,
            suppression_note = suppression_note,
            mismatch_exit_code = mismatch_exit_code,
        )
        executable = "cmd.exe"
        arguments = ["/C", script.path.replace("/", "\\")]
    else:
        script = ctx.actions.declare_file(ctx.label.name + "_authenticity_check.sh")
        script_content = _protoc_authenticity_unix_script(
            protoc = proto_lang_toolchain_info.proto_compiler.executable.path,
            validation_output = validation_output.path,
            expected_version = expected_version,
            severity = severity,
            suppression_note = suppression_note,
            mismatch_exit_code = mismatch_exit_code,
        )
        executable = "bash"
        arguments = [script.path]

    ctx.actions.write(
        output = script,
        content = script_content,
    )
    ctx.actions.run(
        mnemonic = "ProtocAuthenticityCheck",
        outputs = [validation_output],
        tools = [proto_lang_toolchain_info.proto_compiler],
        inputs = [script],
        executable = executable,
        arguments = arguments,
        use_default_shell_env = True,
        toolchain = None,
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
        "_exec_is_windows": attr.label(
            default = ":is_windows",
            cfg = "exec",
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
