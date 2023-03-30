#!/bin/bash

# This script updates src/file_lists.cmake and the checked-in generated code
# for the well-known types, commits the resulting changes, and pushes them.
# This does not do anything useful when run manually, but should be run by our
# GitHub action instead.

set -ex

# Cd to the repo root.
cd $(dirname -- "$0")/..

previous_commit_title=$(git log -1 --pretty='%s')

# Exit early if the previous commit was auto-generated. This reduces the risk
# of a bug causing an infinite loop of auto-generated commits.
if (echo "$previous_commit_title" | grep -q "^Auto-generate files"); then
  echo "Previous commit was auto-generated"
  exit 0
fi

./regenerate_stale_files.sh

# Try to determine the most recent CL or pull request.
pr_from_merge=$(echo "$previous_commit_title" | sed -n 's/^Merge pull request #\([0-9]\+\).*/\1/p')
pr_from_squash=$(echo "$previous_commit_title" | sed -n 's/^.*(#\([0-9]\+\))$/\1/p')
cl=$(git log -1 --pretty='%b' | sed -n 's/^PiperOrigin-RevId: \([0-9]*\)$/\1/p')

if [ ! -z "$pr_from_merge" ]; then
  commit_message="Auto-generate files after PR #$pr_from_merge"
elif [ ! -z "$pr_from_squash" ]; then
  commit_message="Auto-generate files after PR #$pr_from_squash"
elif [ ! -z "$cl" ]; then
  commit_message="Auto-generate files after cl/$cl"
else
  # If we are unable to determine the CL or pull request number, we fall back
  # on this default commit message. Typically this should not occur, but could
  # happen if a pull request was merged via a rebase.
  commit_message="Auto-generate files"
fi

git add -A
git diff --staged --quiet || git commit -am "$commit_message"
git pull --rebase
git push || echo "Conflicting commit hit, retrying in next job..."
