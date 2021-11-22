"""
Generates package naming variables for use with rules_pkg.
"""

load("@rules_pkg//:providers.bzl", "PackageVariablesInfo")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load(":protobuf_version.bzl", "PROTOBUF_VERSION")

def _package_naming_impl(ctx):
  values = {}
  values["version"] = PROTOBUF_VERSION

  # infer from the current cpp toolchain.
  toolchain = find_cpp_toolchain(ctx)
  values["cpu"] = toolchain.cpu
  
  return PackageVariablesInfo(values = values)


package_naming = rule(
  implementation = _package_naming_impl,
    attrs = {
      # Necessary data dependency for find_cpp_toolchain.
      "_cc_toolchain": attr.label(default = Label("@bazel_tools//tools/cpp:current_cc_toolchain")),
    },
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    incompatible_use_toolchain_transition = True,
)
