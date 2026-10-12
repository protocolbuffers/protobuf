"""Tests for cc_binary_deps."""

load("@rules_testing//lib:truth.bzl", "matching")
load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "get_cc_info_artifacts")

def _test_proto_library_direct_files_deps(env, target):
    artifacts = get_cc_info_artifacts(target)

    # Should not depend on APIs for other languages or API versions.
    env.expect.that_collection(artifacts.direct_files + artifacts.static_libs + artifacts.pic_static_libs).not_contains_predicate(
        matching.any(
            matching.file_path_matches("*/net/proto/libproto.a"),
            matching.file_path_matches("*/net/proto/pyproto.py"),
            matching.file_path_matches("*/java/com/google/io/protocol/libprotocol.jar"),
            # Also should not depend on RPC APIs.
            matching.file_path_matches("*/net/rpc/libstubby12_proto_rpc_libs.a"),
        ),
    )

    # Should depend on C++ outputs (pb.cc).
    env.expect.that_collection(artifacts.direct_files_in_package).transform(
        filter = matching.any(
            matching.file_path_matches("*.pb.cc"),
        ),
    ).contains_exactly_predicates([
        matching.file_path_matches("/foo.pb.cc"),
        matching.file_path_matches("/bar.pb.cc"),
    ])

def _test_proto_library_headers_deps(env, target):
    artifacts = get_cc_info_artifacts(target)

    # Should depend on C++ outputs (pb.h).
    env.expect.that_collection(artifacts.compilation_context_headers_in_package).transform(
        filter = matching.file_path_matches("*.pb.h"),
    ).contains_at_least_predicates([
        matching.file_path_matches("/foo.pb.h"),
        matching.file_path_matches("/bar.pb.h"),
        matching.file_path_matches("/baz.pb.h"),
    ])

def _test_proto_library_static_libs_deps(env, target):
    artifacts = get_cc_info_artifacts(target)

    # Should depend on C++ static libraries (PIC or non-PIC depending on toolchain/platform).
    env.expect.that_collection(artifacts.static_libs_in_package + artifacts.pic_static_libs_in_package).contains_at_least_predicates([
        matching.any(
            matching.file_path_matches("/libfoo_proto.a"),
            matching.file_path_matches("/libfoo_proto.pic.a"),
            matching.file_path_matches("/foo_proto.lib"),
        ),
        matching.any(
            matching.file_path_matches("/libbaz_proto.a"),
            matching.file_path_matches("/libbaz_proto.pic.a"),
            matching.file_path_matches("/baz_proto.lib"),
        ),
    ])

def _test_proto_library_linker_files_deps(env, target):
    artifacts = get_cc_info_artifacts(target)

    # Should depend on C++ object outputs (PIC or non-PIC depending on toolchain/platform).
    env.expect.that_collection(artifacts.linker_files_in_package).transform(
        filter = matching.any(
            matching.file_path_matches("*.pb.o"),
            matching.file_path_matches("*.pb.pic.o"),
            matching.file_path_matches("*.pb.obj"),
        ),
    ).contains_at_least_predicates([
        matching.any(
            matching.file_path_matches("/_objs/foo_proto/foo.pb.o"),
            matching.file_path_matches("/_objs/foo_proto/foo.pb.pic.o"),
            matching.file_path_matches("/_objs/foo_proto/foo.pb.obj"),
        ),
        matching.any(
            matching.file_path_matches("/_objs/foo_proto/bar.pb.o"),
            matching.file_path_matches("/_objs/foo_proto/bar.pb.pic.o"),
            matching.file_path_matches("/_objs/foo_proto/bar.pb.obj"),
        ),
        matching.any(
            matching.file_path_matches("/_objs/baz_proto/baz.pb.o"),
            matching.file_path_matches("/_objs/baz_proto/baz.pb.pic.o"),
            matching.file_path_matches("/_objs/baz_proto/baz.pb.obj"),
        ),
    ])

def _test_proto_library_compiler_deps(env, target):
    # Should depend on compiler.
    env.expect.that_target(target).action_named("GenProto").inputs().contains_predicate(
        matching.any(
            matching.file_path_matches("*/net/proto2/compiler/public/protocol_compiler"),
            matching.file_path_matches("*/protoc_minimal"),
            matching.file_path_matches("*/protoc"),
            matching.file_path_matches("*/protoc.exe"),
        ),
    )

def _test_cc_binary_linker_files_deps(env, target):
    package = target.label.package
    inputs = env.expect.that_target(target).action_named("CppLink").inputs()

    # Should depend on C++ static libraries or object files.
    inputs.contains_predicate(
        matching.any(
            matching.file_path_matches(package + "/libfoo_proto.a"),
            matching.file_path_matches(package + "/libfoo_proto.pic.a"),
            matching.file_path_matches(package + "/foo_proto.lib"),
            matching.file_path_matches(package + "/_objs/foo_proto/foo.pb"),
        ),
    )
    inputs.contains_predicate(
        matching.any(
            matching.file_path_matches(package + "/libbaz_proto.a"),
            matching.file_path_matches(package + "/libbaz_proto.pic.a"),
            matching.file_path_matches(package + "/baz_proto.lib"),
            matching.file_path_matches(package + "/_objs/baz_proto/baz.pb"),
        ),
    )

    # Should not depend on APIs for other languages or RPC APIs.
    inputs.not_contains_predicate(
        matching.any(
            matching.file_path_matches("*/net/proto/libproto.a"),
            matching.file_path_matches("*/net/proto/pyproto.py"),
            matching.file_path_matches("*/java/com/google/io/protocol/libprotocol.jar"),
            matching.file_path_matches("*/net/rpc/libstubby12_proto_rpc_libs.a"),
        ),
    )

def _test_proto_library_shared_libs_deps(env, target):
    artifacts = get_cc_info_artifacts(target)

    # Should not depend on shared libs when dynamic linker support is disabled.
    env.expect.that_collection(artifacts.shared_libs_in_package).not_contains_predicate(
        matching.file_path_matches("*.so"),
    )

TESTS = {
    ":foo_cc_proto": [
        _test_proto_library_direct_files_deps,
        _test_proto_library_headers_deps,
        _test_proto_library_static_libs_deps,
        _test_proto_library_linker_files_deps,
    ],
    ":foo_proto": [
        _test_proto_library_compiler_deps,
    ],
    ":cctest_binary": [_test_cc_binary_linker_files_deps],
}

TESTS_NO_DYNAMIC_LINKER = {
    ":foo_cc_proto": [
        _test_proto_library_shared_libs_deps,
    ],
}
