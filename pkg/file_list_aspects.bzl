# Aspect to collect srcs from rules.
#
# Most rule types have providers for outputs (e.g., `cc_library` ->
# CcInfo), but not for inputs. There are cases where we want access to the
# sources, such as generating a list of sources for other build systems.
#
# This file contains Starlark aspects which will collect sources from rules
# and provide them through providers:
#
#   * cc_file_list_aspect (provides CcFileList)
#   * proto_file_list_aspect (provides ProtoFileList)
#
# These are intended to work on direct targets, not transitive targets. (As
# an exception, `cc_file_list_aspect` will expand `test_suite` rules
# recursively.)

def _flatten_target_files(targets):
    return depset(transitive = [target.files for target in targets])

################################################################################
# C++ source file/header support
################################################################################

CcFileList = provider(
    doc = "List of files to be built into a library.",
    fields = {
        # As a rule of thumb, `hdrs` and `textual_hdrs` are the files that
        # would be installed along with a prebuilt library.
        "hdrs": "public header files, including those used by generated code",
        "textual_hdrs": "files which are included but are not self-contained",

        # The `internal_hdrs` are header files which appear in `srcs`.
        # These are only used when compiling the library.
        "internal_hdrs": "internal header files (only used to build .cc files)",
        "srcs": "source files",
    },
)

def _extract_cc_file_list_from_cc_rule(rule_attr):
    # CcInfo is a proxy for what we expect this rule to look like.
    # However, some deps may expose `CcInfo` without having `srcs`,
    # `hdrs`, etc., so we use `getattr` to handle that gracefully.

    internal_hdrs = []
    srcs = []

    # Filter `srcs` so it only contains source files. Headers will go
    # into `internal_headers`.
    for src in _flatten_target_files(getattr(rule_attr, "srcs", [])).to_list():
        if src.extension.lower() in ["c", "cc", "cpp", "cxx"]:
            srcs.append(src)
        else:
            internal_hdrs.append(src)

    return CcFileList(
        hdrs = _flatten_target_files(getattr(rule_attr, "hdrs", [])),
        textual_hdrs = _flatten_target_files(getattr(
            rule_attr,
            "textual_hdrs",
            [],
        )),
        internal_hdrs = depset(internal_hdrs),
        srcs = depset(srcs),
    )

def combine_cc_file_lists(cc_file_lists):
    """Combine several CcFileLists into a single one."""

    # The returned CcFileList will contain depsets of the deps' file lists.
    # These lists hold `depset()`s from each of `deps`.
    srcs = []
    hdrs = []
    internal_hdrs = []
    textual_hdrs = []

    for cfl in cc_file_lists:
        srcs.append(cfl.srcs)
        hdrs.append(cfl.hdrs)
        internal_hdrs.append(cfl.internal_hdrs)
        textual_hdrs.append(cfl.textual_hdrs)

    return CcFileList(
        srcs = depset(transitive = srcs),
        hdrs = depset(transitive = hdrs),
        internal_hdrs = depset(transitive = internal_hdrs),
        textual_hdrs = depset(transitive = textual_hdrs),
    )

def _extract_cc_file_list_from_test_suite(target, rule_attr):
    transitive = []

    for dep in getattr(rule_attr, "tests", []):
        transitive.append(dep[CcFileList])

    # Test suites can list tests explicitly, or they can implicitly
    # expand to "all tests in this package." In the latter case, Bazel
    # exposes the implicit tests via the internal attribute
    # `_implicit_tests`.
    #
    # Unfortunately, these attrs are currently (as of Bazel 5) the only
    # way to get the lists of tests, but this might change in the
    # future (c.f.: https://github.com/bazelbuild/bazel/issues/14993).
    for dep in getattr(rule_attr, "_implicit_tests", []):
        if CcFileList in dep:
            transitive.append(dep[CcFileList])

    if not transitive:
        # We are reaching into Bazel-internal attributes, which have no
        # guarantee that they won't change. If we end up with no tests,
        # print a warning.
        print(("WARNING: no tests were found in test_suite %s. If there " +
               "are no tests in package %s, consider removing the " +
               "test_suite. (Otherwise, this could be due to a bug " +
               "in gen_file_lists.)") %
              (target.label, target.label.package))

    return combine_cc_file_lists(transitive)

def _cc_file_list_aspect_impl(target, ctx):
    # Special case: if this is a test_suite, collect the CcFileLists from
    # its tests.
    if ctx.rule.kind == "test_suite":
        return [_extract_cc_file_list_from_test_suite(target, ctx.rule.attr)]

    # Extract sources from a `cc_library` (or similar):
    if CcInfo in target:
        return [_extract_cc_file_list_from_cc_rule(ctx.rule.attr)]

    return []

cc_file_list_aspect = aspect(
    doc = """
Aspect to provide the list of sources and headers from a rule.

Output is CcFileList. Example:

  cc_library(
      name = "foo",
      srcs = [
          "foo.cc",
          "foo_internal.h",
      ],
      hdrs = ["foo.h"],
      textual_hdrs = ["foo_inl.inc"],
  )
  # produces:
  # CcFileList(
  #     hdrs = depset([File("foo.h")]),
  #     textual_hdrs = depset([File("foo_inl.inc")]),
  #     internal_hdrs = depset([File("foo_internal.h")]),
  #     srcs = depset([File("foo.cc")]),
  # )
""",
    implementation = _cc_file_list_aspect_impl,

    # Allow propagation for test_suite rules:
    attr_aspects = ["tests", "_implicit_tests"],
)

################################################################################
# Proto sources and generated file/header support
################################################################################

ProtoFileList = provider(
    doc = "List of proto files and generated code to be built into a library.",
    fields = {
        # Proto files:
        "proto_srcs": "proto file sources",

        # Generated sources:
        "hdrs": "header files that are expected to be generated",
        "srcs": "source files that are expected to be generated",
    },
)

def _proto_file_list_aspect_impl(target, ctx):
    # We're going to reach directly into the attrs on the traversed rule.
    rule_attr = ctx.rule.attr
    providers = []

    # Extract sources from a `proto_library`:
    if ProtoInfo in target:
        proto_srcs = []
        srcs = []
        hdrs = []
        for src in _flatten_target_files(rule_attr.srcs):
            proto_srcs.append(src)
            srcs.append("%s/%s.pb.cc" % (src.dirname, src.basename))
            hdrs.append("%s/%s.pb.h" % (src.dirname, src.basename))

        providers.append(ProtoFileList(
            proto_srcs = proto_srcs,
            srcs = srcs,
            hdrs = hdrs,
        ))

    return providers

proto_file_list_aspect = aspect(
    doc = """
Aspect to provide the list of sources and headers from a rule.

Output is a ProtoFileList. Example:

  proto_library(
      name = "bar_proto",
      srcs = ["bar.proto"],
  )
  # produces:
  # ProtoFileList(
  #     proto_srcs = ["bar.proto"],
  #     # Generated filenames are synthesized:
  #     hdrs = ["bar.pb.h"],
  #     srcs = ["bar.pb.cc"],
  # )
""",
    implementation = _proto_file_list_aspect_impl,
)
