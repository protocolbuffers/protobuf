"""Unit tests for proto_path_mapping callbacks."""

load("@bazel_skylib//lib:unittest.bzl", "asserts", "unittest")
load(
    "//bazel/common:proto_path_mapping.bzl",
    "proto_path_mapping",
)

_import_virtual = proto_path_mapping.import_virtual_from_file
_import_external = proto_path_mapping.import_external_repo_from_file
_import_main = proto_path_mapping.import_main_output_from_file

# Fake root paths similar to what Blaze generates.
_ROOT_PATH = "bazel-out/k8-fastbuild/bin"
_ALT_ROOT = "bazel-out/foo/k8-fastbuild/bin"

def _make_file(path, root_path = ""):
    """Create a File-like struct for testing."""
    return struct(path = path, root = struct(path = root_path))

def _import_virtual_from_file_test(ctx):
    """Test virtual imports classification.

    Original _import_virtual_proto_path docstring examples:
      'bazel-out/k8-fastbuild/bin/external/foo/e/_virtual_imports/e'
      'bazel-out/foo/k8-fastbuild/bin/e/_virtual_imports/e'
    """
    env = unittest.begin(ctx)

    # 'bazel-out/k8-fastbuild/bin/external/foo/e/_virtual_imports/e'
    virtual_ext = _ROOT_PATH + "/external/foo/e/_virtual_imports/e"
    asserts.equals(
        env,
        "-I" + virtual_ext,
        _import_virtual(_make_file(
            virtual_ext + "/bar.proto",
            _ROOT_PATH + "/external/foo/e",
        )),
    )

    # 'bazel-out/foo/k8-fastbuild/bin/e/_virtual_imports/e'
    virtual_alt = _ALT_ROOT + "/e/_virtual_imports/e"
    asserts.equals(
        env,
        "-I" + virtual_alt,
        _import_virtual(_make_file(
            virtual_alt + "/bar.proto",
            _ALT_ROOT + "/e",
        )),
    )

    # Virtual imports in main workspace (shorter path).
    virtual_short = _ROOT_PATH + "/_virtual_imports/foo"
    asserts.equals(
        env,
        "-I" + virtual_short,
        _import_virtual(_make_file(
            virtual_short + "/bar.proto",
            _ROOT_PATH,
        )),
    )

    # Virtual imports path with no trailing content after name.
    asserts.equals(
        env,
        "-I" + _ROOT_PATH + "/_virtual_imports/foo",
        _import_virtual(_make_file(
            _ROOT_PATH + "/_virtual_imports/foo",
            _ROOT_PATH,
        )),
    )

    # Non-virtual generated file → None.
    asserts.equals(
        env,
        None,
        _import_virtual(_make_file(
            _ROOT_PATH + "/cloud/foo/bar.proto",
            _ROOT_PATH,
        )),
    )

    # Non-virtual source file → None.
    asserts.equals(
        env,
        None,
        _import_virtual(_make_file("external/foo/bar.proto", "")),
    )

    return unittest.end(env)

def _import_external_repo_from_file_test(ctx):
    """Test external repo path classification.

    Original _import_repo_proto_path docstring examples:
      'bazel-out/k8-fastbuild/bin/external/foo'
      'bazel-out/foo/k8-fastbuild/bin'

    Original _import_main_output_proto_path also handled:
      'external/foo' and '../foo'
    (These are source-file external repo paths now handled here.)
    """
    env = unittest.begin(ctx)

    # 'bazel-out/k8-fastbuild/bin/external/foo'
    # Generated file in external repo.
    ext_gen = _make_file(
        _ROOT_PATH + "/external/foo/bar.proto",
        _ROOT_PATH + "/external/foo",
    )
    asserts.equals(
        env,
        "-I" + _ROOT_PATH + "/external/foo",
        _import_external(ext_gen),
    )

    # 'bazel-out/foo/k8-fastbuild/bin'
    # This is a main-output path (no external/ in root_path).
    # The old _import_repo_proto_path caught it via slash count,
    # but content-based classification correctly returns None.
    gen_alt = _make_file(
        _ALT_ROOT + "/cloud/foo/bar.proto",
        _ALT_ROOT,
    )
    asserts.equals(env, None, _import_external(gen_alt))

    # Generated file in main workspace → None.
    gen_main = _make_file(
        _ROOT_PATH + "/cloud/foo/bar.proto",
        _ROOT_PATH,
    )
    asserts.equals(env, None, _import_external(gen_main))

    # 'external/foo' — source file in external repo.
    asserts.equals(
        env,
        "-Iexternal/foo",
        _import_external(_make_file("external/foo/bar.proto", "")),
    )

    # '../foo' — sibling layout external repo.
    asserts.equals(
        env,
        "-I../foo",
        _import_external(_make_file("../foo/bar.proto", "")),
    )

    # Virtual imports in main workspace → None.
    virtual_file = _make_file(
        _ROOT_PATH + "/_virtual_imports/foo/bar.proto",
        _ROOT_PATH,
    )
    asserts.equals(env, None, _import_external(virtual_file))

    # Virtual imports in external repo → None (must not be caught
    # by the "external/" in root_path check).
    virtual_ext_file = _make_file(
        _ROOT_PATH + "/external/foo/e/_virtual_imports/e/bar.proto",
        _ROOT_PATH + "/external/foo/e",
    )
    asserts.equals(env, None, _import_external(virtual_ext_file))

    # Main workspace source → None.
    asserts.equals(
        env,
        None,
        _import_external(_make_file("cloud/foo/bar.proto", "")),
    )

    return unittest.end(env)

