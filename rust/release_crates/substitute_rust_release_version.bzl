"""A rule to textually replace {{VERSION}} with the Rust release version in files."""

load("//:protobuf_version.bzl", "PROTOBUF_RUST_VERSION")

def _get_minor_version(version_str):
    parts = version_str.split(".")
    if len(parts) < 2:
        return 0
    return int(parts[1])

_minor_version = _get_minor_version(PROTOBUF_RUST_VERSION)

# Temporarily append a -release suffix to non-RC versions until we consider the
# release stable. Since "release" lexicographically comes after "rc", Cargo
# will understand that 4.31.0-release is newer than all 4.31.0-rc.N releases.
# Remove the -release tag only if the minor version number is >= 36.
_should_append_release = _minor_version < 36 and PROTOBUF_RUST_VERSION.find("-rc") == -1
PROTOBUF_RUST_VERSION_SUFFIX = "-release" if _should_append_release else ""

def substitute_rust_release_version(src, out, name = None):
    version = PROTOBUF_RUST_VERSION + PROTOBUF_RUST_VERSION_SUFFIX
    native.genrule(
        name = name or ("gen_%s" % out),
        srcs = [src],
        outs = [out],
        cmd = "cat $(SRCS) | sed -e 's/{{VERSION}}/{0}/g' > $(OUTS)".format(version),
    )
