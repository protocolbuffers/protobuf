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

syntax = "proto2";

package protobuf_unittest_selfreferential_options;
option csharp_namespace = "UnitTest.Issues.TestProtos.SelfreferentialOptions";

import "google/protobuf/descriptor.proto";

message FooOptions {
  // Custom field option used in definition of the extension message.
  optional int32 int_opt = 1 [(foo_options) = {
    int_opt: 1
    [foo_int_opt]: 2
    [foo_foo_opt]: {
      int_opt: 3
    }
  }];

  // Custom field option used in definition of the custom option's message.
  optional int32 foo = 2 [(foo_options) = {foo: 1234}];

  extensions 1000 to max;
}

extend google.protobuf.FieldOptions {
  // Custom field option used on the definition of that field option.
  optional int32 bar_options = 1000 [(bar_options) = 1234];

  optional FooOptions foo_options = 1001;
}

extend FooOptions {
   optional int32 foo_int_opt = 1000;
   optional FooOptions foo_foo_opt = 1001;
}
