goog.module('protobuf.runtime.BinaryStorage');

const Storage = goog.require('protobuf.runtime.Storage');
const {checkDefAndNotNull} = goog.require('protobuf.internal.checks');

/**
 * Class storing all the fields of a binary protobuf message.
 *
 * @package
 * @template FieldType
 * @implements {Storage}
 */
class BinaryStorage {
  /**
   * @param {number=} pivot
   */
  constructor(pivot = Storage.DEFAULT_PIVOT) {
    /**
     * Fields having a field number no greater than the pivot value are stored
     * into an array for fast access. A field with field number X is stored into
     * the array position X - 1.
     *
     * @private @const {!Array<!FieldType|undefined>}
     */
    this.array_ = new Array(pivot);

    /**
     * Fields having a field number higher than the pivot value are stored into
     * the map. We create the map only when it's needed, since even an empty map
     * takes up a significant amount of memory.
     *
     * @private {?Map<number, !FieldType>}
     */
    this.map_ = null;
  }

  /**
   * Fields having a field number no greater than the pivot value are stored
   * into an array for fast access. A field with field number X is stored into
   * the array position X - 1.
   * @return {number}
   * @override
   */
  getPivot() {
    return this.array_.length;
  }

  /**
   * Sets a field in the specified field number.
   *
   * @param {number} fieldNumber
   * @param {!FieldType} field
   * @override
   */
  set(fieldNumber, field) {
    if (fieldNumber <= this.getPivot()) {
      this.array_[fieldNumber - 1] = field;
    } else {
      if (this.map_) {
        this.map_.set(fieldNumber, field);
      } else {
        this.map_ = new Map([[fieldNumber, field]]);
      }
    }
  }

  /**
   * Returns a field at the specified field number.
   *
   * @param {number} fieldNumber
   * @return {!FieldType|undefined}
   * @override
   */
  get(fieldNumber) {
    if (fieldNumber <= this.getPivot()) {
      return this.array_[fieldNumber - 1];
    } else {
      return this.map_ ? this.map_.get(fieldNumber) : undefined;
    }
  }

  /**
   * Deletes a field from the specified field number.
   *
   * @param {number} fieldNumber
   * @override
   */
  delete(fieldNumber) {
    if (fieldNumber <= this.getPivot()) {
      delete this.array_[fieldNumber - 1];
    } else {
      if (this.map_) {
        this.map_.delete(fieldNumber);
      }
    }
  }

  /**
   * Executes the provided function once for each field.
   *
   * @param {function(!FieldType, number): void} callback
   * @override
   */
  forEach(callback) {
    this.array_.forEach((field, fieldNumber) => {
      if (field) {
        callback(checkDefAndNotNull(field), fieldNumber + 1);
      }
    });
    if (this.map_) {
      this.map_.forEach(callback);
    }
  }

  /**
   * Creates a shallow copy of the storage.
   *
   * @return {!BinaryStorage}
   * @override
   */
  shallowCopy() {
    const copy = new BinaryStorage(this.getPivot());
    this.forEach(
        (field, fieldNumber) =>
            void copy.set(fieldNumber, field.shallowCopy()));
    return copy;
  }
}

exports = BinaryStorage;
