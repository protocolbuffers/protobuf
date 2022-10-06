#!/bin/bash

set -e

if [[ "$KOKORO_JOB_NAME" =~ "java" ]]; then
  FILTER="^java/.*"
else
  FILTER="^src/.*"
fi

# Find the current branch we're merging into based on the kokoro job.
BRANCH=$(echo $KOKORO_JOB_NAME | sed -r 's,.*protobuf/github/([a-zA-Z0-9.]+)/.*,\1,')
if [ "$BRANCH" = "master" ]; then
  BRANCH="main"
fi

# Find the set of affected files.
AFFECTED_FILES=$(git log --name-only --pretty=format: $BRANCH..HEAD)

for FILE in $AFFECTED_FILES; do
  if [[ "$FILE" =~ "$FILTER" ]]; then
    echo "Found file affecting $KOKORO_JOB_NAME: $FILE"
    AFFECTED=1
  fi
done

if [ -z "$AFFECTED" ]; then
  echo "Modified files do not affect $KOKORO_JOB_NAME, skipping build..."
  exit 1
fi
