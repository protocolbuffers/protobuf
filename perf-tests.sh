#!/bin/sh
# Builds and runs all available benchmarks.  The tree will be built
# multiple times with a few different compiler flag combinations.
# The output will be dumped to stdout and to perf-tests.out.

set -e
MAKETARGET=benchmarks
if [ x$1 == xupb ]; then
  MAKETARGET=upb_benchmarks
fi

rm -f perf-tests.out

make clean
echo "-O3 -DNDEBUG -msse3 -DUPB_THREAD_UNSAFE" > perf-cppflags
make $MAKETARGET
make benchmark | sed -e 's/^/plain./g' | tee -a perf-tests.out

make clean
echo "-O3 -DNDEBUG -fomit-frame-pointer -msse3 -DUPB_THREAD_UNSAFE" > perf-cppflags
make $MAKETARGET
make benchmark | sed -e 's/^/omitfp./g' | tee -a perf-tests.out

if [ x`uname -m` == xx86_64 ]; then
  make clean
  echo "-O3 -DNDEBUG -msse3 -m32 -DUPB_THREAD_UNSAFE" > perf-cppflags
  make upb_benchmarks
  make benchmark | sed -e 's/^/plain32./g' | tee -a perf-tests.out

  make clean
  echo "-O3 -DNDEBUG -fomit-frame-pointer -msse3 -m32 -DUPB_THREAD_UNSAFE" > perf-cppflags
  make upb_benchmarks
  make benchmark | sed -e 's/^/omitfp32./g' | tee -a perf-tests.out
fi

