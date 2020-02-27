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
goog.require('jspb.BinaryConstants');
goog.require('jspb.BinaryReader');
goog.require('jspb.BinaryWriter');
goog.require('jspb.utils');


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
    assertEquals(
        'CgF/',
        writer.getResultBase64String(goog.crypt.base64.Alphabet.DEFAULT));
    assertEquals(
        'CgF_',
        writer.getResultBase64String(
            goog.crypt.base64.Alphabet.WEBSAFE_NO_PADDING));
  });

  it('writes split 64 fields', function() {
    var writer = new jspb.BinaryWriter();
    writer.writeSplitVarint64(1, 0x1, 0x2);
    writer.writeSplitVarint64(1, 0xFFFFFFFF, 0xFFFFFFFF);
    writer.writeSplitFixed64(2, 0x1, 0x2);
    writer.writeSplitFixed64(2, 0xFFFFFFF0, 0xFFFFFFFF);
    function lo(i) {
      return i + 1;
    }
    function hi(i) {
      return i + 2;
    }
    writer.writeRepeatedSplitVarint64(3, [0, 1, 2], lo, hi);
    writer.writeRepeatedSplitFixed64(4, [0, 1, 2], lo, hi);
    writer.writePackedSplitVarint64(5, [0, 1, 2], lo, hi);
    writer.writePackedSplitFixed64(6, [0, 1, 2], lo, hi);

    function bitsAsArray(lowBits, highBits) {
      return [lowBits >>> 0, highBits >>> 0];
    }
    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());
    reader.nextField();
    expect(reader.getFieldNumber()).toEqual(1);
    expect(reader.readSplitVarint64(bitsAsArray)).toEqual([0x1, 0x2]);

    reader.nextField();
    expect(reader.getFieldNumber()).toEqual(1);
    expect(reader.readSplitVarint64(bitsAsArray)).toEqual([
      0xFFFFFFFF, 0xFFFFFFFF
    ]);

    reader.nextField();
    expect(reader.getFieldNumber()).toEqual(2);
    expect(reader.readSplitFixed64(bitsAsArray)).toEqual([0x1, 0x2]);

    reader.nextField();
    expect(reader.getFieldNumber()).toEqual(2);
    expect(reader.readSplitFixed64(bitsAsArray)).toEqual([
      0xFFFFFFF0, 0xFFFFFFFF
    ]);

    for (let i = 0; i < 3; i++) {
      reader.nextField();
      expect(reader.getFieldNumber()).toEqual(3);
      expect(reader.readSplitVarint64(bitsAsArray)).toEqual([i + 1, i + 2]);
    }

    for (let i = 0; i < 3; i++) {
      reader.nextField();
      expect(reader.getFieldNumber()).toEqual(4);
      expect(reader.readSplitFixed64(bitsAsArray)).toEqual([i + 1, i + 2]);
    }

    reader.nextField();
    expect(reader.getFieldNumber()).toEqual(5);
    expect(reader.readPackedInt64String()).toEqual([
      String(2 * 2 ** 32 + 1),
      String(3 * 2 ** 32 + 2),
      String(4 * 2 ** 32 + 3),
    ]);

    reader.nextField();
    expect(reader.getFieldNumber()).toEqual(6);
    expect(reader.readPackedFixed64String()).toEqual([
      String(2 * 2 ** 32 + 1),
      String(3 * 2 ** 32 + 2),
      String(4 * 2 ** 32 + 3),
    ]);
  });

  it('writes zigzag 64 fields', function() {
    // Test cases directly from the protobuf dev guide.
    // https://engdoc.corp.google.com/eng/howto/protocolbuffers/developerguide/encoding.shtml?cl=head#types
    var testCases = [
      {original: '0', zigzag: '0'},
      {original: '-1', zigzag: '1'},
      {original: '1', zigzag: '2'},
      {original: '-2', zigzag: '3'},
      {original: '2147483647', zigzag: '4294967294'},
      {original: '-2147483648', zigzag: '4294967295'},
      // 64-bit extremes, not in dev guide.
      {original: '9223372036854775807', zigzag: '18446744073709551614'},
      {original: '-9223372036854775808', zigzag: '18446744073709551615'},
    ];
    function decimalToLowBits(v) {
      jspb.utils.splitDecimalString(v);
      return jspb.utils.split64Low >>> 0;
    }
    function decimalToHighBits(v) {
      jspb.utils.splitDecimalString(v);
      return jspb.utils.split64High >>> 0;
    }

    var writer = new jspb.BinaryWriter();
    testCases.forEach(function(c) {
      writer.writeSint64String(1, c.original);
      writer.writeSintHash64(1, jspb.utils.decimalStringToHash64(c.original));
      jspb.utils.splitDecimalString(c.original);
      writer.writeSplitZigzagVarint64(
          1, jspb.utils.split64Low, jspb.utils.split64High);
    });

    writer.writeRepeatedSint64String(2, testCases.map(function(c) {
      return c.original;
    }));

    writer.writeRepeatedSintHash64(3, testCases.map(function(c) {
      return jspb.utils.decimalStringToHash64(c.original);
    }));

    writer.writeRepeatedSplitZigzagVarint64(
        4, testCases.map(function(c) {
          return c.original;
        }),
        decimalToLowBits, decimalToHighBits);

    writer.writePackedSint64String(5, testCases.map(function(c) {
      return c.original;
    }));

    writer.writePackedSintHash64(6, testCases.map(function(c) {
      return jspb.utils.decimalStringToHash64(c.original);
    }));

    writer.writePackedSplitZigzagVarint64(
        7, testCases.map(function(c) {
          return c.original;
        }),
        decimalToLowBits, decimalToHighBits);

    // Verify by reading the stream as normal int64 fields and checking with
    // the canonical zigzag encoding of each value.
    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());
    testCases.forEach(function(c) {
      reader.nextField();
      expect(reader.getFieldNumber()).toEqual(1);
      expect(reader.readUint64String()).toEqual(c.zigzag);
      reader.nextField();
      expect(reader.getFieldNumber()).toEqual(1);
      expect(reader.readUint64String()).toEqual(c.zigzag);
      reader.nextField();
      expect(reader.getFieldNumber()).toEqual(1);
      expect(reader.readUint64String()).toEqual(c.zigzag);
    });

    testCases.forEach(function(c) {
      reader.nextField();
      expect(reader.getFieldNumber()).toEqual(2);
      expect(reader.readUint64String()).toEqual(c.zigzag);
    });

    testCases.forEach(function(c) {
      reader.nextField();
      expect(reader.getFieldNumber()).toEqual(3);
      expect(reader.readUint64String()).toEqual(c.zigzag);
    });

    testCases.forEach(function(c) {
      reader.nextField();
      expect(reader.getFieldNumber()).toEqual(4);
      expect(reader.readUint64String()).toEqual(c.zigzag);
    });

    reader.nextField();
    expect(reader.getFieldNumber()).toEqual(5);
    expect(reader.readPackedUint64String()).toEqual(testCases.map(function(c) {
      return c.zigzag;
    }));

    reader.nextField();
    expect(reader.getFieldNumber()).toEqual(6);
    expect(reader.readPackedUint64String()).toEqual(testCases.map(function(c) {
      return c.zigzag;
    }));

    reader.nextField();
    expect(reader.getFieldNumber()).toEqual(7);
    expect(reader.readPackedUint64String()).toEqual(testCases.map(function(c) {
      return c.zigzag;
    }));
  });

  it('writes float32 fields', function() {
    var testCases = [
      0, 1, -1, jspb.BinaryConstants.FLOAT32_MIN,
      -jspb.BinaryConstants.FLOAT32_MIN, jspb.BinaryConstants.FLOAT32_MAX,
      -jspb.BinaryConstants.FLOAT32_MAX, 3.1415927410125732, Infinity,
      -Infinity, NaN
    ];
    var writer = new jspb.BinaryWriter();
    testCases.forEach(function(f) {
      writer.writeFloat(1, f);
    });
    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());
    testCases.forEach(function(f) {
      reader.nextField();
      expect(reader.getFieldNumber()).toEqual(1);
      if (isNaN(f)) {
        expect(isNaN(reader.readFloat())).toEqual(true);
      } else {
        expect(reader.readFloat()).toEqual(f);
      }
    });
  });

  it('writes double fields', function() {
    var testCases = [
      0, 1, -1, Number.MAX_SAFE_INTEGER, Number.MIN_SAFE_INTEGER,
      Number.MAX_VALUE, Number.MIN_VALUE, jspb.BinaryConstants.FLOAT32_MIN,
      -jspb.BinaryConstants.FLOAT32_MIN, jspb.BinaryConstants.FLOAT32_MAX,
      -jspb.BinaryConstants.FLOAT32_MAX, Math.PI, Infinity, -Infinity, NaN
    ];
    var writer = new jspb.BinaryWriter();
    testCases.forEach(function(f) {
      writer.writeDouble(1, f);
    });
    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());
    testCases.forEach(function(f) {
      reader.nextField();
      expect(reader.getFieldNumber()).toEqual(1);
      if (isNaN(f)) {
        expect(isNaN(reader.readDouble())).toEqual(true);
      } else {
        expect(reader.readDouble()).toEqual(f);
      }
    });
  });
});
