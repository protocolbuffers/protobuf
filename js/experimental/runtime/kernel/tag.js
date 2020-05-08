goog.module('protobuf.binary.tag');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const WireType = goog.require('protobuf.binary.WireType');
const {checkCriticalElementIndex, checkCriticalState} = goog.require('protobuf.internal.checks');

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
 * Combines wireType and fieldNumber into a tag.
 * @param {!WireType} wireType
 * @param {number} fieldNumber
 * @return {number}
 */
function createTag(wireType, fieldNumber) {
  return (fieldNumber << 3 | wireType) >>> 0;
}

/**
 * Returns the length, in bytes, of the field in the tag stream, less the tag
 * itself.
 * Note: This moves the cursor in the bufferDecoder.
 * @param {!BufferDecoder} bufferDecoder
 * @param {number} start
 * @param {!WireType} wireType
 * @param {number} fieldNumber
 * @return {number}
 * @private
 */
function getTagLength(bufferDecoder, start, wireType, fieldNumber) {
  bufferDecoder.setCursor(start);
  skipField(bufferDecoder, wireType, fieldNumber);
  return bufferDecoder.cursor() - start;
}

/**
 * @param {number} value
 * @return {number}
 */
function get32BitVarintLength(value) {
  if (value < 0) {
    return 5;
  }
  let size = 1;
  while (value >= 128) {
    size++;
    value >>>= 7;
  }
  return size;
}

/**
 * Skips over a field.
 * Note: If the field is a start group the entire group will be skipped, placing
 * the cursor onto the next field.
 * @param {!BufferDecoder} bufferDecoder
 * @param {!WireType} wireType
 * @param {number} fieldNumber
 */
function skipField(bufferDecoder, wireType, fieldNumber) {
  switch (wireType) {
    case WireType.VARINT:
      checkCriticalElementIndex(
          bufferDecoder.cursor(), bufferDecoder.endIndex());
      bufferDecoder.skipVarint();
      return;
    case WireType.FIXED64:
      bufferDecoder.skip(8);
      return;
    case WireType.DELIMITED:
      checkCriticalElementIndex(
          bufferDecoder.cursor(), bufferDecoder.endIndex());
      const length = bufferDecoder.getUnsignedVarint32();
      bufferDecoder.skip(length);
      return;
    case WireType.START_GROUP:
      const foundGroup = skipGroup_(bufferDecoder, fieldNumber);
      checkCriticalState(foundGroup, 'No end group found.');
      return;
    case WireType.FIXED32:
      bufferDecoder.skip(4);
      return;
    default:
      throw new Error(`Unexpected wire type: ${wireType}`);
  }
}

/**
 * Skips over fields until it finds the end of a given group consuming the stop
 * group tag.
 * @param {!BufferDecoder} bufferDecoder
 * @param {number} groupFieldNumber
 * @return {boolean} Whether the end group tag was found.
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

    if (wireType === WireType.END_GROUP) {
      checkCriticalState(
          groupFieldNumber === fieldNumber,
          `Expected stop group for fieldnumber ${groupFieldNumber} not found.`);
      return true;
    } else {
      skipField(bufferDecoder, wireType, fieldNumber);
    }
  }
  return false;
}

exports = {
  createTag,
  get32BitVarintLength,
  getTagLength,
  skipField,
  tagToWireType,
  tagToFieldNumber,
};
