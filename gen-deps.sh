#!/bin/sh
# This script wraps gcc -MM, which unhelpfully strips the directory
# off of the input filename.  In other words, if you run:
#
# $ gcc -MM src/upb_parse.c
#
# ...the emitted dependency information looks like:
#
# upb_parse.o: src/upb_parse.h [...]
#
# Since upb_parse.o is actually in src, the dependency information is
# not used.  To remedy this, we use the -MT flag (see gcc docs).

set -e
rm -f deps
for file in $@; do
  gcc -MM $file -MT ${file%.*}.o -DUPB_THREAD_UNSAFE -Idescriptor -Isrc -I. >> deps
done
