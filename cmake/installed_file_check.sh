#!/bin/bash

set -e

check_golden() {
  DIR=$1
  TYPE=$2
  echo "Checking $DIR/$TYPE.txt against cmake/installed_$TYPE.txt"
  DIFF="$(diff -u cmake/installed_${TYPE}.txt $DIR/$TYPE.txt)"
  if [ -n "$DIFF" ]; then
    echo "Installed files do not match goldens!"
    echo "$DIFF"
    exit 1
  else
    echo "Installed files match goldens."
  fi
}

check_golden "static/include.txt" "include"
check_golden "shared/include.txt" "include"
check_golden "static/bin.txt" "bin"
check_golden "shared/bin.txt" "bin"
check_golden "static/lib.txt" "static_lib"
check_golden "shared/lib.txt" "shared_lib"
