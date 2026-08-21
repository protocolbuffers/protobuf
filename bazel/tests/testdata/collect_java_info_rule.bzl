"""An rule collecting JavaInfo from java_proto_library's aspect"""

load("@rules_java//java/common:java_common.bzl", "java_common")
load("@rules_java//java/common:java_info.bzl", "JavaInfo")

_Info = provider("Collects JavaInfos", fields = ["aspect_java_infos"])

def _collect_java_info_aspect_impl(target, ctx):
    aspect_java_infos = []
    for dep in ctx.rule.attr.deps:
        aspect_java_infos += dep[_Info].aspect_java_infos
    aspect_java_infos.append(target[JavaInfo])
    return _Info(aspect_java_infos = aspect_java_infos)

_collect_java_info_aspect = aspect(
    attr_aspects = ["deps"],
    fragments = ["java"],
    implementation = _collect_java_info_aspect_impl,
    required_aspect_providers = [[JavaInfo]],
)

def _collect_java_info_impl(ctx):
    aspect_java_infos = []
    for dep in ctx.attr.deps:
        aspect_java_infos += dep[_Info].aspect_java_infos
    merged_java_info = java_common.merge(aspect_java_infos)
    return [merged_java_info]

collect_java_info_rule = rule(
    implementation = _collect_java_info_impl,
    attrs = {
        "deps": attr.label_list(aspects = [_collect_java_info_aspect]),
    },
)
