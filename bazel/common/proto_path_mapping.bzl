"""Path mapping callbacks for proto_common._compile().

These callbacks classify proto File paths by content to produce -I flags
for protoc. They are in a separate file with tight visibility control so
that external users cannot access them.

Three callbacks are used (in order) to preserve the ordering required by
protoc: most specific (virtual imports) → external repo → least specific
(main output). The catch-all (import_main_output_from_file) excludes
files already handled by the first two callbacks.

These callbacks are highly dependent on transitive_sources (depset[File])
generated in third_party/protobuf/bazel/private/proto_info.bzl.

Why file.path and file.root.path instead of transitive_proto_path strings?

  transitive_proto_path (depset[str]) is computed at analysis time in
  proto_info.bzl via _from_root(), which calls src.root.path and bakes
  the result into a string. This bypasses path mapping because the
  execution-root path (e.g. 'bazel-out/k8-fastbuild/bin') is frozen at
  analysis time.

  transitive_sources (depset[File]) contains the same .proto files as
  File objects. When .path and .root.path are evaluated inside a
  map_each callback, Blaze intercepts these calls and rewrites them to
  config-agnostic form (e.g. 'bazel-out/cfg/bin') at execution time.

  - file.path: the full execution path of the file, e.g.
    'bazel-out/k8-fastbuild/bin/external/foo/bar.proto'
  - file.root.path: the root directory portion, e.g.
    'bazel-out/k8-fastbuild/bin/external/foo'. For source files in the
    main workspace, root.path is '.'.

  Every File has a .root attribute (confirmed by proto_info.bzl which
  calls src.root.path without any null check).

  For non-virtual files, file.root.path is the source root — the same
  value _from_root() would produce. For virtual imports, the source root
  is a subdirectory inside root.path, so it is extracted from file.path
  instead.
"""

def import_virtual_from_file(file):
    """map_each callback for virtual imports paths from File objects.

      They're of the form:
      'bazel-out/k8-fastbuild/bin/external/foo/e/_virtual_imports/e' or
      'bazel-out/foo/k8-fastbuild/bin/e/_virtual_imports/e'

    Args:
      file: A File object from proto_info.transitive_sources.

    Returns:
      A string of the form '-I<path>' for the virtual imports root, or
      None if the file is not a virtual import.
    """
    path = file.path
    idx = path.find("_virtual_imports/")
    if idx < 0:
        return None
    end = path.find("/", idx + len("_virtual_imports/"))
    if end >= 0:
        return "-I%s" % path[:end]
    return "-I%s" % path

def import_external_repo_from_file(file):
    """map_each callback for external repository paths from File objects.

      They are of the form:
      'bazel-out/k8-fastbuild/bin/external/foo' or
      'external/foo' or '../foo'

    Args:
      file: A File object from proto_info.transitive_sources.

    Returns:
      A string of the form '-I<path>' for the external repo root, or
      None if the file is not in an external repository.
    """

    # Exclude virtual imports (handled by import_virtual_from_file).
    if import_virtual_from_file(file) != None:
        return None
    root_path = file.root.path
    if root_path and root_path != "." and "external/" in root_path:
        return "-I%s" % root_path
    path = file.path
    if path.startswith("external/"):
        parts = path.split("/", 2)
        if len(parts) >= 2:
            return "-I%s/%s" % (parts[0], parts[1])
    if path.startswith("../"):
        parts = path.split("/", 2)
        if len(parts) >= 2:
            return "-I%s/%s" % (parts[0], parts[1])
    return None

def import_main_output_from_file(file):
    """map_each callback for main output paths from File objects.

      They're of the form:
      'bazel-out/k8-fastbuild/bin'

      Catch-all: returns -I for any file not handled by
      import_virtual_from_file or import_external_repo_from_file.
      Main workspace source files return None (covered by -I.).

    Args:
      file: A File object from proto_info.transitive_sources.

    Returns:
      A string of the form '-I<path>' for the main output root, or
      None if the file is a main workspace source file.
    """
    if import_virtual_from_file(file) != None:
        return None
    if import_external_repo_from_file(file) != None:
        return None
    root_path = file.root.path
    if root_path and root_path != ".":
        return "-I%s" % root_path
    return None

proto_path_mapping = struct(
    import_virtual_from_file = import_virtual_from_file,
    import_external_repo_from_file = import_external_repo_from_file,
    import_main_output_from_file = import_main_output_from_file,
)
