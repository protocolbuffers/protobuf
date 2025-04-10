"""A rule to textually replace {{VERSION}} with the Rust release version in files."""

load("//:protobuf_version.bzl", "PROTOBUF_RUST_VERSION")

# Temporarily append a -release suffix to non-RC versions until we consider the
# release stable. Since "release" lexicographically comes after "rc", Cargo
# will understand that 4.31.0-release is newer than all 4.31.0-rc.N releases.
PROTOBUF_RUST_VERSION_SUFFIX = "-release" if PROTOBUF_RUST_VERSION.find("-rc") == -1 else ""

def substitute_rust_release_version(src, out, name = None):
    version = PROTOBUF_RUST_VERSION + PROTOBUF_RUST_VERSION_SUFFIX
    native.genrule(
        name = name or ("gen_%s" % out),
        srcs = [src],
        outs = [out],
        cmd = "cat $(SRCS) | sed -e 's/{{VERSION}}/{0}/g' > $(OUTS)".format(version),
    )
