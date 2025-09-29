# Protocol Buffers - Google's data interchange format
# Copyright 2025 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Tests for `proto_library`."""

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/common:proto_info.bzl", "ProtoInfo")

_STRIP_IMPORT_PREFIX = "/bazel"

def bazel_proto_library_test_suite(name):
    tests = [
        _test_creates_descriptor_sets,
        _test_descriptor_sets_rule_with_srcs_calls_protoc,
        _test_descriptor_sets_rule_without_srcs_writes_empty_file,
        _test_descriptor_sets_are_exposed_in_provider,
        _test_descriptor_set_output_strict_deps,
        _test_descriptor_set_output_strict_deps_multiple_srcs,
        _test_descriptor_set_output_strict_deps_strict,
        _test_descriptor_set_output_strict_deps_disabled,
        _test_strip_import_prefix_without_deps,
        _test_strict_public_imports_enabled,
        _test_strict_public_imports_disabled,
        _test_strict_public_imports_transitive_exports,
        _test_strip_import_prefix_with_deps,
        _test_exported_stripped_import_prefixes,
        _test_illegal_strip_import_prefix,
        _test_relative_strip_import_prefix,
        _test_absolute_strip_import_prefix,
        _test_absolute_strip_import_prefix_with_slash,
        _test_dot_in_strip_import_prefix,
        _test_dot_dot_in_strip_import_prefix,
        _test_strip_import_prefix_with_strict_proto_deps,
        _test_dep_on_strip_import_prefix_with_strict_proto_deps,
        _test_no_experimental_proto_descriptor_sets_include_source_info,
        _test_source_and_generated_proto_files,
        _test_proto_library,
        _test_proto_library_without_sources,
        _test_proto_library_with_generated_sources,
        _test_proto_library_with_mixed_sources,
    ]

    # Flipping experimental flag in test requires Bazel 8
    if bazel_features.rules.analysis_tests_can_transition_on_experimental_incompatible_flags:
        tests.append(_test_experimental_proto_descriptor_sets_include_source_info)

    test_suite(
        name = name,
        tests = tests,
    )

def _test_creates_descriptor_sets(name):
    util.helper_target(proto_library, name = name + "_alias", deps = [name + "_foo"])
    util.helper_target(proto_library, name = name + "_foo", srcs = ["foo.proto"])
    util.helper_target(proto_library, name = name + "_alias_to_no_srcs", deps = [name + "_no_srcs"])
    util.helper_target(proto_library, name = name + "_no_srcs")

    analysis_test(
        name = name,
        target = [
            name + "_alias",
            name + "_foo",
            name + "_alias_to_no_srcs",
            name + "_no_srcs",
        ],
        impl = _test_creates_descriptor_sets_impl,
    )

def _test_creates_descriptor_sets_impl(env, targets):
    for target in targets:
        env.expect.that_target(target).default_outputs().contains_exactly([
            "{package}/{name}-descriptor-set.proto.bin",
        ])

def _test_descriptor_sets_rule_with_srcs_calls_protoc(name):
    util.helper_target(proto_library, name = name + "_foo", srcs = ["foo.proto"])

    analysis_test(
        name = name,
        target = name + "_foo",
        impl = _test_descriptor_sets_rule_with_srcs_calls_protoc_impl,
    )

def _test_descriptor_sets_rule_with_srcs_calls_protoc_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains_at_least([
        "-I.",
        "--descriptor_set_out=" + action.actual.outputs.to_list()[0].path,
        "{package}/foo.proto",
    ])

# Asserts that we register a FileWriteAction with empty contents if there are no srcs.
def _test_descriptor_sets_rule_without_srcs_writes_empty_file(name):
    util.helper_target(proto_library, name = name + "_no_srcs")

    analysis_test(
        name = name,
        target = name + "_no_srcs",
        impl = _test_descriptor_sets_rule_without_srcs_writes_empty_file_impl,
    )

def _test_descriptor_sets_rule_without_srcs_writes_empty_file_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.content().equals("")

