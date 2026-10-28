#!/bin/bash
set -e

BINARY1_PATH=$1
BINARY2_PATH=$2
NM_TOOL=$3
PLATFORM=$4
OBJDUMP_TOOL=$5

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
    "$NM_TOOL" "$target_file" | grep GCC_except_table | wc -l
  elif [[ "$PLATFORM" == "ios" ]]; then
    local ipa_file=""
    if [[ "$binary" == *.ipa ]]; then
      ipa_file="$binary"
    elif [[ -f "${binary}.ipa" ]]; then
      ipa_file="${binary}.ipa"
    fi
    if [[ -n "$ipa_file" ]]; then
      local extract_dir
      extract_dir=$(mktemp -d -p "$TEST_TMPDIR")
      unzip -q "$ipa_file" -d "$extract_dir"
      local app_name
      app_name=$(basename "$ipa_file" .ipa)
      target_file="$extract_dir/Payload/${app_name}.app/${app_name}"
    fi
    # On Mach-O, exception tables live in sections (__gcc_except_tab,
    # __unwind_info) rather than symbols, so compare section and binary sizes.
    local tool="${OBJDUMP_TOOL:-objdump}"
    local sections
    sections=$("$tool" -h "$target_file" | awk '/^[[:space:]]*[0-9]+/ {print $1, $2, $3, $4, $5}')
    if [[ -z "$sections" ]]; then
      echo "FAIL: No sections found in Mach-O binary $target_file"
      exit 1
    fi
    local bin_size
    bin_size=$(stat -L -c "%s" "$target_file")
    echo "binary_size_bytes $bin_size"
    echo "$sections"
  else
    "$NM_TOOL" "$target_file" | grep GCC_except_table | wc -l
  fi
}

COUNT1=$(count_except_tables "$BINARY1_PATH")
COUNT2=$(count_except_tables "$BINARY2_PATH")

echo "Output 1 exception tables: $COUNT1"
echo "Output 2 exception tables: $COUNT2"

if [[ "$COUNT1" != "$COUNT2" ]]; then
  echo "FAIL: Exception table counts differ ($COUNT1 vs $COUNT2)."
  exit 1
fi

echo "PASS: Exception table counts are identical."
