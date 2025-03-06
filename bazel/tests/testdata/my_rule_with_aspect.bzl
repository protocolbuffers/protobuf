"""An rule collecting JavaInfo from java_proto_library's aspect"""

load("@rules_java//java/common:java_common.bzl", "java_common")
load("@rules_java//java/common:java_info.bzl", "JavaInfo")

_Info = provider("Collects JavaInfos", fields = ["aspect_java_infos"])

def _my_rule_impl(ctx):
    aspect_java_infos = []
    for dep in ctx.attr.deps:
        aspect_java_infos += dep[_Info].aspect_java_infos
    merged_java_info = java_common.merge(aspect_java_infos)
    return [merged_java_info]

def _my_aspect_impl(target, ctx):
    aspect_java_infos = []
    for dep in ctx.rule.attr.deps:
        aspect_java_infos += dep[_Info].aspect_java_infos
    aspect_java_infos.append(target[JavaInfo])
    return _Info(aspect_java_infos = aspect_java_infos)

my_aspect = aspect(
    attr_aspects = ["deps"],
    fragments = ["java"],
    implementation = _my_aspect_impl,
    required_aspect_providers = [[JavaInfo]],
)
my_rule_with_aspect = rule(
    implementation = _my_rule_impl,
    attrs = {
        "deps": attr.label_list(aspects = [my_aspect]),
    },
)
