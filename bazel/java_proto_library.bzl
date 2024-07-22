# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""java_proto_library rule"""

load("//bazel/private:bazel_java_proto_library_rule.bzl", "bazel_java_proto_aspect", "bazel_java_proto_library_rule")  # buildifier: disable=bzl-visibility
load("//bazel/private:toolchain_helpers.bzl", "toolchains")  # buildifier: disable=bzl-visibility

# TODO: replace with toolchain type located in protobuf
_JAVA_PROTO_TOOLCHAIN = "@rules_java//java/proto:toolchain_type"

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
        "distribs": attr.string_list(),
    } | toolchains.if_legacy_toolchain({
        "_aspect_java_proto_toolchain": attr.label(
            default = configuration_field(fragment = "proto", name = "proto_toolchain_for_java"),
        ),
    }),  # buildifier: disable=attr-licenses (attribute called licenses)
    provides = [JavaInfo],
    toolchains = toolchains.use_toolchain(_JAVA_PROTO_TOOLCHAIN),
)
