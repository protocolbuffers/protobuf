/**
 * @fileoverview Tests to make sure LazyAccessor can read data in a backward
 * compatible way even when protobuf schema changes according to the rules
 * defined in
 * https://developers.google.com/protocol-buffers/docs/proto#updating and
 * https://developers.google.com/protocol-buffers/docs/proto3#updating.
 *
 * third_party/protobuf/conformance/binary_json_conformance_suite.cc already
 * covers many compatibility tests, this file covers only the tests not covered
 * by binary_json_conformance_suite. Ultimately all of the tests in this file
 * should be moved to binary_json_conformance_suite.
 */
goog.module('protobuf.binary.LazyAccessorCompatibilityTest');

goog.setTestOnly();

const ByteString = goog.require('protobuf.ByteString');
const Int64 = goog.require('protobuf.Int64');
const LazyAccessor = goog.require('protobuf.binary.LazyAccessor');
const TestMessage = goog.require('protobuf.testing.binary.TestMessage');
const {CHECK_CRITICAL_STATE} = goog.require('protobuf.internal.checks');

/**
 * @param {...number} bytes
 * @return {!ArrayBuffer}
 */
function createArrayBuffer(...bytes) {
  return new Uint8Array(bytes).buffer;
}

/**
 * Returns the Unicode character codes of a string.
 * @param {string} str
 * @return {!Array<number>}
 */
function getCharacterCodes(str) {
  return Array.from(str, (c) => c.charCodeAt(0));
}

describe('optional -> repeated compatibility', () => {
  it('is maintained for scalars', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.setInt32(1, 1);
    const serializedData = oldAccessor.serialize();
    expect(serializedData).toEqual(createArrayBuffer(0x8, 0x1));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getRepeatedInt32Size(1)).toEqual(1);
    expect(newAccessor.getRepeatedInt32Element(1, 0)).toEqual(1);
  });

  it('is maintained for messages', () => {
    const message = new TestMessage(LazyAccessor.createEmpty());
    message.setInt32(1, 1);

    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.setMessage(1, message);
    const serializedData = oldAccessor.serialize();
    expect(serializedData).toEqual(createArrayBuffer(0xA, 0x2, 0x8, 0x1));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getRepeatedMessageSize(1, TestMessage.instanceCreator))
        .toEqual(1);
    expect(
        newAccessor.getRepeatedMessageElement(1, TestMessage.instanceCreator, 0)
            .serialize())
        .toEqual(message.serialize());
  });

  it('is maintained for bytes', () => {
    const message = new TestMessage(LazyAccessor.createEmpty());
    message.setInt32(1, 1);

    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.setBytes(
        1, ByteString.fromArrayBuffer(createArrayBuffer(0xA, 0xB)));
    const serializedData = oldAccessor.serialize();
    expect(serializedData).toEqual(createArrayBuffer(0xA, 0x2, 0xA, 0xB));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getRepeatedBytesSize(1)).toEqual(1);
    expect(newAccessor.getRepeatedBoolElement(1, 0))
        .toEqual(ByteString.fromArrayBuffer(createArrayBuffer(0xA, 0xB)));
  });

  it('is maintained for strings', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.setString(1, 'hello');
    const serializedData = oldAccessor.serialize();
    expect(serializedData)
        .toEqual(createArrayBuffer(0xA, 0x5, 0x68, 0x65, 0x6C, 0x6C, 0x6F));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getRepeatedStringSize(1)).toEqual(1);
    expect(newAccessor.getRepeatedStringElement(1, 0)).toEqual('hello');
  });
});

