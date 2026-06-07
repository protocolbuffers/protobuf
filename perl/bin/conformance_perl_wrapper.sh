#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH="$DIR/..:$LD_LIBRARY_PATH"
exec perl -I"$DIR/../blib/lib" -I"$DIR/../blib/arch" -I"$DIR/../local/lib" "$DIR/conformance_perl.pl"
