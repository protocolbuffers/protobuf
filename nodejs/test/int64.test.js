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

var i64 = protobuf.Int64;
var u64 = protobuf.UInt64;

// Test Int64.
(function() {
  var i = new i64();
  assert.equal(i64.lo(i), 0);
  assert.equal(i64.hi(i), 0);
  assert.equal(i.toString(), "0");

  // (high : uint32, low : uint32) form.
  i = i64.join(0x10000000, 0x00000000);
  assert.equal(i.toString(), "1152921504606846976");
  assert.equal(i64.hi(i), 0x10000000);
  assert.equal(i64.lo(i), 0);

  i = i64.join(0, 4.0 * 1024 * 1024 * 1024 - 1);
  assert.equal(i64.hi(i), 0);
  assert.equal(i64.lo(i), 0xffffffff);
  assert.equal(i.toString(), "4294967295");

  i = new i64(1234);
  assert.equal(i64.hi(i), 0);
  assert.equal(i64.lo(i), 1234);

  i = new i64(-1234);
  assert.equal(i64.hi(i), 0xffffffff);
  assert.equal(i64.lo(i), 0xffffffff - 1233);

  assert.equal(i64.compare(new i64(1234), new i64(1233)), 1);
  assert.equal(i64.compare(new i64(1234), new i64(1235)), -1);
  assert.equal(i64.compare(new i64(1234), new i64(1234)), 0);

  assert.throws(function() { new i64(1e100); });
  assert.throws(function() { new i64(1.234); });

  var i2 = new i64(i);
  assert.equal(i.toString(), i2.toString());

  assert.throws(function() { new u64(i); });

  i = new i64(1234);
  i2 = new u64(i);
  assert.equal(i.toString(), i2.toString());
})();

// Test UInt64.
(function() {
  var i = new u64();
  assert.equal(u64.hi(i), 0);
  assert.equal(u64.lo(i), 0);
  assert.equal(i.toString(), "0");

  // (high : uint32, low : uint32) form.
  i = u64.join(0x10000000, 0x00000000);
  assert.equal(i.toString(), "1152921504606846976");
  assert.equal(u64.hi(i), 0x10000000);
  assert.equal(u64.lo(i), 0);

  i = new u64(1234);
  assert.equal(u64.hi(i), 0);
  assert.equal(u64.lo(i), 1234);

  assert.throws(function() { new u64(-1234); });

  assert.throws(function() { i.set(1e100); });
  assert.throws(function() { i.set(1.234); });

  var i2 = new u64(i);
  assert.equal(i.toString(), i2.toString());

  i = u64.join(0x80000000, 0);
  assert.throws(function() { new i64(i); });

  i = new u64(1234);
  i2 = new i64(i);
  assert.equal(i.toString(), i2.toString());
})();

// Test signed-vs-unsigned comparisons.
(function() {
  var i1 = u64.join(1, 0); // 2^32
  var i2 = i64.join(0, 0xffffffff);
  assert.equal(u64.compare(i1, i2),  1);
  assert.equal(u64.compare(i2, i1), -1);
  assert.equal(i64.compare(i1, i2),  1);
  assert.equal(i64.compare(i2, i1), -1);

  i1 = new i64(-1);
  i2 = new u64(0);
  assert.equal(u64.compare(i1, i2), -1);
  assert.equal(u64.compare(i2, i1),  1);
  assert.equal(i64.compare(i1, i2), -1);
  assert.equal(i64.compare(i2, i1),  1);
})();

// Test string conversions.
(function() {
  var i1 = new i64("9223372036854775807");
  assert.equal(i64.hi(i1), 0x7fffffff);
  assert.equal(i64.lo(i1), 0xffffffff);
  i1 = new i64("-9223372036854775808");
  assert.equal(i64.hi(i1), 0x80000000);
  assert.equal(i64.lo(i1), 0x00000000);
  assert.throws(function() {
    new i64("100000000000000000000000000");
  });
  assert.throws(function() {
    new i64("-100000000000000000000000000");
  });

  var u1 = new u64("18446744073709551615");
  assert.equal(u64.hi(u1), 0xffffffff);
  assert.equal(u64.lo(u1), 0xffffffff);
  assert.throws(function() {
    new u64("100000000000000000000000000000");
  });
  assert.throws(function() {
    new u64("-1");
  });
})();

console.log("int64.test.js PASS");
