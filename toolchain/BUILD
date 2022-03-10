load(":cc_toolchain_config.bzl", "cc_toolchain_config")

package(default_visibility = ["//visibility:public"])

filegroup(name = "empty")

TOOLCHAINS = {
    "osx-x86_64": "cc-compiler-osx-x86_64",
    "osx-aarch_64": "cc-compiler-osx-aarch_64",
    "linux-aarch_64": "cc-compiler-linux-aarch_64",
    "linux-ppcle_64": "cc-compiler-linux-ppcle_64",
    "linux-s390_64": "cc-compiler-linux-s390_64",
    "linux-x86_32": "cc-compiler-linux-x86_32",
    "linux-x86_64": "cc-compiler-linux-x86_64",
    "win32": "cc-compiler-windows-x86_32",
    "win64": "cc-compiler-windows-x86_64",
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
    extra_compiler_flags = [
        "-I/opt/manylinux/2014/aarch64/usr/include/c++/10/aarch64-redhat-linux",
        "-I/opt/manylinux/2014/aarch64/usr/include/c++/10"
    ],
    sysroot = "/opt/manylinux/2014/aarch64",
    linker_path = "/usr/bin/ld",
    target_cpu = "aarch64",
    target_full_name = "aarch64-linux-gnu",
    toolchain_name = "linux_aarch_64",
    # Don't really need this, setting it because it's required.
    toolchain_dir = "/opt/manylinux/2014/aarch64/usr/include",
)

cc_toolchain_config(
    name = "linux-ppcle_64-config",
    bit_flag = "-m64",
    cpp_flag = "-lstdc++",
    extra_compiler_flags = [
        "-I/usr/powerpc64le-linux-gnu/include/c++/8/powerpc64le-linux-gnu/",
        "-I/usr/powerpc64le-linux-gnu/include/c++/8/"
    ],
    extra_include = "/usr/include",
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
    extra_compiler_flags = [
        "-I/usr/s390x-linux-gnu/include/c++/8/s390x-linux-gnu/",
        "-I/usr/s390x-linux-gnu/include/c++/8/"
    ],
    extra_include = "/usr/include",
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
    extra_include = "/usr/include",
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
    extra_include = "/usr/include",
    linker_path = "/usr/bin/ld",
    target_cpu = "x86_64",
    target_full_name = "x86_64-linux-gnu",
    toolchain_dir = "/usr/include/x86_64-linux-gnu",
    toolchain_name = "linux_x86_64",
)

cc_toolchain_config(
    name = "osx-aarch_64-config",
    bit_flag = "-m64",
    cpp_flag = "-lc++",
    extra_compiler_flags = [
        "-I/usr/tools/apple_sdks/xcode_13_0/macosx/usr/include/c++/v1",
        "-I/usr/tools/apple_sdks/xcode_13_0/macosx/usr/include"
    ],
    extra_include = "/usr/include",
    linker_path = "/usr/tools",
    sysroot = "/usr/tools/apple_sdks/xcode_13_0/macosx",
    target_cpu = "aarch64",
    target_full_name = "aarch64-apple-macosx11.3",
    toolchain_dir = "/usr/tools/apple_sdks/xcode_13_0/macosx",
    toolchain_name = "osx_aarch_64",
)

cc_toolchain_config(
    name = "osx-x86_64-config",
    bit_flag = "-m64",
    cpp_flag = "-lc++",
    extra_compiler_flags = [
        "-I/usr/tools/apple_sdks/xcode_13_0/macosx/usr/include/c++/v1",
        "-I/usr/tools/apple_sdks/xcode_13_0/macosx/usr/include"
    ],
    extra_include = "/usr/include",
    linker_path = "/usr/tools",
    sysroot = "/usr/tools/apple_sdks/xcode_13_0/macosx",
    target_cpu = "x86_64",
    target_full_name = "x86_64-apple-macosx11.3",
    toolchain_dir = "/usr/tools/apple_sdks/xcode_13_0/macosx",
    toolchain_name = "osx_x86_64",
)

cc_toolchain_config(
    name = "win32-config",
    bit_flag = "-m32",
    cpp_flag = "-lstdc++",
    extra_compiler_flags = [
        "-isystem/usr/lib/gcc/i686-w64-mingw32/8.3-posix/include/c++",
        "-isystem/usr/lib/gcc/i686-w64-mingw32/8.3-posix/include/c++/i686-w64-mingw32",
        "-fsjlj-exceptions",
    ],
    extra_include = "/usr/lib/gcc/i686-w64-mingw32/8.3-posix/include",
    extra_linker_flags = [
        "-L/usr/lib/gcc/i686-w64-mingw32/8.3-posix",
        "-pthread",
    ],
    linker_path = "/usr/bin/ld",
    target_cpu = "x86_32",
    target_full_name = "i686-w64-mingw32",
    toolchain_dir = "/usr/i686-w64-mingw32/include",
    toolchain_name = "i686-w64-mingw32",
)

cc_toolchain_config(
    name = "win64-config",
    bit_flag = "-m64",
    cpp_flag = "-lstdc++",
    extra_compiler_flags = [
        "-isystem/usr/lib/gcc/x86_64-w64-mingw32/8.3-posix/include/c++/",
        "-isystem/usr/lib/gcc/x86_64-w64-mingw32/8.3-posix/include/c++/x86_64-w64-mingw32",
    ],
    extra_include = "/usr/lib/gcc/x86_64-w64-mingw32/8.3-posix/include",
    extra_linker_flags = [
        "-L/usr/lib/gcc/x86_64-w64-mingw32/8.3-posix",
    ],
    linker_path = "/usr/bin/ld",
    target_cpu = "x86_64",
    target_full_name = "x86_64-w64-mingw32",
    toolchain_dir = "/usr/x86_64-w64-mingw32/include",
    toolchain_name = "x86_64-w64-mingw32",
)
