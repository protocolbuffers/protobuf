// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

syntax = "proto2";

package protobuf_unittest_selfreferential_options;

import "google/protobuf/descriptor.proto";

option csharp_namespace = "UnitTest.Issues.TestProtos.SelfreferentialOptions";

message FooOptions {
  // Custom field option used in definition of the extension message.
  optional int32 int_opt = 1 [(foo_options) = {
    int_opt: 1
    [foo_int_opt]: 2
    [foo_foo_opt]: { int_opt: 3 }
  }];

  // Custom field option used in definition of the custom option's message.
  optional int32 foo = 2 [(foo_options) = { foo: 1234 }];

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
