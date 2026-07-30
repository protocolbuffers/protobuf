"""Unit tests for proto_path_mapping callbacks."""

load("@bazel_skylib//lib:unittest.bzl", "asserts", "unittest")
load(
    "//bazel/common:proto_path_mapping.bzl",
    "proto_path_mapping",
)

_import_virtual = proto_path_mapping.import_virtual_from_file
_import_external = proto_path_mapping.import_external_repo_from_file
_import_main = proto_path_mapping.import_main_output_from_file
_import_proto_path = proto_path_mapping.import_proto_path_from_file
_bucket_sources = proto_path_mapping.bucket_transitive_sources

# Fake root paths similar to what Blaze generates.
_ROOT_PATH = "bazel-out/k8-fastbuild/bin"
_ALT_ROOT = "bazel-out/foo/k8-fastbuild/bin"

def _make_file(path, root_path = ""):
    """Create a File-like struct for testing."""
    return struct(path = path, root = struct(path = root_path))

def _import_virtual_from_file_test(ctx):
    """Test virtual imports classification against original docstring examples and language rules.

    Original docstring examples across the three functions in order:
      1. 'bazel-out/k8-fastbuild/bin/external/foo/e/_virtual_imports/e' (from _import_virtual_proto_path)
      2. 'bazel-out/foo/k8-fastbuild/bin/e/_virtual_imports/e'          (from _import_virtual_proto_path)
      3. 'bazel-out/k8-fastbuild/bin/external/foo'                       (from _import_repo_proto_path)
      4. 'bazel-out/foo/k8-fastbuild/bin'                               (from _import_repo_proto_path)
      5. 'bazel-out/k8-fastbuild/bin'                                    (from _import_main_output_proto_path)
      6. 'external/foo'                                                  (from _import_main_output_proto_path)
      7. '../foo'                                                        (from _import_main_output_proto_path)
    """
    env = unittest.begin(ctx)

    # 1. 'bazel-out/k8-fastbuild/bin/external/foo/e/_virtual_imports/e'
    virtual_ext = _make_file(
        _ROOT_PATH + "/external/foo/e/_virtual_imports/e/bar.proto",
        _ROOT_PATH + "/external/foo/e",
    )
    asserts.equals(
        env,
        "-I" + _ROOT_PATH + "/external/foo/e/_virtual_imports/e",
        _import_virtual(virtual_ext),
    )

    # 2. 'bazel-out/foo/k8-fastbuild/bin/e/_virtual_imports/e'
    virtual_alt = _make_file(
        _ALT_ROOT + "/e/_virtual_imports/e/bar.proto",
        _ALT_ROOT + "/e",
    )
    asserts.equals(
        env,
        "-I" + _ALT_ROOT + "/e/_virtual_imports/e",
        _import_virtual(virtual_alt),
    )

    # 3. 'bazel-out/k8-fastbuild/bin/external/foo' -> None
    ext_gen = _make_file(
        _ROOT_PATH + "/external/foo/bar.proto",
        _ROOT_PATH + "/external/foo",
    )
    asserts.equals(env, None, _import_virtual(ext_gen))

    # 4. 'bazel-out/foo/k8-fastbuild/bin' -> None
    gen_alt = _make_file(
        _ALT_ROOT + "/cloud/foo/bar.proto",
        _ALT_ROOT,
    )
    asserts.equals(env, None, _import_virtual(gen_alt))

    # 5. 'bazel-out/k8-fastbuild/bin' -> None
    gen_main = _make_file(
        _ROOT_PATH + "/cloud/foo/bar.proto",
        _ROOT_PATH,
    )
    asserts.equals(env, None, _import_virtual(gen_main))

    # 6. 'external/foo' -> None
    asserts.equals(
        env,
        None,
        _import_virtual(_make_file("external/foo/bar.proto", "")),
    )

    # 7. '../foo' -> None
    asserts.equals(
        env,
        None,
        _import_virtual(_make_file("../foo/bar.proto", "")),
    )

    # Examples from language proto rules:
    # From py_proto_library.bzl: Python generated _pb2.py file -> None
    asserts.equals(env, None, _import_virtual(_make_file(_ROOT_PATH + "/cloud/foo/bar_pb2.py", _ROOT_PATH)))

    # From java_proto_library.bzl: Java speed source jar -> None
    asserts.equals(env, None, _import_virtual(_make_file(_ROOT_PATH + "/cloud/foo/foo-speed-src.jar", _ROOT_PATH)))

    # From rust/bazel/aspects.bzl: Rust generated entry point file -> None
    asserts.equals(env, None, _import_virtual(_make_file(_ROOT_PATH + "/cloud/foo/bar.rs", _ROOT_PATH)))

    # Additional edge cases:
    # Virtual import in main workspace (shorter path)
    virtual_main = _make_file(
        _ROOT_PATH + "/_virtual_imports/e/bar.proto",
        _ROOT_PATH,
    )
    asserts.equals(
        env,
        "-I" + _ROOT_PATH + "/_virtual_imports/e",
        _import_virtual(virtual_main),
    )

    # Invalid virtual import path with no slash after virtual import name -> None
    invalid_virtual = _make_file(
        _ROOT_PATH + "/_virtual_imports/e",
        _ROOT_PATH,
    )
    asserts.equals(env, None, _import_virtual(invalid_virtual))

    # Non-virtual source file -> None
    src_file = _make_file("cloud/foo/bar.proto", "")
    asserts.equals(env, None, _import_virtual(src_file))

    return unittest.end(env)

