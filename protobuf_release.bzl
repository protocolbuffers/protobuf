"""
Generates package naming variables for use with rules_pkg.
"""

load("@rules_pkg//:providers.bzl", "PackageVariablesInfo")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load(":protobuf_version.bzl", "PROTOC_VERSION")

def _package_naming_impl(ctx):
  values = {}
  values["version"] = PROTOC_VERSION

  # infer from the current cpp toolchain.
  toolchain = find_cpp_toolchain(ctx)
  cpu = toolchain.cpu
  system_name = toolchain.target_gnu_system_name

  # rename cpus to match what we want artifacts to be
  if cpu == "systemz":
    cpu = "s390_64"
  elif cpu == "aarch64":
    cpu = "aarch_64"
  elif cpu == "ppc64":
    cpu = "ppcle_64"

  # use the system name to determine the os and then create platform names
  if "apple" in system_name:
    values["platform"] = "osx-" + cpu
  elif "linux" in system_name:
    values["platform"] = "linux-" + cpu
  elif "mingw" in system_name:
    if cpu == "x86_64":
      values["platform"] = "win64"
    else:
      values["platform"] = "win32"
  else:
    values["platform"] = "unknown"

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
