load(":cc_toolchain_config.bzl", "cc_toolchain_config")
package(default_visibility = ["//visibility:public"])

filegroup(name = "empty")


cc_toolchain_suite(
    name = "clang_suite",
    toolchains = {
        "aarch64": ":cc-compiler-aarch64",
        "ppcle": "cc-compiler-ppcle",
        "s390x": ":cc-compiler-s390x",
        "x86_32": ":cc-compiler-x86_32",
        "x86_64": ":cc-compiler-x86_64",
    },
)

CC_TOOLCHAIN_CPUS = [
    "aarch64",
    "ppcle",
    "s390x",
    "x86_32",
    "x86_64",
]

[
    cc_toolchain(
        name = "cc-compiler-" + cpu,
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
        toolchain_identifier = "linux_" + cpu,
    )
    for cpu in CC_TOOLCHAIN_CPUS
]

cc_toolchain_config(
    name = "aarch64-config",
    bit_flag = "-m64",
    include_flag = "-I/usr/aarch64-linux-gnu/include/c++/10/aarch64-linux-gnu/",
    target_cpu = "aarch64",
    target_full_name = "aarch64-linux-gnu",
    toolchain_dir = "/usr/aarch64-linux-gnu/include",
    toolchain_name = "linux_aarch64",
)

cc_toolchain_config(
    name = "ppcle-config",
    bit_flag = "-m64",
    include_flag = "-I/usr/powerpc64le-linux-gnu/include/c++/10/powerpc64le-linux-gnu/",
    target_cpu = "ppc64",
    target_full_name = "powerpc64le-linux-gnu",
    toolchain_dir = "/usr/powerpc64le-linux-gnu/include",
    toolchain_name = "linux_ppcle",
)

cc_toolchain_config(
    name = "s390x-config",
    bit_flag = "-m64",
    include_flag = "-I/usr/s390x-linux-gnu/include/c++/10/s390x-linux-gnu/",
    target_cpu = "systemz",
    target_full_name = "s390x-linux-gnu",
    toolchain_dir = "/usr/s390x-linux-gnu/include",
    toolchain_name = "linux_s390x",
)

cc_toolchain_config(
    name = "x86_32-config",
    bit_flag = "-m32",
    target_cpu = "x86_32",
    target_full_name = "i386-linux-gnu",
    toolchain_dir = "/usr/include/x86_32-linux-gnu",
    toolchain_name = "linux_x86_32",
)

cc_toolchain_config(
    name = "x86_64-config",
    bit_flag = "-m64",
    target_cpu = "x86_64",
    target_full_name = "x86_64-linux-gnu",
    toolchain_dir = "/usr/include/x86_64-linux-gnu",
    toolchain_name = "linux_x86_64",
)
