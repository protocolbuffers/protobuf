#!/bin/bash

die() {
    echo "Error: $1"
    exit 1
}

objectivec/DevTools/pddm.py \
  --dry-run \
  objectivec/*.[hm] \
  objectivec/Tests/*.[hm] \
  || die "Update by running: objectivec/DevTools/pddm.py objectivec/*.[hm] objectivec/Tests/*.[hm]"

echo "PASS"
