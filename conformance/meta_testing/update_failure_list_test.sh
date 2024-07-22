#!/bin/bash
#

source googletest.sh || exit 1

# Checks for expected behavior of update_failure_list.py when a failure is in
# alphabetical order. 

# Copy the to-update file 
cp "${TEST_SRCDIR}/google3/third_party/protobuf/conformance/meta_testing/to_update_failure_list.txt" \
  "${TEST_UNDECLARED_OUTPUTS_DIR}/to_update_failure_list.txt"

# Give permission to modify it
chmod u+w "${TEST_UNDECLARED_OUTPUTS_DIR}/to_update_failure_list.txt"

# Run update_failure_list.py script
"${TEST_SRCDIR}/google3/third_party/protobuf/conformance/update_failure_list" \
  --add "${TEST_SRCDIR}/google3/third_party/protobuf/conformance/meta_testing/failing_tests.txt" \
  --remove "${TEST_SRCDIR}/google3/third_party/protobuf/conformance/meta_testing/succeeding_tests.txt" \
  "${TEST_UNDECLARED_OUTPUTS_DIR}/to_update_failure_list.txt" 
  
# Compare the to-update file with golden_failure_list.txt for content equality
if cmp -s "${TEST_SRCDIR}/google3/third_party/protobuf/conformance/meta_testing/golden_failure_list.txt" \
  "${TEST_UNDECLARED_OUTPUTS_DIR}/to_update_failure_list.txt"; then
  echo "PASS"
else
  die "FAIL"
fi