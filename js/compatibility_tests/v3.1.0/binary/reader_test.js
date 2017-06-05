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
 * @fileoverview Test cases for jspb's binary protocol buffer reader.
 *
 * There are two particular magic numbers that need to be pointed out -
 * 2^64-1025 is the largest number representable as both a double and an
 * unsigned 64-bit integer, and 2^63-513 is the largest number representable as
 * both a double and a signed 64-bit integer.
 *
 * Test suite is written using Jasmine -- see http://jasmine.github.io/
 *
 * @author aappleby@google.com (Austin Appleby)
 */

goog.require('goog.testing.asserts');
goog.require('jspb.BinaryConstants');
goog.require('jspb.BinaryDecoder');
goog.require('jspb.BinaryReader');
goog.require('jspb.BinaryWriter');



describe('binaryReaderTest', function() {
  /**
   * Tests the reader instance cache.
   */
  it('testInstanceCaches', /** @suppress {visibility} */ function() {
    var writer = new jspb.BinaryWriter();
    var dummyMessage = /** @type {!jspb.BinaryMessage} */({});
    writer.writeMessage(1, dummyMessage, goog.nullFunction);
    writer.writeMessage(2, dummyMessage, goog.nullFunction);

    var buffer = writer.getResultBuffer();

    // Empty the instance caches.
    jspb.BinaryReader.instanceCache_ = [];

    // Allocating and then freeing three decoders should leave us with three in
    // the cache.

    var decoder1 = jspb.BinaryDecoder.alloc();
    var decoder2 = jspb.BinaryDecoder.alloc();
    var decoder3 = jspb.BinaryDecoder.alloc();
    decoder1.free();
    decoder2.free();
    decoder3.free();

    assertEquals(3, jspb.BinaryDecoder.instanceCache_.length);
    assertEquals(0, jspb.BinaryReader.instanceCache_.length);

    // Allocating and then freeing a reader should remove one decoder from its
    // cache, but it should stay stuck to the reader afterwards since we can't
    // have a reader without a decoder.
    jspb.BinaryReader.alloc().free();

    assertEquals(2, jspb.BinaryDecoder.instanceCache_.length);
    assertEquals(1, jspb.BinaryReader.instanceCache_.length);

    // Allocating a reader should remove a reader from the cache.
    var reader = jspb.BinaryReader.alloc(buffer);

    assertEquals(2, jspb.BinaryDecoder.instanceCache_.length);
    assertEquals(0, jspb.BinaryReader.instanceCache_.length);

    // Processing the message reuses the current reader.
    reader.nextField();
    assertEquals(1, reader.getFieldNumber());
    reader.readMessage(dummyMessage, function() {
      assertEquals(0, jspb.BinaryReader.instanceCache_.length);
    });

    reader.nextField();
    assertEquals(2, reader.getFieldNumber());
    reader.readMessage(dummyMessage, function() {
      assertEquals(0, jspb.BinaryReader.instanceCache_.length);
    });

    assertEquals(false, reader.nextField());

    assertEquals(2, jspb.BinaryDecoder.instanceCache_.length);
    assertEquals(0, jspb.BinaryReader.instanceCache_.length);

    // Freeing the reader should put it back into the cache.
    reader.free();

    assertEquals(2, jspb.BinaryDecoder.instanceCache_.length);
    assertEquals(1, jspb.BinaryReader.instanceCache_.length);
  });


  /**
   * @param {number} x
   * @return {number}
   */
  function truncate(x) {
    var temp = new Float32Array(1);
    temp[0] = x;
    return temp[0];
  }


  /**
   * Verifies that misuse of the reader class triggers assertions.
   */
  it('testReadErrors', /** @suppress {checkTypes|visibility} */ function() {
    // Calling readMessage on a non-delimited field should trigger an
    // assertion.
    var reader = jspb.BinaryReader.alloc([8, 1]);
    var dummyMessage = /** @type {!jspb.BinaryMessage} */({});
    reader.nextField();
    assertThrows(function() {
      reader.readMessage(dummyMessage, goog.nullFunction);
    });

    // Reading past the end of the stream should trigger an assertion.
    reader = jspb.BinaryReader.alloc([9, 1]);
    reader.nextField();
    assertThrows(function() {reader.readFixed64()});

    // Reading past the end of a submessage should trigger an assertion.
    reader = jspb.BinaryReader.alloc([10, 4, 13, 1, 1, 1]);
    reader.nextField();
    reader.readMessage(dummyMessage, function() {
      reader.nextField();
      assertThrows(function() {reader.readFixed32()});
    });

    // Skipping an invalid field should trigger an assertion.
    reader = jspb.BinaryReader.alloc([12, 1]);
    reader.nextWireType_ = 1000;
    assertThrows(function() {reader.skipField()});

    // Reading fields with the wrong wire type should assert.
    reader = jspb.BinaryReader.alloc([9, 0, 0, 0, 0, 0, 0, 0, 0]);
    reader.nextField();
    assertThrows(function() {reader.readInt32()});
    assertThrows(function() {reader.readInt32String()});
    assertThrows(function() {reader.readInt64()});
    assertThrows(function() {reader.readInt64String()});
    assertThrows(function() {reader.readUint32()});
    assertThrows(function() {reader.readUint32String()});
    assertThrows(function() {reader.readUint64()});
    assertThrows(function() {reader.readUint64String()});
    assertThrows(function() {reader.readSint32()});
    assertThrows(function() {reader.readBool()});
    assertThrows(function() {reader.readEnum()});

    reader = jspb.BinaryReader.alloc([8, 1]);
    reader.nextField();
    assertThrows(function() {reader.readFixed32()});
    assertThrows(function() {reader.readFixed64()});
    assertThrows(function() {reader.readSfixed32()});
    assertThrows(function() {reader.readSfixed64()});
    assertThrows(function() {reader.readFloat()});
    assertThrows(function() {reader.readDouble()});

    assertThrows(function() {reader.readString()});
    assertThrows(function() {reader.readBytes()});
  });


  /**
   * Tests encoding and decoding of unsigned field types.
   * @param {Function} readField
   * @param {Function} writeField
   * @param {number} epsilon
   * @param {number} upperLimit
   * @param {Function} filter
   * @private
   * @suppress {missingProperties}
   */
  var doTestUnsignedField_ = function(readField,
      writeField, epsilon, upperLimit, filter) {
    assertNotNull(readField);
    assertNotNull(writeField);

    var writer = new jspb.BinaryWriter();

    // Encode zero and limits.
    writeField.call(writer, 1, filter(0));
    writeField.call(writer, 2, filter(epsilon));
    writeField.call(writer, 3, filter(upperLimit));

    // Encode positive values.
    for (var cursor = epsilon; cursor < upperLimit; cursor *= 1.1) {
      writeField.call(writer, 4, filter(cursor));
    }

    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());

    // Check zero and limits.
    reader.nextField();
    assertEquals(1, reader.getFieldNumber());
    assertEquals(filter(0), readField.call(reader));

    reader.nextField();
    assertEquals(2, reader.getFieldNumber());
    assertEquals(filter(epsilon), readField.call(reader));

    reader.nextField();
    assertEquals(3, reader.getFieldNumber());
    assertEquals(filter(upperLimit), readField.call(reader));

    // Check positive values.
    for (var cursor = epsilon; cursor < upperLimit; cursor *= 1.1) {
      reader.nextField();
      if (4 != reader.getFieldNumber()) throw 'fail!';
      if (filter(cursor) != readField.call(reader)) throw 'fail!';
    }
  };


  /**
   * Tests encoding and decoding of signed field types.
   * @param {Function} readField
   * @param {Function} writeField
   * @param {number} epsilon
   * @param {number} lowerLimit
   * @param {number} upperLimit
   * @param {Function} filter
   * @private
   * @suppress {missingProperties}
   */
  var doTestSignedField_ = function(readField,
      writeField, epsilon, lowerLimit, upperLimit, filter) {
    var writer = new jspb.BinaryWriter();

    // Encode zero and limits.
    writeField.call(writer, 1, filter(lowerLimit));
    writeField.call(writer, 2, filter(-epsilon));
    writeField.call(writer, 3, filter(0));
    writeField.call(writer, 4, filter(epsilon));
    writeField.call(writer, 5, filter(upperLimit));

    var inputValues = [];

    // Encode negative values.
    for (var cursor = lowerLimit; cursor < -epsilon; cursor /= 1.1) {
      var val = filter(cursor);
      writeField.call(writer, 6, val);
      inputValues.push({
        fieldNumber: 6,
        value: val
      });
    }

    // Encode positive values.
    for (var cursor = epsilon; cursor < upperLimit; cursor *= 1.1) {
      var val = filter(cursor);
      writeField.call(writer, 7, val);
      inputValues.push({
        fieldNumber: 7,
        value: val
      });
    }

    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());

    // Check zero and limits.
    reader.nextField();
    assertEquals(1, reader.getFieldNumber());
    assertEquals(filter(lowerLimit), readField.call(reader));

    reader.nextField();
    assertEquals(2, reader.getFieldNumber());
    assertEquals(filter(-epsilon), readField.call(reader));

    reader.nextField();
    assertEquals(3, reader.getFieldNumber());
    assertEquals(filter(0), readField.call(reader));

    reader.nextField();
    assertEquals(4, reader.getFieldNumber());
    assertEquals(filter(epsilon), readField.call(reader));

    reader.nextField();
    assertEquals(5, reader.getFieldNumber());
    assertEquals(filter(upperLimit), readField.call(reader));

    for (var i = 0; i < inputValues.length; i++) {
      var expected = inputValues[i];
      reader.nextField();
      assertEquals(expected.fieldNumber, reader.getFieldNumber());
      assertEquals(expected.value, readField.call(reader));
    }
  };


  /**
   * Tests fields that use varint encoding.
   */
  it('testVarintFields', function() {
    assertNotUndefined(jspb.BinaryReader.prototype.readUint32);
    assertNotUndefined(jspb.BinaryWriter.prototype.writeUint32);
    assertNotUndefined(jspb.BinaryReader.prototype.readUint64);
    assertNotUndefined(jspb.BinaryWriter.prototype.writeUint64);
    assertNotUndefined(jspb.BinaryReader.prototype.readBool);
    assertNotUndefined(jspb.BinaryWriter.prototype.writeBool);
    doTestUnsignedField_(
        jspb.BinaryReader.prototype.readUint32,
        jspb.BinaryWriter.prototype.writeUint32,
        1, Math.pow(2, 32) - 1, Math.round);

    doTestUnsignedField_(
        jspb.BinaryReader.prototype.readUint64,
        jspb.BinaryWriter.prototype.writeUint64,
        1, Math.pow(2, 64) - 1025, Math.round);

    doTestSignedField_(
        jspb.BinaryReader.prototype.readInt32,
        jspb.BinaryWriter.prototype.writeInt32,
        1, -Math.pow(2, 31), Math.pow(2, 31) - 1, Math.round);

    doTestSignedField_(
        jspb.BinaryReader.prototype.readInt64,
        jspb.BinaryWriter.prototype.writeInt64,
        1, -Math.pow(2, 63), Math.pow(2, 63) - 513, Math.round);

    doTestSignedField_(
        jspb.BinaryReader.prototype.readEnum,
        jspb.BinaryWriter.prototype.writeEnum,
        1, -Math.pow(2, 31), Math.pow(2, 31) - 1, Math.round);

    doTestUnsignedField_(
        jspb.BinaryReader.prototype.readBool,
        jspb.BinaryWriter.prototype.writeBool,
        1, 1, function(x) { return !!x; });
  });


  /**
   * Tests reading a field from hexadecimal string (format: '08 BE EF').
   * @param {Function} readField
   * @param {number} expected
   * @param {string} hexString
   */
  function doTestHexStringVarint_(readField, expected, hexString) {
    var bytesCount = (hexString.length + 1) / 3;
    var bytes = new Uint8Array(bytesCount);
    for (var i = 0; i < bytesCount; i++) {
      bytes[i] = parseInt(hexString.substring(i * 3, i * 3 + 2), 16);
    }
    var reader = jspb.BinaryReader.alloc(bytes);
    reader.nextField();
    assertEquals(expected, readField.call(reader));
  }


  /**
   * Tests non-canonical redundant varint decoding.
   */
  it('testRedundantVarintFields', function() {
    assertNotNull(jspb.BinaryReader.prototype.readUint32);
    assertNotNull(jspb.BinaryReader.prototype.readUint64);
    assertNotNull(jspb.BinaryReader.prototype.readSint32);
    assertNotNull(jspb.BinaryReader.prototype.readSint64);

    // uint32 and sint32 take no more than 5 bytes
    // 08 - field prefix (type = 0 means varint)
    doTestHexStringVarint_(
      jspb.BinaryReader.prototype.readUint32,
      12, '08 8C 80 80 80 00');

    // 11 stands for -6 in zigzag encoding
    doTestHexStringVarint_(
      jspb.BinaryReader.prototype.readSint32,
      -6, '08 8B 80 80 80 00');

    // uint64 and sint64 take no more than 10 bytes
    // 08 - field prefix (type = 0 means varint)
    doTestHexStringVarint_(
      jspb.BinaryReader.prototype.readUint64,
      12, '08 8C 80 80 80 80 80 80 80 80 00');

    // 11 stands for -6 in zigzag encoding
    doTestHexStringVarint_(
      jspb.BinaryReader.prototype.readSint64,
      -6, '08 8B 80 80 80 80 80 80 80 80 00');
  });


  /**
   * Tests 64-bit fields that are handled as strings.
   */
  it('testStringInt64Fields', function() {
    var writer = new jspb.BinaryWriter();

    var testSignedData = [
      '2730538252207801776',
      '-2688470994844604560',
      '3398529779486536359',
      '3568577411627971000',
      '272477188847484900',
      '-6649058714086158188',
      '-7695254765712060806',
      '-4525541438037104029',
      '-4993706538836508568',
      '4990160321893729138'
    ];
    var testUnsignedData = [
      '7822732630241694882',
      '6753602971916687352',
      '2399935075244442116',
      '8724292567325338867',
      '16948784802625696584',
      '4136275908516066934',
      '3575388346793700364',
      '5167142028379259461',
      '1557573948689737699',
      '17100725280812548567'
    ];

    for (var i = 0; i < testSignedData.length; i++) {
      writer.writeInt64String(2 * i + 1, testSignedData[i]);
      writer.writeUint64String(2 * i + 2, testUnsignedData[i]);
    }

    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());

    for (var i = 0; i < testSignedData.length; i++) {
      reader.nextField();
      assertEquals(2 * i + 1, reader.getFieldNumber());
      assertEquals(testSignedData[i], reader.readInt64String());
      reader.nextField();
      assertEquals(2 * i + 2, reader.getFieldNumber());
      assertEquals(testUnsignedData[i], reader.readUint64String());
    }
  });


  /**
   * Tests fields that use zigzag encoding.
   */
  it('testZigzagFields', function() {
    doTestSignedField_(
        jspb.BinaryReader.prototype.readSint32,
        jspb.BinaryWriter.prototype.writeSint32,
        1, -Math.pow(2, 31), Math.pow(2, 31) - 1, Math.round);

    doTestSignedField_(
        jspb.BinaryReader.prototype.readSint64,
        jspb.BinaryWriter.prototype.writeSint64,
        1, -Math.pow(2, 63), Math.pow(2, 63) - 513, Math.round);
  });


  /**
   * Tests fields that use fixed-length encoding.
   */
  it('testFixedFields', function() {
    doTestUnsignedField_(
        jspb.BinaryReader.prototype.readFixed32,
        jspb.BinaryWriter.prototype.writeFixed32,
        1, Math.pow(2, 32) - 1, Math.round);

    doTestUnsignedField_(
        jspb.BinaryReader.prototype.readFixed64,
        jspb.BinaryWriter.prototype.writeFixed64,
        1, Math.pow(2, 64) - 1025, Math.round);

    doTestSignedField_(
        jspb.BinaryReader.prototype.readSfixed32,
        jspb.BinaryWriter.prototype.writeSfixed32,
        1, -Math.pow(2, 31), Math.pow(2, 31) - 1, Math.round);

    doTestSignedField_(
        jspb.BinaryReader.prototype.readSfixed64,
        jspb.BinaryWriter.prototype.writeSfixed64,
        1, -Math.pow(2, 63), Math.pow(2, 63) - 513, Math.round);
  });


  /**
   * Tests floating point fields.
   */
  it('testFloatFields', function() {
    doTestSignedField_(
        jspb.BinaryReader.prototype.readFloat,
        jspb.BinaryWriter.prototype.writeFloat,
        jspb.BinaryConstants.FLOAT32_MIN,
        -jspb.BinaryConstants.FLOAT32_MAX,
        jspb.BinaryConstants.FLOAT32_MAX,
        truncate);

    doTestSignedField_(
        jspb.BinaryReader.prototype.readDouble,
        jspb.BinaryWriter.prototype.writeDouble,
        jspb.BinaryConstants.FLOAT64_EPS * 10,
        -jspb.BinaryConstants.FLOAT64_MIN,
        jspb.BinaryConstants.FLOAT64_MIN,
        function(x) { return x; });
  });


  /**
   * Tests length-delimited string fields.
   */
  it('testStringFields', function() {
    var s1 = 'The quick brown fox jumps over the lazy dog.';
    var s2 = '人人生而自由，在尊嚴和權利上一律平等。';

    var writer = new jspb.BinaryWriter();

    writer.writeString(1, s1);
    writer.writeString(2, s2);

    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());

    reader.nextField();
    assertEquals(1, reader.getFieldNumber());
    assertEquals(s1, reader.readString());

    reader.nextField();
    assertEquals(2, reader.getFieldNumber());
    assertEquals(s2, reader.readString());
  });


  /**
   * Tests length-delimited byte fields.
   */
  it('testByteFields', function() {
    var message = [];
    var lowerLimit = 1;
    var upperLimit = 256;
    var scale = 1.1;

    var writer = new jspb.BinaryWriter();

    for (var cursor = lowerLimit; cursor < upperLimit; cursor *= 1.1) {
      var len = Math.round(cursor);
      var bytes = [];
      for (var i = 0; i < len; i++) bytes.push(i % 256);

      writer.writeBytes(len, bytes);
    }

    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());

    for (var cursor = lowerLimit; reader.nextField(); cursor *= 1.1) {
      var len = Math.round(cursor);
      if (len != reader.getFieldNumber()) throw 'fail!';

      var bytes = reader.readBytes();
      if (len != bytes.length) throw 'fail!';
      for (var i = 0; i < bytes.length; i++) {
        if (i % 256 != bytes[i]) throw 'fail!';
      }
    }
  });


  /**
   * Tests nested messages.
   */
  it('testNesting', function() {
    var writer = new jspb.BinaryWriter();
    var dummyMessage = /** @type {!jspb.BinaryMessage} */({});

    writer.writeInt32(1, 100);

    // Add one message with 3 int fields.
    writer.writeMessage(2, dummyMessage, function() {
      writer.writeInt32(3, 300);
      writer.writeInt32(4, 400);
      writer.writeInt32(5, 500);
    });

    // Add one empty message.
    writer.writeMessage(6, dummyMessage, goog.nullFunction);

    writer.writeInt32(7, 700);

    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());

    // Validate outermost message.

    reader.nextField();
    assertEquals(1, reader.getFieldNumber());
    assertEquals(100, reader.readInt32());

    reader.nextField();
    assertEquals(2, reader.getFieldNumber());
    reader.readMessage(dummyMessage, function() {
      // Validate embedded message 1.
      reader.nextField();
      assertEquals(3, reader.getFieldNumber());
      assertEquals(300, reader.readInt32());

      reader.nextField();
      assertEquals(4, reader.getFieldNumber());
      assertEquals(400, reader.readInt32());

      reader.nextField();
      assertEquals(5, reader.getFieldNumber());
      assertEquals(500, reader.readInt32());

      assertEquals(false, reader.nextField());
    });

    reader.nextField();
    assertEquals(6, reader.getFieldNumber());
    reader.readMessage(dummyMessage, function() {
      // Validate embedded message 2.

      assertEquals(false, reader.nextField());
    });

    reader.nextField();
    assertEquals(7, reader.getFieldNumber());
    assertEquals(700, reader.readInt32());

    assertEquals(false, reader.nextField());
  });

  /**
   * Tests skipping fields of each type by interleaving them with sentinel
   * values and skipping everything that's not a sentinel.
   */
  it('testSkipField', function() {
    var writer = new jspb.BinaryWriter();

    var sentinel = 123456789;

    // Write varint fields of different sizes.
    writer.writeInt32(1, sentinel);
    writer.writeInt32(1, 1);
    writer.writeInt32(1, 1000);
    writer.writeInt32(1, 1000000);
    writer.writeInt32(1, 1000000000);

    // Write fixed 64-bit encoded fields.
    writer.writeInt32(2, sentinel);
    writer.writeDouble(2, 1);
    writer.writeFixed64(2, 1);
    writer.writeSfixed64(2, 1);

    // Write fixed 32-bit encoded fields.
    writer.writeInt32(3, sentinel);
    writer.writeFloat(3, 1);
    writer.writeFixed32(3, 1);
    writer.writeSfixed32(3, 1);

    // Write delimited fields.
    writer.writeInt32(4, sentinel);
    writer.writeBytes(4, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    writer.writeString(4, 'The quick brown fox jumps over the lazy dog');

    // Write a group with a nested group inside.
    writer.writeInt32(5, sentinel);
    var dummyMessage = /** @type {!jspb.BinaryMessage} */({});
    writer.writeGroup(5, dummyMessage, function() {
      writer.writeInt64(42, 42);
      writer.writeGroup(6, dummyMessage, function() {
        writer.writeInt64(84, 42);
      });
    });

    // Write final sentinel.
    writer.writeInt32(6, sentinel);

    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());

    function skip(field, count) {
      for (var i = 0; i < count; i++) {
        reader.nextField();
        if (field != reader.getFieldNumber()) throw 'fail!';
        reader.skipField();
      }
    }

    reader.nextField();
    assertEquals(1, reader.getFieldNumber());
    assertEquals(sentinel, reader.readInt32());
    skip(1, 4);

    reader.nextField();
    assertEquals(2, reader.getFieldNumber());
    assertEquals(sentinel, reader.readInt32());
    skip(2, 3);

    reader.nextField();
    assertEquals(3, reader.getFieldNumber());
    assertEquals(sentinel, reader.readInt32());
    skip(3, 3);

    reader.nextField();
    assertEquals(4, reader.getFieldNumber());
    assertEquals(sentinel, reader.readInt32());
    skip(4, 2);

    reader.nextField();
    assertEquals(5, reader.getFieldNumber());
    assertEquals(sentinel, reader.readInt32());
    skip(5, 1);

    reader.nextField();
    assertEquals(6, reader.getFieldNumber());
    assertEquals(sentinel, reader.readInt32());
  });


  /**
   * Tests packed fields.
   */
  it('testPackedFields', function() {
    var writer = new jspb.BinaryWriter();

    var sentinel = 123456789;

    var unsignedData = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    var signedData = [-1, 2, -3, 4, -5, 6, -7, 8, -9, 10];
    var floatData = [1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.10];
    var doubleData = [1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.10];
    var boolData = [true, false, true, true, false, false, true, false];

    for (var i = 0; i < floatData.length; i++) {
      floatData[i] = truncate(floatData[i]);
    }

    writer.writeInt32(1, sentinel);

    writer.writePackedInt32(2, signedData);
    writer.writePackedInt64(2, signedData);
    writer.writePackedUint32(2, unsignedData);
    writer.writePackedUint64(2, unsignedData);
    writer.writePackedSint32(2, signedData);
    writer.writePackedSint64(2, signedData);
    writer.writePackedFixed32(2, unsignedData);
    writer.writePackedFixed64(2, unsignedData);
    writer.writePackedSfixed32(2, signedData);
    writer.writePackedSfixed64(2, signedData);
    writer.writePackedFloat(2, floatData);
    writer.writePackedDouble(2, doubleData);
    writer.writePackedBool(2, boolData);
    writer.writePackedEnum(2, unsignedData);

    writer.writeInt32(3, sentinel);

    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());

    reader.nextField();
    assertEquals(sentinel, reader.readInt32());

    reader.nextField();
    assertElementsEquals(reader.readPackedInt32(), signedData);

    reader.nextField();
    assertElementsEquals(reader.readPackedInt64(), signedData);

    reader.nextField();
    assertElementsEquals(reader.readPackedUint32(), unsignedData);

    reader.nextField();
    assertElementsEquals(reader.readPackedUint64(), unsignedData);

    reader.nextField();
    assertElementsEquals(reader.readPackedSint32(), signedData);

    reader.nextField();
    assertElementsEquals(reader.readPackedSint64(), signedData);

    reader.nextField();
    assertElementsEquals(reader.readPackedFixed32(), unsignedData);

    reader.nextField();
    assertElementsEquals(reader.readPackedFixed64(), unsignedData);

    reader.nextField();
    assertElementsEquals(reader.readPackedSfixed32(), signedData);

    reader.nextField();
    assertElementsEquals(reader.readPackedSfixed64(), signedData);

    reader.nextField();
    assertElementsEquals(reader.readPackedFloat(), floatData);

    reader.nextField();
    assertElementsEquals(reader.readPackedDouble(), doubleData);

    reader.nextField();
    assertElementsEquals(reader.readPackedBool(), boolData);

    reader.nextField();
    assertElementsEquals(reader.readPackedEnum(), unsignedData);

    reader.nextField();
    assertEquals(sentinel, reader.readInt32());
  });


  /**
   * Byte blobs inside nested messages should always have their byte offset set
   * relative to the start of the outermost blob, not the start of their parent
   * blob.
   */
  it('testNestedBlobs', function() {
    // Create a proto consisting of two nested messages, with the inner one
    // containing a blob of bytes.

    var fieldTag = (1 << 3) | jspb.BinaryConstants.WireType.DELIMITED;
    var blob = [1, 2, 3, 4, 5];
    var writer = new jspb.BinaryWriter();
    var dummyMessage = /** @type {!jspb.BinaryMessage} */({});

    writer.writeMessage(1, dummyMessage, function() {
      writer.writeMessage(1, dummyMessage, function() {
        writer.writeBytes(1, blob);
      });
    });

    // Peel off the outer two message layers. Each layer should have two bytes
    // of overhead, one for the field tag and one for the length of the inner
    // blob.

    var decoder1 = new jspb.BinaryDecoder(writer.getResultBuffer());
    assertEquals(fieldTag, decoder1.readUnsignedVarint32());
    assertEquals(blob.length + 4, decoder1.readUnsignedVarint32());

    var decoder2 = new jspb.BinaryDecoder(decoder1.readBytes(blob.length + 4));
    assertEquals(fieldTag, decoder2.readUnsignedVarint32());
    assertEquals(blob.length + 2, decoder2.readUnsignedVarint32());

    assertEquals(fieldTag, decoder2.readUnsignedVarint32());
    assertEquals(blob.length, decoder2.readUnsignedVarint32());
    var bytes = decoder2.readBytes(blob.length);

    assertElementsEquals(bytes, blob);
  });


  /**
   * Tests read callbacks.
   */
  it('testReadCallbacks', function() {
    var writer = new jspb.BinaryWriter();
    var dummyMessage = /** @type {!jspb.BinaryMessage} */({});

    // Add an int, a submessage, and another int.
    writer.writeInt32(1, 100);

    writer.writeMessage(2, dummyMessage, function() {
      writer.writeInt32(3, 300);
      writer.writeInt32(4, 400);
      writer.writeInt32(5, 500);
    });

    writer.writeInt32(7, 700);

    // Create the reader and register a custom read callback.
    var reader = jspb.BinaryReader.alloc(writer.getResultBuffer());

    /**
     * @param {!jspb.BinaryReader} reader
     * @return {*}
     */
    function readCallback(reader) {
      reader.nextField();
      assertEquals(3, reader.getFieldNumber());
      assertEquals(300, reader.readInt32());

      reader.nextField();
      assertEquals(4, reader.getFieldNumber());
      assertEquals(400, reader.readInt32());

      reader.nextField();
      assertEquals(5, reader.getFieldNumber());
      assertEquals(500, reader.readInt32());

      assertEquals(false, reader.nextField());
    };

    reader.registerReadCallback('readCallback', readCallback);

    // Read the container message.
    reader.nextField();
    assertEquals(1, reader.getFieldNumber());
    assertEquals(100, reader.readInt32());

    reader.nextField();
    assertEquals(2, reader.getFieldNumber());
    reader.readMessage(dummyMessage, function() {
      // Decode the embedded message using the registered callback.
      reader.runReadCallback('readCallback');
    });

    reader.nextField();
    assertEquals(7, reader.getFieldNumber());
    assertEquals(700, reader.readInt32());

    assertEquals(false, reader.nextField());
  });
});
