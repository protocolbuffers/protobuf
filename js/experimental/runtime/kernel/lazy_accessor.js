/**
 * @fileoverview LazyAccessor is a class to provide type-checked accessing
 * (read/write bool/int32/string/...) on binary data.
 *
 * When creating the LazyAccessor with the binary data, there is no deep
 * decoding done (which requires full type information). The deep decoding is
 * deferred until the first time accessing (when accessors can provide
 * full type information).
 *
 * Because accessors can be statically analyzed and stripped, unlike eager
 * binary decoding (which requires the full type information of all defined
 * fields), LazyAccessor will only need the full type information of used
 * fields.
 */
goog.module('protobuf.binary.LazyAccessor');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const ByteString = goog.require('protobuf.ByteString');
const Int64 = goog.require('protobuf.Int64');
const InternalMessage = goog.require('protobuf.binary.InternalMessage');
const Storage = goog.require('protobuf.binary.Storage');
const WireType = goog.require('protobuf.binary.WireType');
const Writer = goog.require('protobuf.binary.Writer');
const reader = goog.require('protobuf.binary.reader');
const {CHECK_TYPE, checkCriticalElementIndex, checkCriticalState, checkCriticalType, checkCriticalTypeBool, checkCriticalTypeBoolArray, checkCriticalTypeByteString, checkCriticalTypeByteStringArray, checkCriticalTypeDouble, checkCriticalTypeDoubleArray, checkCriticalTypeFloat, checkCriticalTypeFloatIterable, checkCriticalTypeMessageArray, checkCriticalTypeSignedInt32, checkCriticalTypeSignedInt32Array, checkCriticalTypeSignedInt64, checkCriticalTypeSignedInt64Array, checkCriticalTypeString, checkCriticalTypeStringArray, checkCriticalTypeUnsignedInt32, checkCriticalTypeUnsignedInt32Array, checkDefAndNotNull, checkElementIndex, checkFieldNumber, checkFunctionExists, checkState, checkTypeDouble, checkTypeFloat, checkTypeSignedInt32, checkTypeSignedInt64, checkTypeUnsignedInt32} = goog.require('protobuf.internal.checks');
const {Field, IndexEntry} = goog.require('protobuf.binary.field');
const {buildIndex} = goog.require('protobuf.binary.indexer');


/**
 * Validates the index entry has the correct wire type.
 * @param {!IndexEntry} indexEntry
 * @param {!WireType} expected
 */
function validateWireType(indexEntry, expected) {
  const wireType = Field.getWireType(indexEntry);
  checkCriticalState(
      wireType === expected,
      `Expected wire type: ${expected} but found: ${wireType}`);
}

/**
 * Checks if the object implements InternalMessage interface.
 * @param {?} obj
 * @return {!InternalMessage}
 */
function checkIsInternalMessage(obj) {
  const message = /** @type {!InternalMessage} */ (obj);
  checkFunctionExists(message.internalGetKernel);
  return message;
}

/**
 * Checks if the instanceCreator returns an instance that implements the
 * InternalMessage interface.
 * @param {function(!LazyAccessor):T} instanceCreator
 * @template T
 */
function checkInstanceCreator(instanceCreator) {
  if (CHECK_TYPE) {
    const emptyMessage = instanceCreator(LazyAccessor.createEmpty());
    checkFunctionExists(emptyMessage.internalGetKernel);
  }
}

/**
 * Reads the last entry of the index array using the given read function.
 * This is used to implement parsing singular primitive fields.
 * @param {!Array<!IndexEntry>} indexArray
 * @param {!BufferDecoder} bufferDecoder
 * @param {function(!BufferDecoder, number):T} readFunc
 * @param {!WireType} wireType
 * @return {T}
 * @template T
 */
function readOptional(indexArray, bufferDecoder, readFunc, wireType) {
  const index = indexArray.length - 1;
  checkElementIndex(index, indexArray.length);
  const indexEntry = indexArray[index];
  validateWireType(indexEntry, wireType);
  return readFunc(bufferDecoder, Field.getStartIndex(indexEntry));
}

/**
 * Converts all entries of the index array to the template type using given read
 * methods and return an Iterable containing those converted values.
 * Primitive repeated fields may be encoded either packed or unpacked. Thus, two
 * read methods are needed for those two cases.
 * This is used to implement parsing repeated primitive fields.
 * @param {!Array<!IndexEntry>} indexArray
 * @param {!BufferDecoder} bufferDecoder
 * @param {function(!BufferDecoder, number):T} singularReadFunc
 * @param {function(!BufferDecoder, number):!Array<T>} packedReadFunc
 * @param {!WireType} expectedWireType
 * @return {!Array<T>}
 * @template T
 */
function readRepeatedPrimitive(
    indexArray, bufferDecoder, singularReadFunc, packedReadFunc,
    expectedWireType) {
  // Fast path when there is a single packed entry.
  if (indexArray.length === 1 &&
      Field.getWireType(indexArray[0]) === WireType.DELIMITED) {
    return packedReadFunc(bufferDecoder, Field.getStartIndex(indexArray[0]));
  }

  let /** !Array<T> */ result = [];
  for (const indexEntry of indexArray) {
    const wireType = Field.getWireType(indexEntry);
    const startIndex = Field.getStartIndex(indexEntry);
    if (wireType === WireType.DELIMITED) {
      result = result.concat(packedReadFunc(bufferDecoder, startIndex));
    } else {
      validateWireType(indexEntry, expectedWireType);
      result.push(singularReadFunc(bufferDecoder, startIndex));
    }
  }
  return result;
}

/**
 * Converts all entries of the index array to the template type using the given
 * read function and return an Array containing those converted values. This is
 * used to implement parsing repeated non-primitive fields.
 * @param {!Array<!IndexEntry>} indexArray
 * @param {!BufferDecoder} bufferDecoder
 * @param {function(!BufferDecoder, number):T} singularReadFunc
 * @return {!Array<T>}
 * @template T
 */
function readRepeatedNonPrimitive(indexArray, bufferDecoder, singularReadFunc) {
  const result = new Array(indexArray.length);
  for (let i = 0; i < indexArray.length; i++) {
    validateWireType(indexArray[i], WireType.DELIMITED);
    result[i] =
        singularReadFunc(bufferDecoder, Field.getStartIndex(indexArray[i]));
  }
  return result;
}

/**
 * Creates a new bytes array to contain all data of a submessage.
 * When there are multiple entries, merge them together.
 * @param {!Array<!IndexEntry>} indexArray
 * @param {!BufferDecoder} bufferDecoder
 * @return {!BufferDecoder}
 */
function mergeMessageArrays(indexArray, bufferDecoder) {
  const dataArrays = indexArray.map(
      indexEntry =>
          reader.readDelimited(bufferDecoder, Field.getStartIndex(indexEntry)));
  return BufferDecoder.merge(dataArrays);
}

/**
 * @param {!Array<!IndexEntry>} indexArray
 * @param {!BufferDecoder} bufferDecoder
 * @param {number=} pivot
 * @return {!LazyAccessor}
 */
function readAccessor(indexArray, bufferDecoder, pivot = undefined) {
  checkState(indexArray.length > 0);
  let accessorBuffer;
  // Faster access for one member.
  if (indexArray.length === 1) {
    const indexEntry = indexArray[0];
    validateWireType(indexEntry, WireType.DELIMITED);
    accessorBuffer =
        reader.readDelimited(bufferDecoder, Field.getStartIndex(indexEntry));
  } else {
    indexArray.forEach(indexEntry => {
      validateWireType(indexEntry, WireType.DELIMITED);
    });
    accessorBuffer = mergeMessageArrays(indexArray, bufferDecoder);
  }
  return LazyAccessor.fromBufferDecoder_(accessorBuffer, pivot);
}

/**
 * Merges all index entries of the index array using the given read function.
 * This is used to implement parsing singular message fields.
 * @param {!Array<!IndexEntry>} indexArray
 * @param {!BufferDecoder} bufferDecoder
 * @param {function(!LazyAccessor):T} instanceCreator
 * @param {number=} pivot
 * @return {T}
 * @template T
 */
function readMessage(indexArray, bufferDecoder, instanceCreator, pivot) {
  checkInstanceCreator(instanceCreator);
  const accessor = readAccessor(indexArray, bufferDecoder, pivot);
  return instanceCreator(accessor);
}

/**
 * @param {!Writer} writer
 * @param {number} fieldNumber
 * @param {?InternalMessage} value
 */
function writeMessage(writer, fieldNumber, value) {
  writer.writeDelimited(
      fieldNumber, checkDefAndNotNull(value).internalGetKernel().serialize());
}

/**
 * Writes the array of Messages into the writer for the given field number.
 * @param {!Writer} writer
 * @param {number} fieldNumber
 * @param {!Iterable<!InternalMessage>} values
 */
function writeRepeatedMessage(writer, fieldNumber, values) {
  for (const value of values) {
    writeMessage(writer, fieldNumber, value);
  }
}

/**
 * Array.from has a weird type definition in google3/javascript/externs/es6.js
 * and wants the mapping function accept strings.
 * @const {function((string|number)): number}
 */
const fround = /** @type {function((string|number)): number} */ (Math.fround);

