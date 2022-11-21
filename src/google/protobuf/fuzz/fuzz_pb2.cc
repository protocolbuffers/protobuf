// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////

#include "google/protobuf/test_messages_proto2.pb.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // google::protobuf::
    protobuf_test_messages::proto2::TestAllTypesProto2 t;
    std::string out;

    if (t.ParseFromArray(data, size)) {
        t.DebugString();
        t.ShortDebugString();
        t.Utf8DebugString();
        t.SerializeToString(&out);
        // #include "google/protobuf/text_format.h"
        // google::protobuf::TextFormat::PrintToString(t, &out);
    } else if (t.ParsePartialFromArray(data, size)) {
        t.DebugString();
        t.ShortDebugString();
        t.Utf8DebugString();
        t.SerializePartialToString(&out);
    }
    return 0;
}
