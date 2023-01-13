#!/bin/bash
#
# This is the entry point for kicking off a Kokoro job. This path is referenced
# from the .cfg files in this directory.

set -ex

# Change to repo base.
cd $(dirname $0)/../../..

gcloud components update --quiet
gcloud auth configure-docker us-docker.pkg.dev --quiet

docker run $(test -t 0 && echo "-it") -v$PWD:/workspace us-docker.pkg.dev/protobuf-build/containers/test/linux/php:8.0.5-dbg-86edd58c4f987dda7d090d4eca8ac17170353553 "composer test_valgrind"

docker run $(test -t 0 && echo "-it") -v$PWD:/workspace us-docker.pkg.dev/protobuf-build/containers/test/linux/php:7.3.28-dbg-86edd58c4f987dda7d090d4eca8ac17170353553 "composer test && composer test_c"
docker run $(test -t 0 && echo "-it") -v$PWD:/workspace us-docker.pkg.dev/protobuf-build/containers/test/linux/php:7.4.18-dbg-86edd58c4f987dda7d090d4eca8ac17170353553 "composer test && composer test_c"
docker run $(test -t 0 && echo "-it") -v$PWD:/workspace us-docker.pkg.dev/protobuf-build/containers/test/linux/php:8.0.5-dbg-86edd58c4f987dda7d090d4eca8ac17170353553 "composer test && composer test_c"

# Run specialized memory leak & multirequest tests.
docker run $(test -t 0 && echo "-it") -v$PWD:/workspace us-docker.pkg.dev/protobuf-build/containers/test/linux/php:8.0.5-dbg-86edd58c4f987dda7d090d4eca8ac17170353553 "composer test_c && tests/multirequest.sh && tests/memory_leak_test.sh"

# Most of our tests use a debug build of PHP, but we do one build against an opt
# php just in case that surfaces anything unexpected.
docker run $(test -t 0 && echo "-it") -v$PWD:/workspace us-docker.pkg.dev/protobuf-build/containers/test/linux/php:8.0.5-86edd58c4f987dda7d090d4eca8ac17170353553 "composer test && composer test_c"
