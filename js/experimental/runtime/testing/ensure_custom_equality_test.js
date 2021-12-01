/**
 * @fileoverview Tests in this file will fail if our custom equality have not
 * been installed.
 * see b/131864652
 */

goog.module('protobuf.testing.ensureCustomEqualityTest');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const ByteString = goog.require('protobuf.ByteString');

describe('Custom equality', () => {
  it('ensure that custom equality for ArrayBuffer is installed', () => {
    const buffer1 = new ArrayBuffer(4);
    const buffer2 = new ArrayBuffer(4);
    const array = new Uint8Array(buffer1);
    array[0] = 1;
    expect(buffer1).not.toEqual(buffer2);
  });

  it('ensure that custom equality for ByteString is installed', () => {
    const HALLO_IN_BASE64 = 'aGFsbG8=';
    const BYTES_WITH_HALLO = new Uint8Array([
      'h'.charCodeAt(0),
      'a'.charCodeAt(0),
      'l'.charCodeAt(0),
      'l'.charCodeAt(0),
      'o'.charCodeAt(0),
    ]);

    const byteString1 = ByteString.fromBase64String(HALLO_IN_BASE64);
    const byteString2 = ByteString.fromArrayBufferView(BYTES_WITH_HALLO);
    expect(byteString1).toEqual(byteString2);
  });

  it('ensure that custom equality for BufferDecoder is installed', () => {
    const arrayBuffer1 = new Uint8Array([0, 1, 2]).buffer;
    const arrayBuffer2 = new Uint8Array([0, 1, 2]).buffer;

    const bufferDecoder1 = BufferDecoder.fromArrayBuffer(arrayBuffer1);
    const bufferDecoder2 = BufferDecoder.fromArrayBuffer(arrayBuffer2);
    expect(bufferDecoder1).toEqual(bufferDecoder2);
  });
});
