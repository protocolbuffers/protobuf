/**
 * @fileoverview Proto internal runtime checks.
 *
 * Checks are grouped into different severity, see:
 * http://g3doc/javascript/protobuf/README.md#configurable-check-support-in-protocol-buffers
 *
 * Checks are also grouped into different sections:
 *   - CHECK_BOUNDS:
 *       Checks that ensure that indexed access is within bounds
 *       (e.g. an array being accessed past its size).
 *   - CHECK_STATE
 *       Checks related to the state of an object
 *       (e.g. a parser hitting an invalid case).
 *   - CHECK_TYPE:
 *       Checks that relate to type errors (e.g. code receives a number instead
 *       of a string).
 */
goog.module('protobuf.internal.checks');

const ByteString = goog.require('protobuf.ByteString');
const Int64 = goog.require('protobuf.Int64');
const WireType = goog.require('protobuf.binary.WireType');

//
// See
// http://g3doc/javascript/protobuf/README.md#configurable-check-support-in-protocol-buffers
//
/** @define{string} */
const CHECK_LEVEL_DEFINE = goog.define('protobuf.defines.CHECK_LEVEL', '');

/** @define{boolean} */
const POLYFILL_TEXT_ENCODING =
    goog.define('protobuf.defines.POLYFILL_TEXT_ENCODING', true);

/**
 * @const {number}
 */
const MAX_FIELD_NUMBER = Math.pow(2, 29) - 1;

/**
 * The largest finite float32 value.
 * @const {number}
 */
const FLOAT32_MAX = 3.4028234663852886e+38;

/** @enum {number} */
const CheckLevel = {
  DEBUG: 0,
  CRITICAL: 1,
  OFF: 2
};


/** @return {!CheckLevel} */
function calculateCheckLevel() {
  const definedLevel = CHECK_LEVEL_DEFINE.toUpperCase();
  if (definedLevel === '') {
    // user did not set a value, value now just depends on goog.DEBUG
    return goog.DEBUG ? CheckLevel.DEBUG : CheckLevel.CRITICAL;
  }

  if (definedLevel === 'CRITICAL') {
    return CheckLevel.CRITICAL;
  }

  if (definedLevel === 'OFF') {
    return CheckLevel.OFF;
  }

  if (definedLevel === 'DEBUG') {
    return CheckLevel.DEBUG;
  }

  throw new Error(`Unknown value for CHECK_LEVEL: ${CHECK_LEVEL_DEFINE}`);
}

const /** !CheckLevel */ CHECK_LEVEL = calculateCheckLevel();

const /** boolean */ CHECK_STATE = CHECK_LEVEL === CheckLevel.DEBUG;

const /** boolean */ CHECK_CRITICAL_STATE =
    CHECK_LEVEL === CheckLevel.CRITICAL || CHECK_LEVEL === CheckLevel.DEBUG;

const /** boolean */ CHECK_BOUNDS = CHECK_LEVEL === CheckLevel.DEBUG;

const /** boolean */ CHECK_CRITICAL_BOUNDS =
    CHECK_LEVEL === CheckLevel.CRITICAL || CHECK_LEVEL === CheckLevel.DEBUG;

const /** boolean */ CHECK_TYPE = CHECK_LEVEL === CheckLevel.DEBUG;

const /** boolean */ CHECK_CRITICAL_TYPE =
    CHECK_LEVEL === CheckLevel.CRITICAL || CHECK_LEVEL === CheckLevel.DEBUG;

/**
 * Ensures the truth of an expression involving the state of the calling
 * instance, but not involving any parameters to the calling method.
 *
 * For cases where failing fast is pretty important and not failing early could
 * cause bugs that are much harder to debug.
 * @param {boolean} state
 * @param {string=} message
 * @throws {!Error} If the state is false and the check state is critical.
 */
function checkCriticalState(state, message = '') {
  if (!CHECK_CRITICAL_STATE) {
    return;
  }
  if (!state) {
    throw new Error(message);
  }
}

