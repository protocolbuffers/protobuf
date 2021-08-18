
_build_file = """
cc_library(
   name = "python_headers",
   hdrs = glob(["python/**/*.h"]),
   includes = ["python"],
   visibility = ["//visibility:public"],
)
"""

def _find_python_dir(repository_ctx):
  versions = ["3.6", "3.7", "3.8", "3.9", "3.10", "3.11"]
  for version in versions:
    path = "/usr/include/python" + version
    if repository_ctx.path(path + "/" + "Python.h").exists:
      return path
  fail("No Python headers found in /usr/include/python3.* (require 3.6 or newer)")

def _python_headers_impl(repository_ctx):
  path = _find_python_dir(repository_ctx)
  repository_ctx.symlink(path, "python")
  repository_ctx.file("BUILD.bazel", _build_file)


python_headers = repository_rule(
    implementation = _python_headers_impl,
    local = True,
)
