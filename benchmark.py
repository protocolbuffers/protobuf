#!/usr/bin/env python3

import json
import subprocess
import re

def Run(cmd):
  subprocess.check_call(cmd, shell=True)

def RunAgainstBranch(branch, outfile, runs=12):
  tmpfile = "/tmp/bench-output.json"
  Run("rm -rf {}".format(tmpfile))
  Run("git checkout {}".format(branch))
  Run("bazel build -c opt :benchmark")

  Run("./bazel-bin/benchmark --benchmark_out_format=json --benchmark_out={} --benchmark_repetitions={}".format(tmpfile, runs))

  with open(tmpfile) as f:
    bench_json = json.load(f)

  with open(outfile, "w") as f:
    for run in bench_json["benchmarks"]:
      name = re.sub(r'^BM_', 'Benchmark', run["name"])
      if name.endswith("_mean") or name.endswith("_median") or name.endswith("_stddev"):
        continue
      values = (name, run["iterations"], run["cpu_time"])
      print("{} {} {} ns/op".format(*values), file=f)

RunAgainstBranch("master", "/tmp/old.txt")
RunAgainstBranch("decoder", "/tmp/new.txt")

Run("~/go/bin/benchstat /tmp/old.txt /tmp/new.txt")
