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

/**
 * @fileoverview Test cases for jspb's binary protocol buffer writer. In
 * practice BinaryWriter is used to drive the Decoder and Reader test cases,
 * so only writer-specific tests are here.
 *
 * Test suite is written using Jasmine -- see http://jasmine.github.io/
 *
 * @author aappleby@google.com (Austin Appleby)
 */

goog.require('goog.crypt');
goog.require('goog.testing.asserts');
goog.require('jspb.BinaryWriter');


/**
 * @param {function()} func This function should throw an error when run.
 */
function assertFails(func) {
  assertThrows(func);
}


describe('binaryWriterTest', function() {
  /**
   * Verifies that misuse of the writer class triggers assertions.
   */
  it('testWriteErrors', function() {
    // Submessages with invalid field indices should assert.
    var writer = new jspb.BinaryWriter();
    var dummyMessage = /** @type {!jspb.BinaryMessage} */({});

    assertFails(function() {
      writer.writeMessage(-1, dummyMessage, goog.nullFunction);
    });

    // Writing invalid field indices should assert.
    writer = new jspb.BinaryWriter();
    assertFails(function() {writer.writeUint64(-1, 1);});

    // Writing out-of-range field values should assert.
    writer = new jspb.BinaryWriter();

    assertFails(function() {writer.writeInt32(1, -Infinity);});
    assertFails(function() {writer.writeInt32(1, Infinity);});

    assertFails(function() {writer.writeInt64(1, -Infinity);});
    assertFails(function() {writer.writeInt64(1, Infinity);});

    assertFails(function() {writer.writeUint32(1, -1);});
    assertFails(function() {writer.writeUint32(1, Infinity);});

    assertFails(function() {writer.writeUint64(1, -1);});
    assertFails(function() {writer.writeUint64(1, Infinity);});

    assertFails(function() {writer.writeSint32(1, -Infinity);});
    assertFails(function() {writer.writeSint32(1, Infinity);});

    assertFails(function() {writer.writeSint64(1, -Infinity);});
    assertFails(function() {writer.writeSint64(1, Infinity);});

    assertFails(function() {writer.writeFixed32(1, -1);});
    assertFails(function() {writer.writeFixed32(1, Infinity);});

    assertFails(function() {writer.writeFixed64(1, -1);});
    assertFails(function() {writer.writeFixed64(1, Infinity);});

    assertFails(function() {writer.writeSfixed32(1, -Infinity);});
    assertFails(function() {writer.writeSfixed32(1, Infinity);});

    assertFails(function() {writer.writeSfixed64(1, -Infinity);});
    assertFails(function() {writer.writeSfixed64(1, Infinity);});
  });


  /**
   * Basic test of retrieving the result as a Uint8Array buffer
   */
  it('testGetResultBuffer', function() {
    var expected = '0864120b48656c6c6f20776f726c641a0301020320c801';

    var writer = new jspb.BinaryWriter();
    writer.writeUint32(1, 100);
    writer.writeString(2, 'Hello world');
    writer.writeBytes(3, new Uint8Array([1, 2, 3]));
    writer.writeUint32(4, 200);

    var buffer = writer.getResultBuffer();
    assertEquals(expected, goog.crypt.byteArrayToHex(buffer));
  });


  /**
   * Tests websafe encodings for base64 strings.
   */
  it('testWebSafeOption', function() {
    var writer = new jspb.BinaryWriter();
    writer.writeBytes(1, new Uint8Array([127]));
    assertEquals('CgF/', writer.getResultBase64String());
    assertEquals('CgF/', writer.getResultBase64String(false));
    assertEquals('CgF_', writer.getResultBase64String(true));
  });
});
