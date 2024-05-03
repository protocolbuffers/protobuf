#!/bin/bash

${TEST_SRCDIR}/google3/third_party/protobuf/objectivec/pddm \
  --dry-run \
  ${TEST_SRCDIR}/google3/third_party/protobuf/objectivec/*.[hm] \
  ${TEST_SRCDIR}/google3/third_party/protobuf/objectivec/Tests/*.[hm] \
  || die "Update by running: objectivec/DevTools/pddm.py objectivec/*.[hm] objectivec/Tests/*.[hm]"

echo "PASS"
