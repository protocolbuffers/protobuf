# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""The implementation of the `java_proto_library` rule and its aspect."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/private:java_proto_support.bzl", "JavaProtoAspectInfo", "java_compile_for_protos", "java_info_merge_for_protos")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")

_JAVA_PROTO_TOOLCHAIN = Label("//bazel/private:java_toolchain_type")

def _filter_provider(provider, *attrs):
    return [dep[provider] for attr in attrs for dep in attr if provider in dep]

def _bazel_java_proto_aspect_impl(target, ctx):
    """Generates and compiles Java code for a proto_library.

    The function runs protobuf compiler on the `proto_library` target using
    `proto_lang_toolchain` specified by `--proto_toolchain_for_java` flag.
    This generates a source jar.

    After that the source jar is compiled, respecting `deps` and `exports` of
    the `proto_library`.

    Args:
      target: (Target) The `proto_library` target (any target providing `ProtoInfo`.
      ctx: (RuleContext) The rule context.

    Returns:
      ([JavaInfo, JavaProtoAspectInfo]) A JavaInfo describing compiled Java
      version of`proto_library` and `JavaProtoAspectInfo` with all source and
      runtime jars.
    """

    proto_toolchain_info = toolchains.find_toolchain(ctx, "_aspect_java_proto_toolchain", _JAVA_PROTO_TOOLCHAIN)
    source_jar = None
    if proto_common.experimental_should_generate_code(target[ProtoInfo], proto_toolchain_info, "java_proto_library", target.label):
        # Generate source jar using proto compiler.
        source_jar = ctx.actions.declare_file(ctx.label.name + "-speed-src.jar")
        proto_common.compile(
            ctx.actions,
            target[ProtoInfo],
            proto_toolchain_info,
            [source_jar],
            experimental_output_files = "single",
        )

    # Compile Java sources (or just merge if there aren't any)
    deps = _filter_provider(JavaInfo, ctx.rule.attr.deps)
    exports = _filter_provider(JavaInfo, ctx.rule.attr.exports)
    if source_jar and proto_toolchain_info.runtime:
        deps.append(proto_toolchain_info.runtime[JavaInfo])
    java_info, jars = java_compile_for_protos(
        ctx,
        "-speed.jar",
        source_jar,
        deps,
        exports,
    )

    transitive_jars = [dep[JavaProtoAspectInfo].jars for dep in ctx.rule.attr.deps if JavaProtoAspectInfo in dep]
    return [
        java_info,
        JavaProtoAspectInfo(jars = depset(jars, transitive = transitive_jars)),
    ]

bazel_java_proto_aspect = aspect(
    implementation = _bazel_java_proto_aspect_impl,
    attrs = toolchains.if_legacy_toolchain({
        "_aspect_java_proto_toolchain": attr.label(
            default = configuration_field(fragment = "proto", name = "proto_toolchain_for_java"),
        ),
    }),
    toolchains = ["@bazel_tools//tools/jdk:toolchain_type"] + toolchains.use_toolchain(_JAVA_PROTO_TOOLCHAIN),
    attr_aspects = ["deps", "exports"],
    required_providers = [ProtoInfo],
    provides = [JavaInfo, JavaProtoAspectInfo],
    fragments = ["java"],
)

def bazel_java_proto_library_rule(ctx):
    """Merges results of `java_proto_aspect` in `deps`.

    Args:
      ctx: (RuleContext) The rule context.
    Returns:
      ([JavaInfo, DefaultInfo, OutputGroupInfo])
    """
    proto_toolchain = toolchains.find_toolchain(ctx, "_aspect_java_proto_toolchain", _JAVA_PROTO_TOOLCHAIN)
    for dep in ctx.attr.deps:
        proto_common.check_collocated(ctx.label, dep[ProtoInfo], proto_toolchain)

    java_info = java_info_merge_for_protos([dep[JavaInfo] for dep in ctx.attr.deps], merge_java_outputs = False)

    transitive_src_and_runtime_jars = depset(transitive = [dep[JavaProtoAspectInfo].jars for dep in ctx.attr.deps])
    transitive_runtime_jars = depset(transitive = [java_info.transitive_runtime_jars])

    return [
        java_info,
        DefaultInfo(
            files = transitive_src_and_runtime_jars,
            runfiles = ctx.runfiles(transitive_files = transitive_runtime_jars),
        ),
        OutputGroupInfo(default = depset()),
    ]

java_proto_library = rule(
    implementation = bazel_java_proto_library_rule,
    doc = """
<p>
<code>java_proto_library</code> generates Java code from <code>.proto</code> files.
</p>

<p>
<code>deps</code> must point to <a href="protocol-buffer.html#proto_library"><code>proto_library
</code></a> rules.
</p>

<p>
Example:
</p>

<pre class="code">
<code class="lang-starlark">
java_library(
    name = "lib",
    runtime_deps = [":foo_java_proto"],
)

java_proto_library(
    name = "foo_java_proto",
    deps = [":foo_proto"],
)

proto_library(
    name = "foo_proto",
)
</code>
</pre>
    """,
    attrs = {
        "deps": attr.label_list(
            providers = [ProtoInfo],
            aspects = [bazel_java_proto_aspect],
            doc = """
The list of <a href="protocol-buffer.html#proto_library"><code>proto_library</code></a>
rules to generate Java code for.
            """,
        ),
        # buildifier: disable=attr-license (calling attr.license())
        "licenses": attr.license() if hasattr(attr, "license") else attr.string_list(),
    } | toolchains.if_legacy_toolchain({
        "_aspect_java_proto_toolchain": attr.label(
            default = configuration_field(fragment = "proto", name = "proto_toolchain_for_java"),
        ),
    }),  # buildifier: disable=attr-licenses (attribute called licenses)
    provides = [JavaInfo],
    toolchains = toolchains.use_toolchain(_JAVA_PROTO_TOOLCHAIN),
)
