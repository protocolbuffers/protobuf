#!/usr/bin/python
#
# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google LLC nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""A tool to convert BUILD -> CMakeLists.txt.

This tool is very upb-specific at the moment, and should not be seen as a
generic Bazel -> CMake converter.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import sys
import textwrap
import os
import glob
import re

_stages = ["_stage0", "_stage1", ""]
_block_targets = ["libupb.so", "libupb_generator.so", "conformance_upb", "conformance_upb_dynamic_minitable", "upbdev", "protoc-gen-upbdev"]
_special_targets_mapping = {
  "//:protobuf" : "protobuf::libprotobuf",
  "//src/google/protobuf/compiler:code_generator" : "protobuf::libprotoc",
  "//third_party/utf8_range": "utf8_range",
  "@utf8_range": "utf8_range",
}
_directory_sep = re.compile('[/\\:]')

def MappingThirdPartyDep(dep, target_prefix):
  if dep.startswith("//upb/"):
    return "upb_" + _directory_sep.sub("_", dep[6:])
  if dep.startswith("//upb_generator/"):
    return "upb_generator_" + _directory_sep.sub("_", dep[16:])
  if dep.startswith("//lua/"):
    return "lua_" + _directory_sep.sub("_", dep[6:])
  if dep in _special_targets_mapping:
    return _special_targets_mapping[dep]
  if dep.startswith("@com_google_absl//"):
    p = dep.rfind(":")
    if p < 0:
      return "absl::" + dep[dep.rfind("/")+1:]
    return "absl::" + dep[p+1:]
  if dep.startswith(":"):
    return _directory_sep.sub("_", target_prefix + dep[1:])
  if dep.startswith("//"):
    return _directory_sep.sub("_", dep[2:])
  return _directory_sep.sub("_", dep)

def StripFirstChar(deps, target_prefix):
  return [MappingThirdPartyDep(dep, target_prefix) for dep in deps]

def IsSourceFile(name):
  return name.endswith(".c") or name.endswith(".cc")


ADD_LIBRARY_FORMAT = """
add_library(%(name)s %(type)s
    %(sources)s
)
target_include_directories(%(name)s %(keyword)s
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(%(name)s %(keyword)s
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
"""

ADD_EXECUTABLE_FORMAT = """
add_executable(%(name)s
    %(sources)s
)
target_include_directories(%(name)s %(keyword)s
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(%(name)s %(keyword)s
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
set_target_properties(%(name)s PROPERTIES OUTPUT_NAME %(short_name)s EXPORT_NAME %(short_name)s)
"""

ADD_ALIAS_FORMAT = """
add_library(%(name)s ALIAS %(actual)s)
"""

