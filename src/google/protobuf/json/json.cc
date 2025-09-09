// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
  opts.always_print_fields_with_no_presence =
      options.always_print_fields_with_no_presence;
  opts.unquote_int64_if_possible = options.unquote_int64_if_possible;

  // TODO: Drop this setting.
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

  // TODO: Drop this setting.
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
  io::StringOutputStream out(output);
  return MessageToJsonStream(message, &out, options);
}

absl::Status MessageToJsonStream(const Message& message,
                                 io::ZeroCopyOutputStream* json_output,
                                 const PrintOptions& options) {
  google::protobuf::json_internal::WriterOptions opts;
  opts.add_whitespace = options.add_whitespace;
  opts.preserve_proto_field_names = options.preserve_proto_field_names;
  opts.always_print_enums_as_ints = options.always_print_enums_as_ints;
  opts.always_print_fields_with_no_presence =
      options.always_print_fields_with_no_presence;
  opts.unquote_int64_if_possible = options.unquote_int64_if_possible;

  // TODO: Drop this setting.
  opts.allow_legacy_syntax = true;

  return google::protobuf::json_internal::MessageToJsonStream(message, json_output, opts);
}

absl::Status JsonStringToMessage(absl::string_view input, Message* message,
                                 const ParseOptions& options) {
  io::ArrayInputStream input_stream(input.data(), input.size());
  return JsonStreamToMessage(&input_stream, message, options);
}

absl::Status JsonStreamToMessage(io::ZeroCopyInputStream* input,
                                 Message* message,
                                 const ParseOptions& options) {
  google::protobuf::json_internal::ParseOptions opts;
  opts.ignore_unknown_fields = options.ignore_unknown_fields;
  opts.case_insensitive_enum_parsing = options.case_insensitive_enum_parsing;

  // TODO: Drop this setting.
  opts.allow_legacy_syntax = true;

  return google::protobuf::json_internal::JsonStreamToMessage(input, message, opts);
}
}  // namespace json
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
