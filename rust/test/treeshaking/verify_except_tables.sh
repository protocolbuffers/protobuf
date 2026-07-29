#!/bin/bash
set -e

BINARY1_PATH=$1
BINARY2_PATH=$2
NM_TOOL=$3
PLATFORM=$4

if [[ "$PLATFORM" == "not_supported" ]]; then
  echo "Platform not supported for this test, skipping."
  exit 0
fi

# Function to count exception tables in a binary
count_except_tables() {
  local binary=$1
  local target_file="$binary"

  if [[ "$PLATFORM" == "wasm" ]]; then
    # Extract wasm file from tarball if needed
    if file -L "$binary" | grep -q "tar archive"; then
      tar -xf "$binary" -C "$TEST_TMPDIR"
      target_file="$TEST_TMPDIR/treeshaking_app.wasm"
    fi
  fi

  "$NM_TOOL" "$target_file" | grep GCC_except_table | wc -l
}

COUNT1=$(count_except_tables "$BINARY1_PATH")
COUNT2=$(count_except_tables "$BINARY2_PATH")

echo "Output 1 exception tables: $COUNT1"
echo "Output 2 exception tables: $COUNT2"

if [[ "$COUNT1" -ne "$COUNT2" ]]; then
  echo "FAIL: Exception table counts differ ($COUNT1 vs $COUNT2)."
  exit 1
fi

echo "PASS: Exception table counts are identical."
