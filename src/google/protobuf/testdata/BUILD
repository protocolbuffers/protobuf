filegroup(
    name = "testdata",
    srcs = glob(["**"]),
    visibility = [
        # Public, but Protobuf only visibility.
        "//:__subpackages__",
    ],
)
