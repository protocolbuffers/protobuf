// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC nor the names of its
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
