# Description:
#   An example package that contains nested protos that are imported from
#   __init__.py. See testPackageInitializationImport in reflection_test.py for
#   details.

package(
    default_visibility = ["//net/proto2/python/internal:__pkg__"],
)

proto_library(
    name = "inner_proto",
    srcs = ["inner.proto"],
    py_api_version = 2,
)

proto_library(
    name = "outer_proto",
    srcs = ["outer.proto"],
    py_api_version = 2,
    deps = [":inner_proto"],
)

py_library(
    name = "import_test_package",
    srcs = ["__init__.py"],
    deps = [":outer_proto"],
)
