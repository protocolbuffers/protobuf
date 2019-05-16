# A hacky way to work around the fact that native.bazel_version is only
# available from WORKSPACE macros, not BUILD macros or rules.
#
# Hopefully we can remove this if/when this is fixed:
#   https://github.com/bazelbuild/bazel/issues/8305

def _impl(repository_ctx):
    s = "bazel_version = \"" + native.bazel_version + "\""
    repository_ctx.file("bazel_version.bzl", s)
    repository_ctx.file("BUILD", "")

bazel_version_repository = repository_rule(
    implementation = _impl,
    local = True,
)
