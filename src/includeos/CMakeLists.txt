# This file is a part of the IncludeOS unikernel - www.includeos.org
#
# Copyright 2017 Oslo and Akershus University College of Applied Sciences
# and Alfred Bratterud
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required(VERSION 2.8.9)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

set(LIB_PROTOBUF ${CMAKE_CURRENT_SOURCE_DIR}/..)

include_directories(${LIB_PROTOBUF})
include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${LIBCXX_INCLUDE_DIR})
include_directories(${NEWLIB_INCLUDE_DIR})

set(PROTOBUF_SOURCES
  ${LIB_PROTOBUF}/google/protobuf/any.cc
  ${LIB_PROTOBUF}/google/protobuf/any.pb.cc
  ${LIB_PROTOBUF}/google/protobuf/api.pb.cc
  ${LIB_PROTOBUF}/google/protobuf/arena.cc
  ${LIB_PROTOBUF}/google/protobuf/arenastring.cc
  ${LIB_PROTOBUF}/google/protobuf/compiler/importer.cc
  ${LIB_PROTOBUF}/google/protobuf/compiler/parser.cc
  ${LIB_PROTOBUF}/google/protobuf/descriptor.cc
  ${LIB_PROTOBUF}/google/protobuf/descriptor.pb.cc
  ${LIB_PROTOBUF}/google/protobuf/descriptor_database.cc
  ${LIB_PROTOBUF}/google/protobuf/duration.pb.cc
  ${LIB_PROTOBUF}/google/protobuf/dynamic_message.cc
  ${LIB_PROTOBUF}/google/protobuf/empty.pb.cc
  ${LIB_PROTOBUF}/google/protobuf/extension_set.cc
  ${LIB_PROTOBUF}/google/protobuf/extension_set_heavy.cc
  ${LIB_PROTOBUF}/google/protobuf/field_mask.pb.cc
  ${LIB_PROTOBUF}/google/protobuf/generated_message_reflection.cc
  ${LIB_PROTOBUF}/google/protobuf/generated_message_table_driven_lite.cc
  ${LIB_PROTOBUF}/google/protobuf/generated_message_table_driven.cc
  ${LIB_PROTOBUF}/google/protobuf/generated_message_util.cc
  ${LIB_PROTOBUF}/google/protobuf/io/coded_stream.cc
  ${LIB_PROTOBUF}/google/protobuf/io/gzip_stream.cc
  ${LIB_PROTOBUF}/google/protobuf/io/printer.cc
  ${LIB_PROTOBUF}/google/protobuf/io/strtod.cc
  ${LIB_PROTOBUF}/google/protobuf/io/tokenizer.cc
  ${LIB_PROTOBUF}/google/protobuf/io/zero_copy_stream.cc
  ${LIB_PROTOBUF}/google/protobuf/io/zero_copy_stream_impl_lite.cc
  ${LIB_PROTOBUF}/google/protobuf/io/zero_copy_stream_impl.cc
  ${LIB_PROTOBUF}/google/protobuf/map_field.cc
  ${LIB_PROTOBUF}/google/protobuf/message_lite.cc
  ${LIB_PROTOBUF}/google/protobuf/message.cc
  ${LIB_PROTOBUF}/google/protobuf/reflection_ops.cc
  ${LIB_PROTOBUF}/google/protobuf/repeated_field.cc
  ${LIB_PROTOBUF}/google/protobuf/service.cc
  ${LIB_PROTOBUF}/google/protobuf/source_context.pb.cc
  ${LIB_PROTOBUF}/google/protobuf/struct.pb.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/bytestream.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/common.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/int128.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/io_win32.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/mathlimits.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/once.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/status.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/statusor.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/stringpiece.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/stringprintf.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/structurally_valid.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/strutil.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/time.cc
  ${LIB_PROTOBUF}/google/protobuf/stubs/substitute.cc
  ${LIB_PROTOBUF}/google/protobuf/text_format.cc
  ${LIB_PROTOBUF}/google/protobuf/timestamp.pb.cc
  ${LIB_PROTOBUF}/google/protobuf/type.pb.cc
  ${LIB_PROTOBUF}/google/protobuf/unknown_field_set.cc
  ${LIB_PROTOBUF}/google/protobuf/util/delimited_message_util.cc
  ${LIB_PROTOBUF}/google/protobuf/util/field_comparator.cc
  ${LIB_PROTOBUF}/google/protobuf/util/field_mask_util.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/datapiece.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/default_value_objectwriter.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/error_listener.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/field_mask_utility.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/json_escaping.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/json_objectwriter.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/json_stream_parser.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/object_writer.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/proto_writer.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/protostream_objectsource.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/protostream_objectwriter.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/type_info.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/type_info_test_helper.cc
  ${LIB_PROTOBUF}/google/protobuf/util/internal/utility.cc
  ${LIB_PROTOBUF}/google/protobuf/util/json_util.cc
  ${LIB_PROTOBUF}/google/protobuf/util/message_differencer.cc
  ${LIB_PROTOBUF}/google/protobuf/util/time_util.cc
  ${LIB_PROTOBUF}/google/protobuf/util/type_resolver_util.cc
  ${LIB_PROTOBUF}/google/protobuf/wire_format_lite.cc
  ${LIB_PROTOBUF}/google/protobuf/wire_format.cc
  ${LIB_PROTOBUF}/google/protobuf/wrappers.pb.cc
)