def _import_external_repo_from_file_test(ctx):
    """Test external repository path classification against original docstring examples and language rules."""
    env = unittest.begin(ctx)

    # 1. 'bazel-out/k8-fastbuild/bin/external/foo/e/_virtual_imports/e' -> None
    virtual_ext_file = _make_file(
        _ROOT_PATH + "/external/foo/e/_virtual_imports/e/bar.proto",
        _ROOT_PATH + "/external/foo/e",
    )
    asserts.equals(env, None, _import_external(virtual_ext_file))

    # 2. 'bazel-out/foo/k8-fastbuild/bin/e/_virtual_imports/e' -> None
    virtual_alt_file = _make_file(
        _ALT_ROOT + "/e/_virtual_imports/e/bar.proto",
        _ALT_ROOT + "/e",
    )
    asserts.equals(env, None, _import_external(virtual_alt_file))

    # 3. 'bazel-out/k8-fastbuild/bin/external/foo' -> Generated file in external repo.
    ext_gen = _make_file(
        _ROOT_PATH + "/external/foo/bar.proto",
        _ROOT_PATH + "/external/foo",
    )
    asserts.equals(
        env,
        "-I" + _ROOT_PATH + "/external/foo",
        _import_external(ext_gen),
    )

    # 4. 'bazel-out/foo/k8-fastbuild/bin' -> None (main output, not external repo).
    gen_alt = _make_file(
        _ALT_ROOT + "/cloud/foo/bar.proto",
        _ALT_ROOT,
    )
    asserts.equals(env, None, _import_external(gen_alt))

    # 5. 'bazel-out/k8-fastbuild/bin' -> None (main output).
    gen_main = _make_file(
        _ROOT_PATH + "/cloud/foo/bar.proto",
        _ROOT_PATH,
    )
    asserts.equals(env, None, _import_external(gen_main))

    # 6. 'external/foo' — source file in external repo.
    asserts.equals(
        env,
        "-Iexternal/foo",
        _import_external(_make_file("external/foo/bar.proto", "")),
    )

    # 7. '../foo' — sibling layout external repo.
    asserts.equals(
        env,
        "-I../foo",
        _import_external(_make_file("../foo/bar.proto", "")),
    )

    # Examples from language proto rules:
    # From py_proto_library.bzl: External repo Python generated file
    asserts.equals(
        env,
        "-I" + _ROOT_PATH + "/external/foo",
        _import_external(_make_file(_ROOT_PATH + "/external/foo/bar_pb2.py", _ROOT_PATH + "/external/foo")),
    )

    # From go_proto_library_impl.bzl: External repo Go proto source file
    asserts.equals(
        env,
        "-Iexternal/com_google_protobuf",
        _import_external(_make_file("external/com_google_protobuf/src/google/protobuf/timestamp.proto", "")),
    )

    # Additional edge cases:
    # Virtual imports in main workspace -> None.
    virtual_file = _make_file(
        _ROOT_PATH + "/_virtual_imports/foo/bar.proto",
        _ROOT_PATH,
    )
    asserts.equals(env, None, _import_external(virtual_file))

    # Main workspace source -> None.
    asserts.equals(
        env,
        None,
        _import_external(_make_file("cloud/foo/bar.proto", "")),
    )

    return unittest.end(env)

