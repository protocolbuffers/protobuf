"""Starlark definitions for Protobuf benchmark tests.

PLEASE DO NOT DEPEND ON THE CONTENTS OF THIS FILE, IT IS UNSTABLE.
"""
load("//build_defs:internal_shell.bzl", "inline_sh_binary")

def internal_benchmark_test(
        name,
        binary,
        datasets,
        args = [],
        env_vars = []):
    """Benchmark test runner.

    Args:
      name: the name for the test.
      binary: a benchmark test binary.
      datasets: a set of datasets to benchmark.
      args: optional arguments to pass the binary.
      env_vars: environment variables to set in the test.
    """

    dataset_labels = []
    for dataset in datasets:
        dataset_labels.append("$(rootpaths %s)" % dataset)
    inline_sh_binary(
        name = name,
        srcs = datasets,
        tools = [binary],
        cmd = "%s $(rootpath %s) %s %s" % (
            " ".join(env_vars),
            binary,
            " ".join(args),
            " ".join(dataset_labels)),
        testonly = 1,
    )
