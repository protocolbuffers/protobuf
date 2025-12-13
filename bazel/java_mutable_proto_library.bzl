"""java_mutable_proto_library rule"""

load("//bazel/private/google:java_mutable_proto_library_hasservices.bzl", _java_mutable_proto_library_hasservices = "java_mutable_proto_library")
load("//bazel/private/google:java_mutable_proto_library_noservices.bzl", _java_mutable_proto_library_noservices = "java_mutable_proto_library")
load("//devtools/build_cleaner/skylark:build_defs.bzl", "register_extension_info")

def java_mutable_proto_library(**kwargs):
    if "has_services" not in kwargs or kwargs["has_services"]:
        _java_mutable_proto_library_hasservices(**kwargs)
    else:
        _java_mutable_proto_library_noservices(**kwargs)

register_extension_info(
    extension = java_mutable_proto_library,
    label_regex_for_dep = "{extension_name}",
)