/**
 * Wraps an array and exposes it as an Iterable. This class is used to provide
 * immutable access of the array to the caller.
 * @implements {Iterable<T>}
 * @template T
 */
class ArrayIterable {
  /**
   * @param {!Array<T>} array
   */
  constructor(array) {
    /** @private @const {!Array<T>} */
    this.array_ = array;
  }

  /** @return {!Iterator<T>} */
  [Symbol.iterator]() {
    return this.array_[Symbol.iterator]();
  }
}

/**
 * Accesses protobuf fields on binary format data. Binary data is decoded lazily
 * at the first access.
 * @final
 */
class LazyAccessor {
  /**
   * Create a LazyAccessor for the given binary bytes.
   * The bytes array is kept by the LazyAccessor. DON'T MODIFY IT.
   * @param {!ArrayBuffer} arrayBuffer Binary bytes.
   * @param {number=} pivot Fields with a field number no greater than the pivot
   *     value will be stored in an array for fast access. Other fields will be
   *     stored in a map. A higher pivot value can improve runtime performance
   *     at the expense of requiring more memory. It's recommended to set the
   *     value to the max field number of the message unless the field numbers
   *     are too sparse. If the value is not set, a default value specified in
   *     storage.js will be used.
   * @return {!LazyAccessor}
   */
  static fromArrayBuffer(arrayBuffer, pivot = undefined) {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(arrayBuffer);
    return LazyAccessor.fromBufferDecoder_(bufferDecoder, pivot);
  }

  /**
   * Creates an empty LazyAccessor.
   * @param {number=} pivot Fields with a field number no greater than the pivot
   *     value will be stored in an array for fast access. Other fields will be
   *     stored in a map. A higher pivot value can improve runtime performance
   *     at the expense of requiring more memory. It's recommended to set the
   *     value to the max field number of the message unless the field numbers
   *     are too sparse. If the value is not set, a default value specified in
   *     storage.js will be used.
   * @return {!LazyAccessor}
   */
  static createEmpty(pivot = undefined) {
    return new LazyAccessor(/* bufferDecoder= */ null, new Storage(pivot));
  }

  /**
   * Create a LazyAccessor for the given binary bytes.
   * The bytes array is kept by the LazyAccessor. DON'T MODIFY IT.
   * @param {!BufferDecoder} bufferDecoder Binary bytes.
   * @param {number|undefined} pivot
   * @return {!LazyAccessor}
   * @private
   */
  static fromBufferDecoder_(bufferDecoder, pivot) {
    return new LazyAccessor(bufferDecoder, buildIndex(bufferDecoder, pivot));
  }

  /**
   * @param {?BufferDecoder} bufferDecoder Binary bytes. Accessor treats the
   *     bytes as immutable and will never attempt to write to it.
   * @param {!Storage<!Field>} fields A map of field number to Field. The
   *     IndexEntry in each Field needs to be populated with the location of the
   *     field in the binary data.
   * @private
   */
  constructor(bufferDecoder, fields) {
    /** @private @const {?BufferDecoder} */
    this.bufferDecoder_ = bufferDecoder;
    /** @private @const {!Storage<!Field>} */
    this.fields_ = fields;
  }

  /**
   * Creates a shallow copy of the accessor.
   * @return {!LazyAccessor}
   */
  shallowCopy() {
    return new LazyAccessor(this.bufferDecoder_, this.fields_.shallowCopy());
  }

  /**
   * See definition of the pivot parameter on the fromArrayBuffer() method.
   * @return {number}
   */
  getPivot() {
    return this.fields_.getPivot();
  }

  /**
   * Clears the field for the given field number.
   * @param {number} fieldNumber
   */
  clearField(fieldNumber) {
    this.fields_.delete(fieldNumber);
  }

  /**
   * Returns data for a field specified by the given field number. Also cache
   * the data if it doesn't already exist in the cache. When no data is
   * available, return the given default value.
   * @param {number} fieldNumber
   * @param {?T} defaultValue
   * @param {function(!Array<!IndexEntry>, !BufferDecoder):T} readFunc
   * @param {function(!Writer, number, T)=} encoder
   * @return {T}
   * @template T
   * @private
   */
  getFieldWithDefault_(
      fieldNumber, defaultValue, readFunc, encoder = undefined) {
    checkFieldNumber(fieldNumber);

    const field = this.fields_.get(fieldNumber);
    if (field === undefined) {
      return defaultValue;
    }

    if (field.hasDecodedValue()) {
      checkState(!encoder || !!field.getEncoder());
      return field.getDecodedValue();
    }

    const parsed = readFunc(
        checkDefAndNotNull(field.getIndexArray()),
        checkDefAndNotNull(this.bufferDecoder_));
    field.setCache(parsed, encoder);
    return parsed;
  }

  /**
   * Sets data for a singular field specified by the given field number.
   * @param {number} fieldNumber
   * @param {T} value
   * @param {function(!Writer, number, T)} encoder
   * @return {T}
   * @template T
   * @private
   */
  setField_(fieldNumber, value, encoder) {
    checkFieldNumber(fieldNumber);
    this.fields_.set(fieldNumber, Field.fromDecodedValue(value, encoder));
  }

  /**
   * Serializes internal contents to binary format bytes array to the
   * given writer.
   * @param {!Writer} writer
   * @package
   */
  serializeToWriter(writer) {
    // If we use for...of here, jscompiler returns an array of both types for
    // fieldNumber and field without specifying which type is for
    // field, which prevents us to use fieldNumber. Thus, we use
    // forEach here.
    this.fields_.forEach((field, fieldNumber) => {
      // If encoder doesn't exist, there is no need to encode the value
      // because the data in the index is still valid.
      if (field.getEncoder() !== undefined) {
        const encoder = checkDefAndNotNull(field.getEncoder());
        encoder(writer, fieldNumber, field.getDecodedValue());
        return;
      }

      const indexArr = field.getIndexArray();
      if (indexArr) {
        for (const indexEntry of indexArr) {
          writer.writeTag(fieldNumber, Field.getWireType(indexEntry));
          writer.writeBufferDecoder(
              checkDefAndNotNull(this.bufferDecoder_),
              Field.getStartIndex(indexEntry), Field.getWireType(indexEntry));
        }
      }
    });
  }

  /**
   * Serializes internal contents to binary format bytes array.
   * @return {!ArrayBuffer}
   */
  serialize() {
    const writer = new Writer();
    this.serializeToWriter(writer);
    return writer.getAndResetResultBuffer();
  }

  /**
   * Returns whether data exists at the given field number.
   * @param {number} fieldNumber
   * @return {boolean}
   */
  hasFieldNumber(fieldNumber) {
    checkFieldNumber(fieldNumber);
    const field = this.fields_.get(fieldNumber);

    if (field === undefined) {
      return false;
    }

    if (field.getIndexArray() !== null) {
      return true;
    }

    if (Array.isArray(field.getDecodedValue())) {
      // For repeated fields, existence is decided by number of elements.
      return (/** !Array<?> */ (field.getDecodedValue())).length > 0;
    }
    return true;
  }

  /***************************************************************************
   *                        OPTIONAL GETTER METHODS
   ***************************************************************************/

