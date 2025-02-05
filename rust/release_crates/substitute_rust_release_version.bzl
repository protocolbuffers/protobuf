"""A rule to textually replace {{VERSION}} with the Rust release version in files."""

load("//:protobuf_version.bzl", "PROTOBUF_RUST_VERSION")

# Temporarily append a prerelease suffix to our versions until we consider the release stable.
PROTOBUF_RUST_VERSION_WITH_BETA_SUFFIX = PROTOBUF_RUST_VERSION + "-beta2"

def substitute_rust_release_version(src, out, name = None):
    version = PROTOBUF_RUST_VERSION_WITH_BETA_SUFFIX
    native.genrule(
        name = name or ("gen_%s" % out),
        srcs = [src],
        outs = [out],
        cmd = "cat $(SRCS) | sed -e 's/{{VERSION}}/{0}/g' > $(OUTS)".format(version),
    )
