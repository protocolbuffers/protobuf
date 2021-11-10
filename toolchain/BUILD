load(":cc_toolchain_config.bzl", "cc_toolchain_config")

package(default_visibility = ["//visibility:public"])

filegroup(name = "empty")

TOOLCHAINS = {
    "darwin-x86_64": "cc-compiler-osx-x86_64",
    "darwin-aarch_64": "cc-compiler-osx-aarch_64",
    "linux-aarch_64": "cc-compiler-linux-aarch_64",
    "linux-ppcle_64": "cc-compiler-linux-ppcle_64",
    "linux-s390_64": "cc-compiler-linux-s390_64",
    "linux-x86_32": "cc-compiler-linux-x86_32",
    "linux-x86_64": "cc-compiler-linux-x86_64",
}

cc_toolchain_suite(
    name = "clang_suite",
    toolchains = TOOLCHAINS
)

[
    cc_toolchain(
        name = toolchain,
        all_files = ":empty",
        compiler_files = ":empty",
        dwp_files = ":empty",
        dynamic_runtime_lib = ":empty",
        linker_files = ":empty",
        objcopy_files = ":empty",
        output_licenses = ["restricted"],
        static_runtime_lib = ":empty",
        strip_files = ":empty",
        toolchain_config = ":" + cpu + "-config",
        toolchain_identifier = toolchain,
    )
    for cpu, toolchain in TOOLCHAINS.items()
]

cc_toolchain_config(
    name = "linux-aarch_64-config",
    bit_flag = "-m64",
    cpp_flag = "-lstdc++",
    include_flag = ["-I/usr/aarch64-linux-gnu/include/c++/10/aarch64-linux-gnu/",
                    "-I/usr/aarch64-linux-gnu/include/c++/10"],
    linker_path = "/usr/bin/ld",
    target_cpu = "aarch64",
    target_full_name = "aarch64-linux-gnu",
    toolchain_dir = "/usr/aarch64-linux-gnu/include",
    toolchain_name = "linux_aarch_64",
)

cc_toolchain_config(
    name = "linux-ppcle_64-config",
    bit_flag = "-m64",
    cpp_flag = "-lstdc++",
    include_flag = ["-I/usr/powerpc64le-linux-gnu/include/c++/10/powerpc64le-linux-gnu/",
                    "-I/usr/powerpc64le-linux-gnu/include/c++/10/"],
    linker_path = "/usr/bin/ld",
    target_cpu = "ppc64",
    target_full_name = "powerpc64le-linux-gnu",
    toolchain_dir = "/usr/powerpc64le-linux-gnu/include",
    toolchain_name = "linux_ppcle_64",
)

cc_toolchain_config(
    name = "linux-s390_64-config",
    bit_flag = "-m64",
    cpp_flag = "-lstdc++",
    include_flag = ["-I/usr/s390x-linux-gnu/include/c++/10/s390x-linux-gnu/",
                    "-I/usr/s390x-linux-gnu/include/c++/10/"],
    linker_path = "/usr/bin/ld",
    target_cpu = "systemz",
    target_full_name = "s390x-linux-gnu",
    toolchain_dir = "/usr/s390x-linux-gnu/include",
    toolchain_name = "linux_s390_64",
)

cc_toolchain_config(
    name = "linux-x86_32-config",
    bit_flag = "-m32",
    cpp_flag = "-lstdc++",
    linker_path = "/usr/bin/ld",
    target_cpu = "x86_32",
    target_full_name = "i386-linux-gnu",
    toolchain_dir = "/usr/include/i386-linux-gnu",
    toolchain_name = "linux_x86_32",
)

cc_toolchain_config(
    name = "linux-x86_64-config",
    bit_flag = "-m64",
    cpp_flag = "-lstdc++",
    linker_path = "/usr/bin/ld",
    target_cpu = "x86_64",
    target_full_name = "x86_64-linux-gnu",
    toolchain_dir = "/usr/include/x86_64-linux-gnu",
    toolchain_name = "linux_x86_64",
)

cc_toolchain_config(
    name = "darwin-aarch_64-config",
    bit_flag = "-m64",
    cpp_flag = "-lc++",
    include_flag = ["-I/tools/apple_sdks/xcode_13_0/macosx/usr/include/c++/v1",
                    "-I/tools/apple_sdks/xcode_13_0/macosx/usr/include"],
    linker_path = "/tools",
    sysroot = "/tools/apple_sdks/xcode_13_0/macosx",
    target_cpu = "aarch64",
    target_full_name = "aarch64-apple-macosx11.3",
    toolchain_dir = "/tools/apple_sdks/xcode_13_0/macosx",
    toolchain_name = "darwin_aarch_64",
)

cc_toolchain_config(
    name = "darwin-x86_64-config",
    bit_flag = "-m64",
    cpp_flag = "-lc++",
    include_flag = ["-I/tools/apple_sdks/xcode_13_0/macosx/usr/include/c++/v1",
                    "-I/tools/apple_sdks/xcode_13_0/macosx/usr/include"],
    linker_path = "/tools",
    sysroot = "/tools/apple_sdks/xcode_13_0/macosx",
    target_cpu = "x86_64",
    target_full_name = "x86_64-apple-macosx11.3",
    toolchain_dir = "/tools/apple_sdks/xcode_13_0/macosx",
    toolchain_name = "darwin_x86_64",
)
