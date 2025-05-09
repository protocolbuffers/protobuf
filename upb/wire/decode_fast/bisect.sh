#!/bin/bash
#
# Bisect the set of fasttable parsing functions to find the one(s) causing a
# test failure.
#
# Example usage:
#
#   $ blaze test --//third_party/upb:fasttable_enabled=True third_party/upb/upb/...
#
#   # We notice that //third_party/upb/upb/test:test_generated_code is failing
#   # when fasttable is enabled.  We can bisect the set of fasttable parsing
#   # functions with this command:
#
#   $ third_party/upb/upb/wire/decode_fast/bisect.sh third_party/upb/upb/test:test_generated_code

if [[ $# -ne 1 ]]; then
  echo "Usage: bisect.sh [blaze test flags] <test_target(s)>"
  exit 1
fi

/google/data/ro/teams/tetralight/bin/bisect -low 0 -high 128 \
  "blaze test --//third_party/upb:fasttable_enabled=True \
  --per_file_copt=//third_party/upb/upb/wire/decode_fast:select@-DUPB_DECODEFAST_DISABLE_FUNCTIONS_ABOVE=\$X \
  --host_per_file_copt=//third_party/upb/upb/wire/decode_fast:select@-DUPB_DECODEFAST_DISABLE_FUNCTIONS_ABOVE=\$X \
  $@"