def _test_descriptor_sets_are_exposed_in_provider(name):
    util.helper_target(proto_library, name = name + "_alias", deps = [name + "_foo"])
    util.helper_target(proto_library, name = name + "_foo", srcs = ["foo.proto"], deps = [name + "_bar"])
    util.helper_target(proto_library, name = name + "_bar", srcs = ["bar.proto"])
    util.helper_target(proto_library, name = name + "_alias_to_no_srcs", deps = [name + "_no_srcs"])
    util.helper_target(proto_library, name = name + "_no_srcs")

    analysis_test(
        name = name,
        target = [
            name + "_alias",
            name + "_foo",
            name + "_bar",
            name + "_alias_to_no_srcs",
            name + "_no_srcs",
        ],
        impl = _test_descriptor_sets_are_exposed_in_provider_impl,
    )

def _test_descriptor_sets_are_exposed_in_provider_impl(env, targets):
    target = targets[0]  # name + "_alias"
    package = target.label.package
    name = target.label.name[:-len("_alias")]

    provider = target[ProtoInfo]
    env.expect.that_file(provider.direct_descriptor_set).basename().equals(name + "_alias-descriptor-set.proto.bin")
    env.expect.that_depset_of_files(provider.transitive_descriptor_sets).contains_exactly([
        package + "/" + name + "_alias-descriptor-set.proto.bin",
        package + "/" + name + "_foo-descriptor-set.proto.bin",
        package + "/" + name + "_bar" + "-descriptor-set.proto.bin",
    ])

    target = targets[1]  # name + "_foo"
    provider = target[ProtoInfo]
    env.expect.that_file(provider.direct_descriptor_set).basename().equals(name + "_foo-descriptor-set.proto.bin")
    env.expect.that_depset_of_files(provider.transitive_descriptor_sets).contains_exactly([
        package + "/" + name + "_foo-descriptor-set.proto.bin",
        package + "/" + name + "_bar-descriptor-set.proto.bin",
    ])

    target = targets[2]  # name + "_bar"
    provider = target[ProtoInfo]
    env.expect.that_file(provider.direct_descriptor_set).basename().equals(name + "_bar-descriptor-set.proto.bin")
    env.expect.that_depset_of_files(provider.transitive_descriptor_sets).contains_exactly([
        package + "/" + name + "_bar-descriptor-set.proto.bin",
    ])

    target = targets[3]  # name + "_alias_to_no_srcs"
    provider = target[ProtoInfo]
    env.expect.that_file(provider.direct_descriptor_set).basename().equals(name + "_alias_to_no_srcs-descriptor-set.proto.bin")
    env.expect.that_depset_of_files(provider.transitive_descriptor_sets).contains_exactly([
        package + "/" + name + "_alias_to_no_srcs-descriptor-set.proto.bin",
        package + "/" + name + "_no_srcs-descriptor-set.proto.bin",
    ])

    target = targets[4]  # name + "_no_srcs"
    provider = target[ProtoInfo]
    env.expect.that_file(provider.direct_descriptor_set).basename().equals(name + "_no_srcs-descriptor-set.proto.bin")
    env.expect.that_depset_of_files(provider.transitive_descriptor_sets).contains_exactly([
        package + "/" + name + "_no_srcs-descriptor-set.proto.bin",
    ])

def _test_descriptor_set_output_strict_deps(name):
    util.helper_target(proto_library, name = name + "_nodeps", srcs = ["nodeps.proto"])
    util.helper_target(
        proto_library,
        name = name + "_withdeps",
        srcs = ["withdeps.proto"],
        deps = [
            name + "_dep1",
            name + "_dep2",
        ],
    )
    util.helper_target(
        proto_library,
        name = name + "_depends_on_alias",
        srcs = ["depends_on_alias.proto"],
        deps = [name + "_alias"],
    )
    util.helper_target(proto_library, name = name + "_alias", deps = [
        name + "_dep1",
        name + "_dep2",
    ])
    util.helper_target(proto_library, name = name + "_dep1", srcs = ["dep1.proto"])
    util.helper_target(proto_library, name = name + "_dep2", srcs = ["dep2.proto"])

    analysis_test(
        name = name,
        target = [
            name + "_nodeps",
            name + "_withdeps",
            name + "_depends_on_alias",
        ],
        impl = _test_descriptor_set_output_strict_deps_impl,
        config_settings = {"//command_line_option:strict_proto_deps": "error"},
    )

