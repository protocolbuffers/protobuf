"""Gathers output from test runs and create an XML file in JUnit format.

The output files from the individual tests have been written in a directory
structure like:

  $DIR/joblog  (output from "parallel --joblog joblog")
  $DIR/logs/1/cpp/stdout
  $DIR/logs/1/cpp/stderr
  $DIR/logs/1/csharp/stdout
  $DIR/logs/1/csharp/stderr
  $DIR/logs/1/java_jdk7/stdout
  $DIR/logs/1/java_jdk7/stderr
  etc.

This script bundles them into a single output XML file so Jenkins can show
detailed test results.  It runs as the last step before the Jenkins build
finishes.
"""

import os;
import sys;
from yattag import Doc
from collections import defaultdict

def readtests(basedir):
  tests = defaultdict(dict)

  # Sample input (note: separators are tabs).
  #
  # Seq	Host	Starttime	Runtime	Send	Receive	Exitval	Signal	Command
  # 1	:	1456263838.313	0.005	0	0	0	0	echo A
  with open(basedir + "/joblog") as jobs:
    firstline = next(jobs)
    for line in jobs:
      values = line.split("\t")

      name = values[8].split()[-1]
      test = tests[name]
      test["name"] = name
      test["time"] = values[3]

      exitval = values[6]
      if int(exitval):
        # We don't have a more specific message.  User should look at stderr.
        test["failure"] = "TEST FAILURE"
      else:
        test["failure"] = False

  for testname in os.listdir(basedir + "/logs/1"):
    test = tests[testname]

    with open(basedir + "/logs/1/" + testname + "/stdout") as f:
      test["stdout"] = f.read()

    with open(basedir + "/logs/1/" + testname + "/stderr") as f:
      test["stderr"] = f.read()

  # The cpp test is special since it doesn't run under parallel so doesn't show
  # up in the job log.
  tests["cpp"]["name"] = "cpp"

  with open(basedir + '/logs/1/cpp/build_time', 'r') as f:
    tests["cpp"]["time"] = f.read().strip()
  tests["cpp"]["failure"] = False

  ret = tests.values()
  ret.sort(key=lambda x: x["name"])

  return ret

def genxml(tests):
  doc, tag, text = Doc().tagtext()

  with tag("testsuites"):
    with tag("testsuite", name="Protobuf Tests"):
      for test in tests:
        with tag("testcase", name=test["name"], classname=test["name"],
                             time=test["time"]):
          with tag("system-out"):
            text(test["stdout"])
          with tag("system-err"):
            text(test["stderr"])
          if test["failure"]:
            with tag("failure"):
              text(test["failure"])

  return doc.getvalue()

sys.stderr.write("make_test_output.py: writing XML from directory: " +
                 sys.argv[1] + "\n");
print genxml(readtests(sys.argv[1]))
