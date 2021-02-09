"""Definitions for Kotlin proto libraries."""

load(":kt_jvm_proto_library.internal.bzl", _kt_proto_library_helper = "kt_proto_library_helper")
load("//devtools/build_cleaner/skylark:build_defs.bzl", "register_extension_info")

def _lite_proto_name(name):
    return ":%s_DO_NOT_DEPEND_java_lite_proto" % name

def _proto_name(name):
    return ":%s_DO_NOT_DEPEND_java_proto" % name

def kt_jvm_lite_proto_library(
        name,
        deps = None,
        tags = None,
        testonly = None,
        compatible_with = None,
        restricted_to = None,
        visibility = None,
        deprecation = None,
        features = []):
    """
    This rule generates and compiles lite Java and Kotlin APIs for a specified proto_library.

    Args:
      name: A name for the target
      deps: One or more proto_library targets
      tags: List of string tags passed to generated targets.
      testonly: Whether this target is intended only for tests.
      compatible_with: Standard attribute, see http://go/be-common#common.compatible_with
      restricted_to: Standard attribute, see http://go/be-common#common.restricted_to
      visibility: A list of targets allowed to depend on this rule.
      deprecation: Standard attribute, see http://go/be-common#common.deprecation
      features: Features enabled.
    """

    tags = (tags or []) + ["kt_jvm_lite_proto_library"]

    native.java_lite_proto_library(
        name = _lite_proto_name(name)[1:],
        deps = deps,
        testonly = testonly,
        compatible_with = compatible_with,
        visibility = ["//visibility:private"],
        restricted_to = restricted_to,
        tags = tags + ["avoid_dep"],
        deprecation = deprecation,
        features = features,
    )

    _kt_proto_library_helper(
        name = name,
        proto_deps = deps,
        deps = [
            _lite_proto_name(name),
            "//java/com/google/protobuf:protobuf_lite",
            "//java/com/google/protobuf/kotlin:only_for_use_in_proto_generated_code_its_generator_and_tests",
            "//java/com/google/protobuf/kotlin:shared_runtime",
        ],
        exports = [_lite_proto_name(name)],
        testonly = testonly,
        compatible_with = compatible_with,
        restricted_to = restricted_to,
        constraints = ["android"],
        tags = tags,
        variant = "lite",
        visibility = visibility,
        deprecation = deprecation,
        features = features,
    )

def kt_jvm_proto_library(
        name,
        deps = None,
        tags = None,
        testonly = None,
        compatible_with = None,
        restricted_to = None,
        visibility = None,
        deprecation = None,
        features = []):
    """
    This rule generates and compiles full Java and Kotlin APIs for a specified proto_library.

    Args:
      name: A name for the target
      deps: One or more proto_library targets
      tags: List of string tags passed to generated targets.
      testonly: Whether this target is intended only for tests.
      compatible_with: Standard attribute, see http://go/be-common#common.compatible_with
      restricted_to: Standard attribute, see http://go/be-common#common.restricted_to
      visibility: A list of targets allowed to depend on this rule.
      deprecation: Standard attribute, see http://go/be-common#common.deprecation
      features: Features enabled.
    """

    tags = (tags or []) + ["kt_jvm_proto_library"]

    native.java_proto_library(
        name = _proto_name(name)[1:],
        deps = deps,
        testonly = testonly,
        compatible_with = compatible_with,
        visibility = ["//visibility:private"],
        restricted_to = restricted_to,
        tags = tags + ["avoid_dep"],
        deprecation = deprecation,
        features = features,
    )

    _kt_proto_library_helper(
        name = name,
        proto_deps = deps,
        deps = [
            _proto_name(name),
            "//java/com/google/protobuf",
            "//java/com/google/protobuf/kotlin:only_for_use_in_proto_generated_code_its_generator_and_tests",
            "//java/com/google/protobuf/kotlin:shared_runtime",
        ],
        exports = [_proto_name(name)],
        testonly = testonly,
        compatible_with = compatible_with,
        constraints = [],
        restricted_to = restricted_to,
        tags = tags,
        variant = "full",
        visibility = visibility,
        deprecation = deprecation,
        features = features,
    )

register_extension_info(
    extension = kt_jvm_proto_library,
    label_regex_for_dep = "{extension_name}",
)

register_extension_info(
    extension = kt_jvm_lite_proto_library,
    label_regex_for_dep = "{extension_name}",
)