def _import_main_output_from_file_test(ctx):
    """Test main output path classification (catch-all).

    Original _import_main_output_proto_path docstring examples:
      'bazel-out/k8-fastbuild/bin'
      'external/foo'
      '../foo'
    """
    env = unittest.begin(ctx)

    # 'bazel-out/k8-fastbuild/bin'
    # Generated file in main workspace.
    gen_file = _make_file(
        _ROOT_PATH + "/cloud/foo/bar.proto",
        _ROOT_PATH,
    )
    asserts.equals(env, "-I" + _ROOT_PATH, _import_main(gen_file))

    # 'bazel-out/foo/k8-fastbuild/bin' — different bin root.
    gen_alt = _make_file(
        _ALT_ROOT + "/cloud/foo/bar.proto",
        _ALT_ROOT,
    )
    asserts.equals(env, "-I" + _ALT_ROOT, _import_main(gen_alt))

    # Virtual imports → None (handled by import_virtual).
    virtual_file = _make_file(
        _ROOT_PATH + "/_virtual_imports/foo/bar.proto",
        _ROOT_PATH,
    )
    asserts.equals(env, None, _import_main(virtual_file))

    # External repo generated → None (handled by import_external).
    ext_gen = _make_file(
        _ROOT_PATH + "/external/foo/bar.proto",
        _ROOT_PATH + "/external/foo",
    )
    asserts.equals(env, None, _import_main(ext_gen))

    # 'external/foo' → None (handled by import_external).
    asserts.equals(
        env,
        None,
        _import_main(_make_file("external/foo/bar.proto", "")),
    )

    # '../foo' → None (handled by import_external).
    asserts.equals(
        env,
        None,
        _import_main(_make_file("../foo/bar.proto", "")),
    )

    # Main workspace source file → None (covered by -I.).
    asserts.equals(
        env,
        None,
        _import_main(_make_file("cloud/foo/bar.proto", "")),
    )

    # Empty root_path "" → None.
    asserts.equals(
        env,
        None,
        _import_main(_make_file("cloud/foo/bar.proto", "")),
    )

    # '.' root path → None.
    asserts.equals(
        env,
        None,
        _import_main(_make_file("cloud/foo/bar.proto", ".")),
    )

    return unittest.end(env)

def _ordering_test(ctx):
    """Test that the 3 callbacks produce -I flags in correct protoc order.

    Protoc requires -I paths from most specific (longest) to least
    specific (shortest). Uses all 'of the form' examples from the
    original _import_virtual_proto_path, _import_repo_proto_path, and
    _import_main_output_proto_path docstrings.
    """
    env = unittest.begin(ctx)

    # Files representing all 'of the form' examples from the
    # original three _import_*_proto_path functions.
    files = [
        # From _import_virtual_proto_path:
        # 'bazel-out/k8-fastbuild/bin/external/foo/e/_virtual_imports/e'
        _make_file(
            _ROOT_PATH + "/external/foo/e/_virtual_imports/e/bar.proto",
            _ROOT_PATH + "/external/foo/e",
        ),
        # 'bazel-out/foo/k8-fastbuild/bin/e/_virtual_imports/e'
        _make_file(
            _ALT_ROOT + "/e/_virtual_imports/e/bar.proto",
            _ALT_ROOT + "/e",
        ),

        # From _import_repo_proto_path:
        # 'bazel-out/k8-fastbuild/bin/external/foo'
        _make_file(
            _ROOT_PATH + "/external/foo/baz.proto",
            _ROOT_PATH + "/external/foo",
        ),

        # From _import_main_output_proto_path:
        # 'bazel-out/k8-fastbuild/bin'
        _make_file(
            _ROOT_PATH + "/cloud/foo/bar.proto",
            _ROOT_PATH,
        ),
        # 'external/foo' (source file in external repo)
        _make_file("external/foo/bar.proto", ""),
        # '../foo' (sibling layout external repo)
        _make_file("../foo/bar.proto", ""),

        # Main workspace source (should produce None — covered by -I.)
        _make_file("cloud/foo/baz.proto", ""),
    ]

    # Apply the 3 callbacks in order, collecting non-None results.
    ordered_flags = []
    for f in files:
        result = _import_virtual(f)
        if result != None:
            ordered_flags.append(result)
    for f in files:
        result = _import_external(f)
        if result != None:
            ordered_flags.append(result)
    for f in files:
        result = _import_main(f)
        if result != None:
            ordered_flags.append(result)

    # Verify ordering: virtual (longest) → external repo →
    # main output (shortest). Source-file externals ('external/foo',
    # '../foo') are in the external repo bucket.
    asserts.equals(env, 6, len(ordered_flags))

    # Bucket 1: Virtual imports (most specific).
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

    # Bucket 2: External repo paths.
    asserts.equals(
        env,
        "-I" + _ROOT_PATH + "/external/foo",
        ordered_flags[2],
    )
    asserts.equals(env, "-Iexternal/foo", ordered_flags[3])
    asserts.equals(env, "-I../foo", ordered_flags[4])

    # Bucket 3: Main output (least specific).
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
