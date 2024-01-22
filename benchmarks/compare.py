#!/usr/bin/python3
#
# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google LLC nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
    Run("./bazel-bin/benchmarks/benchmark --benchmark_out_format=json --benchmark_out={} --benchmark_repetitions={} --benchmark_min_time=0.05 --benchmark_enable_random_interleaving=true".format(tmpfile, runs))
    with open(tmpfile) as f:
      bench_json = json.load(f)

    # Translate into the format expected by benchstat.
    txt_filename = outbase + ".txt"
    with open(txt_filename, "w") as f:
      for run in bench_json["benchmarks"]:
        if run["run_type"] == "aggregate":
          continue
        name = run["name"]
        name = name.replace(" ", "")
        name = re.sub(r'^BM_', 'Benchmark', name)
        values = (name, run["iterations"], run["cpu_time"])
        print("{} {} {} ns/op".format(*values), file=f)
    Run("sort {} -o {} ".format(txt_filename, txt_filename))

  Run("CC=clang bazel build -c opt --copt=-g --copt=-march=native :conformance_upb"
      + extra_args)
  Run("cp -f bazel-bin/conformance_upb {}.bin".format(outbase))


baseline = "main"
bench_cpu = True
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