/**
 * Ensures the truth of an expression involving the state of the calling
 * instance, but not involving any parameters to the calling method.
 *
 * @param {boolean} state
 * @param {string=} message
 * @throws {!Error} If the state is false and the check state is debug.
 */
function checkState(state, message = '') {
  if (!CHECK_STATE) {
    return;
  }
  checkCriticalState(state, message);
}

/**
 * Ensures that `index` specifies a valid position in an indexable object of
 * size `size`. A position index may range from zero to size, inclusive.
 * @param {number} index
 * @param {number} size
 * @throws {!Error} If the index is out of range and the check state is debug.
 */
function checkPositionIndex(index, size) {
  if (!CHECK_BOUNDS) {
    return;
  }
  checkCriticalPositionIndex(index, size);
}

/**
 * Ensures that `index` specifies a valid position in an indexable object of
 * size `size`. A position index may range from zero to size, inclusive.
 * @param {number} index
 * @param {number} size
 * @throws {!Error} If the index is out of range and the check state is
 * critical.
 */
function checkCriticalPositionIndex(index, size) {
  if (!CHECK_CRITICAL_BOUNDS) {
    return;
  }
  if (index < 0 || index > size) {
    throw new Error(`Index out of bounds: index: ${index} size: ${size}`);
  }
}

/**
 * Ensures that `index` specifies a valid element in an indexable object of
 * size `size`. A element index may range from zero to size, exclusive.
 * @param {number} index
 * @param {number} size
 * @throws {!Error} If the index is out of range and the check state is
 * debug.
 */
function checkElementIndex(index, size) {
  if (!CHECK_BOUNDS) {
    return;
  }
  checkCriticalElementIndex(index, size);
}

/**
 * Ensures that `index` specifies a valid element in an indexable object of
 * size `size`. A element index may range from zero to size, exclusive.
 * @param {number} index
 * @param {number} size
 * @throws {!Error} If the index is out of range and the check state is
 * critical.
 */
function checkCriticalElementIndex(index, size) {
  if (!CHECK_CRITICAL_BOUNDS) {
    return;
  }
  if (index < 0 || index >= size) {
    throw new Error(`Index out of bounds: index: ${index} size: ${size}`);
  }
}

/**
 * Ensures the range of [start, end) is with the range of [0, size).
 * @param {number} start
 * @param {number} end
 * @param {number} size
 * @throws {!Error} If start and end are out of range and the check state is
 * debug.
 */
function checkRange(start, end, size) {
  if (!CHECK_BOUNDS) {
    return;
  }
  checkCriticalRange(start, end, size);
}

/**
 * Ensures the range of [start, end) is with the range of [0, size).
 * @param {number} start
 * @param {number} end
 * @param {number} size
 * @throws {!Error} If start and end are out of range and the check state is
 * critical.
 */
function checkCriticalRange(start, end, size) {
  if (!CHECK_CRITICAL_BOUNDS) {
    return;
  }
  if (start < 0 || end < 0 || start > size || end > size) {
    throw new Error(`Range error: start: ${start} end: ${end} size: ${size}`);
  }
  if (start > end) {
    throw new Error(`Start > end: ${start} > ${end}`);
  }
}

/**
 * Ensures that field number is an integer and within the range of
 * [1, MAX_FIELD_NUMBER].
 * @param {number} fieldNumber
 * @throws {!Error} If the field number is out of range and the check state is
 * debug.
 */
function checkFieldNumber(fieldNumber) {
  if (!CHECK_TYPE) {
    return;
  }
  checkCriticalFieldNumber(fieldNumber);
}

/**
 * Ensures that the value is neither null nor undefined.
 *
 * @param {T} value
 * @return {R}
 *
 * @template T
 * @template R :=
 *     mapunion(T, (V) =>
 *         cond(eq(V, 'null'),
 *             none(),
 *             cond(eq(V, 'undefined'),
 *                 none(),
 *                 V)))
 *  =:
 */