add_library(protobuf STATIC ${PROTOBUF_SOURCES})
add_dependencies(protobuf PrecompiledLibraries)
target_compile_definitions(protobuf PRIVATE HAVE_PTHREAD=0)

install(TARGETS protobuf DESTINATION includeos/${ARCH}/lib)

install(FILES ${LIB_PROTOBUF}/google/protobuf/compiler/parser.h DESTINATION includeos/include/google/protobuf/compiler)
install(FILES ${LIB_PROTOBUF}/google/protobuf/compiler/importer.h DESTINATION includeos/include/google/protobuf/compiler)
install(FILES ${LIB_PROTOBUF}/google/protobuf/wire_format.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/any.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/generated_message_table_driven_lite.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/arenastring.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/extension_set.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/metadata_lite.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/map_entry_lite.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/api.pb.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/generated_enum_reflection.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/service.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/arena_impl.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/descriptor.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/reflection_internal.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/empty.pb.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_tsan.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/common.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_generic_gcc.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/mutex.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/type_traits.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/hash.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/stringprintf.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/stl_util.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_arm64_gcc.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/int128.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/statusor.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/platform_macros.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/substitute.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_x86_msvc.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/bytestream.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_generic_c11_atomic.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomic_sequence_num.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/once.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/casts.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/stringpiece.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_solaris.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/logging.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/status_macros.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/callback.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/port.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/template_util.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/singleton.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/status.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/mathutil.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_ppc_gcc.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/io_win32.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_arm_qnx.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_arm_gcc.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/macros.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/mathlimits.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_x86_gcc.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/fastmem.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/strutil.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_power.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/time.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_mips_gcc.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/atomicops_internals_atomicword_compat.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/map_util.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/shared_ptr.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/stubs/scoped_ptr.h DESTINATION includeos/include/google/protobuf/stubs)
install(FILES ${LIB_PROTOBUF}/google/protobuf/text_format.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/map_entry.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/wrappers.pb.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/timestamp.pb.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/generated_message_util.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/duration.pb.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/map.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/generated_message_reflection.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/repeated_field.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/arena.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/map_field_lite.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/struct.pb.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/descriptor_database.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/has_bits.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/generated_message_table_driven.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/type.pb.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/any.pb.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/map_type_handler.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/wire_format_lite_inl.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/descriptor.pb.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/message.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/message_lite.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/unknown_field_set.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/map_field.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/wire_format_lite.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/implicit_weak_message.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/map_field_inl.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/metadata.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/reflection.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/dynamic_message.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/reflection_ops.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/type_resolver.h DESTINATION includeos/include/google/protobuf/util)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/message_differencer.h DESTINATION includeos/include/google/protobuf/util)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/field_mask_util.h DESTINATION includeos/include/google/protobuf/util)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/delimited_message_util.h DESTINATION includeos/include/google/protobuf/util)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/type_resolver_util.h DESTINATION includeos/include/google/protobuf/util)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/json_util.h DESTINATION includeos/include/google/protobuf/util)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/field_comparator.h DESTINATION includeos/include/google/protobuf/util)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/proto_writer.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/expecting_objectwriter.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/object_source.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/datapiece.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/json_escaping.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/structured_objectwriter.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/location_tracker.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/constants.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/json_objectwriter.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/default_value_objectwriter.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/field_mask_utility.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/object_writer.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/error_listener.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/mock_error_listener.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/protostream_objectsource.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/type_info_test_helper.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/utility.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/json_stream_parser.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/protostream_objectwriter.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/type_info.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/internal/object_location_tracker.h DESTINATION includeos/include/google/protobuf/util/internal)
install(FILES ${LIB_PROTOBUF}/google/protobuf/util/time_util.h DESTINATION includeos/include/google/protobuf/util)
install(FILES ${LIB_PROTOBUF}/google/protobuf/generated_enum_util.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/io/zero_copy_stream_impl_lite.h DESTINATION includeos/include/google/protobuf/io)
install(FILES ${LIB_PROTOBUF}/google/protobuf/io/strtod.h DESTINATION includeos/include/google/protobuf/io)
install(FILES ${LIB_PROTOBUF}/google/protobuf/io/tokenizer.h DESTINATION includeos/include/google/protobuf/io)
install(FILES ${LIB_PROTOBUF}/google/protobuf/io/zero_copy_stream.h DESTINATION includeos/include/google/protobuf/io)
install(FILES ${LIB_PROTOBUF}/google/protobuf/io/zero_copy_stream_impl.h DESTINATION includeos/include/google/protobuf/io)
install(FILES ${LIB_PROTOBUF}/google/protobuf/io/printer.h DESTINATION includeos/include/google/protobuf/io)
install(FILES ${LIB_PROTOBUF}/google/protobuf/io/coded_stream.h DESTINATION includeos/include/google/protobuf/io)
install(FILES ${LIB_PROTOBUF}/google/protobuf/io/gzip_stream.h DESTINATION includeos/include/google/protobuf/io)
install(FILES ${LIB_PROTOBUF}/google/protobuf/io/coded_stream_inl.h DESTINATION includeos/include/google/protobuf/io)
install(FILES ${LIB_PROTOBUF}/google/protobuf/field_mask.pb.h DESTINATION includeos/include/google/protobuf)
install(FILES ${LIB_PROTOBUF}/google/protobuf/source_context.pb.h DESTINATION includeos/include/google/protobuf)
