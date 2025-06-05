# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Implementation of the proto_lang_toolchain rule."""

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")

def _rule_impl(ctx):
    provided_proto_sources = depset(transitive = [bp[ProtoInfo].transitive_sources for bp in ctx.attr.blacklisted_protos]).to_list()

    flag = ctx.attr.command_line
    if flag.find("$(PLUGIN_OUT)") > -1:
        fail("in attribute 'command_line': Placeholder '$(PLUGIN_OUT)' is not supported.")
    flag = flag.replace("$(OUT)", "%s")

    plugin = None
    if ctx.attr.plugin != None:
        plugin = ctx.attr.plugin[DefaultInfo].files_to_run

    if proto_common.INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION:
        proto_compiler = ctx.toolchains[toolchains.PROTO_TOOLCHAIN].proto.proto_compiler
        protoc_opts = ctx.toolchains[toolchains.PROTO_TOOLCHAIN].proto.protoc_opts
    else:
        proto_compiler = ctx.attr._proto_compiler.files_to_run
        protoc_opts = ctx.fragments.proto.experimental_protoc_opts

    if ctx.attr.protoc_minimal_do_not_use:
        proto_compiler = ctx.attr.protoc_minimal_do_not_use.files_to_run

    proto_lang_toolchain_info = ProtoLangToolchainInfo(
        out_replacement_format_flag = flag,
        output_files = ctx.attr.output_files,
        plugin_format_flag = ctx.attr.plugin_format_flag,
        plugin = plugin,
        runtime = ctx.attr.runtime,
        provided_proto_sources = provided_proto_sources,
        proto_compiler = proto_compiler,
        protoc_opts = protoc_opts,
        progress_message = ctx.attr.progress_message,
        mnemonic = ctx.attr.mnemonic,
        allowlist_different_package = ctx.attr.allowlist_different_package,
        toolchain_type = ctx.attr.toolchain_type.label if ctx.attr.toolchain_type else None,
    )
    return [
        DefaultInfo(files = depset(), runfiles = ctx.runfiles()),
        platform_common.ToolchainInfo(proto = proto_lang_toolchain_info),
        # TODO: remove when --incompatible_enable_proto_toolchains is flipped and removed
        proto_lang_toolchain_info,
    ]

proto_lang_toolchain = rule(
    _rule_impl,
    doc = """
<p>If using Bazel, please load the rule from <a href="https://github.com/bazelbuild/rules_proto">
https://github.com/bazelbuild/rules_proto</a>.

<p>Specifies how a LANG_proto_library rule (e.g., <code>java_proto_library</code>) should invoke the
proto-compiler.
Some LANG_proto_library rules allow specifying which toolchain to use using command-line flags;
consult their documentation.

<p>Normally you should not write those kind of rules unless you want to
tune your Java compiler.

<p>There's no compiler. The proto-compiler is taken from the proto_library rule we attach to. It is
passed as a command-line flag to Blaze.
Several features require a proto-compiler to be invoked on the proto_library rule itself.
It's beneficial to enforce the compiler that LANG_proto_library uses is the same as the one
<code>proto_library</code> does.

<h4>Examples</h4>

<p>A simple example would be:
<pre><code class="lang-starlark">
proto_lang_toolchain(
    name = "javalite_toolchain",
    command_line = "--javalite_out=shared,immutable:$(OUT)",
    plugin = ":javalite_plugin",
    runtime = ":protobuf_lite",
)
</code></pre>
    """,
    attrs = {
        "progress_message": attr.string(default = "Generating proto_library %{label}", doc = """
This value will be set as the progress message on protoc action."""),
        "mnemonic": attr.string(default = "GenProto", doc = """
This value will be set as the mnemonic on protoc action."""),
        "command_line": attr.string(mandatory = True, doc = """
This value will be passed to proto-compiler to generate the code. Only include the parts
specific to this code-generator/plugin (e.g., do not include -I parameters)
<ul>
  <li><code>$(OUT)</code> is LANG_proto_library-specific. The rules are expected to define
      how they interpret this variable. For Java, for example, $(OUT) will be replaced with
      the src-jar filename to create.</li>
</ul>"""),
        "output_files": attr.string(values = ["single", "multiple", "legacy"], default = "legacy", doc = """
Controls how <code>$(OUT)</code> in <code>command_line</code> is formatted, either by
a path to a single file or output directory in case of multiple files.
Possible values are: "single", "multiple"."""),
        "plugin_format_flag": attr.string(doc = """
If provided, this value will be passed to proto-compiler to use the plugin.
The value must contain a single %s which is replaced with plugin executable.
<code>--plugin=protoc-gen-PLUGIN=&lt;executable&gt;.</code>"""),
        "plugin": attr.label(
            executable = True,
            cfg = "exec",
            doc = """
If provided, will be made available to the action that calls the proto-compiler, and will be
passed to the proto-compiler:
<code>--plugin=protoc-gen-PLUGIN=&lt;executable&gt;.</code>""",
        ),
        "runtime": attr.label(doc = """
A language-specific library that the generated code is compiled against.
The exact behavior is LANG_proto_library-specific.
Java, for example, should compile against the runtime."""),
        "blacklisted_protos": attr.label_list(
            providers = [ProtoInfo],
            doc = """
No code will be generated for files in the <code>srcs</code> attribute of
<code>blacklisted_protos</code>.
This is used for .proto files that are already linked into proto runtimes, such as
<code>any.proto</code>.""",
        ),
        # TODO: add doc
        "allowlist_different_package": attr.label(
            cfg = "exec",
            providers = [bazel_features.globals.PackageSpecificationInfo] if bazel_features.globals.PackageSpecificationInfo else [],
        ),
        # TODO: add doc
        "toolchain_type": attr.label(),
        # DO NOT USE. For Protobuf incremental changes only: b/305068148.
        "protoc_minimal_do_not_use": attr.label(
            cfg = "exec",
            executable = True,
        ),
    } | ({} if proto_common.INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION else {
        "_proto_compiler": attr.label(
            cfg = "exec",
            executable = True,
            allow_files = True,
            default = configuration_field("proto", "proto_compiler"),
        ),
    }),
    provides = [ProtoLangToolchainInfo],
    fragments = ["proto"],
    toolchains = toolchains.use_toolchain(toolchains.PROTO_TOOLCHAIN),  # Used to obtain protoc
)
