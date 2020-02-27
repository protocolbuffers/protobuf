goog.module('protobuf.binary.Storage');

const {checkDefAndNotNull} = goog.require('protobuf.internal.checks');

/**
 * 85% of the proto fields have a field number <= 24:
 * https://plx.corp.google.com/scripts2/script_5d._f02af6_0000_23b1_a15f_001a1139dd02
 *
 * @type {number}
 */
// LINT.IfChange
const DEFAULT_PIVOT = 24;
// LINT.ThenChange(//depot/google3/third_party/protobuf/javascript/runtime/kernel/storage_test.js,
// //depot/google3/net/proto2/contrib/js_proto/internal/kernel_message_generator.cc)

/**
 * Class storing all the fields of a protobuf message.
 *
 * @package
 * @template FieldType
 */
class Storage {
  /**
   * @param {number=} pivot
   */
  constructor(pivot = DEFAULT_PIVOT) {
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
   */
  getPivot() {
    return this.array_.length;
  }

  /**
   * Sets a field in the specified field number.
   *
   * @param {number} fieldNumber
   * @param {!FieldType} field
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
   * Executes the provided function once for each array element.
   *
   * @param {function(!FieldType, number): void} callback
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
   * @return {!Storage}
   */
  shallowCopy() {
    const copy = new Storage(this.getPivot());
    this.forEach(
        (field, fieldNumber) =>
            void copy.set(fieldNumber, field.shallowCopy()));
    return copy;
  }
}

exports = Storage;