def _test_descriptor_set_output_strict_deps_impl(env, targets):
    target = targets[0]
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains_at_least([
        "--direct_dependencies",
        "{package}/nodeps.proto",
    ])
    action.argv().contains_predicate(matching.str_matches("--direct_dependencies_violation_msg=* //*:*_nodeps"))

    target = targets[1]
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains_at_least([
        "--direct_dependencies",
        "{package}/dep1.proto:{package}/dep2.proto:{package}/withdeps.proto",
    ])

    target = targets[2]
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains_at_least([
        "--direct_dependencies",
        "{package}/dep1.proto:{package}/dep2.proto:{package}/depends_on_alias.proto",
    ])

# When building a proto_library with multiple srcs (say foo.proto and bar.proto), we should allow
# foo.proto to import bar.proto without tripping strict-deps checking. This means that
# --direct_dependencies should list the srcs.
def _test_descriptor_set_output_strict_deps_multiple_srcs(name):
    util.helper_target(proto_library, name = name + "_foo", srcs = ["foo.proto", "bar.proto"])

    analysis_test(
        name = name,
        target = name + "_foo",
        impl = _test_descriptor_set_output_strict_deps_multiple_srcs_impl,
        config_settings = {"//command_line_option:strict_proto_deps": "error"},
    )

def _test_descriptor_set_output_strict_deps_multiple_srcs_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains_at_least([
        "--direct_dependencies",
        "{package}/foo.proto:{package}/bar.proto",
    ])
    action.argv().contains_predicate(matching.str_matches("--direct_dependencies_violation_msg=*"))

def _test_descriptor_set_output_strict_deps_strict(name):
    util.helper_target(proto_library, name = name + "_foo", srcs = ["foo.proto", "bar.proto"])

    analysis_test(
        name = name,
        target = name + "_foo",
        impl = _test_descriptor_set_output_strict_deps_strict_impl,
        config_settings = {"//command_line_option:strict_proto_deps": "strict"},
    )

def _test_descriptor_set_output_strict_deps_strict_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains_at_least([
        "--direct_dependencies",
        "{package}/foo.proto:{package}/bar.proto",
    ])
    action.argv().contains_predicate(matching.str_matches("--direct_dependencies_violation_msg=*"))

def _test_descriptor_set_output_strict_deps_disabled(name):
    util.helper_target(proto_library, name = name + "_foo", srcs = ["foo.proto"])

    analysis_test(
        name = name,
        target = name + "_foo",
        impl = _test_descriptor_set_output_strict_deps_disabled_impl,
        config_settings = {"//command_line_option:strict_proto_deps": "off"},
    )

def _test_descriptor_set_output_strict_deps_disabled_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().not_contains("--direct_dependencies")
    action.argv().not_contains_predicate(matching.str_matches("--direct_dependencies_violation_msg=*"))

def _test_strip_import_prefix_without_deps(name):
    util.helper_target(
        proto_library,
        name = name + "_nodeps",
        srcs = ["foo/nodeps.proto"],
        strip_import_prefix = _STRIP_IMPORT_PREFIX,
    )

    analysis_test(
        name = name,
        target = name + "_nodeps",
        impl = _test_strip_import_prefix_without_deps_impl,
    )

def _test_strip_import_prefix_without_deps_impl(env, target):
    provider = target[ProtoInfo]
    env.expect.that_collection(provider.transitive_proto_path).contains_exactly([
        env.ctx.bin_dir.path + "/" + target.label.package + "/_virtual_imports/" + target.label.name,
    ])

    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains("-I" + env.ctx.bin_dir.path + "/" + target.label.package + "/_virtual_imports/" + target.label.name)

def _test_strict_public_imports_enabled(name):
    util.helper_target(proto_library, name = name + "_foo", srcs = ["foo.proto"])

    analysis_test(
        name = name,
        target = name + "_foo",
        impl = _test_strict_public_imports_enabled_impl,
        config_settings = {"//command_line_option:strict_public_imports": "ERROR"},
    )

