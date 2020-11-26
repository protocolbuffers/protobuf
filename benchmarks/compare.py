#!/usr/bin/env python3
"""Benchmarks the current working directory against a given baseline.

This script benchmarks both size and speed. Sample output:
"""

import contextlib
import json
import os
import re
import subprocess
import sys
import tempfile

@contextlib.contextmanager
def GitWorktree(commit):
  tmpdir = tempfile.mkdtemp()
  subprocess.run(['git', 'worktree', 'add', '-q', '-d', tmpdir, commit], check=True)
  cwd = os.getcwd()
  os.chdir(tmpdir)
  try:
    yield tmpdir
  finally:
    os.chdir(cwd)
    subprocess.run(['git', 'worktree', 'remove', tmpdir], check=True)

def Run(cmd):
  subprocess.check_call(cmd, shell=True)

def Benchmark(outbase, bench_cpu=True, runs=12, fasttable=False):
  tmpfile = "/tmp/bench-output.json"
  Run("rm -rf {}".format(tmpfile))
  #Run("CC=clang bazel test ...")
  if fasttable:
    extra_args = " --//:fasttable_enabled=true"
  else:
    extra_args = ""

  if bench_cpu:
    Run("CC=clang bazel build -c opt --copt=-march=native benchmarks:benchmark" + extra_args)
    Run("./bazel-bin/benchmarks/benchmark --benchmark_out_format=json --benchmark_out={} --benchmark_repetitions={}".format(tmpfile, runs))
    with open(tmpfile) as f:
      bench_json = json.load(f)

    # Translate into the format expected by benchstat.
    with open(outbase + ".txt", "w") as f:
      for run in bench_json["benchmarks"]:
        name = run["name"]
        name = name.replace(" ", "")
        name = re.sub(r'^BM_', 'Benchmark', name)
        if name.endswith("_mean") or name.endswith("_median") or name.endswith("_stddev"):
          continue
        values = (name, run["iterations"], run["cpu_time"])
        print("{} {} {} ns/op".format(*values), file=f)

  Run("CC=clang bazel build -c opt --copt=-g tests:conformance_upb" + extra_args)
  Run("cp -f bazel-bin/tests/conformance_upb {}.bin".format(outbase))


baseline = "master"
bench_cpu = False
fasttable = False

if len(sys.argv) > 1:
  baseline = sys.argv[1]

  # Quickly verify that the baseline exists.
  with GitWorktree(baseline):
    pass

# Benchmark our current directory first, since it's more likely to be broken.
Benchmark("/tmp/new", bench_cpu, fasttable=fasttable)

# Benchmark the baseline.
with GitWorktree(baseline):
  Benchmark("/tmp/old", bench_cpu, fasttable=fasttable)

print()
print()

if bench_cpu:
  Run("~/go/bin/benchstat /tmp/old.txt /tmp/new.txt")

print()
print()

Run("objcopy --strip-debug /tmp/old.bin /tmp/old.bin.stripped")
Run("objcopy --strip-debug /tmp/new.bin /tmp/new.bin.stripped")
Run("~/code/bloaty/bloaty /tmp/new.bin.stripped -- /tmp/old.bin.stripped --debug-file=/tmp/old.bin --debug-file=/tmp/new.bin -d compileunits,symbols")
