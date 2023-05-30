# Protocol Buffers - Google's data interchange format
# Copyright 2013 Google LLC  All rights reserved.
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
#     * Neither the name of Google Inc. nor the names of its
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

# Refactors configuration options set on all Protobuf targets
function(protobuf_configure_target target)
    target_compile_features("${target}" PUBLIC cxx_std_14)
    if (MSVC)
        # Build with multiple processes
        target_compile_options("${target}" PRIVATE /MP)
        # Set source file and execution character sets to UTF-8
        target_compile_options("${target}" PRIVATE /utf-8)
        # MSVC warning suppressions
        target_compile_options("${target}" PRIVATE
            /wd4065 # switch statement contains 'default' but no 'case' labels
            /wd4146 # unary minus operator applied to unsigned type
            /wd4244 # 'conversion' conversion from 'type1' to 'type2', possible loss of data
            /wd4251 # 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
            /wd4267 # 'var' : conversion from 'size_t' to 'type', possible loss of data
            /wd4305 # 'identifier' : truncation from 'type1' to 'type2'
            /wd4307 # 'operator' : integral constant overflow
            /wd4309 # 'conversion' : truncation of constant value
            /wd4334 # 'operator' : result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
            /wd4355 # 'this' : used in base member initializer list
            /wd4506 # no definition for inline function 'function'
            /wd4800 # 'type' : forcing value to bool 'true' or 'false' (performance warning)
            /wd4996 # The compiler encountered a deprecated declaration.
        )
        # Allow big object
        target_compile_options("${target}" PRIVATE /bigobj)
    endif ()
    if (protobuf_UNICODE)
        target_compile_definitions("${target}" PRIVATE -DUNICODE -D_UNICODE)
    endif ()
    target_compile_definitions("${target}" PRIVATE -DGOOGLE_PROTOBUF_CMAKE_BUILD)

    if (protobuf_DISABLE_RTTI)
      target_compile_definitions("${target}" PRIVATE -DGOOGLE_PROTOBUF_NO_RTTI=1)
    endif()

    # The Intel compiler isn't able to deal with noinline member functions of
    # template classes defined in headers.  As such it spams the output with
    #   warning #2196: routine is both "inline" and "noinline"
    # This silences that warning.
    if (CMAKE_CXX_COMPILER_ID MATCHES Intel)
        target_compile_options("${target}" PRIVATE -diag-disable=2196)
    endif()

    if (HAVE_ZLIB)
        target_compile_definitions("${target}" PRIVATE -DHAVE_ZLIB)
    endif ()
endfunction ()
