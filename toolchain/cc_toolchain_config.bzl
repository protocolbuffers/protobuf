load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "artifact_name_pattern",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
    "with_feature_set",
)

all_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

all_compile_actions = [
    ACTION_NAMES.assemble,
    ACTION_NAMES.preprocess_assemble,
    ACTION_NAMES.linkstamp_compile,
    ACTION_NAMES.c_compile,
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.cpp_header_parsing,
    ACTION_NAMES.cpp_module_codegen,
    ACTION_NAMES.cpp_module_compile,
    ACTION_NAMES.clif_match,
    ACTION_NAMES.lto_backend,
]

def _impl(ctx):
  if "mingw" in ctx.attr.target_full_name:
      artifact_name_patterns = [
          artifact_name_pattern(
              category_name = "executable",
              prefix = "",
              extension = ".exe",
          ),
      ]
  else:
      artifact_name_patterns = []

  tool_paths = [
      tool_path(
          name = "gcc",
          path = "/usr/local/bin/clang",
      ),
      tool_path(
          name = "ld",
          path = ctx.attr.linker_path,
      ),
      tool_path(
          name = "ar",
          path = "/usr/local/bin/llvm-ar",
      ),
      tool_path(
          name = "compat-ld",
          path = ctx.attr.linker_path,
      ),
      tool_path(
          name = "cpp",
          path = "/bin/false",
      ),
      tool_path(
          name = "dwp",
          path = "/bin/false",
      ),
      tool_path(
          name = "gcov",
          path = "/bin/false",
      ),
      tool_path(
          name = "nm",
          path = "/bin/false",
      ),
      tool_path(
          name = "objcopy",
          path = "/bin/false",
      ),
      tool_path(
          name = "objdump",
          path = "/bin/false",
      ),
      tool_path(
          name = "strip",
          path = "/bin/false",
      ),
  ]

  linker_flags = feature(
      name = "default_linker_flags",
      enabled = True,
      flag_sets = [
          flag_set(
              actions = all_link_actions,
              flag_groups = [
                  flag_group(
                      flags = [
                          "-B" + ctx.attr.linker_path,
                          "-lstdc++",
                          "--target=" + ctx.attr.target_full_name,
                      ] + ctx.attr.extra_linker_flags,
                  ),
              ],
          ),
      ],
  )

  if "osx" in ctx.attr.target_full_name:
    sysroot_action_set = all_link_actions
  else:
    sysroot_action_set = all_link_actions + all_compile_actions

  sysroot_flags = feature(
      name = "sysroot_flags",
      #Only enable this if a sysroot was specified
      enabled = (ctx.attr.sysroot != ""),
      flag_sets = [
          flag_set(
              actions = sysroot_action_set,
              flag_groups = [
                  flag_group(
                      flags = [
                          "--sysroot",
                          ctx.attr.sysroot,
                      ],
                  ),
              ],
          ),
      ],
  )

  if ctx.attr.target_cpu == "x86_32":
    bit_flag = "-m32"
  else:
    bit_flag = "-m64"
  compiler_flags = feature(
      name = "default_compile_flags",
      enabled = True,
      flag_sets = [
          flag_set(
              actions = all_compile_actions,
              flag_groups = [
                  flag_group(
                      flags = [
                          bit_flag,
                          "-Wall",
                          "-no-canonical-prefixes",
                          "--target=" + ctx.attr.target_full_name,
                          "-fvisibility=hidden",
                      ] + ctx.attr.extra_compiler_flags + [
                          "-isystem",
                          ctx.attr.sysroot,
                      ],
                  ),
              ],
          ),
          flag_set(
              actions = all_compile_actions,
              flag_groups = [flag_group(flags = ["-DNDEBUG", "-O3"])],
              with_features = [with_feature_set(features = ["opt"])],
          ),
          flag_set(
              actions = all_compile_actions,
              flag_groups = [flag_group(flags = ["-g"])],
              with_features = [with_feature_set(features = ["dbg"])],
          ),
          flag_set(
              actions = all_compile_actions,
              flag_groups = [flag_group(flags = ["-O1"])],
              with_features = [with_feature_set(features = ["fastbuild"])],
          ),
      ],
  )

  features = [
      linker_flags,
      compiler_flags,
      sysroot_flags,
      feature(name = "dbg"),
      feature(name = "opt"),
  ]

  if "mingw" in ctx.attr.target_full_name:
      features.append(
          feature(
              name = "targets_windows",
              enabled = True,
          )
      )
  else:
      features.append(
          feature(
              name = "supports_pic",
              enabled = True
          )
      )

  return cc_common.create_cc_toolchain_config_info(
      abi_libc_version = ctx.attr.abi_version,
      abi_version = ctx.attr.abi_version,
      artifact_name_patterns = artifact_name_patterns,
      ctx = ctx,
      compiler = "clang",
      cxx_builtin_include_directories = [
          ctx.attr.sysroot,
          ctx.attr.extra_include,
          "/usr/local/include",
          "/usr/local/lib/clang",
      ],
      features = features,
      host_system_name = "local",
      target_cpu = ctx.attr.target_cpu,
      target_libc = ctx.attr.target_cpu,
      target_system_name = ctx.attr.target_full_name,
      toolchain_identifier = ctx.attr.target_full_name,
      tool_paths = tool_paths,
  )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {
        "abi_version": attr.string(default = "local"),
        "extra_compiler_flags": attr.string_list(),
        "extra_include": attr.string(mandatory = False),
        "extra_linker_flags": attr.string_list(),
        "linker_path": attr.string(mandatory = True),
        "sysroot": attr.string(mandatory = False),
        "target_cpu": attr.string(mandatory = True, values = ["aarch64", "ppc64", "systemz", "x86_32", "x86_64"]),
        "target_full_name": attr.string(mandatory = True),
    },
    provides = [CcToolchainConfigInfo],
)