  /**
   * Returns data as boolean for the given field number.
   * If no default is given, use false as the default.
   * @param {number} fieldNumber
   * @param {boolean=} defaultValue
   * @return {boolean}
   */
  getBoolWithDefault(fieldNumber, defaultValue = false) {
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) =>
            readOptional(indexArray, bytes, reader.readBool, WireType.VARINT));
  }

  /**
   * Returns data as a ByteString for the given field number.
   * If no default is given, use false as the default.
   * @param {number} fieldNumber
   * @param {!ByteString=} defaultValue
   * @return {!ByteString}
   */
  getBytesWithDefault(fieldNumber, defaultValue = ByteString.EMPTY) {
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) => readOptional(
            indexArray, bytes, reader.readBytes, WireType.DELIMITED));
  }

  /**
   * Returns a double for the given field number.
   * If no default is given uses zero as the default.
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getDoubleWithDefault(fieldNumber, defaultValue = 0) {
    checkTypeDouble(defaultValue);
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) => readOptional(
            indexArray, bytes, reader.readDouble, WireType.FIXED64));
  }

  /**
   * Returns a fixed32 for the given field number.
   * If no default is given zero as the default.
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getFixed32WithDefault(fieldNumber, defaultValue = 0) {
    checkTypeUnsignedInt32(defaultValue);
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) => readOptional(
            indexArray, bytes, reader.readFixed32, WireType.FIXED32));
  }

  /**
   * Returns a fixed64 for the given field number.
   * Note: Since g.m.Long does not support unsigned int64 values we are going
   * the Java route here for now and simply output the number as a signed int64.
   * Users can get to individual bits by themselves.
   * @param {number} fieldNumber
   * @param {!Int64=} defaultValue
   * @return {!Int64}
   */
  getFixed64WithDefault(fieldNumber, defaultValue = Int64.getZero()) {
    return this.getSfixed64WithDefault(fieldNumber, defaultValue);
  }

  /**
   * Returns a float for the given field number.
   * If no default is given zero as the default.
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getFloatWithDefault(fieldNumber, defaultValue = 0) {
    checkTypeFloat(defaultValue);
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) => readOptional(
            indexArray, bytes, reader.readFloat, WireType.FIXED32));
  }

  /**
   * Returns a int32 for the given field number.
   * If no default is given zero as the default.
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getInt32WithDefault(fieldNumber, defaultValue = 0) {
    checkTypeSignedInt32(defaultValue);
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) =>
            readOptional(indexArray, bytes, reader.readInt32, WireType.VARINT));
  }

  /**
   * Returns a int64 for the given field number.
   * If no default is given zero as the default.
   * @param {number} fieldNumber
   * @param {!Int64=} defaultValue
   * @return {!Int64}
   */
  getInt64WithDefault(fieldNumber, defaultValue = Int64.getZero()) {
    checkTypeSignedInt64(defaultValue);
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) =>
            readOptional(indexArray, bytes, reader.readInt64, WireType.VARINT));
  }

  /**
   * Returns a sfixed32 for the given field number.
   * If no default is given zero as the default.
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getSfixed32WithDefault(fieldNumber, defaultValue = 0) {
    checkTypeSignedInt32(defaultValue);
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) => readOptional(
            indexArray, bytes, reader.readSfixed32, WireType.FIXED32));
  }

  /**
   * Returns a sfixed64 for the given field number.
   * If no default is given zero as the default.
   * @param {number} fieldNumber
   * @param {!Int64=} defaultValue
   * @return {!Int64}
   */
  getSfixed64WithDefault(fieldNumber, defaultValue = Int64.getZero()) {
    checkTypeSignedInt64(defaultValue);
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) => readOptional(
            indexArray, bytes, reader.readSfixed64, WireType.FIXED64));
  }

  /**
   * Returns a sint32 for the given field number.
   * If no default is given zero as the default.
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getSint32WithDefault(fieldNumber, defaultValue = 0) {
    checkTypeSignedInt32(defaultValue);
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) => readOptional(
            indexArray, bytes, reader.readSint32, WireType.VARINT));
  }

  /**
   * Returns a sint64 for the given field number.
   * If no default is given zero as the default.
   * @param {number} fieldNumber
   * @param {!Int64=} defaultValue
   * @return {!Int64}
   */
  getSint64WithDefault(fieldNumber, defaultValue = Int64.getZero()) {
    checkTypeSignedInt64(defaultValue);
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) => readOptional(
            indexArray, bytes, reader.readSint64, WireType.VARINT));
  }

  /**
   * Returns a string for the given field number.
   * If no default is given uses empty string as the default.
   * @param {number} fieldNumber
   * @param {string=} defaultValue
   * @return {string}
   */
  getStringWithDefault(fieldNumber, defaultValue = '') {
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) => readOptional(
            indexArray, bytes, reader.readString, WireType.DELIMITED));
  }

  /**
   * Returns a uint32 for the given field number.
   * If no default is given zero as the default.
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getUint32WithDefault(fieldNumber, defaultValue = 0) {
    checkTypeUnsignedInt32(defaultValue);
    return this.getFieldWithDefault_(
        fieldNumber, defaultValue,
        (indexArray, bytes) => readOptional(
            indexArray, bytes, reader.readUint32, WireType.VARINT));
  }

  /**
   * Returns a uint64 for the given field number.
   * Note: Since g.m.Long does not support unsigned int64 values we are going
   * the Java route here for now and simply output the number as a signed int64.
   * Users can get to individual bits by themselves.
   * @param {number} fieldNumber
   * @param {!Int64=} defaultValue
   * @return {!Int64}
   */
  getUint64WithDefault(fieldNumber, defaultValue = Int64.getZero()) {
    return this.getInt64WithDefault(fieldNumber, defaultValue);
  }

  /**
   * Returns data as a mutable proto Message for the given field number.
   * If no value has been set, return null.
   * If hasFieldNumber(fieldNumber) == false before calling, it remains false.
   *
   * This method should not be used along with getMessage, since calling
   * getMessageOrNull after getMessage will not register the encoder.
   *
   * @param {number} fieldNumber
   * @param {function(!LazyAccessor):T} instanceCreator
   * @param {number=} pivot
   * @return {?T}
   * @template T
   */
  getMessageOrNull(fieldNumber, instanceCreator, pivot = undefined) {
    return this.getFieldWithDefault_(
        fieldNumber, null,
        (indexArray, bytes) =>
            readMessage(indexArray, bytes, instanceCreator, pivot),
        writeMessage);
  }

  /**
   * Returns data as a mutable proto Message for the given field number.
   * If no value has been set previously, creates and attaches an instance.
   * Postcondition: hasFieldNumber(fieldNumber) == true.
   *
   * This method should not be used along with getMessage, since calling
   * getMessageAttach after getMessage will not register the encoder.
   *
   * @param {number} fieldNumber
   * @param {function(!LazyAccessor):T} instanceCreator
   * @param {number=} pivot
   * @return {T}
   * @template T
   */
  getMessageAttach(fieldNumber, instanceCreator, pivot = undefined) {
    checkInstanceCreator(instanceCreator);
    let instance = this.getMessageOrNull(fieldNumber, instanceCreator, pivot);
    if (!instance) {
      instance = instanceCreator(LazyAccessor.createEmpty());
      this.setField_(fieldNumber, instance, writeMessage);
    }
    return instance;
  }

  /**
   * Returns data as a proto Message for the given field number.
   * If no value has been set, return a default instance.
   * This default instance is guaranteed to be the same instance, unless this
   * field is cleared.
   * Does not register the encoder, so changes made to the returned
   * sub-message will not be included when serializing the parent message.
   * Use getMessageAttach() if the resulting sub-message should be mutable.
   *
   * This method should not be used along with getMessageOrNull or
   * getMessageAttach, since these methods register the encoder.
   *
   * @param {number} fieldNumber
   * @param {function(!LazyAccessor):T} instanceCreator
   * @param {number=} pivot
   * @return {T}
   * @template T
   */
  getMessage(fieldNumber, instanceCreator, pivot = undefined) {
    checkInstanceCreator(instanceCreator);
    const message = this.getFieldWithDefault_(
        fieldNumber, null,
        (indexArray, bytes) =>
            readMessage(indexArray, bytes, instanceCreator, pivot));
    // Returns an empty message as the default value if the field doesn't exist.
    // We don't pass the default value to getFieldWithDefault_ to reduce object
    // allocation.
    return message === null ? instanceCreator(LazyAccessor.createEmpty()) :
                              message;
  }

  /**
   * Returns the accessor for the given singular message, or returns null if
   * it hasn't been set.
   * @param {number} fieldNumber
   * @param {number=} pivot
   * @return {?LazyAccessor}
   */
  getMessageAccessorOrNull(fieldNumber, pivot = undefined) {
    checkFieldNumber(fieldNumber);
    const field = this.fields_.get(fieldNumber);
    if (field === undefined) {
      return null;
    }

    if (field.hasDecodedValue()) {
      return checkIsInternalMessage(field.getDecodedValue())
          .internalGetKernel();
    } else {
      return readAccessor(
          checkDefAndNotNull(field.getIndexArray()),
          checkDefAndNotNull(this.bufferDecoder_), pivot);
    }
  }

  /***************************************************************************
   *                        REPEATED GETTER METHODS
   ***************************************************************************/

  /* Bool */

  /**
   * Returns an Array instance containing boolean values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<boolean>}
   * @private
   */
  getRepeatedBoolArray_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readBool, reader.readPackedBool,
            WireType.VARINT));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {boolean}
   */
  getRepeatedBoolElement(fieldNumber, index) {
    const array = this.getRepeatedBoolArray_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing boolean values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<boolean>}
   */
  getRepeatedBoolIterable(fieldNumber) {
    const array = this.getRepeatedBoolArray_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedBoolSize(fieldNumber) {
    return this.getRepeatedBoolArray_(fieldNumber).length;
  }

  /* Double */

  /**
   * Returns an Array instance containing double values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<number>}
   * @private
   */
  getRepeatedDoubleArray_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readDouble, reader.readPackedDouble,
            WireType.FIXED64));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedDoubleElement(fieldNumber, index) {
    const array = this.getRepeatedDoubleArray_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing double values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedDoubleIterable(fieldNumber) {
    const array = this.getRepeatedDoubleArray_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedDoubleSize(fieldNumber) {
    return this.getRepeatedDoubleArray_(fieldNumber).length;
  }

  /* Fixed32 */

  /**
   * Returns an Array instance containing fixed32 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<number>}
   * @private
   */
  getRepeatedFixed32Array_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readFixed32, reader.readPackedFixed32,
            WireType.FIXED32));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedFixed32Element(fieldNumber, index) {
    const array = this.getRepeatedFixed32Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing fixed32 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedFixed32Iterable(fieldNumber) {
    const array = this.getRepeatedFixed32Array_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedFixed32Size(fieldNumber) {
    return this.getRepeatedFixed32Array_(fieldNumber).length;
  }

  /* Fixed64 */

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!Int64}
   */
  getRepeatedFixed64Element(fieldNumber, index) {
    return this.getRepeatedSfixed64Element(fieldNumber, index);
  }

  /**
   * Returns an Iterable instance containing fixed64 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<!Int64>}
   */
  getRepeatedFixed64Iterable(fieldNumber) {
    return this.getRepeatedSfixed64Iterable(fieldNumber);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedFixed64Size(fieldNumber) {
    return this.getRepeatedSfixed64Size(fieldNumber);
  }

  /* Float */

  /**
   * Returns an Array instance containing float values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<number>}
   * @private
   */
  getRepeatedFloatArray_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readFloat, reader.readPackedFloat,
            WireType.FIXED32));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedFloatElement(fieldNumber, index) {
    const array = this.getRepeatedFloatArray_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing float values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedFloatIterable(fieldNumber) {
    const array = this.getRepeatedFloatArray_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedFloatSize(fieldNumber) {
    return this.getRepeatedFloatArray_(fieldNumber).length;
  }

  /* Int32 */

  /**
   * Returns an Array instance containing int32 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<number>}
   * @private
   */
  getRepeatedInt32Array_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readInt32, reader.readPackedInt32,
            WireType.VARINT));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedInt32Element(fieldNumber, index) {
    const array = this.getRepeatedInt32Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing int32 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedInt32Iterable(fieldNumber) {
    const array = this.getRepeatedInt32Array_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedInt32Size(fieldNumber) {
    return this.getRepeatedInt32Array_(fieldNumber).length;
  }

  /* Int64 */

  /**
   * Returns an Array instance containing int64 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<!Int64>}
   * @private
   */
  getRepeatedInt64Array_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readInt64, reader.readPackedInt64,
            WireType.VARINT));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!Int64}
   */
  getRepeatedInt64Element(fieldNumber, index) {
    const array = this.getRepeatedInt64Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing int64 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<!Int64>}
   */
  getRepeatedInt64Iterable(fieldNumber) {
    const array = this.getRepeatedInt64Array_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedInt64Size(fieldNumber) {
    return this.getRepeatedInt64Array_(fieldNumber).length;
  }

  /* Sfixed32 */

  /**
   * Returns an Array instance containing sfixed32 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<number>}
   * @private
   */
  getRepeatedSfixed32Array_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readSfixed32, reader.readPackedSfixed32,
            WireType.FIXED32));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedSfixed32Element(fieldNumber, index) {
    const array = this.getRepeatedSfixed32Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing sfixed32 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedSfixed32Iterable(fieldNumber) {
    const array = this.getRepeatedSfixed32Array_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedSfixed32Size(fieldNumber) {
    return this.getRepeatedSfixed32Array_(fieldNumber).length;
  }

  /* Sfixed64 */

  /**
   * Returns an Array instance containing sfixed64 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<!Int64>}
   * @private
   */
  getRepeatedSfixed64Array_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readSfixed64, reader.readPackedSfixed64,
            WireType.FIXED64));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!Int64}
   */
  getRepeatedSfixed64Element(fieldNumber, index) {
    const array = this.getRepeatedSfixed64Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing sfixed64 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<!Int64>}
   */
  getRepeatedSfixed64Iterable(fieldNumber) {
    const array = this.getRepeatedSfixed64Array_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedSfixed64Size(fieldNumber) {
    return this.getRepeatedSfixed64Array_(fieldNumber).length;
  }

  /* Sint32 */

  /**
   * Returns an Array instance containing sint32 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<number>}
   * @private
   */
  getRepeatedSint32Array_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readSint32, reader.readPackedSint32,
            WireType.VARINT));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedSint32Element(fieldNumber, index) {
    const array = this.getRepeatedSint32Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing sint32 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedSint32Iterable(fieldNumber) {
    const array = this.getRepeatedSint32Array_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedSint32Size(fieldNumber) {
    return this.getRepeatedSint32Array_(fieldNumber).length;
  }

  /* Sint64 */

  /**
   * Returns an Array instance containing sint64 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<!Int64>}
   * @private
   */
  getRepeatedSint64Array_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readSint64, reader.readPackedSint64,
            WireType.VARINT));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!Int64}
   */
  getRepeatedSint64Element(fieldNumber, index) {
    const array = this.getRepeatedSint64Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing sint64 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<!Int64>}
   */
  getRepeatedSint64Iterable(fieldNumber) {
    const array = this.getRepeatedSint64Array_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedSint64Size(fieldNumber) {
    return this.getRepeatedSint64Array_(fieldNumber).length;
  }

  /* Uint32 */

  /**
   * Returns an Array instance containing uint32 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<number>}
   * @private
   */
  getRepeatedUint32Array_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) => readRepeatedPrimitive(
            indexArray, bytes, reader.readUint32, reader.readPackedUint32,
            WireType.VARINT));
  }

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedUint32Element(fieldNumber, index) {
    const array = this.getRepeatedUint32Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing uint32 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedUint32Iterable(fieldNumber) {
    const array = this.getRepeatedUint32Array_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedUint32Size(fieldNumber) {
    return this.getRepeatedUint32Array_(fieldNumber).length;
  }

  /* Uint64 */

  /**
   * Returns the element at index for the given field number.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!Int64}
   */
  getRepeatedUint64Element(fieldNumber, index) {
    return this.getRepeatedInt64Element(fieldNumber, index);
  }

  /**
   * Returns an Iterable instance containing uint64 values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<!Int64>}
   */
  getRepeatedUint64Iterable(fieldNumber) {
    return this.getRepeatedInt64Iterable(fieldNumber);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedUint64Size(fieldNumber) {
    return this.getRepeatedInt64Size(fieldNumber);
  }

  /* Bytes */

  /**
   * Returns an array instance containing bytes values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<!ByteString>}
   * @private
   */
  getRepeatedBytesArray_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bytes) =>
            readRepeatedNonPrimitive(indexArray, bytes, reader.readBytes));
  }

  /**
   * Returns the element at index for the given field number as a bytes.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!ByteString}
   */
  getRepeatedBytesElement(fieldNumber, index) {
    const array = this.getRepeatedBytesArray_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing bytes values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<!ByteString>}
   */
  getRepeatedBytesIterable(fieldNumber) {
    const array = this.getRepeatedBytesArray_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedBytesSize(fieldNumber) {
    return this.getRepeatedBytesArray_(fieldNumber).length;
  }

  /* String */

  /**
   * Returns an array instance containing string values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Array<string>}
   * @private
   */
  getRepeatedStringArray_(fieldNumber) {
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[],
        (indexArray, bufferDecoder) => readRepeatedNonPrimitive(
            indexArray, bufferDecoder, reader.readString));
  }

  /**
   * Returns the element at index for the given field number as a string.
   * @param {number} fieldNumber
   * @param {number} index
   * @return {string}
   */
  getRepeatedStringElement(fieldNumber, index) {
    const array = this.getRepeatedStringArray_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing string values for the given field
   * number.
   * @param {number} fieldNumber
   * @return {!Iterable<string>}
   */
  getRepeatedStringIterable(fieldNumber) {
    const array = this.getRepeatedStringArray_(fieldNumber);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedStringSize(fieldNumber) {
    return this.getRepeatedStringArray_(fieldNumber).length;
  }

  /* Message */

  /**
   * Returns an Array instance containing boolean values for the given field
   * number.
   * @param {number} fieldNumber
   * @param {function(!LazyAccessor):T} instanceCreator
   * @param {number|undefined} pivot
   * @return {!Array<T>}
   * @template T
   * @private
   */
  getRepeatedMessageArray_(fieldNumber, instanceCreator, pivot) {
    checkInstanceCreator(instanceCreator);
    const bytesInstanceCreator = (bufferDecoder) =>
        instanceCreator(LazyAccessor.fromBufferDecoder_(bufferDecoder, pivot));
    const readMessageFunc = (bufferDecoder, start) =>
        bytesInstanceCreator(reader.readDelimited(bufferDecoder, start));

    const readRepeatedMessageFunc = (indexArray, bufferDecoder) =>
        readRepeatedNonPrimitive(indexArray, bufferDecoder, readMessageFunc);
    const encoder = (writer, fieldNumber, values) =>
        writeRepeatedMessage(writer, fieldNumber, values);
    return this.getFieldWithDefault_(
        fieldNumber, /* defaultValue= */[], readRepeatedMessageFunc, encoder);
  }

  /**
   * Returns the element at index for the given field number as a message.
   * @param {number} fieldNumber
   * @param {function(!LazyAccessor):T} instanceCreator
   * @param {number} index
   * @param {number=} pivot
   * @return {T}
   * @template T
   */
  getRepeatedMessageElement(
      fieldNumber, instanceCreator, index, pivot = undefined) {
    const array =
        this.getRepeatedMessageArray_(fieldNumber, instanceCreator, pivot);
    checkCriticalElementIndex(index, array.length);
    return array[index];
  }

  /**
   * Returns an Iterable instance containing message values for the given field
   * number.
   * @param {number} fieldNumber
   * @param {function(!LazyAccessor):T} instanceCreator
   * @param {number=} pivot
   * @return {!Iterable<T>}
   * @template T
   */
  getRepeatedMessageIterable(fieldNumber, instanceCreator, pivot = undefined) {
    const array =
        this.getRepeatedMessageArray_(fieldNumber, instanceCreator, pivot);
    return new ArrayIterable(array);
  }

  /**
   * Returns an Iterable instance containing message accessors for the given
   * field number.
   * @param {number} fieldNumber
   * @param {number=} pivot
   * @return {!Iterable<!LazyAccessor>}
   */
  getRepeatedMessageAccessorIterable(fieldNumber, pivot = undefined) {
    checkFieldNumber(fieldNumber);

    const field = this.fields_.get(fieldNumber);
    if (!field) {
      return [];
    }

    if (field.hasDecodedValue()) {
      return new ArrayIterable(field.getDecodedValue().map(
          value => checkIsInternalMessage(value).internalGetKernel()));
    }

    const readMessageFunc = (bufferDecoder, start) =>
        LazyAccessor.fromBufferDecoder_(
            reader.readDelimited(bufferDecoder, start), pivot);
    const array = readRepeatedNonPrimitive(
        checkDefAndNotNull(field.getIndexArray()),
        checkDefAndNotNull(this.bufferDecoder_), readMessageFunc);
    return new ArrayIterable(array);
  }

  /**
   * Returns the size of the repeated field.
   * @param {number} fieldNumber
   * @param {function(!LazyAccessor):T} instanceCreator
   * @return {number}
   * @param {number=} pivot
   * @template T
   */
  getRepeatedMessageSize(fieldNumber, instanceCreator, pivot = undefined) {
    return this.getRepeatedMessageArray_(fieldNumber, instanceCreator, pivot)
        .length;
  }

  /***************************************************************************
   *                        OPTIONAL SETTER METHODS
   ***************************************************************************/

  /**
   * Sets a boolean value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {boolean} value
   */
  setBool(fieldNumber, value) {
    checkCriticalTypeBool(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeBool(fieldNumber, value);
    });
  }

  /**
   * Sets a boolean value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {!ByteString} value
   */
  setBytes(fieldNumber, value) {
    checkCriticalTypeByteString(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeBytes(fieldNumber, value);
    });
  }

  /**
   * Sets a double value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {number} value
   */
  setDouble(fieldNumber, value) {
    checkCriticalTypeDouble(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeDouble(fieldNumber, value);
    });
  }

  /**
   * Sets a fixed32 value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {number} value
   */
  setFixed32(fieldNumber, value) {
    checkCriticalTypeUnsignedInt32(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeFixed32(fieldNumber, value);
    });
  }

  /**
   * Sets a uint64 value to the field with the given field number.\
   * Note: Since g.m.Long does not support unsigned int64 values we are going
   * the Java route here for now and simply output the number as a signed int64.
   * Users can get to individual bits by themselves.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  setFixed64(fieldNumber, value) {
    this.setSfixed64(fieldNumber, value);
  }

  /**
   * Sets a float value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {number} value
   */
  setFloat(fieldNumber, value) {
    checkCriticalTypeFloat(value);
    // Eagerly round to 32-bit precision so that reading back after set will
    // yield the same value a reader will receive after serialization.
    const floatValue = Math.fround(value);
    this.setField_(fieldNumber, floatValue, (writer, fieldNumber, value) => {
      writer.writeFloat(fieldNumber, value);
    });
  }

  /**
   * Sets a int32 value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {number} value
   */
  setInt32(fieldNumber, value) {
    checkCriticalTypeSignedInt32(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeInt32(fieldNumber, value);
    });
  }

  /**
   * Sets a int64 value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  setInt64(fieldNumber, value) {
    checkCriticalTypeSignedInt64(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeInt64(fieldNumber, value);
    });
  }

  /**
   * Sets a sfixed32 value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {number} value
   */
  setSfixed32(fieldNumber, value) {
    checkCriticalTypeSignedInt32(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeSfixed32(fieldNumber, value);
    });
  }

  /**
   * Sets a sfixed64 value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  setSfixed64(fieldNumber, value) {
    checkCriticalTypeSignedInt64(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeSfixed64(fieldNumber, value);
    });
  }

  /**
   * Sets a sint32 value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {number} value
   */
  setSint32(fieldNumber, value) {
    checkCriticalTypeSignedInt32(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeSint32(fieldNumber, value);
    });
  }

  /**
   * Sets a sint64 value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  setSint64(fieldNumber, value) {
    checkCriticalTypeSignedInt64(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeSint64(fieldNumber, value);
    });
  }

  /**
   * Sets a boolean value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {string} value
   */
  setString(fieldNumber, value) {
    checkCriticalTypeString(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeString(fieldNumber, value);
    });
  }

  /**
   * Sets a uint32 value to the field with the given field number.
   * @param {number} fieldNumber
   * @param {number} value
   */
  setUint32(fieldNumber, value) {
    checkCriticalTypeUnsignedInt32(value);
    this.setField_(fieldNumber, value, (writer, fieldNumber, value) => {
      writer.writeUint32(fieldNumber, value);
    });
  }

  /**
   * Sets a uint64 value to the field with the given field number.\
   * Note: Since g.m.Long does not support unsigned int64 values we are going
   * the Java route here for now and simply output the number as a signed int64.
   * Users can get to individual bits by themselves.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  setUint64(fieldNumber, value) {
    this.setInt64(fieldNumber, value);
  }

  /**
   * Sets a proto Message to the field with the given field number.
   * Instead of working with the LazyAccessor inside of the message directly, we
   * need the message instance to keep its reference equality for subsequent
   * gettings.
   * @param {number} fieldNumber
   * @param {!InternalMessage} value
   */
  setMessage(fieldNumber, value) {
    checkCriticalType(
        value !== null, 'Given value is not a message instance: null');
    this.setField_(fieldNumber, value, writeMessage);
  }

  /***************************************************************************
   *                        REPEATED SETTER METHODS
   ***************************************************************************/

  /* Bool */

  /**
   * Adds all boolean values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<boolean>} values
   * @param {function(!Writer, number, !Array<boolean>): undefined} encoder
   * @private
   */
  addRepeatedBoolIterable_(fieldNumber, values, encoder) {
    const array = [...this.getRepeatedBoolArray_(fieldNumber), ...values];
    checkCriticalTypeBoolArray(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single boolean value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {boolean} value
   */
  addPackedBoolElement(fieldNumber, value) {
    this.addRepeatedBoolIterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedBool(fieldNumber, values);
        });
  }

  /**
   * Adds all boolean values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<boolean>} values
   */
  addPackedBoolIterable(fieldNumber, values) {
    this.addRepeatedBoolIterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedBool(fieldNumber, values);
        });
  }

  /**
   * Adds a single boolean value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {boolean} value
   */
  addUnpackedBoolElement(fieldNumber, value) {
    this.addRepeatedBoolIterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedBool(fieldNumber, values);
        });
  }

  /**
   * Adds all boolean values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<boolean>} values
   */
  addUnpackedBoolIterable(fieldNumber, values) {
    this.addRepeatedBoolIterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedBool(fieldNumber, values);
        });
  }

  /**
   * Sets a single boolean value into the field for the given field number at
   * the given index. How these values are encoded depends on the given write
   * function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {boolean} value
   * @param {function(!Writer, number, !Array<boolean>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedBoolElement_(fieldNumber, index, value, encoder) {
    checkCriticalTypeBool(value);
    const array = this.getRepeatedBoolArray_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single boolean value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {boolean} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedBoolElement(fieldNumber, index, value) {
    this.setRepeatedBoolElement_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedBool(fieldNumber, values);
        });
  }

  /**
   * Sets all boolean values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<boolean>} values
   */
  setPackedBoolIterable(fieldNumber, values) {
    const /** !Array<boolean> */ array = Array.from(values);
    checkCriticalTypeBoolArray(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedBool(fieldNumber, values);
    });
  }

  /**
   * Sets a single boolean value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {boolean} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedBoolElement(fieldNumber, index, value) {
    this.setRepeatedBoolElement_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedBool(fieldNumber, values);
        });
  }

  /**
   * Sets all boolean values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<boolean>} values
   */
  setUnpackedBoolIterable(fieldNumber, values) {
    const /** !Array<boolean> */ array = Array.from(values);
    checkCriticalTypeBoolArray(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedBool(fieldNumber, values);
    });
  }

  /* Double */

  /**
   * Adds all double values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @private
   */
  addRepeatedDoubleIterable_(fieldNumber, values, encoder) {
    const array = [...this.getRepeatedDoubleArray_(fieldNumber), ...values];
    checkCriticalTypeDoubleArray(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single double value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedDoubleElement(fieldNumber, value) {
    this.addRepeatedDoubleIterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedDouble(fieldNumber, values);
        });
  }

  /**
   * Adds all double values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedDoubleIterable(fieldNumber, values) {
    this.addRepeatedDoubleIterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedDouble(fieldNumber, values);
        });
  }

  /**
   * Adds a single double value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedDoubleElement(fieldNumber, value) {
    this.addRepeatedDoubleIterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedDouble(fieldNumber, values);
        });
  }

  /**
   * Adds all double values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedDoubleIterable(fieldNumber, values) {
    this.addRepeatedDoubleIterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedDouble(fieldNumber, values);
        });
  }

  /**
   * Sets a single double value into the field for the given field number at the
   * given index.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedDoubleElement_(fieldNumber, index, value, encoder) {
    checkCriticalTypeDouble(value);
    const array = this.getRepeatedDoubleArray_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single double value into the field for the given field number at the
   * given index.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedDoubleElement(fieldNumber, index, value) {
    this.setRepeatedDoubleElement_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedDouble(fieldNumber, values);
        });
  }

  /**
   * Sets all double values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedDoubleIterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeDoubleArray(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedDouble(fieldNumber, values);
    });
  }

  /**
   * Sets a single double value into the field for the given field number at the
   * given index.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedDoubleElement(fieldNumber, index, value) {
    this.setRepeatedDoubleElement_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedDouble(fieldNumber, values);
        });
  }

  /**
   * Sets all double values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedDoubleIterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeDoubleArray(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedDouble(fieldNumber, values);
    });
  }

  /* Fixed32 */

  /**
   * Adds all fixed32 values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @private
   */
  addRepeatedFixed32Iterable_(fieldNumber, values, encoder) {
    const array = [...this.getRepeatedFixed32Array_(fieldNumber), ...values];
    checkCriticalTypeUnsignedInt32Array(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single fixed32 value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedFixed32Element(fieldNumber, value) {
    this.addRepeatedFixed32Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedFixed32(fieldNumber, values);
        });
  }

  /**
   * Adds all fixed32 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedFixed32Iterable(fieldNumber, values) {
    this.addRepeatedFixed32Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedFixed32(fieldNumber, values);
        });
  }

  /**
   * Adds a single fixed32 value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedFixed32Element(fieldNumber, value) {
    this.addRepeatedFixed32Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedFixed32(fieldNumber, values);
        });
  }

  /**
   * Adds all fixed32 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedFixed32Iterable(fieldNumber, values) {
    this.addRepeatedFixed32Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedFixed32(fieldNumber, values);
        });
  }

  /**
   * Sets a single fixed32 value into the field for the given field number at
   * the given index. How these values are encoded depends on the given write
   * function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedFixed32Element_(fieldNumber, index, value, encoder) {
    checkCriticalTypeUnsignedInt32(value);
    const array = this.getRepeatedFixed32Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single fixed32 value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedFixed32Element(fieldNumber, index, value) {
    this.setRepeatedFixed32Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedFixed32(fieldNumber, values);
        });
  }

  /**
   * Sets all fixed32 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedFixed32Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeUnsignedInt32Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedFixed32(fieldNumber, values);
    });
  }

  /**
   * Sets a single fixed32 value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedFixed32Element(fieldNumber, index, value) {
    this.setRepeatedFixed32Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedFixed32(fieldNumber, values);
        });
  }

  /**
   * Sets all fixed32 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedFixed32Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeUnsignedInt32Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedFixed32(fieldNumber, values);
    });
  }

  /* Fixed64 */

  /**
   * Adds a single fixed64 value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addPackedFixed64Element(fieldNumber, value) {
    this.addPackedSfixed64Element(fieldNumber, value);
  }

  /**
   * Adds all fixed64 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addPackedFixed64Iterable(fieldNumber, values) {
    this.addPackedSfixed64Iterable(fieldNumber, values);
  }

  /**
   * Adds a single fixed64 value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addUnpackedFixed64Element(fieldNumber, value) {
    this.addUnpackedSfixed64Element(fieldNumber, value);
  }

  /**
   * Adds all fixed64 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addUnpackedFixed64Iterable(fieldNumber, values) {
    this.addUnpackedSfixed64Iterable(fieldNumber, values);
  }

  /**
   * Sets a single fixed64 value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedFixed64Element(fieldNumber, index, value) {
    this.setPackedSfixed64Element(fieldNumber, index, value);
  }

  /**
   * Sets all fixed64 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setPackedFixed64Iterable(fieldNumber, values) {
    this.setPackedSfixed64Iterable(fieldNumber, values);
  }

  /**
   * Sets a single fixed64 value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedFixed64Element(fieldNumber, index, value) {
    this.setUnpackedSfixed64Element(fieldNumber, index, value);
  }

  /**
   * Sets all fixed64 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setUnpackedFixed64Iterable(fieldNumber, values) {
    this.setUnpackedSfixed64Iterable(fieldNumber, values);
  }

  /* Float */

  /**
   * Adds all float values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @private
   */
  addRepeatedFloatIterable_(fieldNumber, values, encoder) {
    checkCriticalTypeFloatIterable(values);
    // Eagerly round to 32-bit precision so that reading back after set will
    // yield the same value a reader will receive after serialization.
    const floatValues = Array.from(values, fround);
    const array = [...this.getRepeatedFloatArray_(fieldNumber), ...floatValues];
    checkCriticalTypeFloatIterable(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single float value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedFloatElement(fieldNumber, value) {
    this.addRepeatedFloatIterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedFloat(fieldNumber, values);
        });
  }

  /**
   * Adds all float values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedFloatIterable(fieldNumber, values) {
    this.addRepeatedFloatIterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedFloat(fieldNumber, values);
        });
  }

  /**
   * Adds a single float value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedFloatElement(fieldNumber, value) {
    this.addRepeatedFloatIterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedFloat(fieldNumber, values);
        });
  }

  /**
   * Adds all float values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedFloatIterable(fieldNumber, values) {
    this.addRepeatedFloatIterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedFloat(fieldNumber, values);
        });
  }

  /**
   * Sets a single float value into the field for the given field number at the
   * given index.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedFloatElement_(fieldNumber, index, value, encoder) {
    checkCriticalTypeFloat(value);
    // Eagerly round to 32-bit precision so that reading back after set will
    // yield the same value a reader will receive after serialization.
    const floatValue = Math.fround(value);
    const array = this.getRepeatedFloatArray_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = floatValue;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single float value into the field for the given field number at the
   * given index.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedFloatElement(fieldNumber, index, value) {
    this.setRepeatedFloatElement_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedFloat(fieldNumber, values);
        });
  }

  /**
   * Sets all float values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedFloatIterable(fieldNumber, values) {
    checkCriticalTypeFloatIterable(values);
    // Eagerly round to 32-bit precision so that reading back after set will
    // yield the same value a reader will receive after serialization.
    const array = Array.from(values, fround);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedFloat(fieldNumber, values);
    });
  }

  /**
   * Sets a single float value into the field for the given field number at the
   * given index.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedFloatElement(fieldNumber, index, value) {
    this.setRepeatedFloatElement_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedFloat(fieldNumber, values);
        });
  }

  /**
   * Sets all float values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedFloatIterable(fieldNumber, values) {
    checkCriticalTypeFloatIterable(values);
    // Eagerly round to 32-bit precision so that reading back after set will
    // yield the same value a reader will receive after serialization.
    const array = Array.from(values, fround);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedFloat(fieldNumber, values);
    });
  }

  /* Int32 */

  /**
   * Adds all int32 values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @private
   */
  addRepeatedInt32Iterable_(fieldNumber, values, encoder) {
    const array = [...this.getRepeatedInt32Array_(fieldNumber), ...values];
    checkCriticalTypeSignedInt32Array(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single int32 value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedInt32Element(fieldNumber, value) {
    this.addRepeatedInt32Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedInt32(fieldNumber, values);
        });
  }

  /**
   * Adds all int32 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedInt32Iterable(fieldNumber, values) {
    this.addRepeatedInt32Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedInt32(fieldNumber, values);
        });
  }

  /**
   * Adds a single int32 value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedInt32Element(fieldNumber, value) {
    this.addRepeatedInt32Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedInt32(fieldNumber, values);
        });
  }

  /**
   * Adds all int32 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedInt32Iterable(fieldNumber, values) {
    this.addRepeatedInt32Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedInt32(fieldNumber, values);
        });
  }

  /**
   * Sets a single int32 value into the field for the given field number at
   * the given index. How these values are encoded depends on the given write
   * function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedInt32Element_(fieldNumber, index, value, encoder) {
    checkCriticalTypeSignedInt32(value);
    const array = this.getRepeatedInt32Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single int32 value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedInt32Element(fieldNumber, index, value) {
    this.setRepeatedInt32Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedInt32(fieldNumber, values);
        });
  }

  /**
   * Sets all int32 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedInt32Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt32Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedInt32(fieldNumber, values);
    });
  }

  /**
   * Sets a single int32 value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedInt32Element(fieldNumber, index, value) {
    this.setRepeatedInt32Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedInt32(fieldNumber, values);
        });
  }

  /**
   * Sets all int32 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedInt32Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt32Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedInt32(fieldNumber, values);
    });
  }

  /* Int64 */

  /**
   * Adds all int64 values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   * @param {function(!Writer, number, !Array<!Int64>): undefined} encoder
   * @private
   */
  addRepeatedInt64Iterable_(fieldNumber, values, encoder) {
    const array = [...this.getRepeatedInt64Array_(fieldNumber), ...values];
    checkCriticalTypeSignedInt64Array(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single int64 value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addPackedInt64Element(fieldNumber, value) {
    this.addRepeatedInt64Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedInt64(fieldNumber, values);
        });
  }

  /**
   * Adds all int64 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addPackedInt64Iterable(fieldNumber, values) {
    this.addRepeatedInt64Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedInt64(fieldNumber, values);
        });
  }

  /**
   * Adds a single int64 value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addUnpackedInt64Element(fieldNumber, value) {
    this.addRepeatedInt64Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedInt64(fieldNumber, values);
        });
  }

  /**
   * Adds all int64 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addUnpackedInt64Iterable(fieldNumber, values) {
    this.addRepeatedInt64Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedInt64(fieldNumber, values);
        });
  }

  /**
   * Sets a single int64 value into the field for the given field number at
   * the given index. How these values are encoded depends on the given write
   * function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @param {function(!Writer, number, !Array<!Int64>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedInt64Element_(fieldNumber, index, value, encoder) {
    checkCriticalTypeSignedInt64(value);
    const array = this.getRepeatedInt64Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single int64 value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedInt64Element(fieldNumber, index, value) {
    this.setRepeatedInt64Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedInt64(fieldNumber, values);
        });
  }

  /**
   * Sets all int64 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setPackedInt64Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt64Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedInt64(fieldNumber, values);
    });
  }

  /**
   * Sets a single int64 value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedInt64Element(fieldNumber, index, value) {
    this.setRepeatedInt64Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedInt64(fieldNumber, values);
        });
  }

  /**
   * Sets all int64 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setUnpackedInt64Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt64Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedInt64(fieldNumber, values);
    });
  }

  /* Sfixed32 */

  /**
   * Adds all sfixed32 values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @private
   */
  addRepeatedSfixed32Iterable_(fieldNumber, values, encoder) {
    const array = [...this.getRepeatedSfixed32Array_(fieldNumber), ...values];
    checkCriticalTypeSignedInt32Array(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single sfixed32 value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedSfixed32Element(fieldNumber, value) {
    this.addRepeatedSfixed32Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedSfixed32(fieldNumber, values);
        });
  }

  /**
   * Adds all sfixed32 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedSfixed32Iterable(fieldNumber, values) {
    this.addRepeatedSfixed32Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedSfixed32(fieldNumber, values);
        });
  }

  /**
   * Adds a single sfixed32 value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedSfixed32Element(fieldNumber, value) {
    this.addRepeatedSfixed32Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedSfixed32(fieldNumber, values);
        });
  }

  /**
   * Adds all sfixed32 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedSfixed32Iterable(fieldNumber, values) {
    this.addRepeatedSfixed32Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedSfixed32(fieldNumber, values);
        });
  }

  /**
   * Sets a single sfixed32 value into the field for the given field number at
   * the given index. How these values are encoded depends on the given write
   * function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedSfixed32Element_(fieldNumber, index, value, encoder) {
    checkCriticalTypeSignedInt32(value);
    const array = this.getRepeatedSfixed32Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single sfixed32 value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedSfixed32Element(fieldNumber, index, value) {
    this.setRepeatedSfixed32Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedSfixed32(fieldNumber, values);
        });
  }

  /**
   * Sets all sfixed32 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedSfixed32Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt32Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedSfixed32(fieldNumber, values);
    });
  }

  /**
   * Sets a single sfixed32 value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedSfixed32Element(fieldNumber, index, value) {
    this.setRepeatedSfixed32Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedSfixed32(fieldNumber, values);
        });
  }

  /**
   * Sets all sfixed32 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedSfixed32Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt32Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedSfixed32(fieldNumber, values);
    });
  }

  /* Sfixed64 */

  /**
   * Adds all sfixed64 values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   * @param {function(!Writer, number, !Array<!Int64>): undefined} encoder
   * @private
   */
  addRepeatedSfixed64Iterable_(fieldNumber, values, encoder) {
    const array = [...this.getRepeatedSfixed64Array_(fieldNumber), ...values];
    checkCriticalTypeSignedInt64Array(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single sfixed64 value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addPackedSfixed64Element(fieldNumber, value) {
    this.addRepeatedSfixed64Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedSfixed64(fieldNumber, values);
        });
  }

  /**
   * Adds all sfixed64 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addPackedSfixed64Iterable(fieldNumber, values) {
    this.addRepeatedSfixed64Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedSfixed64(fieldNumber, values);
        });
  }

  /**
   * Adds a single sfixed64 value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addUnpackedSfixed64Element(fieldNumber, value) {
    this.addRepeatedSfixed64Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedSfixed64(fieldNumber, values);
        });
  }

  /**
   * Adds all sfixed64 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addUnpackedSfixed64Iterable(fieldNumber, values) {
    this.addRepeatedSfixed64Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedSfixed64(fieldNumber, values);
        });
  }

  /**
   * Sets a single sfixed64 value into the field for the given field number at
   * the given index. How these values are encoded depends on the given write
   * function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @param {function(!Writer, number, !Array<!Int64>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedSfixed64Element_(fieldNumber, index, value, encoder) {
    checkCriticalTypeSignedInt64(value);
    const array = this.getRepeatedSfixed64Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single sfixed64 value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedSfixed64Element(fieldNumber, index, value) {
    this.setRepeatedSfixed64Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedSfixed64(fieldNumber, values);
        });
  }

  /**
   * Sets all sfixed64 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setPackedSfixed64Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt64Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedSfixed64(fieldNumber, values);
    });
  }

  /**
   * Sets a single sfixed64 value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedSfixed64Element(fieldNumber, index, value) {
    this.setRepeatedSfixed64Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedSfixed64(fieldNumber, values);
        });
  }

  /**
   * Sets all sfixed64 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setUnpackedSfixed64Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt64Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedSfixed64(fieldNumber, values);
    });
  }

  /* Sint32 */

  /**
   * Adds all sint32 values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @private
   */
  addRepeatedSint32Iterable_(fieldNumber, values, encoder) {
    const array = [...this.getRepeatedSint32Array_(fieldNumber), ...values];
    checkCriticalTypeSignedInt32Array(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single sint32 value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedSint32Element(fieldNumber, value) {
    this.addRepeatedSint32Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedSint32(fieldNumber, values);
        });
  }

  /**
   * Adds all sint32 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedSint32Iterable(fieldNumber, values) {
    this.addRepeatedSint32Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedSint32(fieldNumber, values);
        });
  }

  /**
   * Adds a single sint32 value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedSint32Element(fieldNumber, value) {
    this.addRepeatedSint32Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedSint32(fieldNumber, values);
        });
  }

  /**
   * Adds all sint32 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedSint32Iterable(fieldNumber, values) {
    this.addRepeatedSint32Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedSint32(fieldNumber, values);
        });
  }

  /**
   * Sets a single sint32 value into the field for the given field number at
   * the given index. How these values are encoded depends on the given write
   * function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedSint32Element_(fieldNumber, index, value, encoder) {
    checkCriticalTypeSignedInt32(value);
    const array = this.getRepeatedSint32Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single sint32 value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedSint32Element(fieldNumber, index, value) {
    this.setRepeatedSint32Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedSint32(fieldNumber, values);
        });
  }

  /**
   * Sets all sint32 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedSint32Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt32Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedSint32(fieldNumber, values);
    });
  }

  /**
   * Sets a single sint32 value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedSint32Element(fieldNumber, index, value) {
    this.setRepeatedSint32Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedSint32(fieldNumber, values);
        });
  }

  /**
   * Sets all sint32 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedSint32Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt32Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedSint32(fieldNumber, values);
    });
  }

  /* Sint64 */

  /**
   * Adds all sint64 values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   * @param {function(!Writer, number, !Array<!Int64>): undefined} encoder
   * @private
   */
  addRepeatedSint64Iterable_(fieldNumber, values, encoder) {
    const array = [...this.getRepeatedSint64Array_(fieldNumber), ...values];
    checkCriticalTypeSignedInt64Array(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single sint64 value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addPackedSint64Element(fieldNumber, value) {
    this.addRepeatedSint64Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedSint64(fieldNumber, values);
        });
  }

  /**
   * Adds all sint64 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addPackedSint64Iterable(fieldNumber, values) {
    this.addRepeatedSint64Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedSint64(fieldNumber, values);
        });
  }

  /**
   * Adds a single sint64 value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addUnpackedSint64Element(fieldNumber, value) {
    this.addRepeatedSint64Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedSint64(fieldNumber, values);
        });
  }

  /**
   * Adds all sint64 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addUnpackedSint64Iterable(fieldNumber, values) {
    this.addRepeatedSint64Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedSint64(fieldNumber, values);
        });
  }

  /**
   * Sets a single sint64 value into the field for the given field number at
   * the given index. How these values are encoded depends on the given write
   * function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @param {function(!Writer, number, !Array<!Int64>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedSint64Element_(fieldNumber, index, value, encoder) {
    checkCriticalTypeSignedInt64(value);
    const array = this.getRepeatedSint64Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single sint64 value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedSint64Element(fieldNumber, index, value) {
    this.setRepeatedSint64Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedSint64(fieldNumber, values);
        });
  }

  /**
   * Sets all sint64 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setPackedSint64Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt64Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedSint64(fieldNumber, values);
    });
  }

  /**
   * Sets a single sint64 value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedSint64Element(fieldNumber, index, value) {
    this.setRepeatedSint64Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedSint64(fieldNumber, values);
        });
  }

  /**
   * Sets all sint64 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setUnpackedSint64Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeSignedInt64Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedSint64(fieldNumber, values);
    });
  }

  /* Uint32 */

  /**
   * Adds all uint32 values into the field for the given field number.
   * How these values are encoded depends on the given write function.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @private
   */
  addRepeatedUint32Iterable_(fieldNumber, values, encoder) {
    const array = [...this.getRepeatedUint32Array_(fieldNumber), ...values];
    checkCriticalTypeUnsignedInt32Array(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Adds a single uint32 value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedUint32Element(fieldNumber, value) {
    this.addRepeatedUint32Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writePackedUint32(fieldNumber, values);
        });
  }

  /**
   * Adds all uint32 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedUint32Iterable(fieldNumber, values) {
    this.addRepeatedUint32Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writePackedUint32(fieldNumber, values);
        });
  }

  /**
   * Adds a single uint32 value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedUint32Element(fieldNumber, value) {
    this.addRepeatedUint32Iterable_(
        fieldNumber, [value], (writer, fieldNumber, values) => {
          writer.writeRepeatedUint32(fieldNumber, values);
        });
  }

  /**
   * Adds all uint32 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedUint32Iterable(fieldNumber, values) {
    this.addRepeatedUint32Iterable_(
        fieldNumber, values, (writer, fieldNumber, values) => {
          writer.writeRepeatedUint32(fieldNumber, values);
        });
  }

  /**
   * Sets a single uint32 value into the field for the given field number at
   * the given index. How these values are encoded depends on the given write
   * function.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @param {function(!Writer, number, !Array<number>): undefined} encoder
   * @throws {!Error} if index is out of range when check mode is critical
   * @private
   */
  setRepeatedUint32Element_(fieldNumber, index, value, encoder) {
    checkCriticalTypeUnsignedInt32(value);
    const array = this.getRepeatedUint32Array_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, encoder);
  }

  /**
   * Sets a single uint32 value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedUint32Element(fieldNumber, index, value) {
    this.setRepeatedUint32Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writePackedUint32(fieldNumber, values);
        });
  }

  /**
   * Sets all uint32 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedUint32Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeUnsignedInt32Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writePackedUint32(fieldNumber, values);
    });
  }

  /**
   * Sets a single uint32 value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedUint32Element(fieldNumber, index, value) {
    this.setRepeatedUint32Element_(
        fieldNumber, index, value, (writer, fieldNumber, values) => {
          writer.writeRepeatedUint32(fieldNumber, values);
        });
  }

  /**
   * Sets all uint32 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedUint32Iterable(fieldNumber, values) {
    const array = Array.from(values);
    checkCriticalTypeUnsignedInt32Array(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedUint32(fieldNumber, values);
    });
  }

  /* Uint64 */

  /**
   * Adds a single uint64 value into the field for the given field number.
   * All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addPackedUint64Element(fieldNumber, value) {
    this.addPackedInt64Element(fieldNumber, value);
  }

  /**
   * Adds all uint64 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addPackedUint64Iterable(fieldNumber, values) {
    this.addPackedInt64Iterable(fieldNumber, values);
  }

  /**
   * Adds a single uint64 value into the field for the given field number.
   * All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addUnpackedUint64Element(fieldNumber, value) {
    this.addUnpackedInt64Element(fieldNumber, value);
  }

  /**
   * Adds all uint64 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addUnpackedUint64Iterable(fieldNumber, values) {
    this.addUnpackedInt64Iterable(fieldNumber, values);
  }

  /**
   * Sets a single uint64 value into the field for the given field number at
   * the given index. All values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedUint64Element(fieldNumber, index, value) {
    this.setPackedInt64Element(fieldNumber, index, value);
  }

  /**
   * Sets all uint64 values into the field for the given field number.
   * All these values will be encoded as packed values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setPackedUint64Iterable(fieldNumber, values) {
    this.setPackedInt64Iterable(fieldNumber, values);
  }

  /**
   * Sets a single uint64 value into the field for the given field number at
   * the given index. All values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedUint64Element(fieldNumber, index, value) {
    this.setUnpackedInt64Element(fieldNumber, index, value);
  }

  /**
   * Sets all uint64 values into the field for the given field number.
   * All these values will be encoded as unpacked values.
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setUnpackedUint64Iterable(fieldNumber, values) {
    this.setUnpackedInt64Iterable(fieldNumber, values);
  }

  /* Bytes */

  /**
   * Sets all bytes values into the field for the given field number.
   * @param {number} fieldNumber
   * @param {!Iterable<!ByteString>} values
   */
  setRepeatedBytesIterable(fieldNumber, values) {
    const /** !Array<!ByteString> */ array = Array.from(values);
    checkCriticalTypeByteStringArray(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedBytes(fieldNumber, values);
    });
  }

  /**
   * Adds all bytes values into the field for the given field number.
   * @param {number} fieldNumber
   * @param {!Iterable<!ByteString>} values
   */
  addRepeatedBytesIterable(fieldNumber, values) {
    const array = [...this.getRepeatedBytesArray_(fieldNumber), ...values];
    checkCriticalTypeByteStringArray(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedBytes(fieldNumber, values);
    });
  }

  /**
   * Sets a single bytes value into the field for the given field number at
   * the given index.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!ByteString} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setRepeatedBytesElement(fieldNumber, index, value) {
    checkCriticalTypeByteString(value);
    const array = this.getRepeatedBytesArray_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedBytes(fieldNumber, values);
    });
  }

  /**
   * Adds a single bytes value into the field for the given field number.
   * @param {number} fieldNumber
   * @param {!ByteString} value
   */
  addRepeatedBytesElement(fieldNumber, value) {
    this.addRepeatedBytesIterable(fieldNumber, [value]);
  }

  /* String */

  /**
   * Sets all string values into the field for the given field number.
   * @param {number} fieldNumber
   * @param {!Iterable<string>} values
   */
  setRepeatedStringIterable(fieldNumber, values) {
    const /** !Array<string> */ array = Array.from(values);
    checkCriticalTypeStringArray(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedString(fieldNumber, values);
    });
  }

  /**
   * Adds all string values into the field for the given field number.
   * @param {number} fieldNumber
   * @param {!Iterable<string>} values
   */
  addRepeatedStringIterable(fieldNumber, values) {
    const array = [...this.getRepeatedStringArray_(fieldNumber), ...values];
    checkCriticalTypeStringArray(array);
    // Needs to set it back because the default empty array was not cached.
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedString(fieldNumber, values);
    });
  }

  /**
   * Sets a single string value into the field for the given field number at
   * the given index.
   * @param {number} fieldNumber
   * @param {number} index
   * @param {string} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setRepeatedStringElement(fieldNumber, index, value) {
    checkCriticalTypeString(value);
    const array = this.getRepeatedStringArray_(fieldNumber);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
    // Needs to set it back to set encoder.
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writer.writeRepeatedString(fieldNumber, values);
    });
  }

  /**
   * Adds a single string value into the field for the given field number.
   * @param {number} fieldNumber
   * @param {string} value
   */
  addRepeatedStringElement(fieldNumber, value) {
    this.addRepeatedStringIterable(fieldNumber, [value]);
  }

  /* Message */

  /**
   * Sets all message values into the field for the given field number.
   * @param {number} fieldNumber
   * @param {!Iterable<!InternalMessage>} values
   */
  setRepeatedMessageIterable(fieldNumber, values) {
    const /** !Array<!InternalMessage> */ array = Array.from(values);
    checkCriticalTypeMessageArray(array);
    this.setField_(fieldNumber, array, (writer, fieldNumber, values) => {
      writeRepeatedMessage(writer, fieldNumber, values);
    });
  }

  /**
   * Adds all message values into the field for the given field number.
   * @param {number} fieldNumber
   * @param {!Iterable<!InternalMessage>} values
   * @param {function(!LazyAccessor):!InternalMessage} instanceCreator
   * @param {number=} pivot
   */
  addRepeatedMessageIterable(
      fieldNumber, values, instanceCreator, pivot = undefined) {
    const array = [
      ...this.getRepeatedMessageArray_(fieldNumber, instanceCreator, pivot),
      ...values,
    ];
    checkCriticalTypeMessageArray(array);
    // Needs to set it back with the new array.
    this.setField_(
        fieldNumber, array,
        (writer, fieldNumber, values) =>
            writeRepeatedMessage(writer, fieldNumber, values));
  }

  /**
   * Sets a single message value into the field for the given field number at
   * the given index.
   * @param {number} fieldNumber
   * @param {!InternalMessage} value
   * @param {function(!LazyAccessor):!InternalMessage} instanceCreator
   * @param {number} index
   * @param {number=} pivot
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setRepeatedMessageElement(
      fieldNumber, value, instanceCreator, index, pivot = undefined) {
    checkInstanceCreator(instanceCreator);
    checkCriticalType(
        value !== null, 'Given value is not a message instance: null');
    const array =
        this.getRepeatedMessageArray_(fieldNumber, instanceCreator, pivot);
    checkCriticalElementIndex(index, array.length);
    array[index] = value;
  }

  /**
   * Adds a single message value into the field for the given field number.
   * @param {number} fieldNumber
   * @param {!InternalMessage} value
   * @param {function(!LazyAccessor):!InternalMessage} instanceCreator
   * @param {number=} pivot
   */
  addRepeatedMessageElement(
      fieldNumber, value, instanceCreator, pivot = undefined) {
    this.addRepeatedMessageIterable(
        fieldNumber, [value], instanceCreator, pivot);
  }
}

exports = LazyAccessor;
