"""
Macro to generate exponential dependency proto structure using genrules and proto_library targets.
"""

load("@rules_proto//proto:defs.bzl", "proto_library")

def _pad(n):
    """Zero-pad a number to 2 digits."""
    return str(n) if n >= 10 else "0" + str(n)

def generate_cached_protos(width, depth, package):
    """
    Generate proto files and proto_library targets for exponential dependency testing.
    
    Args:
        width: Number of files per level (except last level which has 1)
        depth: Number of levels
        package: Proto package name
    """
    
    # Calculate all proto file names we need - with package prefix for proper directory structure
    all_files = [package + "/file_00_00.proto"]
    
    for level in range(1, depth + 1):
        num_files = width if level < depth else 1
        for idx in range(num_files):
            all_files.append(package + "/file_" + _pad(level) + "_" + _pad(idx) + ".proto")
    
    all_files.append(package + "/file_entry.proto")
    
    # Single genrule to generate ALL proto files
    _generate_all_protos(
        name = "generate_all_protos",
        width = width,
        depth = depth,
        package = package,
        all_files = all_files,
    )
    
    # Level 0: Base proto_library
    proto_library(
        name = "file_00_00_proto",
        srcs = [":" + package + "/file_00_00.proto"],
        strip_import_prefix = "/csharp/protos",
    )
    
    # Level 1: WIDTH proto_library targets
    for i in range(width):
        i_pad = _pad(i)
        proto_library(
            name = "file_01_" + i_pad + "_proto",
            srcs = [":" + package + "/file_01_" + i_pad + ".proto"],
            strip_import_prefix = "/csharp/protos",
            deps = [":file_00_00_proto"],
        )
    
    # Levels 2 through DEPTH
    for level in range(2, depth + 1):
        num_files = width if level < depth else 1
        level_pad = _pad(level)
        level_prev_pad = _pad(level - 1)
        
        for idx in range(num_files):
            idx_pad = _pad(idx)
            deps = [
                ":file_" + level_prev_pad + "_" + _pad(prev_idx) + "_proto"
                for prev_idx in range(width)
            ]
            
            proto_library(
                name = "file_" + level_pad + "_" + idx_pad + "_proto",
                srcs = [":" + package + "/file_" + level_pad + "_" + idx_pad + ".proto"],
                strip_import_prefix = "/csharp/protos",
                deps = deps,
            )
    
    # Entry point proto_library
    proto_library(
        name = "entry_proto",
        srcs = [":" + package + "/file_entry.proto"],
        strip_import_prefix = "/csharp/protos",
        deps = [":file_" + _pad(depth) + "_00_proto"],
    )

def _generate_all_protos(name, width, depth, package, all_files):
    """Generate a single genrule that creates all proto files.
    
    These files only contain imports to create deep dependency chains.
    Only the entry file has an actual message.
    """
    
    # Build the command to generate all files
    cmd_parts = []
    
    # Level 0 (base file) - with a simple message
    cmd_parts.append("""
cat > $(location """ + package + """/file_00_00.proto) << 'EOFPROTO'
syntax = "proto3";

package """ + package + """;

message Level00Message {
  string field = 1;
}
EOFPROTO
""")
    
    # Level 1 files - import level 0 and have a message referencing it
    for i in range(width):
        i_pad = _pad(i)
        cmd_parts.append("""
cat > $(location """ + package + """/file_01_""" + i_pad + """.proto) << 'EOFPROTO'
syntax = "proto3";

package """ + package + """;

import \"""" + package + """/file_00_00.proto\";

message Level01Message""" + i_pad + """ {
  Level00Message field = 1;
}
EOFPROTO
""")
    
    # Levels 2 through DEPTH - just imports, no messages
    for level in range(2, depth + 1):
        num_files = width if level < depth else 1
        level_pad = _pad(level)
        level_prev_pad = _pad(level - 1)
        
        for idx in range(num_files):
            idx_pad = _pad(idx)
            
            import_lines = []
            for prev_idx in range(width):
                import_lines.append('import "' + package + '/file_' + level_prev_pad + '_' + _pad(prev_idx) + '.proto";')
            imports_str = "\n".join(import_lines)
            
            cmd_parts.append("""
cat > $(location """ + package + """/file_""" + level_pad + "_" + idx_pad + """.proto) << 'EOFPROTO'
syntax = "proto3";

package """ + package + """;

""" + imports_str + """
EOFPROTO
""")
    
    # Entry point file - only this one has a message
    cmd_parts.append("""
cat > $(location """ + package + """/file_entry.proto) << 'EOFPROTO'
syntax = "proto3";

package """ + package + """;

import \"""" + package + """/file_""" + _pad(depth) + """_00.proto\";

message Example {
  string test_field = 1;
}
EOFPROTO
""")
    
    native.genrule(
        name = name,
        outs = all_files,
        cmd = "\n".join(cmd_parts),
    )