def _import_main_output_from_file_test(ctx):
    """Test main output path classification against original docstring examples and language rules."""
    env = unittest.begin(ctx)

    # 1. 'bazel-out/k8-fastbuild/bin/external/foo/e/_virtual_imports/e' -> None
    virtual_ext = _make_file(
        _ROOT_PATH + "/external/foo/e/_virtual_imports/e/bar.proto",
        _ROOT_PATH + "/external/foo/e",
    )
    asserts.equals(env, None, _import_main(virtual_ext))

    # 2. 'bazel-out/foo/k8-fastbuild/bin/e/_virtual_imports/e' -> None
    virtual_alt = _make_file(
        _ALT_ROOT + "/e/_virtual_imports/e/bar.proto",
        _ALT_ROOT + "/e",
    )
    asserts.equals(env, None, _import_main(virtual_alt))

    # 3. 'bazel-out/k8-fastbuild/bin/external/foo' -> None (external repo).
    ext_gen = _make_file(
        _ROOT_PATH + "/external/foo/bar.proto",
        _ROOT_PATH + "/external/foo",
    )
    asserts.equals(env, None, _import_main(ext_gen))

    # 4. 'bazel-out/foo/k8-fastbuild/bin' — different bin root in main workspace.
    gen_alt = _make_file(
        _ALT_ROOT + "/cloud/foo/bar.proto",
        _ALT_ROOT,
    )
    asserts.equals(env, "-I" + _ALT_ROOT, _import_main(gen_alt))

    # 5. 'bazel-out/k8-fastbuild/bin' — generated file in main workspace.
    gen_file = _make_file(
        _ROOT_PATH + "/cloud/foo/bar.proto",
        _ROOT_PATH,
    )
    asserts.equals(env, "-I" + _ROOT_PATH, _import_main(gen_file))

    # 6. 'external/foo' -> None (handled by import_external).
    asserts.equals(
        env,
        None,
        _import_main(_make_file("external/foo/bar.proto", "")),
    )

    # 7. '../foo' -> None (handled by import_external).
    asserts.equals(
        env,
        None,
        _import_main(_make_file("../foo/bar.proto", "")),
    )

    # Examples from language proto rules:
    # From py_proto_library.bzl: Python generated _pb2.py in main workspace
    asserts.equals(
        env,
        "-I" + _ROOT_PATH,
        _import_main(_make_file(_ROOT_PATH + "/cloud/foo/bar_pb2.py", _ROOT_PATH)),
    )

    # From java_proto_library.bzl: Java speed source jar in main workspace
    asserts.equals(
        env,
        "-I" + _ROOT_PATH,
        _import_main(_make_file(_ROOT_PATH + "/cloud/foo/foo-speed-src.jar", _ROOT_PATH)),
    )

    # From rust/bazel/aspects.bzl: Rust generated entry point in main workspace
    asserts.equals(
        env,
        "-I" + _ROOT_PATH,
        _import_main(_make_file(_ROOT_PATH + "/cloud/foo/bar.rs", _ROOT_PATH)),
    )

    # Additional edge cases:
    # Main workspace source file -> None (covered by -I.).
    asserts.equals(
        env,
        None,
        _import_main(_make_file("cloud/foo/bar.proto", "")),
    )

    # Empty root_path "" -> None.
    asserts.equals(
        env,
        None,
        _import_main(_make_file("cloud/foo/bar.proto", "")),
    )

    # '.' root path -> None.
    asserts.equals(
        env,
        None,
        _import_main(_make_file("cloud/foo/bar.proto", ".")),
    )

    return unittest.end(env)

def _ordering_test(ctx):
    """Test that bucket_transitive_sources + single map_each produces -I flags in correct protoc order."""
    env = unittest.begin(ctx)

    # Interleaved inputs (simulating unordered depset iteration)
    files = [
        # Main output example
        _make_file(
            _ROOT_PATH + "/cloud/foo/bar.proto",
            _ROOT_PATH,
        ),
        # External repo example
        _make_file(
            _ROOT_PATH + "/external/foo/baz.proto",
            _ROOT_PATH + "/external/foo",
        ),
        # Virtual import examples
        _make_file(
            _ROOT_PATH + "/external/foo/e/_virtual_imports/e/bar.proto",
            _ROOT_PATH + "/external/foo/e",
        ),
        _make_file(
            _ALT_ROOT + "/e/_virtual_imports/e/bar.proto",
            _ALT_ROOT + "/e",
        ),
        # External source examples
        _make_file("external/foo/bar.proto", ""),
        _make_file("../foo/bar.proto", ""),
        # Main workspace source
        _make_file("cloud/foo/baz.proto", ""),
    ]

    # Single add_all behavior: bucket_transitive_sources + single map_each
    ordered_files = _bucket_sources(struct(to_list = lambda: files))
    ordered_flags = []
    for f in ordered_files:
        result = _import_proto_path(f)
        if result != None and result not in ordered_flags:
            ordered_flags.append(result)

    asserts.equals(env, 6, len(ordered_flags))

    # Bucket 1: Virtual imports
    asserts.equals(
        env,
        "-I" + _ROOT_PATH + "/external/foo/e/_virtual_imports/e",
        ordered_flags[0],
    )
    asserts.equals(
        env,
        "-I" + _ALT_ROOT + "/e/_virtual_imports/e",
        ordered_flags[1],
    )

    # Bucket 2: External repo paths
    asserts.equals(
        env,
        "-I" + _ROOT_PATH + "/external/foo",
        ordered_flags[2],
    )
    asserts.equals(env, "-Iexternal/foo", ordered_flags[3])
    asserts.equals(env, "-I../foo", ordered_flags[4])

    # Bucket 3: Main output
    asserts.equals(env, "-I" + _ROOT_PATH, ordered_flags[5])

    return unittest.end(env)

import_virtual_test = unittest.make(_import_virtual_from_file_test)
import_external_repo_test = unittest.make(_import_external_repo_from_file_test)
import_main_output_test = unittest.make(_import_main_output_from_file_test)
ordering_test = unittest.make(_ordering_test)

def proto_path_mapping_test_suite():
    """Creates the test targets and test suite for path mapping tests."""
    unittest.suite(
        "proto_path_mapping_test",
        import_virtual_test,
        import_external_repo_test,
        import_main_output_test,
        ordering_test,
    )
