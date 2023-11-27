# Protocol Buffers - Google's data interchange format
# Copyright 2013 Google LLC  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

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
