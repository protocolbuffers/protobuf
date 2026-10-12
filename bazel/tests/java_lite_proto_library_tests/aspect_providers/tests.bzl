"""Tests that java_lite_proto_library aspect exposes a JavaInfo provider with the correct output jars."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

JarInfo = provider(
    doc = "Test provider collecting JavaInfo output jars via aspect.",
    fields = ["jars"],
)

def _foo_aspect_impl(target, ctx):
    jars = depset(
        target[JavaInfo].outputs.jars,
        transitive = [dep[JarInfo].jars for dep in ctx.rule.attr.deps],
    )
    return [JarInfo(jars = jars)]

foo_aspect = aspect(
    implementation = _foo_aspect_impl,
    attr_aspects = ["deps"],
    required_aspect_providers = [JavaInfo],
)

def _foo_rule_impl(ctx):
    return [JarInfo(jars = ctx.attr.dep[JarInfo].jars)]

foo_rule = rule(
    implementation = _foo_rule_impl,
    attrs = {"dep": attr.label(aspects = [foo_aspect])},
)

def _test_java_lite_proto_library_aspect_providers(env, target):
    jars = target[JarInfo].jars.to_list()
    env.expect.that_collection(jars).has_size(1)
    java_output = jars[0]
    env.expect.that_str(java_output.class_jar.basename).equals("libfoo_proto-lite.jar")
    env.expect.that_bool(java_output.compile_jar != None).equals(True)
    env.expect.that_collection(java_output.source_jars.to_list()).has_size(1)
    env.expect.that_bool(java_output.compile_jdeps != None).equals(True)

TESTS = {
    ":foo_rule": [_test_java_lite_proto_library_aspect_providers],
}