class BuildFileFunctions(object):
  def __init__(self, converter, subdir, target_prefix):
    self.converter = converter
    self.subdir = subdir
    self.target_prefix = target_prefix
    self.alias_default_name = os.path.basename(os.path.normpath(subdir))

  def _add_deps(self, kwargs, keyword="", stage = ""):
    if "deps" not in kwargs:
      return
    self.converter.toplevel += "target_link_libraries(%s%s\n  %s)\n" % (
        self.target_prefix + kwargs["name"] + stage,
        keyword,
        "\n  ".join(StripFirstChar(kwargs["deps"], self.target_prefix))
    )

  def _add_bootstrap_deps(self, kwargs, keyword="", stage = "", deps_key = "bootstrap_deps", target_name = None):
    if deps_key not in kwargs:
      return
    if not target_name:
      target_name = kwargs["name"]
    self.converter.toplevel += "target_link_libraries({0}{1}\n  {2})\n".format(
        self.target_prefix + target_name + stage,
        keyword,
        "\n  ".join([dep + stage for dep in StripFirstChar(kwargs[deps_key], self.target_prefix)])
    )

  def load(self, *args):
    pass

  def cc_library(self, **kwargs):
    if kwargs["name"].endswith("amalgamation"):
      return
    if kwargs["name"] in _block_targets:
      return
    if "testonly" in kwargs:
      return
    files = kwargs.get("srcs", []) + kwargs.get("hdrs", [])
    found_files = []
    pregenerated_files = [
        "CMakeLists.txt", "descriptor.upb.h", "descriptor.upb.c"
    ]
    for file in files:
      if os.path.basename(file) in pregenerated_files:
        found_files.append("../" + self.subdir + "cmake/" +file)
      else:
        found_files.append("../" + self.subdir +file)

    target_name = self.target_prefix + kwargs["name"]
    if list(filter(IsSourceFile, files)):
      # Has sources, make this a normal library.
      self.converter.toplevel += ADD_LIBRARY_FORMAT % {
          "name": target_name,
          "type": "",
          "keyword": "PUBLIC",
          "sources": "\n  ".join(found_files),
      }
      self.converter.export_targets.append(target_name)
      self._add_deps(kwargs, " PUBLIC")
    else:
      # Header-only library, have to do a couple things differently.
      # For some info, see:
      #  http://mariobadr.com/creating-a-header-only-library-with-cmake.html
      self.converter.toplevel += ADD_LIBRARY_FORMAT % {
          "name": target_name,
          "type": "INTERFACE",
          "keyword": "INTERFACE",
          "sources": "",
      }
      self.converter.export_targets.append(target_name)
      self._add_deps(kwargs, " INTERFACE")

    self.converter.written_targets.add(target_name)
    if self.target_prefix == "upb_" and self.alias_default_name != kwargs["name"]:
      self.converter.toplevel += "set_target_properties(%s PROPERTIES EXPORT_NAME %s)\n" % (
        target_name,
        kwargs["name"],
      )
    if self.alias_default_name == kwargs["name"]:
      self.converter.toplevel += ADD_ALIAS_FORMAT % {
        "name": self.target_prefix[:-1],
        "actual": target_name,
      }
      self.converter.toplevel += "set_target_properties(%s PROPERTIES EXPORT_NAME %s)\n" % (
        target_name,
        self.target_prefix[:-1],
      )
      if self.alias_default_name in self.converter.alias:
        for alias_name in self.converter.alias[self.alias_default_name]:
          self.converter.toplevel += ADD_ALIAS_FORMAT % {
            "name": alias_name,
            "actual": target_name,
          }
    if target_name in self.converter.alias:
      for alias_name in self.converter.alias[target_name]:
        if alias_name != self.alias_default_name:
          self.converter.toplevel += ADD_ALIAS_FORMAT % {
            "name": alias_name,
            "actual": target_name,
          }

  def cc_binary(self, **kwargs):
    if kwargs["name"] in _block_targets:
      return

    files = kwargs.get("srcs", []) + kwargs.get("hdrs", [])
    found_files = []
    for file in files:
      found_files.append("../" + self.subdir + file)

    self.converter.toplevel += "if (UPB_ENABLE_CODEGEN)\n"
    self.converter.toplevel += ADD_EXECUTABLE_FORMAT % {
        "name": self.target_prefix + kwargs["name"],
        "keyword": "PRIVATE",
        "sources": "\n  ".join(found_files),
        "short_name": kwargs["name"],
    }
    self.converter.export_codegen_targets.append(self.target_prefix + kwargs["name"])
    self._add_deps(kwargs, " PRIVATE")
    self.converter.toplevel += "endif()\n"

  def cc_test(self, **kwargs):
    # Disable this until we properly support upb_proto_library().
    # self.converter.toplevel += "add_executable(%s\n  %s)\n" % (
    #     kwargs["name"],
    #     "\n  ".join(kwargs["srcs"])
    # )
    # self.converter.toplevel += "add_test(NAME %s COMMAND %s)\n" % (
    #     kwargs["name"],
    #     kwargs["name"],
    # )

    # if "data" in kwargs:
    #   for data_dep in kwargs["data"]:
    #     self.converter.toplevel += textwrap.dedent("""\
    #       add_custom_command(
    #           TARGET %s POST_BUILD
    #           COMMAND ${CMAKE_COMMAND} -E copy
    #                   ${CMAKE_SOURCE_DIR}/%s
    #                   ${CMAKE_CURRENT_BINARY_DIR}/%s)\n""" % (
    #       kwargs["name"], data_dep, data_dep
    #     ))

    # self._add_deps(kwargs)
    pass

  def cc_fuzz_test(self, **kwargs):
    pass

  def pkg_files(self, **kwargs):
    pass

  def py_library(self, **kwargs):
    pass

  def py_binary(self, **kwargs):
    pass

  def lua_proto_library(self, **kwargs):
    pass

  def sh_test(self, **kwargs):
    pass

  def make_shell_script(self, **kwargs):
    pass

  def exports_files(self, files, **kwargs):
    pass

  def proto_library(self, **kwargs):
    pass

  def cc_proto_library(self, **kwargs):
    pass

  def staleness_test(self, **kwargs):
    pass

  def upb_amalgamation(self, **kwargs):
    pass

  def upb_proto_library(self, **kwargs):
    pass

  def upb_minitable_proto_library(self, **kwargs):
    pass

  def upb_proto_library_copts(self, **kwargs):
    pass

  def upb_proto_reflection_library(self, **kwargs):
    pass

  def upb_proto_srcs(self, **kwargs):
    pass

  def genrule(self, **kwargs):
    pass

  def config_setting(self, **kwargs):
    pass

  def upb_fasttable_enabled(self, **kwargs):
    pass

  def select(self, arg_dict):
    return []

  def glob(self, *args, **kwargs):
    return []

  def licenses(self, *args):
    pass

  def filegroup(self, **kwargs):
    pass

  def map_dep(self, arg):
    return arg

  def package_group(self, **kwargs):
    pass

  def bool_flag(self, **kwargs):
    pass

  def proto_lang_toolchain(self, **kwargs):
    pass

  def upb_c_proto_library(self, **kwargs):
    pass

  def compile_edition_defaults(self, **kwargs):
    pass

  def embed_edition_defaults(self, **kwargs):
    pass

  def bootstrap_upb_proto_library(self, **kwargs):
    if kwargs["name"] in _block_targets:
      return
    if "oss_src_files" not in kwargs:
      return
    oss_src_files = kwargs["oss_src_files"]
    if not oss_src_files:
      return
    if "base_dir" not in kwargs:
      base_dir = self.subdir
    else:
      base_dir = self.subdir + kwargs["base_dir"]
    while base_dir.endswith("/") or base_dir.endswith("\\"):
      base_dir = base_dir[0:-1]

    oss_src_files_prefix = [".".join(x.split(".")[0:-1]) for x in oss_src_files]
    self.converter.toplevel += "if (UPB_ENABLE_CODEGEN)\n"
    # Stage0
    self.converter.toplevel += ADD_LIBRARY_FORMAT % {
        "name": self.target_prefix + kwargs["name"] + _stages[0],
        "type": "",
        "keyword": "PUBLIC",
        "sources": "\n  ".join(["../{0}/stage0/{1}.upb.h\n  ../{0}/stage0/{1}.upb.c".format(base_dir, x) for x in oss_src_files_prefix]),
    }
    self.converter.toplevel += "target_include_directories({0}\n".format(self.target_prefix + kwargs["name"] + _stages[0])
    self.converter.toplevel += "  BEFORE PUBLIC \"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../" + "{0}/stage0>\")\n".format(base_dir)
    self.converter.toplevel += "target_link_libraries({0} PUBLIC\n".format(self.target_prefix + kwargs["name"] + _stages[0])
    self.converter.toplevel += "  upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me\n"
    self.converter.toplevel += "  upb_mini_table)\n".format(self.target_prefix + kwargs["name"] + _stages[0])
    self._add_bootstrap_deps(kwargs, " PUBLIC", _stages[0], "deps")

    # Stage1
    stage1_generated_dir = "${CMAKE_CURRENT_BINARY_DIR}/" + _stages[1] + "/" + self.target_prefix + kwargs["name"]
    self.converter.toplevel += "file(MAKE_DIRECTORY \"{0}\")\n".format(stage1_generated_dir)
    self.converter.toplevel += "add_custom_command(\n"
    self.converter.toplevel += "  OUTPUT\n    {0}\n".format(
      "\n    ".join([
        "\n    ".join(["{0}/{1}.upb.h\n    {0}/{1}.upb.c".format(stage1_generated_dir, x) for x in oss_src_files_prefix]),
        "\n    ".join(["{0}/{1}.upb_minitable.h\n    {0}/{1}.upb_minitable.c".format(stage1_generated_dir, x) for x in oss_src_files_prefix])
      ])
    )
    self.converter.toplevel += "  DEPENDS\n    {0}\n".format(
      "\n    ".join(["{0}/{1}".format("${UPB_HOST_INCLUDE_DIR}", x) for x in oss_src_files])
    )
    self.converter.toplevel += "  COMMAND\n"
    self.converter.toplevel += "    \"${PROTOC_PROGRAM}\"\n    \"-I${UPB_HOST_INCLUDE_DIR}\"\n"
    self.converter.toplevel += "    \"--plugin=protoc-gen-upb=\\$<TARGET_FILE:upb_generator_protoc-gen-upb_stage0>\"\n"
    self.converter.toplevel += "    \"--plugin=protoc-gen-upb_minitable=\\$<TARGET_FILE:upb_generator_protoc-gen-upb_minitable_stage0>\"\n"
    self.converter.toplevel += "    \"--upb_out={0}\"\n".format(stage1_generated_dir)
    self.converter.toplevel += "    \"--upb_minitable_out={0}\"\n".format(stage1_generated_dir)
    self.converter.toplevel += "    {0}\n".format(
      "\n    ".join(["{0}/{1}".format("${UPB_HOST_INCLUDE_DIR}", x) for x in oss_src_files])
    )
    self.converter.toplevel += ")\n"

    self.converter.toplevel += ADD_LIBRARY_FORMAT % {
        "name": self.target_prefix + kwargs["name"] + "_minitable" + _stages[1],
        "type": "",
        "keyword": "PUBLIC",
        "sources": "\n  ".join(["{0}/{1}.upb_minitable.h\n  {0}/{1}.upb_minitable.c".format(stage1_generated_dir, x) for x in oss_src_files_prefix]),
    }
    self.converter.toplevel += "target_include_directories({0}\n".format(self.target_prefix + kwargs["name"] + "_minitable" + _stages[1])
    self.converter.toplevel += "  BEFORE PUBLIC \"$<BUILD_INTERFACE:{0}>\")\n".format(stage1_generated_dir)
    self.converter.toplevel += "target_link_libraries({0} PUBLIC\n".format(self.target_prefix + kwargs["name"] + "_minitable" + _stages[1])
    self.converter.toplevel += "  upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me\n"
    self.converter.toplevel += ")\n".format(self.target_prefix + kwargs["name"] + "_minitable" + _stages[1])
    self._add_bootstrap_deps(kwargs, " PUBLIC", _stages[1], "deps", kwargs["name"] + "_minitable")

    self.converter.toplevel += ADD_LIBRARY_FORMAT % {
        "name": self.target_prefix + kwargs["name"] + _stages[1],
        "type": "",
        "keyword": "PUBLIC",
        "sources": "\n  ".join(["{0}/{1}.upb.h\n  {0}/{1}.upb.c".format(stage1_generated_dir, x) for x in oss_src_files_prefix]),
    }
    self.converter.toplevel += "target_include_directories({0}\n".format(self.target_prefix + kwargs["name"] + _stages[1])
    self.converter.toplevel += "  BEFORE PUBLIC \"$<BUILD_INTERFACE:{0}>\")\n".format(stage1_generated_dir)
    self.converter.toplevel += "target_link_libraries({0} PUBLIC\n".format(self.target_prefix + kwargs["name"] + _stages[1])
    self.converter.toplevel += "  upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me\n"
    self.converter.toplevel += "  " + self.target_prefix + kwargs["name"] + "_minitable" + _stages[1] + "\n"
    self.converter.toplevel += ")\n".format(self.target_prefix + kwargs["name"] + _stages[1])
    self._add_bootstrap_deps(kwargs, " PUBLIC", _stages[1], "deps")

    # Stage2
    stage2_generated_dir = "${CMAKE_CURRENT_BINARY_DIR}/stage2/" + self.target_prefix + kwargs["name"]
    self.converter.toplevel += "file(MAKE_DIRECTORY \"{0}\")\n".format(stage2_generated_dir)
    self.converter.toplevel += "add_custom_command(\n"
    self.converter.toplevel += "  OUTPUT\n    {0}\n".format(
      "\n    ".join([
        "\n    ".join(["{0}/{1}.upb.h\n    {0}/{1}.upb.c".format(stage2_generated_dir, x) for x in oss_src_files_prefix]),
        "\n    ".join(["{0}/{1}.upb_minitable.h\n    {0}/{1}.upb_minitable.c".format(stage2_generated_dir, x) for x in oss_src_files_prefix]),
      ])
    )
    self.converter.toplevel += "  DEPENDS\n    {0}\n".format(
      "\n    ".join(["{0}/{1}".format("${UPB_HOST_INCLUDE_DIR}", x) for x in oss_src_files])
    )
    self.converter.toplevel += "  COMMAND\n"
    self.converter.toplevel += "    \"${PROTOC_PROGRAM}\"\n    \"-I${UPB_HOST_INCLUDE_DIR}\"\n"
    self.converter.toplevel += "    \"--plugin=protoc-gen-upb=\\$<TARGET_FILE:upb_generator_protoc-gen-upb_stage1>\"\n"
    self.converter.toplevel += "    \"--plugin=protoc-gen-upb_minitable=\\$<TARGET_FILE:upb_generator_protoc-gen-upb_minitable_stage1>\"\n"
    self.converter.toplevel += "    \"--upb_out={0}\"\n".format(stage2_generated_dir)
    self.converter.toplevel += "    \"--upb_minitable_out={0}\"\n".format(stage2_generated_dir)
    self.converter.toplevel += "    {0}\n".format(
      "\n    ".join(["{0}/{1}".format("${UPB_HOST_INCLUDE_DIR}", x) for x in oss_src_files])
    )
    self.converter.toplevel += ")\n"

    self.converter.toplevel += "add_custom_command(\n"
    self.converter.toplevel += "  OUTPUT\n    {0}\n".format(
      "\n    ".join([
        "\n    ".join(["{0}/{1}.upbdefs.h\n    {0}/{1}.upbdefs.c".format(stage2_generated_dir, x) for x in oss_src_files_prefix]),
        "\n    ".join(["{0}/{1}_pb.lua".format(stage2_generated_dir, x) for x in oss_src_files_prefix])
      ])
    )
    self.converter.toplevel += "  DEPENDS\n    {0}\n".format(
      "\n    ".join(["{0}/{1}".format("${UPB_HOST_INCLUDE_DIR}", x) for x in oss_src_files])
    )
    self.converter.toplevel += "  COMMAND\n"
    self.converter.toplevel += "    \"${PROTOC_PROGRAM}\"\n    \"-I${UPB_HOST_INCLUDE_DIR}\"\n"
    self.converter.toplevel += "    \"--plugin=protoc-gen-upbdefs=\\$<TARGET_FILE:upb_generator_protoc-gen-upbdefs>\"\n"
    self.converter.toplevel += "    \"--plugin=protoc-gen-lua=\\$<TARGET_FILE:lua_protoc-gen-lua>\"\n"
    self.converter.toplevel += "    \"--upbdefs_out={0}\"\n".format(stage2_generated_dir)
    self.converter.toplevel += "    \"--lua_out={0}\"\n".format(stage2_generated_dir)
    self.converter.toplevel += "    {0}\n".format(
      "\n    ".join(["{0}/{1}".format("${UPB_HOST_INCLUDE_DIR}", x) for x in oss_src_files])
    )
    self.converter.toplevel += ")\n"

    self.converter.toplevel += ADD_LIBRARY_FORMAT % {
        "name": self.target_prefix + kwargs["name"] + "_minitable" + _stages[2],
        "type": "",
        "keyword": "PUBLIC",
        "sources": "\n  ".join(["{0}/{1}.upb_minitable.h\n  {0}/{1}.upb_minitable.c".format(stage2_generated_dir, x) for x in oss_src_files_prefix]),
    }
    self.converter.toplevel += "target_include_directories({0}\n".format(self.target_prefix + kwargs["name"] + "_minitable" + _stages[2])
    self.converter.toplevel += "  BEFORE PUBLIC \"$<BUILD_INTERFACE:{0}>\")\n".format(stage2_generated_dir)
    self.converter.toplevel += "target_link_libraries({0} PUBLIC\n".format(self.target_prefix + kwargs["name"] + "_minitable" + _stages[2])
    self.converter.toplevel += "  upb_upb\n"
    self.converter.toplevel += ")\n".format(self.target_prefix + kwargs["name"] + "_minitable" + _stages[2])
    self._add_bootstrap_deps(kwargs, " PUBLIC", _stages[2], "deps", kwargs["name"] + "_minitable")

    self.converter.toplevel += ADD_LIBRARY_FORMAT % {
        "name": self.target_prefix + kwargs["name"] + _stages[2],
        "type": "",
        "keyword": "PUBLIC",
        "sources": "\n  ".join(["{0}/{1}.upb.h\n  {0}/{1}.upb.c".format(stage2_generated_dir, x) for x in oss_src_files_prefix]),
    }
    self.converter.toplevel += "target_include_directories({0}\n".format(self.target_prefix + kwargs["name"] + _stages[2])
    self.converter.toplevel += "  BEFORE PUBLIC \"$<BUILD_INTERFACE:{0}>\")\n".format(stage2_generated_dir)
    self.converter.toplevel += "target_link_libraries({0} PUBLIC\n".format(self.target_prefix + kwargs["name"] + _stages[2])
    self.converter.toplevel += "  upb_upb\n"
    self.converter.toplevel += "  " + self.target_prefix + kwargs["name"] + "_minitable" + _stages[2] + "\n"
    self.converter.toplevel += ")\n".format(self.target_prefix + kwargs["name"] + _stages[2])
    self._add_bootstrap_deps(kwargs, " PUBLIC", _stages[2], "deps")

    self.converter.toplevel += ADD_LIBRARY_FORMAT % {
        "name": self.target_prefix + kwargs["name"] + _stages[2] + "_defs",
        "type": "",
        "keyword": "PUBLIC",
        "sources": "\n  ".join(["{0}/{1}.upbdefs.h\n  {0}/{1}.upbdefs.c".format(stage2_generated_dir, x) for x in oss_src_files_prefix]),
    }
    self.converter.toplevel += "target_include_directories({0}\n".format(self.target_prefix + kwargs["name"] + _stages[2] + "_defs")
    self.converter.toplevel += "  BEFORE PUBLIC \"$<BUILD_INTERFACE:{0}>\")\n".format(stage2_generated_dir)
    self.converter.toplevel += "target_link_libraries({0} PUBLIC\n".format(self.target_prefix + kwargs["name"] + _stages[2] + "_defs")
    self.converter.toplevel += "  {0}\n".format(self.target_prefix + kwargs["name"] + _stages[2])
    self.converter.toplevel += ")\n".format(self.target_prefix + kwargs["name"] + _stages[2])

    self.converter.toplevel += "install(\n"
    self.converter.toplevel += "  FILES\n    {0}\n".format(
      "\n    ".join([
        "\n    ".join(["{0}/{1}.upb.h\n    {0}/{1}.upb.c".format(stage2_generated_dir, x) for x in oss_src_files_prefix]),
        "\n    ".join(["{0}/{1}.upb_minitable.h\n    {0}/{1}.upb_minitable.c".format(stage2_generated_dir, x) for x in oss_src_files_prefix]),
        "\n    ".join(["{0}/{1}.upbdefs.h\n    {0}/{1}.upbdefs.c".format(stage2_generated_dir, x) for x in oss_src_files_prefix]),
        "\n    ".join(["{0}/{1}_pb.lua".format(stage2_generated_dir, x) for x in oss_src_files_prefix])
      ])
    )
    self.converter.toplevel += "  DESTINATION \"include/{0}\"\n".format(os.path.dirname(oss_src_files_prefix[0]))
    self.converter.toplevel += ")\n"

    self.converter.export_codegen_targets.append(self.target_prefix + kwargs["name"] + _stages[2])
    self.converter.export_codegen_targets.append(self.target_prefix + kwargs["name"] + _stages[2] + "_minitable")
    self.converter.export_codegen_targets.append(self.target_prefix + kwargs["name"] + _stages[2] + "_defs")

    self.converter.toplevel += "endif()\n"

  def bootstrap_cc_library(self, **kwargs):
    if kwargs["name"] in _block_targets:
      return
    files = kwargs.get("srcs", []) + kwargs.get("hdrs", [])
    found_files = []
    for file in files:
      found_files.append("../" + self.subdir + file)

    self.converter.toplevel += "if (UPB_ENABLE_CODEGEN)\n"
    for stage in _stages:
      stage_name = self.target_prefix + kwargs["name"] + stage
      if list(filter(IsSourceFile, files)):
        # Has sources, make this a normal library.
        self.converter.toplevel += ADD_LIBRARY_FORMAT % {
            "name": stage_name,
            "type": "",
            "keyword": "PUBLIC",
            "sources": "\n  ".join(found_files),
        }
        self._add_deps(kwargs, " PUBLIC", stage)
        self._add_bootstrap_deps(kwargs, " PUBLIC", stage)
      else:
        # Header-only library, have to do a couple things differently.
        # For some info, see:
        #  http://mariobadr.com/creating-a-header-only-library-with-cmake.html
        self.converter.toplevel += ADD_LIBRARY_FORMAT % {
            "name": stage_name,
            "type": "INTERFACE",
            "keyword": "INTERFACE",
            "sources": "",
        }
        self._add_deps(kwargs, " INTERFACE", stage)
        self._add_bootstrap_deps(kwargs, " INTERFACE", stage)
    target_name = self.target_prefix + kwargs["name"]
    self.converter.export_codegen_targets.append(target_name)
    self.converter.written_targets.add(target_name)
    if self.alias_default_name == kwargs["name"]:
      self.converter.toplevel += ADD_ALIAS_FORMAT % {
        "name": self.target_prefix[:-1],
        "actual": target_name,
      }
      self.converter.toplevel += "set_target_properties(%s PROPERTIES EXPORT_NAME %s)\n" % (
        target_name,
        self.target_prefix[:-1],
      )
      if self.alias_default_name in self.converter.alias:
        for alias_name in self.converter.alias[self.alias_default_name]:
          self.converter.toplevel += ADD_ALIAS_FORMAT % {
            "name": alias_name,
            "actual": target_name,
          }
    if target_name in self.converter.alias:
      for alias_name in self.converter.alias[target_name]:
        if alias_name != self.alias_default_name:
          self.converter.toplevel += ADD_ALIAS_FORMAT % {
            "name": alias_name,
            "actual": target_name,
          }
    self.converter.toplevel += "endif()\n"

  def bootstrap_cc_binary(self, **kwargs):
    if kwargs["name"] in _block_targets:
      return
    files = kwargs.get("srcs", []) + kwargs.get("hdrs", [])
    found_files = []
    for file in files:
      found_files.append("../" + self.subdir + file)

    # Has sources, make this a normal library.
    self.converter.toplevel += "if (UPB_ENABLE_CODEGEN)\n"
    for stage in _stages:
      stage_name = self.target_prefix + kwargs["name"] + stage
      self.converter.toplevel += ADD_EXECUTABLE_FORMAT % {
          "name": stage_name,
          "keyword": "PRIVATE",
          "sources": "\n  ".join(found_files),
          "short_name": kwargs["name"] + stage,
      }
      self._add_deps(kwargs, " PRIVATE", stage)
      self._add_bootstrap_deps(kwargs, " PRIVATE", stage)
    self.converter.export_codegen_targets.append(self.target_prefix + kwargs["name"])
    self.converter.toplevel += "endif()\n"

  def alias(self, **kwargs):
    actual = kwargs["actual"]
    if actual.startswith("//"):
      actual = actual[2:]
    actual = _directory_sep.sub("_", actual)
    alias_name = self.target_prefix + kwargs["name"]
    if alias_name == actual:
      return
    if actual in self.converter.alias:
      self.converter.alias[actual].append(alias_name)
    else:
      self.converter.alias[actual] = [alias_name]
    if actual in self.converter.written_targets:
      self.converter.toplevel += ADD_ALIAS_FORMAT % {
        "name": alias_name,
        "actual": actual,
      }