def _test_strict_public_imports_enabled_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains("--allowed_public_imports=")

def _test_strict_public_imports_disabled(name):
    util.helper_target(proto_library, name = name + "_foo", srcs = ["foo.proto"])

    analysis_test(
        name = name,
        target = name + "_foo",
        impl = _test_strict_public_imports_disabled_impl,
        config_settings = {"//command_line_option:strict_public_imports": "OFF"},
    )

def _test_strict_public_imports_disabled_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().not_contains("--allowed_public_imports=")

def _test_strict_public_imports_transitive_exports(name):
    util.helper_target(
        proto_library,
        name = name + "_top",
        srcs = ["top.proto"],
        exports = [
            name + "_exported1",
        ],
        deps = [
            name + "_exported1",
            name + "_notexported",
        ],
    )
    util.helper_target(proto_library, name = name + "_exported1", srcs = ["exported1.proto"])
    util.helper_target(proto_library, name = name + "_notexported", srcs = ["notexported.proto"])

    analysis_test(
        name = name,
        target = name + "_top",
        impl = _test_strict_public_imports_transitive_exports_impl,
        config_settings = {"//command_line_option:strict_public_imports": "ERROR"},
    )

def _test_strict_public_imports_transitive_exports_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    expected_imports = "{package}/exported1.proto"
    action.argv().contains_at_least(["--allowed_public_imports", expected_imports])

def _test_strip_import_prefix_with_deps(name):
    util.helper_target(
        proto_library,
        name = name + "_withdeps",
        srcs = ["foo/withdeps.proto"],
        strip_import_prefix = _STRIP_IMPORT_PREFIX,
        deps = [
            name + "_dep1",
            name + "_dep2",
        ],
    )
    util.helper_target(proto_library, name = name + "_dep1", srcs = ["foo/dep1.proto"])
    util.helper_target(
        proto_library,
        name = name + "_dep2",
        srcs = ["bar/dep2.proto"],
        strip_import_prefix = _STRIP_IMPORT_PREFIX,
    )

    analysis_test(
        name = name,
        target = name + "_withdeps",
        impl = _test_strip_import_prefix_with_deps_impl,
    )

def _test_strip_import_prefix_with_deps_impl(env, target):
    provider = target[ProtoInfo]
    binfiles = env.ctx.bin_dir.path
    package = target.label.package
    env.expect.that_collection(provider.transitive_proto_path).contains_exactly([
        binfiles + "/" + package + "/_virtual_imports/" + target.label.name,
        binfiles + "/" + package + "/_virtual_imports/" + target.label.name.replace("withdeps", "dep2"),
        ".",
    ])

def _test_exported_stripped_import_prefixes(name):
    util.helper_target(proto_library, name = name + "_ad", strip_import_prefix = _STRIP_IMPORT_PREFIX, srcs = ["ad.proto"])
    util.helper_target(proto_library, name = name + "_ae", strip_import_prefix = _STRIP_IMPORT_PREFIX, srcs = ["ae.proto"])
    util.helper_target(
        proto_library,
        name = name + "_a",
        strip_import_prefix = _STRIP_IMPORT_PREFIX,
        srcs = ["a.proto"],
        exports = [name + "_ae"],
        deps = [name + "_ad"],
    )

    analysis_test(
        name = name,
        target = name + "_a",
        impl = _test_exported_stripped_import_prefixes_impl,
    )

def _test_exported_stripped_import_prefixes_impl(env, target):
    provider = target[ProtoInfo]
    package = target.label.package

    # exported proto source roots should be the source root of the rule and the direct source roots
    # of its exports and nothing else (not the exports of its exports or the deps of its exports
    # or the exports of its deps)
    env.expect.that_depset_of_files(provider.check_deps_sources).contains_exactly([
        package + "/_virtual_imports/" + target.label.name + "/tests/a.proto",
    ])

def _test_illegal_strip_import_prefix(name):
    util.helper_target(
        proto_library,
        name = name + "_a",
        srcs = ["a.proto"],
        strip_import_prefix = "foo",
    )

    analysis_test(
        name = name,
        target = name + "_a",
        impl = _test_illegal_strip_import_prefix_impl,
        expect_failure = True,
    )

