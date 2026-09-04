# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Implementation of the proto_plugin rule."""

ProtoPluginInfo = provider(
    doc = "Describes an additional protoc plugin run by proto_lang_toolchain.",
    fields = {
        "name": "(str) The plugin name, passed to the proto compiler as " +
                "--plugin=protoc-gen-<name> and --<name>_out. Set by the rule's " +
                "`plugin_name` attribute, defaulting to the target's name.",
        "plugin": "(FilesToRunProvider|None) The plugin executable, or None if the " +
                  "plugin is built into the proto compiler itself.",
    },
)

def _proto_plugin_impl(ctx):
    return [
        ProtoPluginInfo(
            name = ctx.attr.plugin_name or ctx.label.name,
            plugin = ctx.attr.runtime[DefaultInfo].files_to_run if ctx.attr.runtime else None,
        ),
    ]

proto_plugin = rule(
    implementation = _proto_plugin_impl,
    doc = """
Describes a protoc plugin that can be passed to the <code>plugins</code> attribute of
<code>proto_lang_toolchain</code>. For each plugin the proto compiler is invoked with
<code>--plugin=protoc-gen-&lt;name&gt;=&lt;runtime&gt;</code> and <code>--&lt;name&gt;_out</code>
pointing at the same output location as the <code>$(OUT)</code> in the toolchain's
<code>command_line</code>, so plugins can add code to the primary generator's output via
insertion points.

<p><code>&lt;name&gt;</code> is the <code>plugin_name</code> attribute, or the target's name
when that attribute is not set.""",
    attrs = {
        "plugin_name": attr.string(
            doc = """
The name protoc knows the plugin by, used for both
<code>--plugin=protoc-gen-&lt;plugin_name&gt;</code> and <code>--&lt;plugin_name&gt;_out</code>.
protoc requires the two to match, so this single name drives both flags.
Defaults to the target's name.""",
        ),
        "runtime": attr.label(
            doc = "The plugin binary. If absent, assume the plugin is a built-in to protoc itself.",
            cfg = "exec",
            executable = True,
        ),
    },
    provides = [ProtoPluginInfo],
)
