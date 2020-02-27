/**
 * @fileoverview Utilities to index a binary proto by fieldnumbers without
 * relying on strutural proto information.
 */
goog.module('protobuf.binary.indexer');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const Storage = goog.require('protobuf.binary.Storage');
const WireType = goog.require('protobuf.binary.WireType');
const {Field} = goog.require('protobuf.binary.field');
const {checkCriticalElementIndex, checkCriticalState} = goog.require('protobuf.internal.checks');

/**
 * Appends a new entry in the index array for the given field number.
 * @param {!Storage<!Field>} storage
 * @param {number} fieldNumber
 * @param {!WireType} wireType
 * @param {number} startIndex
 */
function addIndexEntry(storage, fieldNumber, wireType, startIndex) {
  const field = storage.get(fieldNumber);
  if (field !== undefined) {
    field.addIndexEntry(wireType, startIndex);
  } else {
    storage.set(fieldNumber, Field.fromFirstIndexEntry(wireType, startIndex));
  }
}

/**
 * Returns wire type stored in a tag.
 * Protos store the wire type as the first 3 bit of a tag.
 * @param {number} tag
 * @return {!WireType}
 */
function tagToWireType(tag) {
  return /** @type {!WireType} */ (tag & 0x07);
}

/**
 * Returns the field number stored in a tag.
 * Protos store the field number in the upper 29 bits of a 32 bit number.
 * @param {number} tag
 * @return {number}
 */
function tagToFieldNumber(tag) {
  return tag >>> 3;
}

/**
 * An Indexer that indexes a given binary protobuf by fieldnumber.
 */
class Indexer {
  /**
   * @param {!BufferDecoder} bufferDecoder
   * @private
   */
  constructor(bufferDecoder) {
    /** @private @const {!BufferDecoder} */
    this.bufferDecoder_ = bufferDecoder;
  }

  /**
   * @param {number|undefined} pivot
   * @return {!Storage<!Field>}
   */
  index(pivot) {
    this.bufferDecoder_.setCursor(this.bufferDecoder_.startIndex());

    const storage = new Storage(pivot);
    while (this.bufferDecoder_.hasNext()) {
      const tag = this.bufferDecoder_.getUnsignedVarint32();
      const wireType = tagToWireType(tag);
      const fieldNumber = tagToFieldNumber(tag);
      checkCriticalState(
          fieldNumber > 0, `Invalid field number ${fieldNumber}`);

      addIndexEntry(
          storage, fieldNumber, wireType, this.bufferDecoder_.cursor());

      checkCriticalState(
          !this.skipField_(wireType, fieldNumber),
          'Found unmatched stop group.');
    }
    return storage;
  }

  /**
   * Skips over fields until the next field of the message.
   * @param {!WireType} wireType
   * @param {number} fieldNumber
   * @return {boolean} Whether the field we skipped over was a stop group.
   * @private
   */
  skipField_(wireType, fieldNumber) {
    switch (wireType) {
      case WireType.VARINT:
        checkCriticalElementIndex(
            this.bufferDecoder_.cursor(), this.bufferDecoder_.endIndex());
        this.bufferDecoder_.skipVarint(this.bufferDecoder_.cursor());
        return false;
      case WireType.FIXED64:
        this.bufferDecoder_.skip(8);
        return false;
      case WireType.DELIMITED:
        checkCriticalElementIndex(
            this.bufferDecoder_.cursor(), this.bufferDecoder_.endIndex());
        const length = this.bufferDecoder_.getUnsignedVarint32();
        this.bufferDecoder_.skip(length);
        return false;
      case WireType.START_GROUP:
        checkCriticalState(this.skipGroup_(fieldNumber), 'No end group found.');
        return false;
      case WireType.END_GROUP:
        // Signal that we found a stop group to the caller
        return true;
      case WireType.FIXED32:
        this.bufferDecoder_.skip(4);
        return false;
      default:
        throw new Error(`Invalid wire type: ${wireType}`);
    }
  }

  /**
   * Skips over fields until it finds the end of a given group.
   * @param {number} groupFieldNumber
   * @return {boolean} Returns true if an end was found.
   * @private
   */
  skipGroup_(groupFieldNumber) {
    // On a start group we need to keep skipping fields until we find a
    // corresponding stop group
    // Note: Since we are calling skipField from here nested groups will be
    // handled by recursion of this method and thus we will not see a nested
    // STOP GROUP here unless there is something wrong with the input data.
    while (this.bufferDecoder_.hasNext()) {
      const tag = this.bufferDecoder_.getUnsignedVarint32();
      const wireType = tagToWireType(tag);
      const fieldNumber = tagToFieldNumber(tag);

      if (this.skipField_(wireType, fieldNumber)) {
        checkCriticalState(
            groupFieldNumber === fieldNumber,
            `Expected stop group for fieldnumber ${
                groupFieldNumber} not found.`);
        return true;
      }
    }
    return false;
  }
}

/**
 * Creates an index of field locations in a given binary protobuf.
 * @param {!BufferDecoder} bufferDecoder
 * @param {number|undefined} pivot
 * @return {!Storage<!Field>}
 * @package
 */
function buildIndex(bufferDecoder, pivot) {
  return new Indexer(bufferDecoder).index(pivot);
}

exports = {
  buildIndex,
};
