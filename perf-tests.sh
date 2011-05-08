#!/bin/sh
# Builds and runs all available benchmarks.  The tree will be built
# multiple times with a few different compiler flag combinations.
# The output will be dumped to stdout and to perf-tests.out.

set -e
MAKETARGET=benchmarks
if [ x$1 = xupb ]; then
  MAKETARGET=upb_benchmarks
fi

rm -f perf-tests.out

run_with_flags () {
  FLAGS=$1
  NAME=$2

  make clean
  echo "$FLAGS" > perf-cppflags
  make upb_benchmarks
  make benchmark | sed -e "s/^/$NAME./g" | tee -a perf-tests.out
}

#if [ x`uname -m` = xx86_64 ]; then
  run_with_flags "-DNDEBUG -m32" "plain32"
  run_with_flags "-DNDEBUG -fomit-frame-pointer -m32" "omitfp32"
#fi

run_with_flags "-DNDEBUG " "plain"
run_with_flags "-DNDEBUG -fomit-frame-pointer" "omitfp"
run_with_flags "-DNDEBUG -DUPB_USE_JIT_X64" "jit"
