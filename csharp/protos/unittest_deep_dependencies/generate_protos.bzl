"""
Macro to generate exponential dependency proto structure for both cached and notcached variants.
Generates proto files into separate subdirectories without duplication.
"""

load("//bazel:proto_library.bzl", "proto_library")

def _pad(n):
    """Zero-pad a number to 2 digits."""
    return str(n) if n >= 10 else "0" + str(n)

def generate_deep_dependencies_protos(width, depth):
    """
    Generate proto files and proto_library targets for both cached and notcached variants.

    Args:
        width: Number of files per level (except last level which has 1)
        depth: Number of levels
    """

    # Generate both variants
    _generate_variant("cached", width, depth, "unittest_deep_dependencies_cached")
    _generate_variant("notcached", width, depth, "unittest_deep_dependencies_not_cached")

def _generate_variant(variant, width, depth, package):
    """
    Generate proto files and libraries for a specific variant.

    Args:
        variant: "cached" or "notcached"
        width: Number of files per level
        depth: Number of levels
        package: Proto package name
    """

    # Calculate all proto file names - use package name for the directory
    all_files = [package + "/file_00_00.proto"]

    for level in range(1, depth + 1):
        num_files = width if level < depth else 1
        for idx in range(num_files):
            all_files.append(package + "/file_" + _pad(level) + "_" + _pad(idx) + ".proto")

    all_files.append(package + "/file_entry.proto")

    # Single genrule to generate ALL proto files for this variant
    _generate_all_protos(
        name = "generate_" + variant + "_protos",
        variant = variant,
        width = width,
        depth = depth,
        package = package,
        all_files = all_files,
    )

    # Level 0: Base proto_library
    proto_library(
        name = variant + "_file_00_00_proto",
        srcs = [":" + package + "/file_00_00.proto"],
        strip_import_prefix = "/third_party/protobuf/csharp/protos/unittest_deep_dependencies",
    )

    # Level 1: WIDTH proto_library targets
    for i in range(width):
        i_pad = _pad(i)
        proto_library(
            name = variant + "_file_01_" + i_pad + "_proto",
            srcs = [":" + package + "/file_01_" + i_pad + ".proto"],
            strip_import_prefix = "/third_party/protobuf/csharp/protos/unittest_deep_dependencies",
            deps = [":" + variant + "_file_00_00_proto"],
            exports = [":" + variant + "_file_00_00_proto"],
        )

    # Levels 2 through DEPTH
    for level in range(2, depth + 1):
        num_files = width if level < depth else 1
        level_pad = _pad(level)
        level_prev_pad = _pad(level - 1)

        for idx in range(num_files):
            idx_pad = _pad(idx)
            deps = [
                ":" + variant + "_file_" + level_prev_pad + "_" + _pad(prev_idx) + "_proto"
                for prev_idx in range(width)
            ]

            proto_library(
                name = variant + "_file_" + level_pad + "_" + idx_pad + "_proto",
                srcs = [":" + package + "/file_" + level_pad + "_" + idx_pad + ".proto"],
                strip_import_prefix = "/third_party/protobuf/csharp/protos/unittest_deep_dependencies",
                deps = deps,
                exports = deps,
            )

    # Entry point proto_library
    proto_library(
        name = variant + "_entry_proto",
        srcs = [":" + package + "/file_entry.proto"],
        strip_import_prefix = "/third_party/protobuf/csharp/protos/unittest_deep_dependencies",
        deps = [":" + variant + "_file_" + _pad(depth) + "_00_proto"],
        exports = [":" + variant + "_file_" + _pad(depth) + "_00_proto"],
    )

    # Generate C# source code (.pb.cs) from these generated .proto files
    _generate_csharp_sources(
        name = variant + "_csharp_gen",
        variant = variant,
        all_files = all_files,
    )

