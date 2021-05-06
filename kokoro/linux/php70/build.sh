#!/bin/bash
#
# This is the entry point for kicking off a Kokoro job. This path is referenced
# from the .cfg files in this directory.

exec ../test_php.sh gcr.io/protobuf-build/php/linux:7.0.33-dbg-14a06550010c0649bf69b6c9b803c1ca609bbb6d
