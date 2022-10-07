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

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_LINE_CONSUMER_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_LINE_CONSUMER_H__

#include <string>
#include <vector>

#include "google/protobuf/io/zero_copy_stream.h"

// Must be included last
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

// TODO(b/250947994): PROTOC_EXPORT is only used to allow the CMake build to
// find/link these in the unittest, this is not public api.

// Helper for parsing simple files.
class PROTOC_EXPORT LineConsumer {
 public:
  LineConsumer();
  virtual ~LineConsumer();
  virtual bool ConsumeLine(const absl::string_view& line,
                           std::string* out_error) = 0;
};

bool PROTOC_EXPORT ParseSimpleFile(const std::string& path,
                                   LineConsumer* line_consumer,
                                   std::string* out_error);

bool PROTOC_EXPORT ParseSimpleStream(io::ZeroCopyInputStream& input_stream,
                                     const std::string& stream_name,
                                     LineConsumer* line_consumer,
                                     std::string* out_error);

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_LINE_CONSUMER_H__
