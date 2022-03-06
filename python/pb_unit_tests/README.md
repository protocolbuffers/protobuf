
# Protobuf Unit Tests

This directory contains wrappers around the Python unit tests defined in
the protobuf repo.  Python+upb is intended to be a drop-in replacement for
protobuf Python, so we should be able to pass the same set of unit tests.

Our wrappers contain exclusion lists for tests we know we are not currently
passing.  Ideally these exclusion lists will become empty once Python+upb is
fully implemented.  However there may be a few edge cases that we decide
are not worth matching with perfect parity.
