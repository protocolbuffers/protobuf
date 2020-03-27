/**
 * @fileoverview Utilities to index a binary proto by fieldnumbers without
 * relying on strutural proto information.
 */
goog.module('protobuf.binary.indexer');

const BinaryStorage = goog.require('protobuf.runtime.BinaryStorage');
const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const WireType = goog.require('protobuf.binary.WireType');
const {Field} = goog.require('protobuf.binary.field');
const {checkCriticalElementIndex, checkCriticalState} = goog.require('protobuf.internal.checks');

/**
 * Appends a new entry in the index array for the given field number.
 * @param {!BinaryStorage<!Field>} storage
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
 * Creates an index of field locations in a given binary protobuf.
 * @param {!BufferDecoder} bufferDecoder
 * @param {number|undefined} pivot
 * @return {!BinaryStorage<!Field>}
 * @package
 */
function buildIndex(bufferDecoder, pivot) {
  bufferDecoder.setCursor(bufferDecoder.startIndex());

  const storage = new BinaryStorage(pivot);
  while (bufferDecoder.hasNext()) {
    const tag = bufferDecoder.getUnsignedVarint32();
    const wireType = tagToWireType(tag);
    const fieldNumber = tagToFieldNumber(tag);
    checkCriticalState(fieldNumber > 0, `Invalid field number ${fieldNumber}`);

    addIndexEntry(storage, fieldNumber, wireType, bufferDecoder.cursor());

    checkCriticalState(
        !skipField_(bufferDecoder, wireType, fieldNumber),
        'Found unmatched stop group.');
  }
  return storage;
}

/**
 * Skips over fields until the next field of the message.
 * @param {!BufferDecoder} bufferDecoder
 * @param {!WireType} wireType
 * @param {number} fieldNumber
 * @return {boolean} Whether the field we skipped over was a stop group.
 * @private
 */
function skipField_(bufferDecoder, wireType, fieldNumber) {
  switch (wireType) {
    case WireType.VARINT:
      checkCriticalElementIndex(
          bufferDecoder.cursor(), bufferDecoder.endIndex());
      bufferDecoder.skipVarint();
      return false;
    case WireType.FIXED64:
      bufferDecoder.skip(8);
      return false;
    case WireType.DELIMITED:
      checkCriticalElementIndex(
          bufferDecoder.cursor(), bufferDecoder.endIndex());
      const length = bufferDecoder.getUnsignedVarint32();
      bufferDecoder.skip(length);
      return false;
    case WireType.START_GROUP:
      checkCriticalState(
          skipGroup_(bufferDecoder, fieldNumber), 'No end group found.');
      return false;
    case WireType.END_GROUP:
      // Signal that we found a stop group to the caller
      return true;
    case WireType.FIXED32:
      bufferDecoder.skip(4);
      return false;
    default:
      throw new Error(`Invalid wire type: ${wireType}`);
  }
}

/**
 * Skips over fields until it finds the end of a given group.
 * @param {!BufferDecoder} bufferDecoder
 * @param {number} groupFieldNumber
 * @return {boolean} Returns true if an end was found.
 * @private
 */
function skipGroup_(bufferDecoder, groupFieldNumber) {
  // On a start group we need to keep skipping fields until we find a
  // corresponding stop group
  // Note: Since we are calling skipField from here nested groups will be
  // handled by recursion of this method and thus we will not see a nested
  // STOP GROUP here unless there is something wrong with the input data.
  while (bufferDecoder.hasNext()) {
    const tag = bufferDecoder.getUnsignedVarint32();
    const wireType = tagToWireType(tag);
    const fieldNumber = tagToFieldNumber(tag);

    if (skipField_(bufferDecoder, wireType, fieldNumber)) {
      checkCriticalState(
          groupFieldNumber === fieldNumber,
          `Expected stop group for fieldnumber ${groupFieldNumber} not found.`);
      return true;
    }
  }
  return false;
}

exports = {
  buildIndex,
};