describe('LazyAccessor repeated -> optional compatibility', () => {
  it('is maintained for unpacked scalars', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.addUnpackedInt32Element(1, 0);
    oldAccessor.addUnpackedInt32Element(1, 1);
    const serializedData = oldAccessor.serialize();
    expect(serializedData).toEqual(createArrayBuffer(0x8, 0x0, 0x8, 0x1));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getInt32WithDefault(1)).toEqual(1);
    expect(newAccessor.serialize()).toEqual(serializedData);
  });

  // repeated -> optional transformation is not supported for packed fields yet:
  // go/proto-schema-repeated
  it('is not maintained for packed scalars', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.addPackedInt32Element(1, 0);
    oldAccessor.addPackedInt32Element(1, 1);
    const serializedData = oldAccessor.serialize();
    expect(serializedData).toEqual(createArrayBuffer(0xA, 0x2, 0x0, 0x1));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    if (CHECK_CRITICAL_STATE) {
      expect(() => newAccessor.getInt32WithDefault(1)).toThrow();
    }
  });

  it('is maintained for messages', () => {
    const message1 = new TestMessage(LazyAccessor.createEmpty());
    message1.setInt32(1, 1);
    const message2 = new TestMessage(LazyAccessor.createEmpty());
    message2.setInt32(1, 2);
    message2.setInt32(2, 3);

    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.addRepeatedMessageElement(
        1, message1, TestMessage.instanceCreator);
    oldAccessor.addRepeatedMessageElement(
        1, message2, TestMessage.instanceCreator);
    const serializedData = oldAccessor.serialize();
    expect(serializedData)
        .toEqual(createArrayBuffer(
            0xA, 0x2, 0x8, 0x1, 0xA, 0x4, 0x8, 0x2, 0x10, 0x3));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    // Values from message1 and message2 have been merged
    const newMessage = newAccessor.getMessage(1, TestMessage.instanceCreator);
    expect(newMessage.getRepeatedInt32Size(1)).toEqual(2);
    expect(newMessage.getRepeatedInt32Element(1, 0)).toEqual(1);
    expect(newMessage.getRepeatedInt32Element(1, 1)).toEqual(2);
    expect(newMessage.getInt32WithDefault(2)).toEqual(3);
    expect(newMessage.serialize())
        .toEqual(createArrayBuffer(0x8, 0x1, 0x8, 0x2, 0x10, 0x3));
  });

  it('is maintained for bytes', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.addRepeatedBytesElement(
        1, ByteString.fromArrayBuffer(createArrayBuffer(0xA, 0xB)));
    oldAccessor.addRepeatedBytesElement(
        1, ByteString.fromArrayBuffer(createArrayBuffer(0xC, 0xD)));
    const serializedData = oldAccessor.serialize();
    expect(serializedData)
        .toEqual(createArrayBuffer(0xA, 0x2, 0xA, 0xB, 0xA, 0x2, 0xC, 0xD));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getBytesWithDefault(1))
        .toEqual(ByteString.fromArrayBuffer(createArrayBuffer(0xC, 0xD)));
    expect(newAccessor.serialize()).toEqual(serializedData);
  });

  it('is maintained for strings', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.addRepeatedStringElement(1, 'hello');
    oldAccessor.addRepeatedStringElement(1, 'world');
    const serializedData = oldAccessor.serialize();
    expect(serializedData)
        .toEqual(createArrayBuffer(
            0xA, 0x5, ...getCharacterCodes('hello'), 0xA, 0x5,
            ...getCharacterCodes('world')));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getStringWithDefault(1)).toEqual('world');
    expect(newAccessor.serialize()).toEqual(serializedData);
  });
});

describe('Type change', () => {
  it('is supported for fixed32 -> sfixed32', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.setFixed32(1, 4294967295);
    const serializedData = oldAccessor.serialize();
    expect(serializedData)
        .toEqual(createArrayBuffer(0xD, 0xFF, 0xFF, 0xFF, 0xFF));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getSfixed32WithDefault(1)).toEqual(-1);
    expect(newAccessor.serialize()).toEqual(serializedData);
  });

  it('is supported for sfixed32 -> fixed32', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.setSfixed32(1, -1);
    const serializedData = oldAccessor.serialize();
    expect(serializedData)
        .toEqual(createArrayBuffer(0xD, 0xFF, 0xFF, 0xFF, 0xFF));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getFixed32WithDefault(1)).toEqual(4294967295);
    expect(newAccessor.serialize()).toEqual(serializedData);
  });

  it('is supported for fixed64 -> sfixed64', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.setFixed64(1, Int64.fromHexString('0xFFFFFFFFFFFFFFFF'));
    const serializedData = oldAccessor.serialize();
    expect(serializedData)
        .toEqual(createArrayBuffer(
            0x9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getSfixed64WithDefault(1)).toEqual(Int64.fromInt(-1));
    expect(newAccessor.serialize()).toEqual(serializedData);
  });

  it('is supported for sfixed64 -> fixed64', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.setSfixed64(1, Int64.fromInt(-1));
    const serializedData = oldAccessor.serialize();
    expect(serializedData)
        .toEqual(createArrayBuffer(
            0x9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getFixed64WithDefault(1))
        .toEqual(Int64.fromHexString('0xFFFFFFFFFFFFFFFF'));
    expect(newAccessor.serialize()).toEqual(serializedData);
  });

  it('is supported for bytes -> message', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    oldAccessor.setBytes(
        1, ByteString.fromArrayBuffer(createArrayBuffer(0x8, 0x1)));
    const serializedData = oldAccessor.serialize();
    expect(serializedData).toEqual(createArrayBuffer(0xA, 0x2, 0x8, 0x1));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    const message = newAccessor.getMessage(1, TestMessage.instanceCreator);
    expect(message.getInt32WithDefault(1)).toEqual(1);
    expect(message.serialize()).toEqual(createArrayBuffer(0x8, 0x1));
    expect(newAccessor.serialize()).toEqual(serializedData);
  });

  it('is supported for message -> bytes', () => {
    const oldAccessor = LazyAccessor.createEmpty();
    const message = new TestMessage(LazyAccessor.createEmpty());
    message.setInt32(1, 1);
    oldAccessor.setMessage(1, message);
    const serializedData = oldAccessor.serialize();
    expect(serializedData).toEqual(createArrayBuffer(0xA, 0x2, 0x8, 0x1));

    const newAccessor = LazyAccessor.fromArrayBuffer(serializedData);
    expect(newAccessor.getBytesWithDefault(1))
        .toEqual(ByteString.fromArrayBuffer(createArrayBuffer(0x8, 0x1)));
    expect(newAccessor.serialize()).toEqual(serializedData);
  });
});
