/**
 * @fileoverview Utilities to index a binary proto by fieldnumbers without
 * relying on strutural proto information.
 */
goog.module('protobuf.binary.indexer');

const BinaryStorage = goog.require('protobuf.runtime.BinaryStorage');
const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const WireType = goog.require('protobuf.binary.WireType');
const {Field} = goog.require('protobuf.binary.field');
const {checkCriticalState} = goog.require('protobuf.internal.checks');
const {skipField, tagToFieldNumber, tagToWireType} = goog.require('protobuf.binary.tag');

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
    skipField(bufferDecoder, wireType, fieldNumber);
  }
  return storage;
}

exports = {
  buildIndex,
  tagToWireType,
};
