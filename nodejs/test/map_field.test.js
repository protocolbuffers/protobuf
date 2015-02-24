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

// Shorthand for field type constants.
var f = protobuf.FieldDescriptor;

(function() {
  var m = new generated_code.TestMessage();

  m.map_int32_string.set(42, "test1");
  m.map_int64_string.set(protobuf.Int64.join(42, 84), "test2");
  m.map_bool_string.set(false, "test3");
  m.map_string_string.set("asdf", "test4");
  m.map_string_msg.set("jkl;",
      new generated_code.TestMessage({optional_int32: 42}));
  m.map_string_enum.set("qwerty", generated_code.TestEnum.B);
  m.map_string_int32.set("abcd", 1234);
  m.map_string_bool.set("uiop", true);

  var m2 = generated_code.TestMessage.decodeBinary(
      generated_code.TestMessage.encodeBinary(m));

  assert.equal(m2.map_int32_string.get(42), "test1");
  assert.equal(m2.map_int64_string.get(protobuf.Int64.join(42, 84)), "test2");
  assert.equal(m2.map_bool_string.get(false), "test3");
  assert.equal(m2.map_string_string.get("asdf"), "test4");
  assert.equal(m2.map_string_msg.get("jkl;").optional_int32, 42);
  assert.equal(m2.map_string_enum.get("qwerty"), generated_code.TestEnum.B);
  assert.equal(m2.map_string_int32.get("abcd"), 1234);
  assert.equal(m2.map_string_bool.get("uiop"), true);

  var m3 = generated_code.TestMessage.decodeJson(
      generated_code.TestMessage.encodeJson(m));

  assert.equal(m3.map_int32_string.get(42), "test1");
  assert.equal(m3.map_int64_string.get(protobuf.Int64.join(42, 84)), "test2");
  assert.equal(m3.map_bool_string.get(false), "test3");
  assert.equal(m3.map_string_string.get("asdf"), "test4");
  assert.equal(m3.map_string_msg.get("jkl;").optional_int32, 42);
  assert.equal(m3.map_string_enum.get("qwerty"), generated_code.TestEnum.B);
  assert.equal(m3.map_string_int32.get("abcd"), 1234);
  assert.equal(m3.map_string_bool.get("uiop"), true);
})();

(function() {
  var m = new generated_code.TestMessage();

  for (var i = 0; i < 100; i++) {
    m.map_string_int32.set(i.toString(), i + 42);
  }

  var m2 = generated_code.TestMessage.decodeBinary(
      generated_code.TestMessage.encodeBinary(m));

  assert.equal(m2.map_string_int32.keys.length, 100);
  for (var i = 0; i < 100; i++) {
    assert.equal(m2.map_string_int32.get(i.toString()), i + 42);
  }

  var m3 = generated_code.TestMessage.decodeJson(
      generated_code.TestMessage.encodeJson(m));

  assert.equal(m3.map_string_int32.keys.length, 100);
  for (var i = 0; i < 100; i++) {
    assert.equal(m3.map_string_int32.get(i.toString()), i + 42);
  }
})();

// Test constructor args for map fields.
(function() {
  var m = new generated_code.TestMessage({
    map_string_int32: { "asdf": 42, "jkl;": 84 }
  });

  assert.equal(m.map_string_int32.get("asdf"), 42);
  assert.equal(m.map_string_int32.get("jkl;"), 84);
})();

// Test assignment and type-checking of map objects.
(function() {
  var m = new generated_code.TestMessage();

  m.map_string_int32 = new protobuf.Map(f.TYPE_STRING, f.TYPE_INT32,
      { "a": 1, "b": 2 });
  assert.equal(m.map_string_int32.get("a"), 1);
  assert.equal(m.map_string_int32.get("b"), 2);

  m.map_string_msg = new protobuf.Map(f.TYPE_STRING, f.TYPE_MESSAGE,
      generated_code.TestMessage,
      { "a": new generated_code.TestMessage({optional_int32: 42}) });
  assert.equal(m.map_string_msg.get("a").optional_int32, 42);

  assert.throws(function() {
    m.map_string_int32 = new protobuf.Map(f.TYPE_INT32, f.TYPE_STRING);
  });

  assert.throws(function() {
    m.map_string_msg = new protobuf.Map(f.TYPE_STRING, f.TYPE_MESSAGE,
        generated_code.TestMessage.NestedMessage);
  });

})();

console.log("map_field.test.js PASS");
