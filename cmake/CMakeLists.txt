cmake_minimum_required(VERSION 3.5)

message(WARNING "Calling of cmake with source directory set to \"cmake\" subdirectory of Protocol Buffers project is deprecated. Top-level directory of Protocol Buffers project should be used instead.")

project(protobuf C CXX)

set(protobuf_DEPRECATED_CMAKE_SUBDIRECTORY_USAGE TRUE)

include(../CMakeLists.txt)
