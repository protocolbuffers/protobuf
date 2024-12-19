"""A rule to textually replace {{VERSION}} with the Rust release version in files."""

load("//:protobuf_version.bzl", "PROTOBUF_RUST_VERSION")

def substitute_rust_release_version(name = None, src, out):
    version = PROTOBUF_RUST_VERSION + "-beta2"
    native.genrule(
        name = name or ("gen_%s" % out),
        srcs = [src],
        outs = [out],
        cmd = "cat $(SRCS) | sed -e 's/{{VERSION}}/{0}/g' > $(OUTS)".format(version),
    )
