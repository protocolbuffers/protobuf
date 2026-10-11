This directory contains one directory per crate that we release.

Each directory contains a pkg_tar() which packages up the crate into a tar which
can be pushed to crates.io.

New development happens in the `google_protobuf*` crates. The legacy
`protobuf*` crates are kept as re-exports of their `google_protobuf*`
equivalents so that existing users keep building, but they set
`publish = false` and are no longer pushed to crates.io.