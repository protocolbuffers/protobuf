# Protobuf Python runtime
#
# See also code generation logic under /src/google/protobuf/compiler/python.
#
# Most users should depend upon public aliases in the root:
#   //:protobuf_python
#   //:well_known_types_py_pb2

load("@rules_pkg//:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_python//python:defs.bzl", "py_library")
load("//:protobuf.bzl", "internal_py_proto_library")
load("//build_defs:arch_tests.bzl", "aarch64_test", "x86_64_test")
load("//build_defs:cpp_opts.bzl", "COPTS")
load("//conformance:defs.bzl", "conformance_test")
load("//src/google/protobuf/editions:defaults.bzl", "compile_edition_defaults", "embed_edition_defaults")
load(":internal.bzl", "internal_copy_files", "internal_py_test")

def build_targets(name):
    """
    Declares the build targets of the //python package.

    Args:
      name: unused.
    """
    py_library(
        name = "protobuf_python",
        data = select({
            "//conditions:default": [],
            ":use_fast_cpp_protos": [
                ":google/protobuf/internal/_api_implementation.so",
                ":google/protobuf/pyext/_message.so",
            ],
        }),
        visibility = ["//:__pkg__"],
        deps = [
            ":python_srcs",
            ":well_known_types_py_pb2",
        ],
    )

    native.config_setting(
        name = "use_fast_cpp_protos",
        values = {
            "define": "use_fast_cpp_protos=true",
        },
    )

    internal_py_proto_library(
        name = "well_known_types_py_pb2",
        srcs = [":copied_wkt_proto_files"],
        include = ".",
        default_runtime = "",
        protoc = "//:protoc",
        srcs_version = "PY2AND3",
        visibility = [
            "//:__pkg__",
            "//upb:__subpackages__",
        ],
    )

    internal_copy_files(
        name = "copied_wkt_proto_files",
        srcs = [
            "//:well_known_type_protos",
            "//src/google/protobuf:descriptor_proto_srcs",
            "//src/google/protobuf/compiler:plugin.proto",
        ],
        strip_prefix = "src",
    )

    native.cc_binary(
        name = "google/protobuf/internal/_api_implementation.so",
        srcs = ["google/protobuf/internal/api_implementation.cc"],
        copts = COPTS + [
            "-DPYTHON_PROTO2_CPP_IMPL_V2",
        ],
        linkshared = 1,
        linkstatic = 1,
        tags = [
            # Exclude this target from wildcard expansion (//...) because it may
            # not even be buildable. It will be built if it is needed according
            # to :use_fast_cpp_protos.
            # https://docs.bazel.build/versions/master/be/common-definitions.html#common-attributes
            "manual",
        ],
        deps = select({
            "//conditions:default": [],
            ":use_fast_cpp_protos": ["//external:python_headers"],
        }),
    )

    native.config_setting(
        name = "allow_oversize_protos",
        values = {
            "define": "allow_oversize_protos=true",
        },
    )

    native.cc_binary(
        name = "google/protobuf/pyext/_message.so",
        srcs = native.glob([
            "google/protobuf/pyext/*.cc",
            "google/protobuf/pyext/*.h",
        ]),
        copts = COPTS + [
            "-DGOOGLE_PROTOBUF_HAS_ONEOF=1",
        ] + select({
            "//conditions:default": [],
            ":allow_oversize_protos": ["-DPROTOBUF_PYTHON_ALLOW_OVERSIZE_PROTOS=1"],
        }),
        includes = ["."],
        linkshared = 1,
        linkstatic = 1,
        tags = [
            # Exclude this target from wildcard expansion (//...) because it may
            # not even be buildable. It will be built if it is needed according
            # to :use_fast_cpp_protos.
            # https://docs.bazel.build/versions/master/be/common-definitions.html#common-attributes
            "manual",
        ],
        deps = [
            ":proto_api",
            "//:protobuf",
        ] + select({
            "//conditions:default": [],
            ":use_fast_cpp_protos": ["//external:python_headers"],
        }),
    )

    aarch64_test(
        name = "aarch64_test",
        bazel_binaries = [
            "google/protobuf/internal/_api_implementation.so",
            "google/protobuf/pyext/_message.so",
        ],
    )

    x86_64_test(
        name = "x86_64_test",
        bazel_binaries = [
            "google/protobuf/internal/_api_implementation.so",
            "google/protobuf/pyext/_message.so",
        ],
    )

    compile_edition_defaults(
        name = "python_edition_defaults",
        srcs = ["//:descriptor_proto"],
        maximum_edition = "2023",
        minimum_edition = "PROTO2",
    )

    embed_edition_defaults(
        name = "embedded_python_edition_defaults_generate",
        defaults = "python_edition_defaults",
        output = "google/protobuf/internal/python_edition_defaults.py",
        placeholder = "DEFAULTS_VALUE",
        template = "google/protobuf/internal/python_edition_defaults.py.template",
    )

    native.filegroup(
        name = "python_src_files",
        srcs = native.glob(
            [
                "google/protobuf/**/*.py",
            ],
            exclude = [
                "google/protobuf/internal/*_test.py",
                "google/protobuf/internal/test_util.py",
                "google/protobuf/internal/import_test_package/__init__.py",
            ],
        ) + ["google/protobuf/internal/python_edition_defaults.py"],
    )

    py_library(
        name = "python_srcs",
        srcs = [":python_src_files"],
        imports = ["python"],
        srcs_version = "PY2AND3",
        visibility = [
            "//:__pkg__",
            "//upb:__subpackages__",
        ],
    )

    py_library(
        name = "python_test_srcs",
        srcs = native.glob([
            "google/protobuf/internal/*_test.py",
        ]) + [
            "google/protobuf/internal/import_test_package/__init__.py",
            "google/protobuf/internal/test_util.py",
            "//python/google/protobuf/internal/numpy:__init__.py",
            "//python/google/protobuf/internal/numpy:numpy_test.py",
        ],
        imports = ["python"],
        srcs_version = "PY3",
        visibility = [
            "//:__pkg__",
            "//upb:__subpackages__",
        ],
    )

    ################################################################################
    # Tests
    ################################################################################

    internal_copy_files(
        name = "copied_test_proto_files",
        testonly = 1,
        srcs = [
            "//:test_proto_srcs",
            "//src/google/protobuf/util:test_proto_srcs",
        ],
        strip_prefix = "src",
    )

    internal_copy_files(
        name = "copied_conformance_test_files",
        testonly = 1,
        srcs = [
            "//src/google/protobuf:test_messages_proto2.proto",
            "//src/google/protobuf:test_messages_proto3.proto",
            "//src/google/protobuf/editions:golden/test_messages_proto2_editions.proto",
            "//src/google/protobuf/editions:golden/test_messages_proto3_editions.proto",
        ],
        strip_prefix = "src",
    )

    internal_py_proto_library(
        name = "python_common_test_protos",
        testonly = 1,
        srcs = [":copied_test_proto_files"],
        include = ".",
        default_runtime = "",
        protoc = "//:protoc",
        srcs_version = "PY2AND3",
        visibility = ["//:__pkg__"],
        deps = [":well_known_types_py_pb2"],
    )

    internal_py_proto_library(
        name = "python_specific_test_protos",
        testonly = 1,
        srcs = native.glob([
            "google/protobuf/internal/*.proto",
            "google/protobuf/internal/import_test_package/*.proto",
        ]),
        include = ".",
        default_runtime = ":protobuf_python",
        protoc = "//:protoc",
        srcs_version = "PY2AND3",
        visibility = ["//:__pkg__"],
        deps = [":python_common_test_protos"],
    )

    internal_py_proto_library(
        name = "conformance_test_py_proto",
        testonly = 1,
        srcs = [":copied_conformance_test_files"],
        include = ".",
        default_runtime = "//:protobuf_python",
        protoc = "//:protoc",
        visibility = [
            "//conformance:__pkg__",
            "//python:__subpackages__",
        ],
        deps = [":well_known_types_py_pb2"],
    )

    py_library(
        name = "python_test_lib",
        testonly = 1,
        srcs = [
            "google/protobuf/internal/import_test_package/__init__.py",
            "google/protobuf/internal/test_util.py",
        ],
        imports = ["python"],
        srcs_version = "PY2AND3",
        visibility = ["//python:__subpackages__"],
        deps = [
            ":protobuf_python",
            ":python_common_test_protos",
            ":python_specific_test_protos",
        ],
    )

    internal_py_test(
        name = "descriptor_database_test",
        srcs = ["google/protobuf/internal/descriptor_database_test.py"],
    )

    internal_py_test(
        name = "descriptor_pool_test",
        srcs = ["google/protobuf/internal/descriptor_pool_test.py"],
    )

    internal_py_test(
        name = "descriptor_test",
        srcs = ["google/protobuf/internal/descriptor_test.py"],
    )

    internal_py_test(
        name = "field_mask_test",
        srcs = ["google/protobuf/internal/field_mask_test.py"],
    )

    internal_py_test(
        name = "generator_test",
        srcs = ["google/protobuf/internal/generator_test.py"],
    )

    internal_py_test(
        name = "import_test",
        srcs = ["google/protobuf/internal/import_test.py"],
    )

    internal_py_test(
        name = "json_format_test",
        srcs = ["google/protobuf/internal/json_format_test.py"],
    )

    internal_py_test(
        name = "keywords_test",
        srcs = ["google/protobuf/internal/keywords_test.py"],
    )

    internal_py_test(
        name = "message_factory_test",
        srcs = ["google/protobuf/internal/message_factory_test.py"],
    )

    internal_py_test(
        name = "message_test",
        srcs = ["google/protobuf/internal/message_test.py"],
        data = ["//src/google/protobuf:testdata"],
    )

    internal_py_test(
        name = "proto_builder_test",
        srcs = ["google/protobuf/internal/proto_builder_test.py"],
    )

    internal_py_test(
        name = "reflection_test",
        srcs = ["google/protobuf/internal/reflection_test.py"],
    )

    internal_py_test(
        name = "service_reflection_test",
        srcs = ["google/protobuf/internal/service_reflection_test.py"],
    )

    internal_py_test(
        name = "symbol_database_test",
        srcs = ["google/protobuf/internal/symbol_database_test.py"],
    )

    internal_py_test(
        name = "text_encoding_test",
        srcs = ["google/protobuf/internal/text_encoding_test.py"],
    )

    internal_py_test(
        name = "text_format_test",
        srcs = ["google/protobuf/internal/text_format_test.py"],
        data = ["//src/google/protobuf:testdata"],
    )

    internal_py_test(
        name = "unknown_fields_test",
        srcs = ["google/protobuf/internal/unknown_fields_test.py"],
    )

    internal_py_test(
        name = "well_known_types_test",
        srcs = ["google/protobuf/internal/well_known_types_test.py"],
    )

    internal_py_test(
        name = "wire_format_test",
        srcs = ["google/protobuf/internal/wire_format_test.py"],
    )

    native.cc_library(
        name = "proto_api",
        hdrs = ["google/protobuf/proto_api.h"],
        visibility = ["//visibility:public"],
        deps = [
            "//external:python_headers",
        ],
    )

    internal_py_test(
        name = "python_version_test",
        srcs = ["python_version_test.py"],
    )

    conformance_test(
        name = "conformance_test",
        env = {"PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION": "python"},
        failure_list = "//conformance:failure_list_python.txt",
        target_compatible_with = select({
            "@system_python//:none": ["@platforms//:incompatible"],
            ":use_fast_cpp_protos": ["@platforms//:incompatible"],
            "//conditions:default": [],
        }),
        maximum_edition = "2023",
        testee = "//conformance:conformance_python",
        text_format_failure_list = "//conformance:text_format_failure_list_python.txt",
    )

    # Note: this requires --define=use_fast_cpp_protos=true
    conformance_test(
        name = "conformance_test_cpp",
        env = {"PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION": "cpp"},
        failure_list = "//conformance:failure_list_python.txt",
        target_compatible_with = select({
            "@system_python//:none": ["@platforms//:incompatible"],
            ":use_fast_cpp_protos": [],
            "//conditions:default": ["@platforms//:incompatible"],
        }),
        maximum_edition = "2023",
        testee = "//conformance:conformance_python",
        text_format_failure_list = "//conformance:text_format_failure_list_python_cpp.txt",
    )

    conformance_test(
        name = "conformance_test_upb",
        env = {"PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION": "upb"},
        failure_list = "//conformance:failure_list_python_upb.txt",
        target_compatible_with = select({
            "@system_python//:none": ["@platforms//:incompatible"],
            ":use_fast_cpp_protos": ["@platforms//:incompatible"],
            "//conditions:default": [],
        }),
        maximum_edition = "2023",
        testee = "//conformance:conformance_python",
        text_format_failure_list = "//conformance:text_format_failure_list_python_upb.txt",
    )

    ################################################################################
    # Distribution files
    ################################################################################

    pkg_files(
        name = "python_source_files",
        srcs = [
            ":python_src_files",
            "README.md",
            "google/__init__.py",
            "setup.cfg",
        ],
        strip_prefix = "",
        visibility = ["//python/dist:__pkg__"],
    )

    pkg_files(
        name = "dist_files",
        srcs = native.glob([
            "google/**/*.proto",
            "google/**/*.py",
            "google/protobuf/internal/*.cc",
            "google/protobuf/pyext/*.cc",
            "google/protobuf/pyext/*.h",
        ]) + [
            "BUILD.bazel",
            "MANIFEST.in",
            "README.md",
            "build_targets.bzl",
            "google/protobuf/proto_api.h",
            "google/protobuf/pyext/README",
            "google/protobuf/python_protobuf.h",
            "internal.bzl",
            "python_version_test.py",
            "setup.cfg",
            "setup.py",
        ],
        strip_prefix = strip_prefix.from_root(""),
        visibility = ["//pkg:__pkg__"],
    )
