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
