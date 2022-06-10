// Copyright (c) 2009-2022, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <assert.h>

#include <iostream>
#include <string>

#include "google/protobuf/compiler/plugin.upb.h"
#include "google/protobuf/compiler/plugin.upbdefs.h"
#include "upb/json_decode.h"
#include "upb/json_encode.h"
#include "upb/mini_descriptor.h"
#include "upb/upb.h"
#include "upbc/code_generator_request.h"
#include "upbc/code_generator_request.upb.h"
#include "upbc/code_generator_request.upbdefs.h"
#include "upbc/subprocess.h"

static constexpr char kDefaultPlugin[] = "protoc_dart_plugin";

static std::string JsonEncode(const upbc_CodeGeneratorRequest* request,
                              upb_Arena* a) {
  upb_DefPool* s = upb_DefPool_New();
  const upb_MessageDef* m = upbc_CodeGeneratorRequest_getmsgdef(s);

  upb_Status status;
  upb_Status_Clear(&status);

  const size_t json_size = upb_JsonEncode(request, m, s, 0, NULL, 0, &status);
  assert(upb_Status_IsOk(&status));

  char* json_buf = (char*)upb_Arena_Malloc(a, json_size + 1);

  (void)upb_JsonEncode(request, m, s, 0, json_buf, json_size + 1, &status);
  assert(upb_Status_IsOk(&status));

  upb_DefPool_Free(s);

  return std::string(json_buf, json_size);
}

static google_protobuf_compiler_CodeGeneratorResponse* JsonDecode(
    const std::string& json, upb_Arena* a) {
  google_protobuf_compiler_CodeGeneratorResponse* response =
      google_protobuf_compiler_CodeGeneratorResponse_new(a);

  upb_DefPool* s = upb_DefPool_New();
  const upb_MessageDef* m = google_protobuf_compiler_CodeGeneratorResponse_getmsgdef(s);

  upb_Status status;
  upb_Status_Clear(&status);

  (void)upb_JsonDecode(json.c_str(), json.size(), response, m, s, 0, a,
                       &status);
  assert(upb_Status_IsOk(&status));

  upb_DefPool_Free(s);

  return response;
}

static std::string Serialize(
    const google_protobuf_compiler_CodeGeneratorResponse* response, upb_Arena* a) {
  size_t len = 0;
  const char* buf =
      google_protobuf_compiler_CodeGeneratorResponse_serialize(response, a, &len);
  return std::string(buf, len);
}

int main() {
  upb_Arena* a = upb_Arena_New();

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

  // Wrap the request inside a upbc_CodeGeneratorRequest.
  upb_Status status;
  upb_Status_Clear(&status);
  auto outer_request = upbc_MakeCodeGeneratorRequest(inner_request, a, &status);
  if (!upb_Status_IsOk(&status)) {
    std::cerr << status.msg << std::endl;
    return -1;
  }

  const std::string json_request = JsonEncode(outer_request, a);

  // Launch the subprocess.
  upbc::Subprocess subprocess;
  subprocess.Start(plugin, upbc::Subprocess::SEARCH_PATH);

  // Exchange JSON strings with the subprocess.
  std::string json_response, error;
  const bool ok = subprocess.Communicate(json_request, &json_response, &error);
  if (!ok) {
    // Dump the JSON request to stderr if we can't launch the next plugin.
    std::cerr << json_request << std::endl;
    return -1;
  }

  // Decode and serialize the JSON response.
  const auto response = JsonDecode(json_response, a);
  const std::string output = Serialize(response, a);

  // Question: Is this sufficient for sending reliably to stdout?
  std::cout << output;

  upb_Arena_Free(a);
  return 0;
}
