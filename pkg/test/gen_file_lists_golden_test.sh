#!/bin/bash

set -ex

if cmp "$1" "$2"; then
  # Files are identical, as expected.
  exit 0;
else
  echo "Golden file $1 does not match generated output $2:"
  echo "--------------------------------------------------"
  diff -u "$1" "$2"
  echo "--------------------------------------------------"
  exit 1
fi
