/**
 * @fileoverview Tests for indexer.js.
 */
goog.module('protobuf.runtime.MessageSetTest');

goog.setTestOnly();

const InternalMessage = goog.require('protobuf.binary.InternalMessage');
const Kernel = goog.require('protobuf.runtime.Kernel');
const MessageSet = goog.require('protobuf.runtime.MessageSet');

/**
 * @implements {InternalMessage}
 */
class KernelWrapper {
  static createEmpty() {
    return new KernelWrapper(Kernel.createEmpty());
  }

  constructor(kernel) {
    this.kernel = kernel;
  }

  /**
   * @return {!Kernel}
   * @override
   */
  internalGetKernel() {
    return this.kernel;
  }
}

/**
 * @param {...number} bytes
 * @return {!ArrayBuffer}
 */
function createArrayBuffer(...bytes) {
  return new Uint8Array(bytes).buffer;
}

describe('MessageSet does', () => {
  it('returns no messages for empty set', () => {
    const messageSet = MessageSet.createEmpty();
    expect(messageSet.getMessageOrNull(12345, k => new KernelWrapper(k)))
        .toBeNull();
  });

  it('returns no kernel for empty set', () => {
    const messageSet = MessageSet.createEmpty();
    expect(messageSet.getMessageAccessorOrNull(12345)).toBeNull();
  });

  it('returns message that has been set', () => {
    const messageSet = MessageSet.createEmpty();
    const message = KernelWrapper.createEmpty();
    messageSet.setMessage(12345, message);
    expect(messageSet.getMessageOrNull(12345, k => new KernelWrapper(k)))
        .toBe(message);
  });

  it('returns null for cleared message', () => {
    const messageSet = MessageSet.createEmpty();
    const message = KernelWrapper.createEmpty();
    messageSet.setMessage(12345, message);
    messageSet.clearMessage(12345);
    expect(messageSet.getMessageAccessorOrNull(12345)).toBeNull();
  });

  it('returns false for not present message', () => {
    const messageSet = MessageSet.createEmpty();
    expect(messageSet.hasMessage(12345)).toBe(false);
  });

  it('returns true for present message', () => {
    const messageSet = MessageSet.createEmpty();
    const message = KernelWrapper.createEmpty();
    messageSet.setMessage(12345, message);
    expect(messageSet.hasMessage(12345)).toBe(true);
  });

  it('returns false for cleared message', () => {
    const messageSet = MessageSet.createEmpty();
    const message = KernelWrapper.createEmpty();
    messageSet.setMessage(12345, message);
    messageSet.clearMessage(12345);
    expect(messageSet.hasMessage(12345)).toBe(false);
  });

  it('pick the last value in the stream', () => {
    const arrayBuffer = createArrayBuffer(
        0x52,  // Tag (field:10, length delimited)
        0x14,  // Length of 20 bytes
        0x0B,  // Start group fieldnumber 1
        0x10,  // Tag (field 2, varint)
        0xB9,  // 12345
        0x60,  // 12345
        0x1A,  // Tag (field 3, length delimited)
        0x03,  // length 3
        0xA0,  // Tag (fieldnumber 20, varint)
        0x01,  // Tag (fieldnumber 20, varint)
        0x1E,  // 30
        0x0C,  // Stop Group field number 1
        // second group
        0x0B,  // Start group fieldnumber 1
        0x10,  // Tag (field 2, varint)
        0xB9,  // 12345
        0x60,  // 12345
        0x1A,  // Tag (field 3, length delimited)
        0x03,  // length 3
        0xA0,  // Tag (fieldnumber 20, varint)
        0x01,  // Tag (fieldnumber 20, varint)
        0x01,  // 1
        0x0C   // Stop Group field number 1
    );

    const outterMessage = Kernel.fromArrayBuffer(arrayBuffer);

    const messageSet = outterMessage.getMessage(10, MessageSet.fromKernel);

    const message =
        messageSet.getMessageOrNull(12345, k => new KernelWrapper(k))
            .internalGetKernel();
    expect(message.getInt32WithDefault(20)).toBe(1);
  });

  it('removes duplicates when read', () => {
    const arrayBuffer = createArrayBuffer(
        0x52,  // Tag (field:10, length delimited)
        0x14,  // Length of 20 bytes
        0x0B,  // Start group fieldnumber 1
        0x10,  // Tag (field 2, varint)
        0xB9,  // 12345
        0x60,  // 12345
        0x1A,  // Tag (field 3, length delimited)
        0x03,  // length 3
        0xA0,  // Tag (fieldnumber 20, varint)
        0x01,  // Tag (fieldnumber 20, varint)
        0x1E,  // 30
        0x0C,  // Stop Group field number 1
        // second group
        0x0B,  // Start group fieldnumber 1
        0x10,  // Tag (field 2, varint)
        0xB9,  // 12345
        0x60,  // 12345
        0x1A,  // Tag (field 3, length delimited)
        0x03,  // length 3
        0xA0,  // Tag (fieldnumber 20, varint)
        0x01,  // Tag (fieldnumber 20, varint)
        0x01,  // 1
        0x0C   // Stop Group field number 1
    );


    const outterMessage = Kernel.fromArrayBuffer(arrayBuffer);
    outterMessage.getMessageAttach(10, MessageSet.fromKernel);

    expect(outterMessage.serialize())
        .toEqual(createArrayBuffer(
            0x52,  // Tag (field:10, length delimited)
            0x0A,  // Length of 10 bytes
            0x0B,  // Start group fieldnumber 1
            0x10,  // Tag (field 2, varint)
            0xB9,  // 12345
            0x60,  // 12345
            0x1A,  // Tag (field 3, length delimited)
            0x03,  // length 3
            0xA0,  // Tag (fieldnumber 20, varint)
            0x01,  // Tag (fieldnumber 20, varint)
            0x01,  // 1
            0x0C   // Stop Group field number 1
            ));
  });
});

