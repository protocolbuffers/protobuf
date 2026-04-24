#!/bin/bash
set -e

BINARY1_PATH=$1
BINARY2_PATH=$2
PLATFORM=$3

if [[ "$PLATFORM" == "not_supported" ]]; then
  echo "Platform not supported for this test, skipping."
  exit 0
fi

# Always follow symlinks (-L) to get the actual file/binary size
SIZE1=$(stat -L -c%s "$BINARY1_PATH")
SIZE2=$(stat -L -c%s "$BINARY2_PATH")

echo "Output 1 size: $SIZE1"
echo "Output 2 size: $SIZE2"

if [[ "$SIZE1" -ne "$SIZE2" ]]; then
  echo "FAIL: Output sizes differ ($SIZE1 vs $SIZE2). Treeshaking might not be working."
  exit 1
fi

echo "PASS: Output sizes are identical."
