# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Support for compiling protoc generated Java code."""

load("@rules_java//java/private:proto_support.bzl", "compile", "merge")  # buildifier: disable=bzl-visibility

# The provider is used to collect source and runtime jars in the `proto_library` dependency graph.
JavaProtoAspectInfo = provider("JavaProtoAspectInfo", fields = ["jars"])

java_info_merge_for_protos = merge

def java_compile_for_protos(ctx, output_jar_suffix, source_jar = None, deps = [], exports = [], injecting_rule_kind = "java_proto_library"):
    """Compiles Java source jar returned by proto compiler.

    Use this call for java_xxx_proto_library. It uses java_common.compile with
    some checks disabled (via javacopts) and jspecify disabled, so that the
    generated code passes.

    It also takes care that input source jar is not repackaged with a different
    name.

    When `source_jar` is `None`, the function only merges `deps` and `exports`.

    Args:
      ctx: (RuleContext) Used to call `java_common.compile`
      output_jar_suffix: (str) How to name the output jar. For example: `-speed.jar`.
      source_jar: (File) Input source jar (may be `None`).
      deps: (list[JavaInfo]) `deps` of the `proto_library`.
      exports: (list[JavaInfo]) `exports` of the `proto_library`.
      injecting_rule_kind: (str) Rule kind requesting the compilation.
        It's embedded into META-INF of the produced runtime jar, for debugging.
    Returns:
      ((JavaInfo, list[File])) JavaInfo of this target and list containing source
      and runtime jar, when they are created.
    """
    if source_jar != None:
        path, sep, filename = ctx.label.name.rpartition("/")
        output_jar = ctx.actions.declare_file(path + sep + "lib" + filename + output_jar_suffix)
        java_toolchain = ctx.toolchains["@bazel_tools//tools/jdk:toolchain_type"].java
        java_info = compile(
            ctx = ctx,
            output = output_jar,
            java_toolchain = java_toolchain,
            source_jars = [source_jar],
            deps = deps,
            exports = exports,
            output_source_jar = source_jar,
            injecting_rule_kind = injecting_rule_kind,
            javac_opts = java_toolchain._compatible_javacopts.get("proto", depset()),
            enable_jspecify = False,
            include_compilation_info = False,
        )
        jars = [source_jar, output_jar]
    else:
        # If there are no proto sources just pass along the compilation dependencies.
        java_info = merge(deps + exports, merge_java_outputs = False, merge_source_jars = False)
        jars = []
    return java_info, jars