function checkDefAndNotNull(value) {
  if (CHECK_TYPE) {
    // Note that undefined == null.
    if (value == null) {
      throw new Error(`Value can't be null`);
    }
  }
  return value;
}

/**
 * Ensures that the value exists and is a function.
 *
 * @param {function(?): ?} func
 */
function checkFunctionExists(func) {
  if (CHECK_TYPE) {
    if (typeof func !== 'function') {
      throw new Error(`${func} is not a function`);
    }
  }
}

/**
 * Ensures that field number is an integer and within the range of
 * [1, MAX_FIELD_NUMBER].
 * @param {number} fieldNumber
 * @throws {!Error} If the field number is out of range and the check state is
 * critical.
 */
function checkCriticalFieldNumber(fieldNumber) {
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  if (fieldNumber <= 0 || fieldNumber > MAX_FIELD_NUMBER) {
    throw new Error(`Field number is out of range: ${fieldNumber}`);
  }
}

/**
 * Ensures that wire type is valid.
 * @param {!WireType} wireType
 * @throws {!Error} If the wire type is invalid and the check state is debug.
 */
function checkWireType(wireType) {
  if (!CHECK_TYPE) {
    return;
  }
  checkCriticalWireType(wireType);
}

/**
 * Ensures that wire type is valid.
 * @param {!WireType} wireType
 * @throws {!Error} If the wire type is invalid and the check state is critical.
 */
function checkCriticalWireType(wireType) {
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  if (wireType < WireType.VARINT || wireType > WireType.FIXED32) {
    throw new Error(`Invalid wire type: ${wireType}`);
  }
}

/**
 * Ensures the given value has the correct type.
 * @param {boolean} expression
 * @param {string} errorMsg
 * @throws {!Error} If the value has the wrong type and the check state is
 * critical.
 */
function checkCriticalType(expression, errorMsg) {
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  if (!expression) {
    throw new Error(errorMsg);
  }
}

/**
 * Checks whether a given object is an array.
 * @param {*} value
 * @return {!Array<*>}
 */
function checkCriticalTypeArray(value) {
  checkCriticalType(
      Array.isArray(value), `Must be an array, but got: ${value}`);
  return /** @type {!Array<*>} */ (value);
}

/**
 * Checks whether a given object is an iterable.
 * @param {*} value
 * @return {!Iterable<*>}
 */
function checkCriticalTypeIterable(value) {
  checkCriticalType(
      !!value[Symbol.iterator], `Must be an iterable, but got: ${value}`);
  return /** @type {!Iterable<*>} */ (value);
}

/**
 * Checks whether a given object is a boolean.
 * @param {*} value
 */
function checkCriticalTypeBool(value) {
  checkCriticalType(
      typeof value === 'boolean', `Must be a boolean, but got: ${value}`);
}

/**
 * Checks whether a given object is an array of boolean.
 * @param {*} values
 */
function checkCriticalTypeBoolArray(values) {
  // TODO(b/134765672)
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  const array = checkCriticalTypeArray(values);
  for (const value of array) {
    checkCriticalTypeBool(value);
  }
}

/**
 * Checks whether a given object is a ByteString.
 * @param {*} value
 */
function checkCriticalTypeByteString(value) {
  checkCriticalType(
      value instanceof ByteString, `Must be a ByteString, but got: ${value}`);
}

/**
 * Checks whether a given object is an array of ByteString.
 * @param {*} values
 */
function checkCriticalTypeByteStringArray(values) {
  // TODO(b/134765672)
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  const array = checkCriticalTypeArray(values);
  for (const value of array) {
    checkCriticalTypeByteString(value);
  }
}

/**
 * Checks whether a given object is a number.
 * @param {*} value
 * @throws {!Error} If the value is not float and the check state is debug.
 */
function checkTypeDouble(value) {
  if (!CHECK_TYPE) {
    return;
  }
  checkCriticalTypeDouble(value);
}

