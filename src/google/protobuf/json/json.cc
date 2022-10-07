// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/json/json.h"

#include <string>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/json/internal/parser.h"
#include "google/protobuf/json/internal/unparser.h"
#include "google/protobuf/util/type_resolver.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json {

absl::Status BinaryToJsonStream(google::protobuf::util::TypeResolver* resolver,
                                const std::string& type_url,
                                io::ZeroCopyInputStream* binary_input,
                                io::ZeroCopyOutputStream* json_output,
                                const PrintOptions& options) {
  google::protobuf::json_internal::WriterOptions opts;
  opts.add_whitespace = options.add_whitespace;
  opts.preserve_proto_field_names = options.preserve_proto_field_names;
  opts.always_print_enums_as_ints = options.always_print_enums_as_ints;
  opts.always_print_primitive_fields = options.always_print_primitive_fields;

  // TODO(b/234868512): Drop this setting.
  opts.allow_legacy_syntax = true;

  return google::protobuf::json_internal::BinaryToJsonStream(
      resolver, type_url, binary_input, json_output, opts);
}

absl::Status BinaryToJsonString(google::protobuf::util::TypeResolver* resolver,
                                const std::string& type_url,
                                const std::string& binary_input,
                                std::string* json_output,
                                const PrintOptions& options) {
  io::ArrayInputStream input_stream(binary_input.data(), binary_input.size());
  io::StringOutputStream output_stream(json_output);
  return BinaryToJsonStream(resolver, type_url, &input_stream, &output_stream,
                            options);
}

absl::Status JsonToBinaryStream(google::protobuf::util::TypeResolver* resolver,
                                const std::string& type_url,
                                io::ZeroCopyInputStream* json_input,
                                io::ZeroCopyOutputStream* binary_output,
                                const ParseOptions& options) {
  google::protobuf::json_internal::ParseOptions opts;
  opts.ignore_unknown_fields = options.ignore_unknown_fields;
  opts.case_insensitive_enum_parsing = options.case_insensitive_enum_parsing;

  // TODO(b/234868512): Drop this setting.
  opts.allow_legacy_syntax = true;

  return google::protobuf::json_internal::JsonToBinaryStream(
      resolver, type_url, json_input, binary_output, opts);
}

absl::Status JsonToBinaryString(google::protobuf::util::TypeResolver* resolver,
                                const std::string& type_url,
                                absl::string_view json_input,
                                std::string* binary_output,
                                const ParseOptions& options) {
  io::ArrayInputStream input_stream(json_input.data(), json_input.size());
  io::StringOutputStream output_stream(binary_output);
  return JsonToBinaryStream(resolver, type_url, &input_stream, &output_stream,
                            options);
}

absl::Status MessageToJsonString(const Message& message, std::string* output,
                                 const PrintOptions& options) {
  google::protobuf::json_internal::WriterOptions opts;
  opts.add_whitespace = options.add_whitespace;
  opts.preserve_proto_field_names = options.preserve_proto_field_names;
  opts.always_print_enums_as_ints = options.always_print_enums_as_ints;
  opts.always_print_primitive_fields = options.always_print_primitive_fields;

  // TODO(b/234868512): Drop this setting.
  opts.allow_legacy_syntax = true;

  return google::protobuf::json_internal::MessageToJsonString(message, output, opts);
}

absl::Status JsonStringToMessage(absl::string_view input, Message* message,
                                 const ParseOptions& options) {
  google::protobuf::json_internal::ParseOptions opts;
  opts.ignore_unknown_fields = options.ignore_unknown_fields;
  opts.case_insensitive_enum_parsing = options.case_insensitive_enum_parsing;

  // TODO(b/234868512): Drop this setting.
  opts.allow_legacy_syntax = true;

  return google::protobuf::json_internal::JsonStringToMessage(input, message, opts);
}
}  // namespace json
}  // namespace protobuf
}  // namespace google