def _test_illegal_strip_import_prefix_impl(env, target):
    env.expect.that_target(target).failures().contains_predicate(
        matching.str_matches(".proto file '*' is not under the specified strip prefix"),
    )

def _test_relative_strip_import_prefix(name):
    util.helper_target(
        proto_library,
        name = name + "_d",
        srcs = ["c/d.proto"],
        strip_import_prefix = "c",
    )

    analysis_test(
        name = name,
        target = name + "_d",
        impl = _test_relative_strip_import_prefix_impl,
    )

def _test_relative_strip_import_prefix_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains("-I" + env.ctx.bin_dir.path + "/" + target.label.package + "/_virtual_imports/" + target.label.name)

def _test_absolute_strip_import_prefix(name):
    util.helper_target(
        proto_library,
        name = name + "_d",
        srcs = ["c/d.proto"],
        strip_import_prefix = _STRIP_IMPORT_PREFIX,
    )

    analysis_test(
        name = name,
        target = name + "_d",
        impl = _test_absolute_strip_import_prefix_impl,
    )

def _test_absolute_strip_import_prefix_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains("-I" + env.ctx.bin_dir.path + "/" + target.label.package + "/_virtual_imports/" + target.label.name)

def _test_absolute_strip_import_prefix_with_slash(name):
    util.helper_target(
        proto_library,
        name = name + "_d",
        srcs = ["c/d.proto"],
        strip_import_prefix = _STRIP_IMPORT_PREFIX + "/",
    )

    analysis_test(
        name = name,
        target = name + "_d",
        impl = _test_absolute_strip_import_prefix_with_slash_impl,
    )

def _test_absolute_strip_import_prefix_with_slash_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains("-I" + env.ctx.bin_dir.path + "/" + target.label.package + "/_virtual_imports/" + target.label.name)

def _test_dot_in_strip_import_prefix(name):
    util.helper_target(
        proto_library,
        name = name + "_d",
        srcs = ["c/d.proto"],
        strip_import_prefix = "./c",
    )

    analysis_test(
        name = name,
        target = name + "_d",
        impl = _test_dot_in_strip_import_prefix_impl,
        expect_failure = True,
    )

def _test_dot_in_strip_import_prefix_impl(env, target):
    env.expect.that_target(target).failures().contains_predicate(
        matching.str_matches("should be normalized"),
    )

def _test_dot_dot_in_strip_import_prefix(name):
    util.helper_target(
        proto_library,
        name = name + "_d",
        srcs = ["c/d.proto"],
        strip_import_prefix = "../b/c",
    )

    analysis_test(
        name = name,
        target = name + "_d",
        impl = _test_dot_dot_in_strip_import_prefix_impl,
        expect_failure = True,
    )

def _test_dot_dot_in_strip_import_prefix_impl(env, target):
    env.expect.that_target(target).failures().contains_predicate(
        matching.str_matches("should be normalized"),
    )

def _test_strip_import_prefix_with_strict_proto_deps(name):
    util.helper_target(
        proto_library,
        name = name + "_d",
        srcs = ["c/d.proto", "c/e.proto"],
        strip_import_prefix = "c",
    )

    analysis_test(
        name = name,
        target = name + "_d",
        impl = _test_strip_import_prefix_with_strict_proto_deps_impl,
        config_settings = {"//command_line_option:strict_proto_deps": "STRICT"},
    )

def _test_strip_import_prefix_with_strict_proto_deps_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains_at_least(["--direct_dependencies", "d.proto:e.proto"])

def _test_dep_on_strip_import_prefix_with_strict_proto_deps(name):
    util.helper_target(
        proto_library,
        name = name + "_d",
        srcs = ["c/d.proto"],
        strip_import_prefix = "c",
    )
    util.helper_target(
        proto_library,
        name = name + "_e",
        srcs = ["e.proto"],
        deps = [name + "_d"],
    )

    analysis_test(
        name = name,
        target = name + "_e",
        impl = _test_dep_on_strip_import_prefix_with_strict_proto_deps_impl,
        config_settings = {"//command_line_option:strict_proto_deps": "STRICT"},
    )

