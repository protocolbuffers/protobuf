"""This module contains unit tests for rust_proto_library and its aspect."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("//bazel:proto_library.bzl", "proto_library")
load("//rust:defs.bzl", "rust_cc_proto_library", "rust_upb_proto_library")
load(
    "//rust/bazel:aspects.bzl",
    "RustProtoInfo",
    "rust_cc_proto_library_aspect",
)
load(":defs.bzl", "ActionsInfo", "attach_cc_aspect", "attach_upb_aspect")

def _find_actions_with_mnemonic(actions, mnemonic):
    actions = [a for a in actions if a.mnemonic == mnemonic]
    if not actions:
        fail("Couldn't find action with mnemonic {} among {}".format(
            mnemonic,
            [a.mnemonic for a in actions],
        ))
    return actions

CHECK_CRATE_MAPPING_EXPECTED_CONTENT = """_ao__ao__y__y_rust_y_test_y_rust_proto_library_unit_test_x_grand_d_parent_proto
2
rust/test/rust_proto_library_unit_test/grandparent1.proto
rust/test/rust_proto_library_unit_test/grandparent2.proto
_ao__ao__y__y_rust_y_test_y_rust_proto_library_unit_test_x_parent_proto
1
rust/test/rust_proto_library_unit_test/parent.proto
_ao__ao__y__y_rust_y_test_y_rust_proto_library_unit_test_x_parent2_proto
1
rust/test/rust_proto_library_unit_test/parent2.proto
"""

def _check_crate_mapping(actions, target_name):
    fw_actions = _find_actions_with_mnemonic(actions, "FileWrite")
    crate_mapping_action = None
    for a in fw_actions:
        outputs = a.outputs.to_list()
        output = [o for o in outputs if o.basename == target_name + ".rust_crate_mapping"]
        if output:
            crate_mapping_action = a
    if not crate_mapping_action:
        fail("Couldn't find action outputting {}.rust_crate_mapping among {}".format(
            target_name,
            fw_actions,
        ))

    if crate_mapping_action.content != CHECK_CRATE_MAPPING_EXPECTED_CONTENT:
        fail("The crate mapping file content didn't match. Was: {}".format(
            crate_mapping_action.content,
        ))

    protoc_action = [
        a
        for a in actions
        for i in a.inputs.to_list()
        if "rust_crate_mapping" in i.basename
    ]
    if not protoc_action:
        fail("Couldn't find action with the rust_crate_mapping as input")
    if protoc_action[0].mnemonic != "GenProto":
        fail(
            "Action that had rust_crate_mapping as input wasn't a GenProto action, but {}",
            protoc_action[0].mnemonic,
        )

def _find_rust_lib_input(inputs, target_name):
    inputs = inputs.to_list()
    input = [i for i in inputs if i.basename.startswith("lib" + target_name) and
                                  (i.basename.endswith(".rlib") or i.basename.endswith(".rmeta"))]
    if not input:
        fail("Couldn't find lib{}-<hash>.rlib or lib{}-<hash>.rmeta among {}".format(
            target_name,
            target_name,
            [i.basename for i in inputs],
        ))
    return input[0]

def _relevant_linker_inputs(ltl):
    return ltl.objects + ltl.pic_objects + (
        [ltl.static_library] if ltl.static_library else []
    ) + (
        [ltl.pic_static_library] if ltl.pic_static_library else []
    )

def _find_linker_input(rust_proto_info, basename_substring):
    cc_info = rust_proto_info.dep_variant_infos[0].cc_info
    for linker_input in cc_info.linking_context.linker_inputs.to_list():
        for ltl in linker_input.libraries:
            for file in _relevant_linker_inputs(ltl):
                if basename_substring in file.basename:
                    return file

    fail("Couldn't find file with suffix {} in {}".format(
        basename_substring,
        [
            f.basename
            for input in cc_info.linking_context.linker_inputs.to_list()
            for ltl in input.libraries
            for f in _relevant_linker_inputs(ltl)
        ],
    ))

####################################################################################################

def _rust_upb_aspect_test_impl(ctx):
    env = analysistest.begin(ctx)
    target_under_test = analysistest.target_under_test(env)
    actions = target_under_test[ActionsInfo].actions
    rustc_action = _find_actions_with_mnemonic(actions, "Rustc")[0]

    # The protoc action needs to be given the crate mapping file
    _check_crate_mapping(actions, "child_proto")

    # The action needs to have the Rust runtime as an input
    _find_rust_lib_input(rustc_action.inputs, "protobuf")

    # The action needs to have rlibs for direct deps
    _find_rust_lib_input(rustc_action.inputs, "parent")
    _find_rust_lib_input(rustc_action.inputs, "parent2")

    # The action needs to produce a .rlib artifact (sometimes .rmeta as well, not tested here).
    asserts.true(env, rustc_action.outputs.to_list()[0].path.endswith(".rlib"))

    return analysistest.end(env)

rust_upb_aspect_test = analysistest.make(_rust_upb_aspect_test_impl)

def _test_upb_aspect():
    attach_upb_aspect(name = "child_proto_with_upb_aspect", dep = ":child_proto")

    rust_upb_aspect_test(
        name = "rust_upb_aspect_test",
        target_under_test = ":child_proto_with_upb_aspect",
    )

####################################################################################################
def _rust_cc_aspect_test_impl(ctx):
    env = analysistest.begin(ctx)
    target_under_test = analysistest.target_under_test(env)
    actions = target_under_test[ActionsInfo].actions
    rustc_action = _find_actions_with_mnemonic(actions, "Rustc")[0]

    _check_crate_mapping(actions, "child_proto")

    # The rustc action needs to have the Rust runtime as an input
    _find_rust_lib_input(rustc_action.inputs, "protobuf")

    # The action needs to have rlibs for direct deps
    _find_rust_lib_input(rustc_action.inputs, "parent")
    _find_rust_lib_input(rustc_action.inputs, "parent2")

    # The action needs to produce a .rlib artifact (sometimes .rmeta as well, not tested here).
    asserts.true(env, rustc_action.outputs.to_list()[0].path.endswith(".rlib"))

    # The aspect needs to provide CcInfo that passes UPB gencode to the eventual linking.
    _find_linker_input(target_under_test[RustProtoInfo], "child.pb")
    _find_linker_input(target_under_test[RustProtoInfo], "parent.pb")

    return analysistest.end(env)

rust_cc_aspect_test = analysistest.make(_rust_cc_aspect_test_impl)

def _test_cc_aspect():
    attach_cc_aspect(name = "child_proto_with_cc_aspect", dep = ":child_proto")

    rust_cc_aspect_test(
        name = "rust_cc_aspect_test",
        target_under_test = ":child_proto_with_cc_aspect",
    )

####################################################################################################

def _rust_outputs_test_impl(ctx):
    env = analysistest.begin(ctx)
    target_under_test = analysistest.target_under_test(env)

    label_to_files = {
        "child_cpp_rust_proto": ["generated.c.rs", "child.c.pb.rs"],
        "child_upb_rust_proto": ["generated.u.rs", "child.u.pb.rs"],
    }
    expected_outputs = label_to_files[target_under_test.label.name]
    actual_outputs = target_under_test.files.to_list()
    asserts.equals(env, len(expected_outputs), len(actual_outputs))
    for expected in expected_outputs:
        found = False
        for actual in actual_outputs:
            if actual.path.endswith(expected):
                found = True
                break
        asserts.true(env, found)

    return analysistest.end(env)

rust_outputs_test = analysistest.make(_rust_outputs_test_impl)

def _test_cc_outputs():
    rust_cc_proto_library(
        name = "child_cpp_rust_proto",
        deps = [":child_proto"],
    )

    rust_outputs_test(
        name = "rust_cc_outputs_test",
        target_under_test = ":child_cpp_rust_proto",
    )

def _test_upb_outputs():
    rust_upb_proto_library(
        name = "child_upb_rust_proto",
        deps = [":child_proto"],
    )

    rust_outputs_test(
        name = "rust_upb_outputs_test",
        target_under_test = ":child_upb_rust_proto",
    )

# Crate naming testing
#
# The crate_names_aspect collects the labels of RustProtoInfo.dep_variant_infos
# and RustProtoInfo.exports_dep_variant_infos for each transitive dependency of
# a target under test in a CrateNamesAspectInfo.
#
#
CrateNameInfo = provider(
    """Collects the labels of direct and exported dep_variants of a single rust_proto_library target.""",
    fields = {
        "dep_variants": "List(string): labels of crates that this rust_proto crate directly provides to client rust_proto libraries.",
        "exports_dep_variants": "List(string): labels of rust_proto crates exported by this rust_proto crate.",
    },
)

CrateNamesAspectInfo = provider(
    """Collects the CrateNameInfo-s for a rust_proto_library target and its transitive dependencies.""",
    fields = {
        "infos": "Dict(string, CrateNameInfo): dictionary from labels of transitive dependencies to their CrateNameInfo.",
    },
)

def _crate_names_aspect_impl(target, ctx):
    infos = {}

    rust_proto_info = target[RustProtoInfo] if RustProtoInfo in target else None
    dep_variant_infos = rust_proto_info.dep_variant_infos if rust_proto_info else []
    dep_variants = [str(info.crate_info.owner) for info in dep_variant_infos if info.crate_info]
    exports_dep_variant_infos = rust_proto_info.exports_dep_variant_infos if rust_proto_info else []
    exports_dep_variants = [str(info.crate_info.owner) for info in exports_dep_variant_infos if info.crate_info]
    infos[str(target.label)] = CrateNameInfo(
        dep_variants = dep_variants,
        exports_dep_variants = exports_dep_variants,
    )

    deps = getattr(ctx.rule.attr, "deps", [])
    for dep in deps:
        if CrateNamesAspectInfo in dep:
            infos |= dep[CrateNamesAspectInfo].infos

    return [CrateNamesAspectInfo(infos = infos)]

crate_names_aspect = aspect(
    implementation = _crate_names_aspect_impl,
    attr_aspects = ["deps", "dep"],
    required_aspect_providers = [RustProtoInfo],
    requires = [rust_cc_proto_library_aspect],
)

def _attach_crate_names_aspect_impl(ctx):
    return [ctx.attr.dep[CrateNamesAspectInfo]]

attach_crate_names_aspect = rule(
    implementation = _attach_crate_names_aspect_impl,
    attrs = {
        "dep": attr.label(aspects = [crate_names_aspect]),
    },
)

def _crate_names_test_impl(ctx):
    env = analysistest.begin(ctx)
    target_under_test = analysistest.target_under_test(env)

    infos = target_under_test[CrateNamesAspectInfo].infos

    def pattern_matches_label(pattern, label):
        return label.endswith(pattern)

    def match(env, kind, patterns, labels):
        context = "while matching %s" % kind
        asserts.equals(env, len(patterns), len(labels), "%s: labels %s and patterns %s must have the same size" % (context, labels, patterns))
        matching_labels = {p: [] for p in patterns}
        for p in patterns:
            for la in labels:
                if pattern_matches_label(p, la):
                    matching_labels[p].append(la)
        for p in patterns:
            if not matching_labels[p]:
                fail("%s: pattern %s did not match any of the labels %s" % (context, p, labels))
            if len(matching_labels[p]) != 1:
                fail("%s: pattern %s matched multiple labels %s" % (context, p, matching_labels[p]))

    for p, expectations in json.decode(ctx.attr.expectations).items():
        matches_any = False
        for la, info in infos.items():
            if not pattern_matches_label(p, la):
                continue
            if not expectations:
                fail("no expectations recorded for pattern %s" % p)
            matches_any = True
            kinds = ["dep_variants", "exports_dep_variants"]
            for key in expectations.keys():
                if key not in kinds:
                    fail("expectation kind %s is unsupported; supported kinds: %s" % (key, kinds))

            for kind in kinds:
                if kind in expectations:
                    match(
                        env,
                        kind = kind,
                        patterns = expectations[kind],
                        labels = getattr(info, kind),
                    )

        if not matches_any:
            fail("pattern %s did not match any labels from %s" % (p, infos.keys()))

    return analysistest.end(env)

def expectations(expectations_dict):
    """
    Use it as an input to the crate_names_test.expectations attribute below.

    A pattern is a string. It matches a label if it's a suffix of the label,
    for example the pattern ":a" matches the label "//foo:a".

    A dependency key is a string, either "dep_variants", or "exports_dep_variants".

    A dependency dict is a dictionary where keys are dependency keys and
    values are lists of patterns. A dependency dict matches a target if
    its deps and exported deps patterns match exactly the deps and exported
    deps of the targets.

    An expectations dict D is a dictionary from patterns to dependency dicts.
    It matches a target if each key K matches some transitive dependency T,
    and its dependency dict D[K] matches T.

    For example, the dependency dict:

    {
      ":b": {"dep_variants": [":b"], "exports_dep_variants": []},
      ":a": {"dep_variants": [":a"], "exports_dep_variants": [":b"]},
    }


    matches a target that has:
    * a transitive dependency :b with no exports, and
    * a transitive dependency matching :a: that exports a target matching :b.

    proto_library(name = "b")
    proto_library(name = "a", deps = ["b"], exports = ["b"])

    Args:
      expectations_dict: (dict(string, dict(string, list(string)))) an expectations dict.

    Returns:
        Returns a JSON-encoded string of the expectations.
    """

    def check_pattern(context, p):
        if type(p) != "string":
            fail("%s: expected a string, found %s of type %s" % (context, p, type(p)))

    def check_list_of_patterns(context, ps):
        if type(ps) != "list":
            fail("%s: expected a list of patterns, found %s of type %s" % (context, ps, type(ps)))
        for p in ps:
            check_pattern(context, p)

    def check_dep_dict(context, d):
        if type(d) != "dict":
            fail("%s: expected a dict, found %s of type %s" % (context, d, type(d)))
        keys = ["dep_variants", "exports_dep_variants"]
        for k in d.keys():
            if k not in keys:
                fail("%s: expected a key from %s, found %s" % (context, keys, k))
        for k, la in d.items():
            check_list_of_patterns("%s: key %s" % (context, k), la)

    def check_expectations_dict(d):
        if type(d) != "dict":
            fail("expected a dict, found %s of type %s" % (d, type(d)))

        context = "while checking expectations dict %s" % (d)
        for p, dep_dict in d.items():
            check_pattern(context, p)
            check_dep_dict("%s for pattern %s" % (context, p), dep_dict)

    check_expectations_dict(expectations_dict)

    return json.encode(expectations_dict)

crate_names_test = analysistest.make(
    _crate_names_test_impl,
    attrs = {
        "expectations": attr.string(
            doc = """The result of expectations().""",
        ),
    },
)

def _test_crate_names():
    proto_library(
        name = "e",
        srcs = ["e.proto"],
    )

    proto_library(
        name = "d",
        srcs = ["d.proto"],
    )

    proto_library(
        name = "c",
        srcs = ["c.proto"],
        deps = ["d", "e"],
        exports = ["d"],
    )

    proto_library(
        name = "c2",
        srcs = [],
        deps = ["c"],
    )

    proto_library(
        name = "b",
        srcs = ["b.proto"],
        deps = ["c2"],
        exports = ["c"],
    )

    proto_library(
        name = "a",
        srcs = ["a.proto"],
        deps = ["b"],
    )

    rust_cc_proto_library(
        name = "a_rust_proto",
        deps = ["a"],
    )

    attach_crate_names_aspect(
        name = "a_rust_proto_with_crate_names",
        dep = "a_rust_proto",
    )

    crate_names_test(
        name = "a_rust_proto_crate_names_test",
        target_under_test = "a_rust_proto_with_crate_names",
        expectations = expectations({
            ":e": {"dep_variants": [":e"], "exports_dep_variants": []},
            ":d": {"dep_variants": [":d"], "exports_dep_variants": []},
            # c exports d but not e
            ":c": {"dep_variants": [":c"], "exports_dep_variants": [":d"]},
            # the srcsless c2 forwards the exports of c
            ":c2": {"dep_variants": [":c"], "exports_dep_variants": [":d"]},
            # b exports c -- reachable via the srcsless c2 and d -- exported by c.
            ":b": {"dep_variants": [":b"], "exports_dep_variants": [":c", ":d"]},
            # a has no exports.
            ":a": {"dep_variants": [":a"], "exports_dep_variants": []},
        }),
    )

    native.test_suite(
        name = "crate_names_tests",
        tests = [
            ":a_rust_proto_crate_names_test",
        ],
    )

def rust_proto_library_unit_test(name):
    """Sets up rust_proto_library_unit_test test suite.

    Args:
      name: name of the test suite"""
    proto_library(
        # Use a '-' in the target name to test that its replaced by a '_' in the crate name.
        name = "grand-parent_proto",
        srcs = ["grandparent1.proto", "grandparent2.proto"],
    )
    proto_library(
        name = "parent_proto",
        srcs = ["parent.proto"],
        deps = [":grand-parent_proto"],
    )
    proto_library(name = "parent2_proto", srcs = ["parent2.proto"])
    proto_library(
        name = "child_proto",
        srcs = ["child.proto"],
        deps = [":parent_proto", ":parent2_proto"],
    )

    _test_upb_aspect()
    _test_cc_aspect()
    _test_cc_outputs()
    _test_upb_outputs()
    _test_crate_names()

    native.test_suite(
        name = name,
        tests = [
            ":rust_upb_aspect_test",
            ":rust_cc_aspect_test",
            ":rust_cc_outputs_test",
            ":rust_upb_outputs_test",
            ":crate_names_tests",
        ],
    )