class Converter(object):
  def __init__(self):
    self.toplevel = ""
    self.if_lua = ""
    self.export_targets = []
    self.export_codegen_targets = []
    self.written_targets = set()
    self.alias = {}

  def convert(self):
    return self.template % {
        "toplevel": converter.toplevel,
        "export_targets": ' '.join(converter.export_targets),
        "export_codegen_targets": ' '.join(converter.export_codegen_targets)
    }

  template = textwrap.dedent("""\
    # This file was generated from BUILD using tools/make_cmakelists.py.

    cmake_minimum_required(VERSION 3.10...3.24)

    project(upb)
    set(CMAKE_C_STANDARD 99)

    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
    if(CMAKE_SOURCE_DIR STREQUAL upb_SOURCE_DIR)
      if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
        set(CMAKE_CXX_STANDARD 23)
      elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
        set(CMAKE_CXX_STANDARD 20)
      else()
        set(CMAKE_CXX_STANDARD 17)
      endif()
      set(CMAKE_CXX_STANDARD_REQUIRED ON)
    endif()
    if(CMAKE_CROSSCOMPILING)
      option(UPB_ENABLE_CODEGEN "Build codegen plugins of upb" OFF)
    else()
      option(UPB_ENABLE_CODEGEN "Build codegens plugins of upb" ON)
    endif()

    # Prevent CMake from setting -rdynamic on Linux (!!).
    SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
    SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

    # Set default build type.
    if(NOT CMAKE_BUILD_TYPE)
      message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
      set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
          "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
          FORCE)
    endif()

    # When using Ninja, compiler output won't be colorized without this.
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-fdiagnostics-color=always SUPPORTS_COLOR_ALWAYS)
    if(SUPPORTS_COLOR_ALWAYS)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
    endif()

    # Implement ASAN/UBSAN options
    if(UPB_ENABLE_ASAN)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
    endif()

    if(UPB_ENABLE_UBSAN)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
    endif()

    find_package(utf8_range QUIET)
    if(NOT TARGET utf8_range AND TARGET utf8_range::utf8_range)
      add_library(utf8_range ALIAS utf8_range::utf8_range)
      if(EXISTS "${utf8_range_DIR}/../../include/utf8_range.h")
        include_directories("${utf8_range_DIR}/../../include/")
      elseif(EXISTS "${utf8_range_DIR}/../../../include/utf8_range.h")
        include_directories("${utf8_range_DIR}/../../../include/")
      endif()
    elseif(NOT TARGET utf8_range)
      if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../third_party/utf8_range")
        # utf8_range is already installed
        include_directories("${CMAKE_CURRENT_LIST_DIR}/../../third_party/utf8_range")
      else()
        include(FetchContent)
        FetchContent_Declare(
          utf8_range
          GIT_REPOSITORY "https://github.com/protocolbuffers/utf8_range.git"
          GIT_TAG "d863bc33e15cba6d873c878dcca9e6fe52b2f8cb"
        )
        FetchContent_GetProperties(utf8_range)
        if(NOT utf8_range_POPULATED)
          FetchContent_Populate(utf8_range)
          include_directories(${utf8_range_SOURCE_DIR})
        endif()
      endif()
    endif()

    if(APPLE)
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup -flat_namespace")
    elseif(UNIX)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id")
    endif()

    if (MSVC)
      add_compile_options(/wd4146 /wd4703 -D_CRT_SECURE_NO_WARNINGS)
    endif()

    enable_testing()

    if (UPB_ENABLE_CODEGEN)
      find_package(absl CONFIG REQUIRED)
      find_package(protobuf CONFIG REQUIRED)
      if(NOT UPB_HOST_INCLUDE_DIR)
        if(TARGET protobuf::libprotobuf)
          get_target_property(UPB_HOST_INCLUDE_DIR protobuf::libprotobuf INTERFACE_INCLUDE_DIRECTORIES)
        elseif(Protobuf_INCLUDE_DIR)
          set(UPB_HOST_INCLUDE_DIR "${Protobuf_INCLUDE_DIR}")
        else()
          set(UPB_HOST_INCLUDE_DIR "${PROTOBUF_INCLUDE_DIR}")
        endif()
      endif()
    endif()

    %(toplevel)s

    if (UPB_ENABLE_CODEGEN)
      set(PROTOC_PROGRAM "\$<TARGET_FILE:protobuf::protoc>")
      set(PROTOC_GEN_UPB_PROGRAM "\$<TARGET_FILE:upb_generator_protoc-gen-upb>")
      set(PROTOC_GEN_UPB_MINITABLE_PROGRAM "\$<TARGET_FILE:upb_generator_protoc-gen-upb_minitable>")
      set(PROTOC_GEN_UPBDEFS_PROGRAM "\$<TARGET_FILE:upb_generator_protoc-gen-upbdefs>")
      set(PROTOC_GEN_UPBLUA_PROGRAM "\$<TARGET_FILE:lua_protoc-gen-lua>")

      unset(UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_LUAS)
      unset(UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_HEADERS)
      unset(UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_SOURCES)
      unset(UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_FILES)
      set(UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_NAMES any api duration empty
          field_mask source_context struct timestamp type wrappers)
      foreach(PROTO_NAME IN LISTS UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_NAMES)
        list(APPEND UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_FILES
              "${UPB_HOST_INCLUDE_DIR}/google/protobuf/${PROTO_NAME}.proto")
        list(APPEND UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_LUAS
              "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}_pb.lua")
        list(APPEND UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_HEADERS
              "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upb.h"
              "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upb_minitable.h"
              "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upbdefs.h")
        list(APPEND UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_SOURCES
              "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upb.c"
              "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upb_minitable.c"
              "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upbdefs.c")
      endforeach()
      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/stage2")
      add_custom_command(
        OUTPUT ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_LUAS}
              ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_HEADERS}
              ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_SOURCES}
        DEPENDS ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_FILES}
        COMMAND
          "${PROTOC_PROGRAM}"
          "-I${UPB_HOST_INCLUDE_DIR}"
          "--plugin=protoc-gen-upb=${PROTOC_GEN_UPB_PROGRAM}"
          "--plugin=protoc-gen-upb_minitable=${PROTOC_GEN_UPB_MINITABLE_PROGRAM}"
          "--plugin=protoc-gen-upbdefs=${PROTOC_GEN_UPBDEFS_PROGRAM}"
          "--plugin=protoc-gen-lua=${PROTOC_GEN_UPBLUA_PROGRAM}"
          "--upb_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types"
          "--upb_minitable_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types"
          "--upbdefs_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types"
          "--lua_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types"
          ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_FILES}
      )
      add_library(upb_well_known_types ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_HEADERS}
        ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_SOURCES})
      target_include_directories(upb_well_known_types PUBLIC "\$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types>")
      set_target_properties(upb_well_known_types PROPERTIES EXPORT_NAME "well_known_types")
      target_link_libraries(upb_well_known_types PUBLIC upb_upb upb_descriptor_upb_proto)
    endif()

    include(GNUInstallDirs)
    install(
      DIRECTORY ../../upb
      DESTINATION include
      FILES_MATCHING
      PATTERN "*.h"
      PATTERN "*.hpp"
      PATTERN "*.inc"
    )
    install(TARGETS
      %(export_targets)s
      EXPORT upb-config
    )
    if (UPB_ENABLE_CODEGEN)
      install(
        FILES
          ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_LUAS}
          ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_HEADERS}
        DESTINATION include/google/protobuf
      )
      install(
        DIRECTORY ../../lua/
        DESTINATION share/upb/lua
      )
      install(TARGETS
        upb_well_known_types
        %(export_codegen_targets)s
        ${UPB_CODEGEN_TARGETS}
        EXPORT upb-config
      )
    endif()
    install(EXPORT upb-config NAMESPACE protobuf:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/upb")
  """)

