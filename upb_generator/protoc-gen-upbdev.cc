// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <iostream>
#include <string>

#include "google/protobuf/compiler/plugin.upb.h"
#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb_generator/subprocess.h"
#include "upb_generator/upbdev.h"

static constexpr char kDefaultPlugin[] = "protoc_dart_plugin";

int main() {
  upb_Arena* a = upb_Arena_New();
  upb_Status status;
  upb_Status_Clear(&status);

  // Read (binary) stdin into a string.
  const std::string input = {std::istreambuf_iterator<char>(std::cin),
                             std::istreambuf_iterator<char>()};

  // Parse the request.
  auto inner_request = google_protobuf_compiler_CodeGeneratorRequest_parse(
      input.c_str(), input.size(), a);

  // Check the request for a plugin name.
  std::string plugin = kDefaultPlugin;
  if (google_protobuf_compiler_CodeGeneratorRequest_has_parameter(inner_request)) {
    auto param = google_protobuf_compiler_CodeGeneratorRequest_parameter(inner_request);
    plugin = std::string(param.data, param.size);
  }

  // Wrap the request inside a upb_CodeGeneratorRequest and JSON-encode it.
  const upb_StringView sv =
      upbdev_ProcessInput(input.data(), input.size(), a, &status);
  if (!upb_Status_IsOk(&status)) {
    std::cerr << status.msg << '\n';
    return -1;
  }

  // Launch the subprocess.
  upb::generator::Subprocess subprocess;
  subprocess.Start(plugin, upb::generator::Subprocess::SEARCH_PATH);

  // Exchange JSON strings with the subprocess.
  const std::string json_request = std::string(sv.data, sv.size);
  std::string json_response, error;
  const bool ok = subprocess.Communicate(json_request, &json_response, &error);
  if (!ok) {
    // Dump the JSON request to stderr if we can't launch the next plugin.
    std::cerr << json_request << '\n';
    return -1;
  }

  // Decode, serialize, and write the JSON response.
  upbdev_ProcessOutput(json_response.data(), json_response.size(), a, &status);
  if (!upb_Status_IsOk(&status)) {
    std::cerr << status.msg << '\n';
    return -1;
  }

  upb_Arena_Free(a);
  return 0;
}