/**
 * Checks whether a given object is a number.
 * @param {*} value
 * @throws {!Error} If the value is not float and the check state is critical.
 */
function checkCriticalTypeDouble(value) {
  checkCriticalType(
      typeof value === 'number', `Must be a number, but got: ${value}`);
}

/**
 * Checks whether a given object is an array of double.
 * @param {*} values
 */
function checkCriticalTypeDoubleArray(values) {
  // TODO(b/134765672)
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  const array = checkCriticalTypeArray(values);
  for (const value of array) {
    checkCriticalTypeDouble(value);
  }
}

/**
 * Checks whether a given object is a number.
 * @param {*} value
 * @throws {!Error} If the value is not signed int32 and the check state is
 *     debug.
 */
function checkTypeSignedInt32(value) {
  if (!CHECK_TYPE) {
    return;
  }
  checkCriticalTypeSignedInt32(value);
}

/**
 * Checks whether a given object is a number.
 * @param {*} value
 * @throws {!Error} If the value is not signed int32 and the check state is
 *     critical.
 */
function checkCriticalTypeSignedInt32(value) {
  checkCriticalTypeDouble(value);
  const valueAsNumber = /** @type {number} */ (value);
  if (CHECK_CRITICAL_TYPE) {
    if (valueAsNumber < -Math.pow(2, 31) || valueAsNumber > Math.pow(2, 31) ||
        !Number.isInteger(valueAsNumber)) {
      throw new Error(`Must be int32, but got: ${valueAsNumber}`);
    }
  }
}

/**
 * Checks whether a given object is an array of numbers.
 * @param {*} values
 */
function checkCriticalTypeSignedInt32Array(values) {
  // TODO(b/134765672)
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  const array = checkCriticalTypeArray(values);
  for (const value of array) {
    checkCriticalTypeSignedInt32(value);
  }
}

/**
 * Ensures that value is a long instance.
 * @param {*} value
 * @throws {!Error} If the value is not a long instance and check state is
 *     debug.
 */
function checkTypeSignedInt64(value) {
  if (!CHECK_TYPE) {
    return;
  }
  checkCriticalTypeSignedInt64(value);
}

/**
 * Ensures that value is a long instance.
 * @param {*} value
 * @throws {!Error} If the value is not a long instance and check state is
 *     critical.
 */
function checkCriticalTypeSignedInt64(value) {
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  if (!(value instanceof Int64)) {
    throw new Error(`Must be Int64 instance, but got: ${value}`);
  }
}

/**
 * Checks whether a given object is an array of long instances.
 * @param {*} values
 * @throws {!Error} If values is not an array of long instances.
 */
function checkCriticalTypeSignedInt64Array(values) {
  // TODO(b/134765672)
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  const array = checkCriticalTypeArray(values);
  for (const value of array) {
    checkCriticalTypeSignedInt64(value);
  }
}

/**
 * Checks whether a given object is a number and within float32 precision.
 * @param {*} value
 * @throws {!Error} If the value is not float and the check state is debug.
 */
function checkTypeFloat(value) {
  if (!CHECK_TYPE) {
    return;
  }
  checkCriticalTypeFloat(value);
}

/**
 * Checks whether a given object is a number and within float32 precision.
 * @param {*} value
 * @throws {!Error} If the value is not float and the check state is critical.
 */
function checkCriticalTypeFloat(value) {
  checkCriticalTypeDouble(value);
  if (CHECK_CRITICAL_TYPE) {
    const valueAsNumber = /** @type {number} */ (value);
    if (Number.isFinite(valueAsNumber) &&
        (valueAsNumber > FLOAT32_MAX || valueAsNumber < -FLOAT32_MAX)) {
      throw new Error(
          `Given number does not fit into float precision: ${value}`);
    }
  }
}

/**
 * Checks whether a given object is an iterable of floats.
 * @param {*} values
 */
