// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

var protobuf = require('google_protobuf');
var assert = require('assert');
var generated_code = require('./generated_code');

(function() {
  var m = new generated_code.TestMessage({
    optional_int32: 42,
    optional_bool: true,
    optional_string: "hello world",
    repeated_string: ["a", "b", "c"],
    repeated_msg: [new generated_code.TestMessage({optional_int32:1})]
  });

  assert.equal(m.optional_int32, 42);
  assert.equal(m.optional_string, "hello world");
  assert.equal(m.repeated_string[2], "c");
  assert.equal(m.repeated_msg[0].optional_int32, 1);

  m.oneof_int32 = 42;
  m.oneof_bool = true;
  assert.equal(m.oneof_int32, undefined);
  assert.equal(m.my_oneof, "oneof_bool");

  var n = new generated_code.TestMessage.NestedMessage({foo: 42});
  assert.equal(n.foo, 42);

  assert.equal(generated_code.TestEnum.C, 3);
  assert.equal(generated_code.TestEnum.descriptor.name, "A.B.C.TestEnum");
})();

console.log("generated_code.test.js PASS");
