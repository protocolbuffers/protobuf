#!/bin/bash
# Script to run specific Perl tests with AddressSanitizer preloaded and local library paths correctly set.

if [[ "$#" -eq 0 ]]; then
    echo "Usage: $0 <test1.t> [test2.t ...]"
    echo "Error: A list of .t files is required as arguments."
    exit 1
fi

echo "--- Building protoc-gen-perl-pb ---"
make protoc-gen-perl-pb || exit 1
echo "--- Build finished ---"

export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libasan.so.8
export LD_LIBRARY_PATH=".:$LD_LIBRARY_PATH"
export ASAN_OPTIONS="detect_leaks=0:abort_on_error=1"

# -Mblib ensures we use the compiled XS in blib/
# -It/lib for test helper modules
# -b for blib
prove -Mblib -It/lib -bv "$@"