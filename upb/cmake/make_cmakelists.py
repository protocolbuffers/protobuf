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

def StripFirstChar(deps):
  return [dep[1:] for dep in deps]

def IsSourceFile(name):
  return name.endswith(".c") or name.endswith(".cc")


ADD_LIBRARY_FORMAT = """
add_library(%(name)s %(type)s
    %(sources)s
)
target_include_directories(%(name)s %(keyword)s
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
"""


class BuildFileFunctions(object):
  def __init__(self, converter):
    self.converter = converter

  def _add_deps(self, kwargs, keyword=""):
    if "deps" not in kwargs:
      return
    self.converter.toplevel += "target_link_libraries(%s%s\n  %s)\n" % (
        kwargs["name"],
        keyword,
        "\n  ".join(StripFirstChar(kwargs["deps"]))
    )

  def load(self, *args):
    pass
  
  def cc_library(self, **kwargs):
    if kwargs["name"].endswith("amalgamation"):
      return
    if kwargs["name"] == "upbc_generator":
      return
    if kwargs["name"] == "lupb":
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
        found_files.append("../cmake/" + file)
      else:
        found_files.append("../" + file)

    if list(filter(IsSourceFile, files)):
      # Has sources, make this a normal library.
      self.converter.toplevel += ADD_LIBRARY_FORMAT % {
          "name": kwargs["name"],
          "type": "",
          "keyword": "PUBLIC",
          "sources": "\n  ".join(found_files),
      }
      self._add_deps(kwargs)
    else:
      # Header-only library, have to do a couple things differently.
      # For some info, see:
      #  http://mariobadr.com/creating-a-header-only-library-with-cmake.html
      self.converter.toplevel += ADD_LIBRARY_FORMAT % {
          "name": kwargs["name"],
          "type": "INTERFACE",
          "keyword": "INTERFACE",
          "sources": "",
      }
      self._add_deps(kwargs, " INTERFACE")

  def cc_binary(self, **kwargs):
    pass

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

  def bootstrap_upb_proto_library(self, **kwargs):
    pass

  def bootstrap_cc_library(self, **kwargs):
    pass

  def alias(self, **kwargs):
    pass


class Converter(object):
  def __init__(self):
    self.toplevel = ""
    self.if_lua = ""

  def convert(self):
    return self.template % {
        "toplevel": converter.toplevel,
    }

  template = textwrap.dedent("""\
    # This file was generated from BUILD using tools/make_cmakelists.py.

    cmake_minimum_required(VERSION 3.10...3.24)

    project(upb)
    set(CMAKE_C_STANDARD 99)

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

    if(NOT TARGET utf8_range)
      if(EXISTS ../../external/utf8_range)
        # utf8_range is already installed
        include_directories(../external/utf8_range)
      elseif(EXISTS ../../utf8_range)
        include_directories(../../utf8_range)
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

    enable_testing()

    %(toplevel)s

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
exec(open(sys.argv[1]).read(), GetDict(BuildFileFunctions(converter)))  # BUILD

with open(sys.argv[2], "w") as f:
  f.write(converter.convert())
