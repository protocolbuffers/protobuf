#!/bin/bash
# Helper script to run Perl tests in XS mode with parallel execution.
# Supports passing specific test files as arguments.

export LD_LIBRARY_PATH=".:$LD_LIBRARY_PATH"

# Default to running all tests in t/ if no arguments are provided
if [[ $# -eq 0 ]]; then
  exec prove -j"$(nproc)" -b -It/lib t/*.t
else
  exec prove -j"$(nproc)" -b -It/lib "$@"
fi
