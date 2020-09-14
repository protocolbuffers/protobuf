#!/usr/bin/env python3

import json
import subprocess
import re

def Run(cmd):
  subprocess.check_call(cmd, shell=True)

def RunAgainstBranch(branch, outbase, runs=12):
  tmpfile = "/tmp/bench-output.json"
  Run("rm -rf {}".format(tmpfile))
  Run("git checkout {}".format(branch))
  Run("bazel build -c opt :benchmark")

  Run("./bazel-bin/benchmark --benchmark_out_format=json --benchmark_out={} --benchmark_repetitions={}".format(tmpfile, runs))

  Run("bazel build -c opt --copt=-g :conformance_upb")
  Run("cp -f bazel-bin/conformance_upb {}.bin".format(outbase))

  with open(tmpfile) as f:
    bench_json = json.load(f)

  with open(outbase + ".txt", "w") as f:
    for run in bench_json["benchmarks"]:
      name = re.sub(r'^BM_', 'Benchmark', run["name"])
      if name.endswith("_mean") or name.endswith("_median") or name.endswith("_stddev"):
        continue
      values = (name, run["iterations"], run["cpu_time"])
      print("{} {} {} ns/op".format(*values), file=f)

RunAgainstBranch("6e140c267cc9bf6a4ef89d3f9a842209d7537599", "/tmp/old")
RunAgainstBranch("encoder", "/tmp/new")

print()
print()

Run("~/go/bin/benchstat /tmp/old.txt /tmp/new.txt")

print()
print()

Run("objcopy --strip-debug /tmp/old.bin /tmp/old.bin.stripped")
Run("objcopy --strip-debug /tmp/new.bin /tmp/new.bin.stripped")
Run("~/code/bloaty/bloaty /tmp/new.bin.stripped -- /tmp/old.bin.stripped --debug-file=/tmp/old.bin --debug-file=/tmp/new.bin -d compileunits,symbols")
