#!/bin/bash
for file in $(find . -type f); do
  if [ "$(head -c 2 $file)" == "#!" ]; then
    chmod u+x $file
  else
    chmod a-x $file
  fi
done
