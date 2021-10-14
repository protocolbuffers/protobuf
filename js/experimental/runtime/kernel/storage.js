goog.module('protobuf.runtime.Storage');

/**
 * Interface for getting and storing fields of a protobuf message.
 *
 * @interface
 * @package
 * @template FieldType
 */
class Storage {
  /**
   * Returns the pivot value.
   *
   * @return {number}
   */
  getPivot() {}

  /**
   * Sets a field in the specified field number.
   *
   * @param {number} fieldNumber
   * @param {!FieldType} field
   */
  set(fieldNumber, field) {}

  /**
   * Returns a field at the specified field number.
   *
   * @param {number} fieldNumber
   * @return {!FieldType|undefined}
   */
  get(fieldNumber) {}

  /**
   * Deletes a field from the specified field number.
   *
   * @param {number} fieldNumber
   */
  delete(fieldNumber) {}

  /**
   * Executes the provided function once for each field.
   *
   * @param {function(!FieldType, number): void} callback
   */
  forEach(callback) {}

  /**
   * Creates a shallow copy of the storage.
   *
   * @return {!Storage}
   */
  shallowCopy() {}
}

/**
 * 85% of the proto fields have a field number <= 24:
 * https://plx.corp.google.com/scripts2/script_5d._f02af6_0000_23b1_a15f_001a1139dd02
 *
 * @type {number}
 */
// LINT.IfChange
Storage.DEFAULT_PIVOT = 24;
// LINT.ThenChange(//depot/google3/third_party/protobuf/javascript/runtime/kernel/binary_storage_test.js,
// //depot/google3/net/proto2/contrib/js_proto/internal/kernel_message_generator.cc)

exports = Storage;