data = {}
converter = Converter()

def GetDict(obj):
  ret = {}
  ret["UPB_DEFAULT_COPTS"] = []  # HACK
  ret["UPB_DEFAULT_CPPOPTS"] = []  # HACK
  for k in dir(obj):
    if not k.startswith("_"):
      ret[k] = getattr(obj, k);
  return ret

globs = GetDict(converter)

# We take the BUILD path as a command-line argument to ensure that we can find
# it regardless of how exactly Bazel was invoked.
build_dir = os.path.dirname(sys.argv[1])
for build_file in glob.glob(os.path.join(build_dir, "*", 'BUILD')):
  subdir = os.path.dirname(os.path.relpath(build_file, build_dir))
  exec(open(build_file).read(), GetDict(BuildFileFunctions(converter, subdir + "/", "upb_" + _directory_sep.sub("_", subdir) + "_")))  # BUILD
exec(open(sys.argv[1]).read(), GetDict(BuildFileFunctions(converter, "", "upb_")))  # BUILD
exec(open(os.path.join(build_dir, '..', 'upb_generator', 'BUILD')).read(), GetDict(BuildFileFunctions(converter, "../upb_generator/", "upb_generator_")))  # util/BUILD
exec(open(os.path.join(build_dir, '..', 'lua', 'BUILD.bazel')).read(), GetDict(BuildFileFunctions(converter, "../lua/", "lua_")))  # util/BUILD

with open(sys.argv[2], "w") as f:
  f.write(converter.convert())
