load(
    ":build_defs.bzl",
    "generated_file_staleness_test",
)
load(
    "//bazel:build_defs.bzl",
    "make_shell_script",
)

licenses(["notice"])

exports_files(["staleness_test.py"])

py_library(
    name = "staleness_test_lib",
    testonly = 1,
    srcs = ["staleness_test_lib.py"],
)

py_binary(
    name = "make_cmakelists",
    srcs = ["make_cmakelists.py"],
)

genrule(
    name = "gen_cmakelists",
    srcs = [
        "//:BUILD",
        "//:WORKSPACE",
        "//:cmake_files",
        "//third_party/wyhash:cmake_files",
        ":cmake_files",
    ],
    outs = ["generated-in/CMakeLists.txt"],
    cmd = "$(location :make_cmakelists) $@",
    tools = [":make_cmakelists"],
)

genrule(
    name = "copy_json_ragel",
    srcs = ["//:upb/json/parser.c"],
    outs = ["generated-in/upb/json/parser.c"],
    cmd = "cp $< $@",
)

genrule(
    name = "copy_protos",
    srcs = ["//:descriptor_upb_proto"],
    outs = [
        "generated-in/google/protobuf/descriptor.upb.c",
        "generated-in/google/protobuf/descriptor.upb.h",
    ],
    cmd = "cp $(SRCS) $(@D)/generated-in/google/protobuf",
)

generated_file_staleness_test(
    name = "test_generated_files",
    outs = [
        "CMakeLists.txt",
        "google/protobuf/descriptor.upb.c",
        "google/protobuf/descriptor.upb.h",
        "upb/json/parser.c",
    ],
    generated_pattern = "generated-in/%s",
)

# Test the CMake build #########################################################

filegroup(
    name = "cmake_files",
    srcs = glob([
        "**/*",
    ]),
)

make_shell_script(
    name = "gen_run_cmake_build",
    out = "run_cmake_build.sh",
    contents = "find . && mkdir build && cd build && cmake ../cmake && make -j8 && make test",
)

sh_test(
    name = "cmake_build",
    srcs = ["run_cmake_build.sh"],
    data = [
        ":cmake_files",
        "//:cmake_files",
        "//third_party/wyhash:cmake_files",
    ],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
