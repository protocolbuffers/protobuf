load("@rules_python//python:defs.bzl", "py_library")

# Consume `six.py` as `__init__.py` for compatibility
# with `--incompatible_default_to_explicit_init_py`.
# https://github.com/protocolbuffers/protobuf/pull/6795#issuecomment-546060749
# https://github.com/bazelbuild/bazel/issues/10076
genrule(
    name = "copy_six",
    srcs = ["six-1.12.0/six.py"],
    outs = ["__init__.py"],
    cmd = "cp $< $(@)",
)

py_library(
    name = "six",
    srcs = ["__init__.py"],
    srcs_version = "PY2AND3",
    visibility = ["//visibility:public"],
)