describe('Optional MessageSet does', () => {
  // message Bar {
  //  optional MessageSet mset = 10;
  //}
  //
  // message Foo {
  //  extend proto2.bridge.MessageSet {
  //    optional Foo message_set_extension = 12345;
  //  }
  //  optional int32 f20 = 20;
  //}

  it('encode as a field', () => {
    const fooMessage = Kernel.createEmpty();
    fooMessage.setInt32(20, 30);

    const messageSet = MessageSet.createEmpty();
    messageSet.setMessage(12345, new KernelWrapper(fooMessage));

    const barMessage = Kernel.createEmpty();
    barMessage.setMessage(10, messageSet);

    expect(barMessage.serialize())
        .toEqual(createArrayBuffer(
            0x52,  // Tag (field:10, length delimited)
            0x0A,  // Length of 10 bytes
            0x0B,  // Start group fieldnumber 1
            0x10,  // Tag (field 2, varint)
            0xB9,  // 12345
            0x60,  // 12345
            0x1A,  // Tag (field 3, length delimited)
            0x03,  // length 3
            0xA0,  // Tag (fieldnumber 20, varint)
            0x01,  // Tag (fieldnumber 20, varint)
            0x1E,  // 30
            0x0C   // Stop Group field number 1
            ));
  });

  it('deserializes', () => {
    const fooMessage = Kernel.createEmpty();
    fooMessage.setInt32(20, 30);

    const messageSet = MessageSet.createEmpty();
    messageSet.setMessage(12345, new KernelWrapper(fooMessage));


    const barMessage = Kernel.createEmpty();
    barMessage.setMessage(10, messageSet);

    const arrayBuffer = barMessage.serialize();

    const barMessageParsed = Kernel.fromArrayBuffer(arrayBuffer);
    expect(barMessageParsed.hasFieldNumber(10)).toBe(true);

    const messageSetParsed =
        barMessageParsed.getMessage(10, MessageSet.fromKernel);

    const fooMessageParsed =
        messageSetParsed.getMessageOrNull(12345, k => new KernelWrapper(k))
            .internalGetKernel();

    expect(fooMessageParsed.getInt32WithDefault(20)).toBe(30);
  });
});