def _test_dep_on_strip_import_prefix_with_strict_proto_deps_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains_at_least(["--direct_dependencies", "d.proto:{package}/e.proto"])

def _test_no_experimental_proto_descriptor_sets_include_source_info(name):
    util.helper_target(
        proto_library,
        name = name + "_a_proto",
        srcs = ["a.proto"],
    )

    analysis_test(
        name = name,
        target = name + "_a_proto",
        impl = _test_no_experimental_proto_descriptor_sets_include_source_info_impl,
    )

def _test_no_experimental_proto_descriptor_sets_include_source_info_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().not_contains("--include_source_info")

def _test_experimental_proto_descriptor_sets_include_source_info(name):
    util.helper_target(
        proto_library,
        name = name + "_a_proto",
        srcs = ["a.proto"],
    )

    analysis_test(
        name = name,
        target = name + "_a_proto",
        impl = _test_experimental_proto_descriptor_sets_include_source_info_impl,
        config_settings = {"//command_line_option:experimental_proto_descriptor_sets_include_source_info": "true"},
    )

def _test_experimental_proto_descriptor_sets_include_source_info_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains("--include_source_info")

def _test_source_and_generated_proto_files(name):
    util.helper_target(
        native.genrule,
        name = name + "_g",
        outs = ["g.proto"],
        cmd = "",
    )
    util.helper_target(
        proto_library,
        name = name + "_p",
        srcs = ["s.proto", ":g.proto"],
    )

    analysis_test(
        name = name,
        target = name + "_p",
        impl = _test_source_and_generated_proto_files_impl,
    )

def _test_source_and_generated_proto_files_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{name}-descriptor-set.proto.bin",
    )
    action.argv().contains_at_least(["-I.", "-I" + env.ctx.genfiles_dir.path])

def _test_proto_library(name):
    util.helper_target(
        proto_library,
        name = name + "_foo",
        srcs = ["a.proto", "b.proto", "c.proto"],
    )

    analysis_test(
        name = name,
        target = name + "_foo",
        impl = _test_proto_library_impl,
    )

def _test_proto_library_impl(env, target):
    provider = target[ProtoInfo]
    env.expect.that_depset_of_files(provider.direct_sources).contains_exactly([
        target.label.package + "/a.proto",
        target.label.package + "/b.proto",
        target.label.package + "/c.proto",
    ])

def _test_proto_library_without_sources(name):
    util.helper_target(
        proto_library,
        name = name + "_foo",
    )

    analysis_test(
        name = name,
        target = name + "_foo",
        impl = _test_proto_library_without_sources_impl,
    )

def _test_proto_library_without_sources_impl(env, target):
    provider = target[ProtoInfo]
    env.expect.that_depset_of_files(provider.direct_sources).contains_exactly([])

def _test_proto_library_with_generated_sources(name):
    util.helper_target(
        native.genrule,
        name = name + "_g",
        outs = ["generated.proto"],
        cmd = "",
    )
    util.helper_target(
        proto_library,
        name = name + "_foo",
        srcs = [":generated.proto"],
    )

    analysis_test(
        name = name,
        target = name + "_foo",
        impl = _test_proto_library_with_generated_sources_impl,
    )

def _test_proto_library_with_generated_sources_impl(env, target):
    provider = target[ProtoInfo]
    env.expect.that_depset_of_files(provider.direct_sources).contains_exactly([
        target.label.package + "/generated.proto",
    ])

def _test_proto_library_with_mixed_sources(name):
    util.helper_target(
        native.genrule,
        name = name + "_g",
        outs = ["generated2.proto"],
        cmd = "",
    )
    util.helper_target(
        proto_library,
        name = name + "_foo",
        srcs = [
            "a.proto",
            ":generated2.proto",
        ],
    )

    analysis_test(
        name = name,
        target = name + "_foo",
        impl = _test_proto_library_with_mixed_sources_impl,
    )

def _test_proto_library_with_mixed_sources_impl(env, target):
    provider = target[ProtoInfo]
    env.expect.that_depset_of_files(provider.direct_sources).contains_exactly([
        target.label.package + "/a.proto",
        target.label.package + "/generated2.proto",
    ])
