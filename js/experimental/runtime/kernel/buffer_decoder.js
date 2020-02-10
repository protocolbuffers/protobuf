/**
 * @fileoverview A buffer implementation that can decode data for protobufs.
 */

goog.module('protobuf.binary.BufferDecoder');

const ByteString = goog.require('protobuf.ByteString');
const functions = goog.require('goog.functions');
const {POLYFILL_TEXT_ENCODING, checkCriticalElementIndex, checkCriticalPositionIndex, checkState} = goog.require('protobuf.internal.checks');
const {byteStringFromUint8ArrayUnsafe} = goog.require('protobuf.byteStringInternal');
const {concatenateByteArrays} = goog.require('protobuf.binary.uint8arrays');
const {decode} = goog.require('protobuf.binary.textencoding');

/**
 * Returns a valid utf-8 decoder function based on TextDecoder if available or
 * a polyfill.
 * Some of the environments we run in do not have TextDecoder defined.
 * TextDecoder is faster than our polyfill so we prefer it over the polyfill.
 * @return {function(!DataView): string}
 */
function getStringDecoderFunction() {
  if (goog.global['TextDecoder']) {
    const textDecoder = new goog.global['TextDecoder']('utf-8', {fatal: true});
    return bytes => textDecoder.decode(bytes);
  }
  if (POLYFILL_TEXT_ENCODING) {
    return decode;
  } else {
    throw new Error(
        'TextDecoder is missing. ' +
        'Enable protobuf.defines.POLYFILL_TEXT_ENCODING.');
  }
}

/** @type {function(): function(!DataView): string} */
const stringDecoderFunction =
    functions.cacheReturnValue(() => getStringDecoderFunction());

/** @type {function(): !DataView} */
const emptyDataView =
    functions.cacheReturnValue(() => new DataView(new ArrayBuffer(0)));

class BufferDecoder {
  /**
   * @param {!Array<!BufferDecoder>} bufferDecoders
   * @return {!BufferDecoder}
   */
  static merge(bufferDecoders) {
    const uint8Arrays = bufferDecoders.map(b => b.asUint8Array());
    const bytesArray = concatenateByteArrays(uint8Arrays);
    return BufferDecoder.fromArrayBuffer(bytesArray.buffer);
  }

  /**
   * @param {!ArrayBuffer} arrayBuffer
   * @return {!BufferDecoder}
   */
  static fromArrayBuffer(arrayBuffer) {
    return new BufferDecoder(
        new DataView(arrayBuffer), 0, arrayBuffer.byteLength);
  }

  /**
   * @param {!DataView} dataView
   * @param {number} startIndex
   * @param {number} length
   * @private
   */
  constructor(dataView, startIndex, length) {
    /** @private @const {!DataView} */
    this.dataView_ = dataView;
    /** @private @const {number} */
    this.startIndex_ = startIndex;
    /** @private @const {number} */
    this.endIndex_ = this.startIndex_ + length;
  }

  /**
   * Returns the start index of the underlying buffer.
   * @return {number}
   */
  startIndex() {
    return this.startIndex_;
  }

  /**
   * Returns the end index of the underlying buffer.
   * @return {number}
   */
  endIndex() {
    return this.endIndex_;
  }

  /**
   * Returns the length of the underlying buffer.
   * @return {number}
   */
  length() {
    return this.endIndex_ - this.startIndex_;
  }

  /**
   * Returns a float32 from a given index
   * @param {number} index
   * @return {number}
   */
  getFloat32(index) {
    return this.dataView_.getFloat32(index, true);
  }

  /**
   * Returns a float64 from a given index
   * @param {number} index
   * @return {number}
   */
  getFloat64(index) {
    return this.dataView_.getFloat64(index, true);
  }

  /**
   * Returns an int32 from a given index
   * @param {number} index
   * @return {number}
   */
  getInt32(index) {
    return this.dataView_.getInt32(index, true);
  }

  /**
   * @param {number} index
   * @return {number}
   */
  getUint8(index) {
    return this.dataView_.getUint8(index);
  }

  /**
   * Returns a uint32 from a given index
   * @param {number} index
   * @return {number}
   */
  getUint32(index) {
    return this.dataView_.getUint32(index, true);
  }

  /**
   * Returns two JS numbers each representing 32 bits of a 64 bit number.
   * @param {number} index
   * @return {{lowBits: number, highBits: number, dataStart: number}}
   */
  getVarint(index) {
    let start = index;
    let lowBits = 0;
    let highBits = 0;

    for (let shift = 0; shift < 28; shift += 7) {
      const b = this.dataView_.getUint8(start++);
      lowBits |= (b & 0x7F) << shift;
      if ((b & 0x80) === 0) {
        return {lowBits, highBits, dataStart: start};
      }
    }

    const middleByte = this.dataView_.getUint8(start++);

    // last four bits of the first 32 bit number
    lowBits |= (middleByte & 0x0F) << 28;

    // 3 upper bits are part of the next 32 bit number
    highBits = (middleByte & 0x70) >> 4;

    if ((middleByte & 0x80) === 0) {
      return {lowBits, highBits, dataStart: start};
    }


    for (let shift = 3; shift <= 31; shift += 7) {
      const b = this.dataView_.getUint8(start++);
      highBits |= (b & 0x7F) << shift;
      if ((b & 0x80) === 0) {
        return {lowBits, highBits, dataStart: start};
      }
    }

    checkState(false, 'Data is longer than 10 bytes');
    return {lowBits, highBits, dataStart: start};
  }

  /**
   * Skips over a varint at a given index and returns the next position.
   * @param {number} index Start of the data.
   * @return {number} Position of the first byte after the varint.
   * @package
   */
  skipVarint(index) {
    let cursor = index;
    checkCriticalElementIndex(cursor, this.endIndex());
    while (this.dataView_.getUint8(cursor++) & 0x80) {
      checkCriticalElementIndex(cursor, this.endIndex());
    }
    checkCriticalPositionIndex(cursor, index + 10);
    return cursor;
  }

  /**
   * @param {number} startIndex
   * @param {number} length
   * @return {!BufferDecoder}
   */
  subBufferDecoder(startIndex, length) {
    checkState(
        startIndex >= this.startIndex(),
        `Current start: ${this.startIndex()}, subBufferDecoder start: ${
            startIndex}`);
    checkState(length >= 0, `Length: ${length}`);
    checkState(
        startIndex + length <= this.endIndex(),
        `Current end: ${this.endIndex()}, subBufferDecoder start: ${
            startIndex}, subBufferDecoder length: ${length}`);
    return new BufferDecoder(this.dataView_, startIndex, length);
  }

  /**
   * Returns the buffer as a string.
   * @return {string}
   */
  asString() {
    // TODO: Remove this check when we no longer need to support IE
    const stringDataView = this.length() === 0 ?
        emptyDataView() :
        new DataView(this.dataView_.buffer, this.startIndex_, this.length());
    return stringDecoderFunction()(stringDataView);
  }

  /**
   * Returns the buffer as a ByteString.
   * @return {!ByteString}
   */
  asByteString() {
    return byteStringFromUint8ArrayUnsafe(this.asUint8Array());
  }

  /**
   * Returns the DataView as an Uint8Array. DO NOT MODIFY or expose the
   * underlying buffer.
   *
   * @package
   * @return {!Uint8Array}
   */
  asUint8Array() {
    return new Uint8Array(
        this.dataView_.buffer, this.startIndex_, this.length());
  }
}

exports = BufferDecoder;
