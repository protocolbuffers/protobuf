#!/bin/bash
#
# This is the entry point for kicking off a Kokoro job. This path is referenced
# from the .cfg files in this directory.

set -ex

cd $(dirname $0)

# Most of our tests use a debug build of PHP, but we do one build against an opt
# php just in case that surfaces anything unexpected.
../test_php.sh gcr.io/protobuf-build/php/linux:8.0.5-14a06550010c0649bf69b6c9b803c1ca609bbb6d

../test_php.sh gcr.io/protobuf-build/php/linux:7.0.33-dbg-14a06550010c0649bf69b6c9b803c1ca609bbb6d
../test_php.sh gcr.io/protobuf-build/php/linux:7.3.28-dbg-14a06550010c0649bf69b6c9b803c1ca609bbb6d
../test_php.sh gcr.io/protobuf-build/php/linux:7.4.18-dbg-14a06550010c0649bf69b6c9b803c1ca609bbb6d
../test_php.sh gcr.io/protobuf-build/php/linux:8.0.5-dbg-14a06550010c0649bf69b6c9b803c1ca609bbb6d