function checkCriticalTypeFloatIterable(values) {
  // TODO(b/134765672)
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  const iterable = checkCriticalTypeIterable(values);
  for (const value of iterable) {
    checkCriticalTypeFloat(value);
  }
}

/**
 * Checks whether a given object is a string.
 * @param {*} value
 */
function checkCriticalTypeString(value) {
  checkCriticalType(
      typeof value === 'string', `Must be string, but got: ${value}`);
}

/**
 * Checks whether a given object is an array of string.
 * @param {*} values
 */
function checkCriticalTypeStringArray(values) {
  // TODO(b/134765672)
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  const array = checkCriticalTypeArray(values);
  for (const value of array) {
    checkCriticalTypeString(value);
  }
}

/**
 * Ensures that value is a valid unsigned int32.
 * @param {*} value
 * @throws {!Error} If the value is out of range and the check state is debug.
 */
function checkTypeUnsignedInt32(value) {
  if (!CHECK_TYPE) {
    return;
  }
  checkCriticalTypeUnsignedInt32(value);
}

/**
 * Ensures that value is a valid unsigned int32.
 * @param {*} value
 * @throws {!Error} If the value is out of range and the check state
 * is critical.
 */
function checkCriticalTypeUnsignedInt32(value) {
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  checkCriticalTypeDouble(value);
  const valueAsNumber = /** @type {number} */ (value);
  if (valueAsNumber < 0 || valueAsNumber > Math.pow(2, 32) - 1 ||
      !Number.isInteger(valueAsNumber)) {
    throw new Error(`Must be uint32, but got: ${value}`);
  }
}

/**
 * Checks whether a given object is an array of unsigned int32.
 * @param {*} values
 */
function checkCriticalTypeUnsignedInt32Array(values) {
  // TODO(b/134765672)
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  const array = checkCriticalTypeArray(values);
  for (const value of array) {
    checkCriticalTypeUnsignedInt32(value);
  }
}

/**
 * Checks whether a given object is an array of message.
 * @param {*} values
 */
function checkCriticalTypeMessageArray(values) {
  // TODO(b/134765672)
  if (!CHECK_CRITICAL_TYPE) {
    return;
  }
  const array = checkCriticalTypeArray(values);
  for (const value of array) {
    checkCriticalType(
        value !== null, 'Given value is not a message instance: null');
  }
}

exports = {
  checkDefAndNotNull,
  checkCriticalElementIndex,
  checkCriticalFieldNumber,
  checkCriticalPositionIndex,
  checkCriticalRange,
  checkCriticalState,
  checkCriticalTypeBool,
  checkCriticalTypeBoolArray,
  checkCriticalTypeByteString,
  checkCriticalTypeByteStringArray,
  checkCriticalTypeDouble,
  checkTypeDouble,
  checkCriticalTypeDoubleArray,
  checkTypeFloat,
  checkCriticalTypeFloat,
  checkCriticalTypeFloatIterable,
  checkCriticalTypeMessageArray,
  checkCriticalTypeSignedInt32,
  checkCriticalTypeSignedInt32Array,
  checkCriticalTypeSignedInt64,
  checkTypeSignedInt64,
  checkCriticalTypeSignedInt64Array,
  checkCriticalTypeString,
  checkCriticalTypeStringArray,
  checkCriticalTypeUnsignedInt32,
  checkCriticalTypeUnsignedInt32Array,
  checkCriticalType,
  checkCriticalWireType,
  checkElementIndex,
  checkFieldNumber,
  checkFunctionExists,
  checkPositionIndex,
  checkRange,
  checkState,
  checkTypeUnsignedInt32,
  checkTypeSignedInt32,
  checkWireType,
  CHECK_BOUNDS,
  CHECK_CRITICAL_BOUNDS,
  CHECK_STATE,
  CHECK_CRITICAL_STATE,
  CHECK_TYPE,
  CHECK_CRITICAL_TYPE,
  MAX_FIELD_NUMBER,
  POLYFILL_TEXT_ENCODING,
};
