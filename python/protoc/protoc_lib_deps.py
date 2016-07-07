# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
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

# AUTO-GENERATED dependent files
CC_FILES=['google/protobuf/compiler/zip_writer.cc', 'google/protobuf/compiler/subprocess.cc', 'google/protobuf/compiler/python/python_generator.cc', 'google/protobuf/compiler/plugin.pb.cc', 'google/protobuf/compiler/plugin.cc', 'google/protobuf/compiler/cpp/cpp_string_field.cc', 'google/protobuf/compiler/cpp/cpp_service.cc', 'google/protobuf/compiler/cpp/cpp_primitive_field.cc', 'google/protobuf/compiler/cpp/cpp_message_field.cc', 'google/protobuf/compiler/cpp/cpp_message.cc', 'google/protobuf/compiler/cpp/cpp_map_field.cc', 'google/protobuf/compiler/cpp/cpp_helpers.cc', 'google/protobuf/compiler/cpp/cpp_generator.cc', 'google/protobuf/compiler/cpp/cpp_file.cc', 'google/protobuf/compiler/cpp/cpp_field.cc', 'google/protobuf/compiler/cpp/cpp_extension.cc', 'google/protobuf/compiler/cpp/cpp_enum_field.cc', 'google/protobuf/compiler/cpp/cpp_enum.cc', 'google/protobuf/compiler/command_line_interface.cc', 'google/protobuf/compiler/code_generator.cc', 'google/protobuf/wrappers.pb.cc', 'google/protobuf/wire_format.cc', 'google/protobuf/util/type_resolver_util.cc', 'google/protobuf/util/time_util.cc', 'google/protobuf/util/message_differencer.cc', 'google/protobuf/util/json_util.cc', 'google/protobuf/util/internal/utility.cc', 'google/protobuf/util/internal/type_info_test_helper.cc', 'google/protobuf/util/internal/type_info.cc', 'google/protobuf/util/internal/protostream_objectwriter.cc', 'google/protobuf/util/internal/protostream_objectsource.cc', 'google/protobuf/util/internal/proto_writer.cc', 'google/protobuf/util/internal/object_writer.cc', 'google/protobuf/util/internal/json_stream_parser.cc', 'google/protobuf/util/internal/json_objectwriter.cc', 'google/protobuf/util/internal/json_escaping.cc', 'google/protobuf/util/internal/field_mask_utility.cc', 'google/protobuf/util/internal/error_listener.cc', 'google/protobuf/util/internal/default_value_objectwriter.cc', 'google/protobuf/util/internal/datapiece.cc', 'google/protobuf/util/field_mask_util.cc', 'google/protobuf/util/field_comparator.cc', 'google/protobuf/unknown_field_set.cc', 'google/protobuf/type.pb.cc', 'google/protobuf/timestamp.pb.cc', 'google/protobuf/text_format.cc', 'google/protobuf/stubs/substitute.cc', 'google/protobuf/stubs/mathlimits.cc', 'google/protobuf/struct.pb.cc', 'google/protobuf/source_context.pb.cc', 'google/protobuf/service.cc', 'google/protobuf/reflection_ops.cc', 'google/protobuf/message.cc', 'google/protobuf/map_field.cc', 'google/protobuf/io/zero_copy_stream_impl.cc', 'google/protobuf/io/tokenizer.cc', 'google/protobuf/io/strtod.cc', 'google/protobuf/io/printer.cc', 'google/protobuf/io/gzip_stream.cc', 'google/protobuf/generated_message_reflection.cc', 'google/protobuf/field_mask.pb.cc', 'google/protobuf/extension_set_heavy.cc', 'google/protobuf/empty.pb.cc', 'google/protobuf/dynamic_message.cc', 'google/protobuf/duration.pb.cc', 'google/protobuf/descriptor_database.cc', 'google/protobuf/descriptor.pb.cc', 'google/protobuf/descriptor.cc', 'google/protobuf/compiler/parser.cc', 'google/protobuf/compiler/importer.cc', 'google/protobuf/api.pb.cc', 'google/protobuf/any.pb.cc', 'google/protobuf/any.cc', 'google/protobuf/wire_format_lite.cc', 'google/protobuf/stubs/time.cc', 'google/protobuf/stubs/strutil.cc', 'google/protobuf/stubs/structurally_valid.cc', 'google/protobuf/stubs/stringprintf.cc', 'google/protobuf/stubs/stringpiece.cc', 'google/protobuf/stubs/statusor.cc', 'google/protobuf/stubs/status.cc', 'google/protobuf/stubs/once.cc', 'google/protobuf/stubs/int128.cc', 'google/protobuf/stubs/common.cc', 'google/protobuf/stubs/bytestream.cc', 'google/protobuf/stubs/atomicops_internals_x86_msvc.cc', 'google/protobuf/stubs/atomicops_internals_x86_gcc.cc', 'google/protobuf/repeated_field.cc', 'google/protobuf/message_lite.cc', 'google/protobuf/io/zero_copy_stream_impl_lite.cc', 'google/protobuf/io/zero_copy_stream.cc', 'google/protobuf/io/coded_stream.cc', 'google/protobuf/generated_message_util.cc', 'google/protobuf/extension_set.cc', 'google/protobuf/arenastring.cc', 'google/protobuf/arena.cc']
PROTO_FILES=['google/protobuf/wrappers.proto', 'google/protobuf/type.proto', 'google/protobuf/timestamp.proto', 'google/protobuf/struct.proto', 'google/protobuf/source_context.proto', 'google/protobuf/field_mask.proto', 'google/protobuf/empty.proto', 'google/protobuf/duration.proto', 'google/protobuf/descriptor.proto', 'google/protobuf/compiler/plugin.proto', 'google/protobuf/api.proto', 'google/protobuf/any.proto']
