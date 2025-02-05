# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""A Starlark implementation of the java_lite_proto_library rule."""

load("@rules_java//java/common:java_common.bzl", "java_common")
load("@rules_java//java/common:java_info.bzl", "JavaInfo")
load("@rules_java//java/common:proguard_spec_info.bzl", "ProguardSpecInfo")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/private:java_proto_support.bzl", "JavaProtoAspectInfo", "java_compile_for_protos", "java_info_merge_for_protos")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")

_PROTO_TOOLCHAIN_ATTR = "_aspect_proto_toolchain_for_javalite"

_JAVA_LITE_PROTO_TOOLCHAIN = Label("//bazel/private:javalite_toolchain_type")

def _aspect_impl(target, ctx):
    """Generates and compiles Java code for a proto_library dependency graph.

    Args:
      target: (Target) The `proto_library` target.
      ctx: (RuleContext) The rule context.

    Returns:
      ([JavaInfo, JavaProtoAspectInfo]) A JavaInfo describing compiled Java
      version of`proto_library` and `JavaProtoAspectInfo` with all source and
      runtime jars.
    """

    deps = [dep[JavaInfo] for dep in ctx.rule.attr.deps]
    exports = [exp[JavaInfo] for exp in ctx.rule.attr.exports]
    proto_toolchain_info = toolchains.find_toolchain(
        ctx,
        "_aspect_proto_toolchain_for_javalite",
        _JAVA_LITE_PROTO_TOOLCHAIN,
    )
    source_jar = None

    if proto_common.experimental_should_generate_code(target[ProtoInfo], proto_toolchain_info, "java_lite_proto_library", target.label):
        source_jar = ctx.actions.declare_file(ctx.label.name + "-lite-src.jar")
        proto_common.compile(
            ctx.actions,
            target[ProtoInfo],
            proto_toolchain_info,
            [source_jar],
            experimental_output_files = "single",
        )
        runtime = proto_toolchain_info.runtime
        if runtime:
            deps.append(runtime[JavaInfo])

    java_info, jars = java_compile_for_protos(
        ctx,
        "-lite.jar",
        source_jar,
        deps,
        exports,
        injecting_rule_kind = "java_lite_proto_library",
    )
    transitive_jars = [dep[JavaProtoAspectInfo].jars for dep in ctx.rule.attr.deps]

    return [
        java_info,
        JavaProtoAspectInfo(jars = depset(jars, transitive = transitive_jars)),
    ]

_java_lite_proto_aspect = aspect(
    implementation = _aspect_impl,
    attr_aspects = ["deps", "exports"],
    attrs = toolchains.if_legacy_toolchain({
        _PROTO_TOOLCHAIN_ATTR: attr.label(
            default = configuration_field(fragment = "proto", name = "proto_toolchain_for_java_lite"),
        ),
    }),
    fragments = ["java"],
    required_providers = [ProtoInfo],
    provides = [JavaInfo, JavaProtoAspectInfo],
    toolchains = ["@bazel_tools//tools/jdk:toolchain_type"] +
                 toolchains.use_toolchain(_JAVA_LITE_PROTO_TOOLCHAIN),
)

def _rule_impl(ctx):
    """Merges results of `java_proto_aspect` in `deps`.

    `java_lite_proto_library` is identical to `java_proto_library` in every respect, except it
    builds JavaLite protos.
    Implementation of this rule is built on the implementation of `java_proto_library`.

    Args:
      ctx: (RuleContext) The rule context.
    Returns:
      ([JavaInfo, DefaultInfo, OutputGroupInfo, ProguardSpecInfo])
    """
    proto_toolchain_info = toolchains.find_toolchain(
        ctx,
        "_aspect_proto_toolchain_for_javalite",
        _JAVA_LITE_PROTO_TOOLCHAIN,
    )
    for dep in ctx.attr.deps:
        proto_common.check_collocated(ctx.label, dep[ProtoInfo], proto_toolchain_info)

    runtime = proto_toolchain_info.runtime

    if runtime:
        proguard_provider_specs = runtime[ProguardSpecInfo]
    else:
        proguard_provider_specs = ProguardSpecInfo(depset())

    java_info = java_info_merge_for_protos([dep[JavaInfo] for dep in ctx.attr.deps], merge_java_outputs = False)

    transitive_src_and_runtime_jars = depset(transitive = [dep[JavaProtoAspectInfo].jars for dep in ctx.attr.deps])
    transitive_runtime_jars = depset(transitive = [java_info.transitive_runtime_jars])

    if hasattr(java_common, "add_constraints"):
        java_info = java_common.add_constraints(java_info, constraints = ["android"])

    return [
        java_info,
        DefaultInfo(
            files = transitive_src_and_runtime_jars,
            runfiles = ctx.runfiles(transitive_files = transitive_runtime_jars),
        ),
        OutputGroupInfo(default = depset()),
        proguard_provider_specs,
    ]

java_lite_proto_library = rule(
    implementation = _rule_impl,
    doc = """
<p>
<code>java_lite_proto_library</code> generates Java code from <code>.proto</code> files.
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
    runtime_deps = [":foo"],
)

java_lite_proto_library(
    name = "foo",
    deps = [":bar"],
)

proto_library(
    name = "bar",
)
</code>
</pre>
""",
    attrs = {
        "deps": attr.label_list(providers = [ProtoInfo], aspects = [_java_lite_proto_aspect], doc = """
The list of <a href="protocol-buffer.html#proto_library"><code>proto_library</code></a>
rules to generate Java code for.
"""),
    } | toolchains.if_legacy_toolchain({
        _PROTO_TOOLCHAIN_ATTR: attr.label(
            default = configuration_field(fragment = "proto", name = "proto_toolchain_for_java_lite"),
        ),
    }),
    provides = [JavaInfo],
    toolchains = toolchains.use_toolchain(_JAVA_LITE_PROTO_TOOLCHAIN),
)

# public re-export, note that we can't rename the original symbol because that changes the aspect id
java_lite_proto_aspect = _java_lite_proto_aspect
