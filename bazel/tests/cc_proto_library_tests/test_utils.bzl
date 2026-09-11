"""Common test utils for cc_proto_library tests."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel/private/oss:cc_proto_library.bzl", "cc_proto_aspect")

# We apply cc_proto_aspect as a testing_aspect because actions from the
# aspect we're looking for (like 'GenProto') may not be visible in Starlark
# unless applied this way.
CC_PROTO_TESTING_ASPECT = util.make_testing_aspect(aspects = [cc_proto_aspect])

def _filter_in_package(files, target):
    return [f for f in files if f.owner.package == target.label.package]

def get_cc_info_artifacts(target):
    """Returns a struct containing artifacts from the CcInfo provider of a target.

    Args:
      target: The target to get the CcInfo artifacts from.

    Returns:
      A struct containing the shared libraries, linker files, compiler inputs, and static libraries
      from the CcInfo provider of the target.
    """
    cc_info = target[CcInfo]
    linker_inputs = cc_info.linking_context.linker_inputs.to_list()
    linker_files_direct = []
    pic_static_libs = []
    static_libs = []
    shared_libs = []
    for li in linker_inputs:
        for lib in li.libraries:
            if lib.dynamic_library:
                shared_libs.append(lib.dynamic_library)
            if lib.interface_library:
                shared_libs.append(lib.interface_library)
            if lib.pic_static_library:
                pic_static_libs.append(lib.pic_static_library)
            if lib.static_library:
                static_libs.append(lib.static_library)
            linker_files_direct.extend(lib.objects)
            linker_files_direct.extend(lib.pic_objects)

    linker_files = depset(linker_files_direct).to_list()
    compilation_context_headers = cc_info.compilation_context.headers.to_list()
    direct_files = target[DefaultInfo].files.to_list()

    return struct(
        shared_libs = shared_libs,
        shared_libs_in_package = _filter_in_package(shared_libs, target),
        linker_files = linker_files,
        linker_files_in_package = _filter_in_package(linker_files, target),
        compilation_context_headers = compilation_context_headers,
        compilation_context_headers_in_package = _filter_in_package(compilation_context_headers, target),
        compiler_inputs = compilation_context_headers,
        compiler_inputs_in_package = _filter_in_package(compilation_context_headers, target),
        direct_files = direct_files,
        direct_files_in_package = _filter_in_package(direct_files, target),
        pic_static_libs = pic_static_libs,
        pic_static_libs_in_package = _filter_in_package(pic_static_libs, target),
        static_libs = static_libs,
        static_libs_in_package = _filter_in_package(static_libs, target),
    )
