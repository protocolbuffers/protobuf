#!/bin/bash
set -e
BINARY_PATH=$1
NM_TOOL=$2
PLATFORM=$3

if [[ "$PLATFORM" == "not_supported" ]]; then
  echo "Platform not supported for this test, skipping."
  exit 0
fi

function check_symbol() {
  local pattern=$1
  local expected=$2 # "true" or "false"

  local target_file="$BINARY_PATH"

  if [[ "$PLATFORM" == "wasm" ]]; then
    echo "Platform is wasm"
    echo "File type of $BINARY_PATH (following symlinks) is:"
    file -L "$BINARY_PATH"
    # Extract wasm file from tarball if needed
    if file -L "$BINARY_PATH" | grep -q "tar archive"; then
      echo "Extracting tarball to $TEST_TMPDIR..."
      tar -xf "$BINARY_PATH" -C "$TEST_TMPDIR"
      target_file="$TEST_TMPDIR/treeshaking_app.wasm"
      echo "New target file: $target_file"
    fi
  elif [[ "$PLATFORM" == "ios" ]]; then
    local ipa_file=""
    if [[ "$BINARY_PATH" == *.ipa ]]; then
      ipa_file="$BINARY_PATH"
    elif [[ -f "${BINARY_PATH}.ipa" ]]; then
      ipa_file="${BINARY_PATH}.ipa"
    fi
    if [[ -n "$ipa_file" ]]; then
      local extract_dir
      extract_dir=$(mktemp -d -p "$TEST_TMPDIR")
      unzip -q "$ipa_file" -d "$extract_dir"
      local app_name
      app_name=$(basename "$ipa_file" .ipa)
      target_file="$extract_dir/Payload/${app_name}.app/${app_name}"
    fi
  fi

  # Use the provided nm tool
  SYMBOLS=$("$NM_TOOL" -C "$target_file" | grep "$pattern" || true)

  if [[ "$expected" == "true" && -z "$SYMBOLS" ]]; then
    echo "FAIL: Expected symbol $pattern not found" && exit 1
  elif [[ "$expected" == "false" && -n "$SYMBOLS" ]]; then
    echo "FAIL: Unused symbol $pattern found in binary" && exit 1
  fi
}

# Verify Rust/C++ Thunks (C++ Kernel)
check_symbol "UsedMessage_new" "true"
check_symbol "UnusedMessage_new" "false"
