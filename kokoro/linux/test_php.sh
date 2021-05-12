#!/bin/bash

set -ex

test -t 1 && USE_TTY="-it" 
docker run ${USE_TTY} -v$(realpath $(dirname $0)/../..):/workspace $1 "composer test && composer test_c"
