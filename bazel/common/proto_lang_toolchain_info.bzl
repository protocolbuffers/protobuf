"""ProtoLangToolchainInfo"""

load("//bazel/private:native.bzl", "native_proto_common")

# Use Starlark implementation only if native_proto_common.ProtoLangToolchainInfo doesn't exist
ProtoLangToolchainInfo = getattr(native_proto_common, "ProtoLangToolchainInfo", provider(
    doc = """Specifies how to generate language-specific code from .proto files.
            Used by LANG_proto_library rules.""",
    fields = dict(
        out_replacement_format_flag = """(str) Format string used when passing output to the plugin
          used by proto compiler.""",
        output_files = """("single","multiple","legacy") Format out_replacement_format_flag with
          a path to single file or a directory in case of multiple files.""",
        plugin_format_flag = "(str) Format string used when passing plugin to proto compiler.",
        plugin = "(FilesToRunProvider) Proto compiler plugin.",
        runtime = "(Target) Runtime.",
        provided_proto_sources = "(list[File]) Proto sources provided by the toolchain.",
        proto_compiler = "(FilesToRunProvider) Proto compiler.",
        protoc_opts = "(list[str]) Options to pass to proto compiler.",
        progress_message = "(str) Progress message to set on the proto compiler action.",
        mnemonic = "(str) Mnemonic to set on the proto compiler action.",
        allowlist_different_package = """(Target) Allowlist to create lang_proto_library in a
          different package than proto_library""",
        toolchain_type = """(Label) Toolchain type that was used to obtain this info""",
    ),
))
