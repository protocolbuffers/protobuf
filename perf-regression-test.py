#!/usr/bin/env python
# Use to test the current working directory's performance against HEAD.

import os

os.system("""
set -e
set -v

# Generate numbers for baseline.
rm -rf perf-tmp
git clone . perf-tmp
(cd perf-tmp && ../perf-tests.sh upb)
cp perf-tmp/perf-tests.out perf-tests.baseline

# Generate numbers for working directory.
./perf-tests.sh upb""")

baseline = {}
baseline_file = open("perf-tests.baseline")
for line in baseline_file:
  test, speed = line.split(":")
  baseline[test] = int(speed)

print("\n\n=== PERFORMANCE REGRESSION TEST RESULTS:\n")
wd_file = open("perf-tests.out")
for line in wd_file:
  test, speed = line.split(":")
  baseline_val = baseline[test]
  change = float(int(speed) - baseline_val) / float(baseline_val) * 100
  print "%s: %d -> %d (%0.2f)" % (test, baseline_val, int(speed), change)
