#!/bin/bash

cd $(dirname $0)

set -ex

PORT=12345
TIMEOUT=10

./compile_extension.sh

run_test() {
  echo
  echo "Running memory leak test, args: $@"

  EXTRA_ARGS=""
  ARGS="-d xdebug.profiler_enable=0 -d display_errors=on -dextension=../ext/google/protobuf/modules/protobuf.so"

  for i in "$@"; do
    case $i in
      --keep_descriptors)
        EXTRA_ARGS=-dprotobuf.keep_descriptor_pool_after_request=1
        shift
        ;;
    esac
  done

  export ZEND_DONT_UNLOAD_MODULES=1
  export USE_ZEND_ALLOC=0

  if valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --suppressions=valgrind.supp --num-callers=100 php $ARGS $EXTRA_ARGS memory_leak_test.php; then
    echo "Memory leak test SUCCEEDED"
  else
    echo "Memory leak test FAILED"
    exit 1
  fi
}

run_test
run_test --keep_descriptors
