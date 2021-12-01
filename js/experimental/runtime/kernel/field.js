/**
 * @fileoverview Contains classes that hold data for a protobuf field.
 */

goog.module('protobuf.binary.field');

const WireType = goog.requireType('protobuf.binary.WireType');
const Writer = goog.requireType('protobuf.binary.Writer');
const {checkDefAndNotNull, checkState} = goog.require('protobuf.internal.checks');

/**
 * Number of bits taken to represent a wire type.
 * @const {number}
 */
const WIRE_TYPE_LENGTH_BITS = 3;

/** @const {number} */
const WIRE_TYPE_EXTRACTOR = (1 << WIRE_TYPE_LENGTH_BITS) - 1;

/**
 * An IndexEntry consists of the wire type and the position of a field in the
 * binary data. The wire type and the position are encoded into a single number
 * to save memory, which can be decoded using Field.getWireType() and
 * Field.getStartIndex() methods.
 * @typedef {number}
 */
let IndexEntry;

/**
 * An entry containing the index into the binary data and/or the corresponding
 * cached JS object(s) for a field.
 * @template T
 * @final
 * @package
 */
class Field {
  /**
   * Creates a field and inserts the wireType and position of the first
   * occurrence of a field.
   * @param {!WireType} wireType
   * @param {number} startIndex
   * @return {!Field}
   */
  static fromFirstIndexEntry(wireType, startIndex) {
    return new Field([Field.encodeIndexEntry(wireType, startIndex)]);
  }

  /**
   * @param {T} decodedValue The cached JS object decoded from the binary data.
   * @param {function(!Writer, number, T):void|undefined} encoder Write function
   *     to encode the cache into binary bytes.
   * @return {!Field}
   * @template T
   */
  static fromDecodedValue(decodedValue, encoder) {
    return new Field(null, decodedValue, encoder);
  }

  /**
   * @param {!WireType} wireType
   * @param {number} startIndex
   * @return {!IndexEntry}
   */
  static encodeIndexEntry(wireType, startIndex) {
    return startIndex << WIRE_TYPE_LENGTH_BITS | wireType;
  }

  /**
   * @param {!IndexEntry} indexEntry
   * @return {!WireType}
   */
  static getWireType(indexEntry) {
    return /** @type {!WireType} */ (indexEntry & WIRE_TYPE_EXTRACTOR);
  }

  /**
   * @param {!IndexEntry} indexEntry
   * @return {number}
   */
  static getStartIndex(indexEntry) {
    return indexEntry >> WIRE_TYPE_LENGTH_BITS;
  }

  /**
   * @param {?Array<!IndexEntry>} indexArray
   * @param {T=} decodedValue
   * @param {function(!Writer, number, T):void=} encoder
   * @private
   */
  constructor(indexArray, decodedValue = undefined, encoder = undefined) {
    checkState(
        !!indexArray || decodedValue !== undefined,
        'At least one of indexArray and decodedValue must be set');

    /** @private {?Array<!IndexEntry>} */
    this.indexArray_ = indexArray;
    /** @private {T|undefined} */
    this.decodedValue_ = decodedValue;
    // TODO: Consider storing an enum to represent encoder
    /** @private {function(!Writer, number, T)|undefined} */
    this.encoder_ = encoder;
  }

  /**
   * Adds a new IndexEntry.
   * @param {!WireType} wireType
   * @param {number} startIndex
   */
  addIndexEntry(wireType, startIndex) {
    checkDefAndNotNull(this.indexArray_)
        .push(Field.encodeIndexEntry(wireType, startIndex));
  }

  /**
   * Returns the array of IndexEntry.
   * @return {?Array<!IndexEntry>}
   */
  getIndexArray() {
    return this.indexArray_;
  }

  /**
   * Caches the decoded value and sets the write function to encode cache into
   * binary bytes.
   * @param {T} decodedValue
   * @param {function(!Writer, number, T):void|undefined} encoder
   */
  setCache(decodedValue, encoder) {
    this.decodedValue_ = decodedValue;
    this.encoder_ = encoder;
    this.maybeRemoveIndexArray_();
  }

  /**
   * If the decoded value has been set.
   * @return {boolean}
   */
  hasDecodedValue() {
    return this.decodedValue_ !== undefined;
  }

  /**
   * Returns the cached decoded value. The value needs to be set when this
   * method is called.
   * @return {T}
   */
  getDecodedValue() {
    // Makes sure that the decoded value in the cache has already been set. This
    // prevents callers from doing `if (field.getDecodedValue()) {...}` to check
    // if a value exist in the cache, because the check might return false even
    // if the cache has a valid value set (e.g. 0 or empty string).
    checkState(this.decodedValue_ !== undefined);
    return this.decodedValue_;
  }

  /**
   * Returns the write function to encode cache into binary bytes.
   * @return {function(!Writer, number, T)|undefined}
   */
  getEncoder() {
    return this.encoder_;
  }

  /**
   * Returns a copy of the field, containing the original index entries and a
   * shallow copy of the cache.
   * @return {!Field}
   */
  shallowCopy() {
    // Repeated fields are arrays in the cache.
    // We have to copy the array to make sure that modifications to a repeated
    // field (e.g. add) are not seen on a cloned accessor.
    const copiedCache = this.hasDecodedValue() ?
        (Array.isArray(this.getDecodedValue()) ? [...this.getDecodedValue()] :
                                                 this.getDecodedValue()) :
        undefined;
    return new Field(this.getIndexArray(), copiedCache, this.getEncoder());
  }

  /**
   * @private
   */
  maybeRemoveIndexArray_() {
    checkState(
        this.encoder_ === undefined || this.decodedValue_ !== undefined,
        'Encoder exists but decoded value doesn\'t');
    if (this.encoder_ !== undefined) {
      this.indexArray_ = null;
    }
  }
}

exports = {
  IndexEntry,
  Field,
};
