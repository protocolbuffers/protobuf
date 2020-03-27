workspace(name = "com_google_protobuf_examples")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# This com_google_protobuf repository is required for proto_library rule.
# It provides the protocol compiler binary (i.e., protoc).
#
# We declare it as local_repository so we can test changes
# before they get merged. You'll want to use the following instead:
#
# http_archive(
#     name = "com_google_protobuf",
#     strip_prefix = "protobuf-master",
#     urls = ["https://github.com/protocolbuffers/protobuf/archive/master.zip"],
# )
local_repository(
    name = "com_google_protobuf",
    path = "..",
)

# Similar to com_google_protobuf but for Java lite. If you are building
# for Android, the lite version should be preferred because it has a much
# smaller code size.
local_repository(
    name = "com_google_protobuf_javalite",
    path = "..",
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()