def _generate_all_protos(name, variant, width, depth, package, all_files):
    """Generate a single genrule that creates all proto files for a variant.

    These files only contain imports to create deep dependency chains.
    Only the entry file has an actual message.
    """

    # Build the command to generate all files
    cmd_parts = []

    if variant == "cached":
        # Cached version - Level 0 with a message
        cmd_parts.append("""
mkdir -p $(RULEDIR)/%s
cat > $(RULEDIR)/%s/file_00_00.proto << 'EOFPROTO'
syntax = "proto3";

package %s;

message Level00Message {
  string field = 1;
}
EOFPROTO
""" % (package, package, package))

        # Level 1 files - with messages
        for i in range(width):
            i_pad = _pad(i)
            cmd_parts.append("""
cat > $(RULEDIR)/%s/file_01_%s.proto << 'EOFPROTO'
syntax = "proto3";

package %s;

import public "%s/file_00_00.proto";

message Level01Message%s {
  Level00Message field = 1;
}
EOFPROTO
""" % (package, i_pad, package, package, i_pad))
    else:
        # Notcached version - Level 0 empty
        cmd_parts.append("""
mkdir -p $(RULEDIR)/%s
cat > $(RULEDIR)/%s/file_00_00.proto << 'EOFPROTO'
syntax = "proto3";

package %s;
EOFPROTO
""" % (package, package, package))

        # Level 1 files - just imports
        for i in range(width):
            i_pad = _pad(i)
            cmd_parts.append("""
cat > $(RULEDIR)/%s/file_01_%s.proto << 'EOFPROTO'
syntax = "proto3";

package %s;

import public "%s/file_00_00.proto";
EOFPROTO
""" % (package, i_pad, package, package))

    # Levels 2 through DEPTH - just imports, no messages
    for level in range(2, depth + 1):
        num_files = width if level < depth else 1
        level_pad = _pad(level)
        level_prev_pad = _pad(level - 1)

        for idx in range(num_files):
            idx_pad = _pad(idx)

            import_lines = []
            for prev_idx in range(width):
                import_lines.append('import public "%s/file_%s_%s.proto";' % (package, level_prev_pad, _pad(prev_idx)))
            imports_str = "\n".join(import_lines)

            cmd_parts.append("""
cat > $(RULEDIR)/%s/file_%s_%s.proto << 'EOFPROTO'
syntax = "proto3";

package %s;

%s
EOFPROTO
""" % (package, level_pad, idx_pad, package, imports_str))

    # Entry point file
    if variant == "cached":
        message_type = "Example"
    else:
        message_type = "ExampleNotCached"

    cmd_parts.append("""
cat > $(RULEDIR)/%s/file_entry.proto << 'EOFPROTO'
syntax = "proto3";

package %s;

import public "%s/file_%s_00.proto";

message %s {
  string test_field = 1;
}
EOFPROTO
""" % (package, package, package, _pad(depth), message_type))

    native.genrule(
        name = name,
        outs = all_files,
        cmd = "".join(cmd_parts),
    )

def _generate_csharp_sources(name, variant, all_files):
    """Generate C# source files from generated proto files using a single genrule."""

    base_namespace = "UnittestDeepDependenciesCached" if variant == "cached" else "UnittestDeepDependenciesNotCached"

    # Translate each proto file path to its expected C# output file name
    csharp_outputs = []
    for f in all_files:
        basename = f.split("/")[-1]  # "file_00_00.proto"
        name_no_ext = basename.split(".proto")[0]  # "file_00_00"
        csharp_name = ""
        for part in name_no_ext.split("_"):
            if len(part) > 0:
                csharp_name += part[0].upper() + part[1:]

        csharp_outputs.append(variant + "_cs/" + csharp_name + ".pb.cs")

    # Invoke protoc hermetically with the correct base namespace and include path
    native.genrule(
        name = name,
        testonly = True,
        srcs = [":" + f for f in all_files],
        outs = csharp_outputs,
        cmd = (
            "$(location //net/proto2/compiler/public:protocol_compiler) " +
            "--csharp_out=$(RULEDIR)/" + variant + "_cs " +
            "--csharp_opt=file_extension=.pb.cs " +
            "--csharp_opt=base_namespace=" + base_namespace + " " +
            "-I$$(dirname $$(dirname $(location :" + all_files[0] + "))) " +
            "$(SRCS)"
        ),
        tools = ["//net/proto2/compiler/public:protocol_compiler"],
        visibility = ["//src/google/protobuf/csharp:__subpackages__"],
    )
