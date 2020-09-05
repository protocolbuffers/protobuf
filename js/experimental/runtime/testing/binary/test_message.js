/**
 * @fileoverview Kernel wrapper message.
 */
goog.module('protobuf.testing.binary.TestMessage');

const ByteString = goog.require('protobuf.ByteString');
const Int64 = goog.require('protobuf.Int64');
const InternalMessage = goog.require('protobuf.binary.InternalMessage');
const Kernel = goog.require('protobuf.runtime.Kernel');

/**
 * A protobuf message implemented as a Kernel wrapper.
 * @implements {InternalMessage}
 */
class TestMessage {
  /**
   * @return {!TestMessage}
   */
  static createEmpty() {
    return TestMessage.instanceCreator(Kernel.createEmpty());
  }

  /**
   * @param {!Kernel} kernel
   * @return {!TestMessage}
   */
  static instanceCreator(kernel) {
    return new TestMessage(kernel);
  }

  /**
   * @param {!Kernel} kernel
   */
  constructor(kernel) {
    /** @private @const {!Kernel} */
    this.kernel_ = kernel;
  }

  /**
   * @override
   * @return {!Kernel}
   */
  internalGetKernel() {
    return this.kernel_;
  }

  /**
   * @return {!ArrayBuffer}
   */
  serialize() {
    return this.kernel_.serialize();
  }

  /**
   * @param {number} fieldNumber
   * @param {boolean=} defaultValue
   * @return {boolean}
   */
  getBoolWithDefault(fieldNumber, defaultValue = false) {
    return this.kernel_.getBoolWithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {!ByteString=} defaultValue
   * @return {!ByteString}
   */
  getBytesWithDefault(fieldNumber, defaultValue = ByteString.EMPTY) {
    return this.kernel_.getBytesWithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getDoubleWithDefault(fieldNumber, defaultValue = 0) {
    return this.kernel_.getDoubleWithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getFixed32WithDefault(fieldNumber, defaultValue = 0) {
    return this.kernel_.getFixed32WithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64=} defaultValue
   * @return {!Int64}
   */
  getFixed64WithDefault(fieldNumber, defaultValue = Int64.getZero()) {
    return this.kernel_.getFixed64WithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getFloatWithDefault(fieldNumber, defaultValue = 0) {
    return this.kernel_.getFloatWithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getInt32WithDefault(fieldNumber, defaultValue = 0) {
    return this.kernel_.getInt32WithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64=} defaultValue
   * @return {!Int64}
   */
  getInt64WithDefault(fieldNumber, defaultValue = Int64.getZero()) {
    return this.kernel_.getInt64WithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getSfixed32WithDefault(fieldNumber, defaultValue = 0) {
    return this.kernel_.getSfixed32WithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64=} defaultValue
   * @return {!Int64}
   */
  getSfixed64WithDefault(fieldNumber, defaultValue = Int64.getZero()) {
    return this.kernel_.getSfixed64WithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getSint32WithDefault(fieldNumber, defaultValue = 0) {
    return this.kernel_.getSint32WithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64=} defaultValue
   * @return {!Int64}
   */
  getSint64WithDefault(fieldNumber, defaultValue = Int64.getZero()) {
    return this.kernel_.getSint64WithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {string=} defaultValue
   * @return {string}
   */
  getStringWithDefault(fieldNumber, defaultValue = '') {
    return this.kernel_.getStringWithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {number=} defaultValue
   * @return {number}
   */
  getUint32WithDefault(fieldNumber, defaultValue = 0) {
    return this.kernel_.getUint32WithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64=} defaultValue
   * @return {!Int64}
   */
  getUint64WithDefault(fieldNumber, defaultValue = Int64.getZero()) {
    return this.kernel_.getUint64WithDefault(fieldNumber, defaultValue);
  }

  /**
   * @param {number} fieldNumber
   * @param {function(!Kernel):T} instanceCreator
   * @return {?T}
   * @template T
   */
  getMessageOrNull(fieldNumber, instanceCreator) {
    return this.kernel_.getMessageOrNull(fieldNumber, instanceCreator);
  }

  /**
   * @param {number} fieldNumber
   * @param {function(!Kernel):T} instanceCreator
   * @return {T}
   * @template T
   */
  getMessageAttach(fieldNumber, instanceCreator) {
    return this.kernel_.getMessageAttach(fieldNumber, instanceCreator);
  }

  /**
   * @param {number} fieldNumber
   * @param {function(!Kernel):T} instanceCreator
   * @return {T}
   * @template T
   */
  getMessage(fieldNumber, instanceCreator) {
    return this.kernel_.getMessage(fieldNumber, instanceCreator);
  }

  /**
   * @param {number} fieldNumber
   * @return {?Kernel}
   * @template T
   */
  getMessageAccessorOrNull(fieldNumber) {
    return this.kernel_.getMessageAccessorOrNull(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {boolean}
   */
  getRepeatedBoolElement(fieldNumber, index) {
    return this.kernel_.getRepeatedBoolElement(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<boolean>}
   */
  getRepeatedBoolIterable(fieldNumber) {
    return this.kernel_.getRepeatedBoolIterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedBoolSize(fieldNumber) {
    return this.kernel_.getRepeatedBoolSize(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedDoubleElement(fieldNumber, index) {
    return this.kernel_.getRepeatedDoubleElement(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedDoubleIterable(fieldNumber) {
    return this.kernel_.getRepeatedDoubleIterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedDoubleSize(fieldNumber) {
    return this.kernel_.getRepeatedDoubleSize(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedFixed32Element(fieldNumber, index) {
    return this.kernel_.getRepeatedFixed32Element(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedFixed32Iterable(fieldNumber) {
    return this.kernel_.getRepeatedFixed32Iterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedFixed32Size(fieldNumber) {
    return this.kernel_.getRepeatedFixed32Size(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!Int64}
   */
  getRepeatedFixed64Element(fieldNumber, index) {
    return this.kernel_.getRepeatedFixed64Element(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<!Int64>}
   */
  getRepeatedFixed64Iterable(fieldNumber) {
    return this.kernel_.getRepeatedFixed64Iterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedFixed64Size(fieldNumber) {
    return this.kernel_.getRepeatedFixed64Size(fieldNumber);
  }
  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedFloatElement(fieldNumber, index) {
    return this.kernel_.getRepeatedFloatElement(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedFloatIterable(fieldNumber) {
    return this.kernel_.getRepeatedFloatIterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedFloatSize(fieldNumber) {
    return this.kernel_.getRepeatedFloatSize(fieldNumber);
  }
  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedInt32Element(fieldNumber, index) {
    return this.kernel_.getRepeatedInt32Element(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedInt32Iterable(fieldNumber) {
    return this.kernel_.getRepeatedInt32Iterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedInt32Size(fieldNumber) {
    return this.kernel_.getRepeatedInt32Size(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!Int64}
   */
  getRepeatedInt64Element(fieldNumber, index) {
    return this.kernel_.getRepeatedInt64Element(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<!Int64>}
   */
  getRepeatedInt64Iterable(fieldNumber) {
    return this.kernel_.getRepeatedInt64Iterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedInt64Size(fieldNumber) {
    return this.kernel_.getRepeatedInt64Size(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedSfixed32Element(fieldNumber, index) {
    return this.kernel_.getRepeatedSfixed32Element(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedSfixed32Iterable(fieldNumber) {
    return this.kernel_.getRepeatedSfixed32Iterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedSfixed32Size(fieldNumber) {
    return this.kernel_.getRepeatedSfixed32Size(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!Int64}
   */
  getRepeatedSfixed64Element(fieldNumber, index) {
    return this.kernel_.getRepeatedSfixed64Element(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<!Int64>}
   */
  getRepeatedSfixed64Iterable(fieldNumber) {
    return this.kernel_.getRepeatedSfixed64Iterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedSfixed64Size(fieldNumber) {
    return this.kernel_.getRepeatedSfixed64Size(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedSint32Element(fieldNumber, index) {
    return this.kernel_.getRepeatedSint32Element(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedSint32Iterable(fieldNumber) {
    return this.kernel_.getRepeatedSint32Iterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedSint32Size(fieldNumber) {
    return this.kernel_.getRepeatedSint32Size(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!Int64}
   */
  getRepeatedSint64Element(fieldNumber, index) {
    return this.kernel_.getRepeatedSint64Element(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<!Int64>}
   */
  getRepeatedSint64Iterable(fieldNumber) {
    return this.kernel_.getRepeatedSint64Iterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedSint64Size(fieldNumber) {
    return this.kernel_.getRepeatedSint64Size(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {number}
   */
  getRepeatedUint32Element(fieldNumber, index) {
    return this.kernel_.getRepeatedUint32Element(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<number>}
   */
  getRepeatedUint32Iterable(fieldNumber) {
    return this.kernel_.getRepeatedUint32Iterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedUint32Size(fieldNumber) {
    return this.kernel_.getRepeatedUint32Size(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!Int64}
   */
  getRepeatedUint64Element(fieldNumber, index) {
    return this.kernel_.getRepeatedUint64Element(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<!Int64>}
   */
  getRepeatedUint64Iterable(fieldNumber) {
    return this.kernel_.getRepeatedUint64Iterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedUint64Size(fieldNumber) {
    return this.kernel_.getRepeatedUint64Size(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {!ByteString}
   */
  getRepeatedBytesElement(fieldNumber, index) {
    return this.kernel_.getRepeatedBytesElement(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<!ByteString>}
   */
  getRepeatedBytesIterable(fieldNumber) {
    return this.kernel_.getRepeatedBytesIterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedBytesSize(fieldNumber) {
    return this.kernel_.getRepeatedBytesSize(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @return {string}
   */
  getRepeatedStringElement(fieldNumber, index) {
    return this.kernel_.getRepeatedStringElement(fieldNumber, index);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<string>}
   */
  getRepeatedStringIterable(fieldNumber) {
    return this.kernel_.getRepeatedStringIterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @return {number}
   */
  getRepeatedStringSize(fieldNumber) {
    return this.kernel_.getRepeatedStringSize(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {function(!Kernel):T} instanceCreator
   * @param {number} index
   * @return {T}
   * @template T
   */
  getRepeatedMessageElement(fieldNumber, instanceCreator, index) {
    return this.kernel_.getRepeatedMessageElement(
        fieldNumber, instanceCreator, index);
  }

  /**
   * @param {number} fieldNumber
   * @param {function(!Kernel):T} instanceCreator
   * @return {!Iterable<T>}
   * @template T
   */
  getRepeatedMessageIterable(fieldNumber, instanceCreator) {
    return this.kernel_.getRepeatedMessageIterable(
        fieldNumber, instanceCreator);
  }

  /**
   * @param {number} fieldNumber
   * @return {!Iterable<!Kernel>}
   * @template T
   */
  getRepeatedMessageAccessorIterable(fieldNumber) {
    return this.kernel_.getRepeatedMessageAccessorIterable(fieldNumber);
  }

  /**
   * @param {number} fieldNumber
   * @param {function(!Kernel):T} instanceCreator
   * @return {number}
   * @template T
   */
  getRepeatedMessageSize(fieldNumber, instanceCreator) {
    return this.kernel_.getRepeatedMessageSize(fieldNumber, instanceCreator);
  }

  /**
   * @param {number} fieldNumber
   * @param {boolean} value
   */
  setBool(fieldNumber, value) {
    this.kernel_.setBool(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!ByteString} value
   */
  setBytes(fieldNumber, value) {
    this.kernel_.setBytes(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  setDouble(fieldNumber, value) {
    this.kernel_.setDouble(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  setFixed32(fieldNumber, value) {
    this.kernel_.setFixed32(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  setFixed64(fieldNumber, value) {
    this.kernel_.setFixed64(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  setFloat(fieldNumber, value) {
    this.kernel_.setFloat(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  setInt32(fieldNumber, value) {
    this.kernel_.setInt32(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  setInt64(fieldNumber, value) {
    this.kernel_.setInt64(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  setSfixed32(fieldNumber, value) {
    this.kernel_.setSfixed32(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  setSfixed64(fieldNumber, value) {
    this.kernel_.setSfixed64(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  setSint32(fieldNumber, value) {
    this.kernel_.setSint32(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  setSint64(fieldNumber, value) {
    this.kernel_.setSint64(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {string} value
   */
  setString(fieldNumber, value) {
    this.kernel_.setString(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  setUint32(fieldNumber, value) {
    this.kernel_.setUint32(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  setUint64(fieldNumber, value) {
    this.kernel_.setUint64(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {T} value
   * @template T
   */
  setMessage(fieldNumber, value) {
    this.kernel_.setMessage(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {boolean} value
   */
  addPackedBoolElement(fieldNumber, value) {
    this.kernel_.addPackedBoolElement(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<boolean>} values
   */
  addPackedBoolIterable(fieldNumber, values) {
    this.kernel_.addPackedBoolIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {boolean} value
   */
  addUnpackedBoolElement(fieldNumber, value) {
    this.kernel_.addUnpackedBoolElement(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<boolean>} values
   */
  addUnpackedBoolIterable(fieldNumber, values) {
    this.kernel_.addUnpackedBoolIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {boolean} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedBoolElement(fieldNumber, index, value) {
    this.kernel_.setPackedBoolElement(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<boolean>} values
   */
  setPackedBoolIterable(fieldNumber, values) {
    this.kernel_.setPackedBoolIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {boolean} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedBoolElement(fieldNumber, index, value) {
    this.kernel_.setUnpackedBoolElement(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<boolean>} values
   */
  setUnpackedBoolIterable(fieldNumber, values) {
    this.kernel_.setUnpackedBoolIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedDoubleElement(fieldNumber, value) {
    this.kernel_.addPackedDoubleElement(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedDoubleIterable(fieldNumber, values) {
    this.kernel_.addPackedDoubleIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedDoubleElement(fieldNumber, value) {
    this.kernel_.addUnpackedDoubleElement(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedDoubleIterable(fieldNumber, values) {
    this.kernel_.addUnpackedDoubleIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedDoubleElement(fieldNumber, index, value) {
    this.kernel_.setPackedDoubleElement(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedDoubleIterable(fieldNumber, values) {
    this.kernel_.setPackedDoubleIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedDoubleElement(fieldNumber, index, value) {
    this.kernel_.setUnpackedDoubleElement(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedDoubleIterable(fieldNumber, values) {
    this.kernel_.setUnpackedDoubleIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedFixed32Element(fieldNumber, value) {
    this.kernel_.addPackedFixed32Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedFixed32Iterable(fieldNumber, values) {
    this.kernel_.addPackedFixed32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedFixed32Element(fieldNumber, value) {
    this.kernel_.addUnpackedFixed32Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedFixed32Iterable(fieldNumber, values) {
    this.kernel_.addUnpackedFixed32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedFixed32Element(fieldNumber, index, value) {
    this.kernel_.setPackedFixed32Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedFixed32Iterable(fieldNumber, values) {
    this.kernel_.setPackedFixed32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedFixed32Element(fieldNumber, index, value) {
    this.kernel_.setUnpackedFixed32Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedFixed32Iterable(fieldNumber, values) {
    this.kernel_.setUnpackedFixed32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addPackedFixed64Element(fieldNumber, value) {
    this.kernel_.addPackedFixed64Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addPackedFixed64Iterable(fieldNumber, values) {
    this.kernel_.addPackedFixed64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addUnpackedFixed64Element(fieldNumber, value) {
    this.kernel_.addUnpackedFixed64Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addUnpackedFixed64Iterable(fieldNumber, values) {
    this.kernel_.addUnpackedFixed64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedFixed64Element(fieldNumber, index, value) {
    this.kernel_.setPackedFixed64Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setPackedFixed64Iterable(fieldNumber, values) {
    this.kernel_.setPackedFixed64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedFixed64Element(fieldNumber, index, value) {
    this.kernel_.setUnpackedFixed64Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setUnpackedFixed64Iterable(fieldNumber, values) {
    this.kernel_.setUnpackedFixed64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedFloatElement(fieldNumber, value) {
    this.kernel_.addPackedFloatElement(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedFloatIterable(fieldNumber, values) {
    this.kernel_.addPackedFloatIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedFloatElement(fieldNumber, value) {
    this.kernel_.addUnpackedFloatElement(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedFloatIterable(fieldNumber, values) {
    this.kernel_.addUnpackedFloatIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedFloatElement(fieldNumber, index, value) {
    this.kernel_.setPackedFloatElement(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedFloatIterable(fieldNumber, values) {
    this.kernel_.setPackedFloatIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedFloatElement(fieldNumber, index, value) {
    this.kernel_.setUnpackedFloatElement(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedFloatIterable(fieldNumber, values) {
    this.kernel_.setUnpackedFloatIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedInt32Element(fieldNumber, value) {
    this.kernel_.addPackedInt32Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedInt32Iterable(fieldNumber, values) {
    this.kernel_.addPackedInt32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedInt32Element(fieldNumber, value) {
    this.kernel_.addUnpackedInt32Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedInt32Iterable(fieldNumber, values) {
    this.kernel_.addUnpackedInt32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedInt32Element(fieldNumber, index, value) {
    this.kernel_.setPackedInt32Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedInt32Iterable(fieldNumber, values) {
    this.kernel_.setPackedInt32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedInt32Element(fieldNumber, index, value) {
    this.kernel_.setUnpackedInt32Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedInt32Iterable(fieldNumber, values) {
    this.kernel_.setUnpackedInt32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addPackedInt64Element(fieldNumber, value) {
    this.kernel_.addPackedInt64Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addPackedInt64Iterable(fieldNumber, values) {
    this.kernel_.addPackedInt64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addUnpackedInt64Element(fieldNumber, value) {
    this.kernel_.addUnpackedInt64Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addUnpackedInt64Iterable(fieldNumber, values) {
    this.kernel_.addUnpackedInt64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedInt64Element(fieldNumber, index, value) {
    this.kernel_.setPackedInt64Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setPackedInt64Iterable(fieldNumber, values) {
    this.kernel_.setPackedInt64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedInt64Element(fieldNumber, index, value) {
    this.kernel_.setUnpackedInt64Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setUnpackedInt64Iterable(fieldNumber, values) {
    this.kernel_.setUnpackedInt64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedSfixed32Element(fieldNumber, value) {
    this.kernel_.addPackedSfixed32Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedSfixed32Iterable(fieldNumber, values) {
    this.kernel_.addPackedSfixed32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedSfixed32Element(fieldNumber, value) {
    this.kernel_.addUnpackedSfixed32Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedSfixed32Iterable(fieldNumber, values) {
    this.kernel_.addUnpackedSfixed32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedSfixed32Element(fieldNumber, index, value) {
    this.kernel_.setPackedSfixed32Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedSfixed32Iterable(fieldNumber, values) {
    this.kernel_.setPackedSfixed32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedSfixed32Element(fieldNumber, index, value) {
    this.kernel_.setUnpackedSfixed32Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedSfixed32Iterable(fieldNumber, values) {
    this.kernel_.setUnpackedSfixed32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addPackedSfixed64Element(fieldNumber, value) {
    this.kernel_.addPackedSfixed64Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addPackedSfixed64Iterable(fieldNumber, values) {
    this.kernel_.addPackedSfixed64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addUnpackedSfixed64Element(fieldNumber, value) {
    this.kernel_.addUnpackedSfixed64Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addUnpackedSfixed64Iterable(fieldNumber, values) {
    this.kernel_.addUnpackedSfixed64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedSfixed64Element(fieldNumber, index, value) {
    this.kernel_.setPackedSfixed64Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setPackedSfixed64Iterable(fieldNumber, values) {
    this.kernel_.setPackedSfixed64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedSfixed64Element(fieldNumber, index, value) {
    this.kernel_.setUnpackedSfixed64Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setUnpackedSfixed64Iterable(fieldNumber, values) {
    this.kernel_.setUnpackedSfixed64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedSint32Element(fieldNumber, value) {
    this.kernel_.addPackedSint32Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedSint32Iterable(fieldNumber, values) {
    this.kernel_.addPackedSint32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedSint32Element(fieldNumber, value) {
    this.kernel_.addUnpackedSint32Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedSint32Iterable(fieldNumber, values) {
    this.kernel_.addUnpackedSint32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedSint32Element(fieldNumber, index, value) {
    this.kernel_.setPackedSint32Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedSint32Iterable(fieldNumber, values) {
    this.kernel_.setPackedSint32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedSint32Element(fieldNumber, index, value) {
    this.kernel_.setUnpackedSint32Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedSint32Iterable(fieldNumber, values) {
    this.kernel_.setUnpackedSint32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addPackedSint64Element(fieldNumber, value) {
    this.kernel_.addPackedSint64Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addPackedSint64Iterable(fieldNumber, values) {
    this.kernel_.addPackedSint64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addUnpackedSint64Element(fieldNumber, value) {
    this.kernel_.addUnpackedSint64Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addUnpackedSint64Iterable(fieldNumber, values) {
    this.kernel_.addUnpackedSint64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedSint64Element(fieldNumber, index, value) {
    this.kernel_.setPackedSint64Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setPackedSint64Iterable(fieldNumber, values) {
    this.kernel_.setPackedSint64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedSint64Element(fieldNumber, index, value) {
    this.kernel_.setUnpackedSint64Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setUnpackedSint64Iterable(fieldNumber, values) {
    this.kernel_.setUnpackedSint64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addPackedUint32Element(fieldNumber, value) {
    this.kernel_.addPackedUint32Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addPackedUint32Iterable(fieldNumber, values) {
    this.kernel_.addPackedUint32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} value
   */
  addUnpackedUint32Element(fieldNumber, value) {
    this.kernel_.addUnpackedUint32Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  addUnpackedUint32Iterable(fieldNumber, values) {
    this.kernel_.addUnpackedUint32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedUint32Element(fieldNumber, index, value) {
    this.kernel_.setPackedUint32Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setPackedUint32Iterable(fieldNumber, values) {
    this.kernel_.setPackedUint32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {number} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedUint32Element(fieldNumber, index, value) {
    this.kernel_.setUnpackedUint32Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<number>} values
   */
  setUnpackedUint32Iterable(fieldNumber, values) {
    this.kernel_.setUnpackedUint32Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addPackedUint64Element(fieldNumber, value) {
    this.kernel_.addPackedUint64Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addPackedUint64Iterable(fieldNumber, values) {
    this.kernel_.addPackedUint64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  addUnpackedUint64Element(fieldNumber, value) {
    this.kernel_.addUnpackedUint64Element(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  addUnpackedUint64Iterable(fieldNumber, values) {
    this.kernel_.addUnpackedUint64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setPackedUint64Element(fieldNumber, index, value) {
    this.kernel_.setPackedUint64Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setPackedUint64Iterable(fieldNumber, values) {
    this.kernel_.setPackedUint64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!Int64} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setUnpackedUint64Element(fieldNumber, index, value) {
    this.kernel_.setUnpackedUint64Element(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!Int64>} values
   */
  setUnpackedUint64Iterable(fieldNumber, values) {
    this.kernel_.setUnpackedUint64Iterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!ByteString>} values
   */
  setRepeatedBytesIterable(fieldNumber, values) {
    this.kernel_.setRepeatedBytesIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<!ByteString>} values
   */
  addRepeatedBytesIterable(fieldNumber, values) {
    this.kernel_.addRepeatedBytesIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {!ByteString} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setRepeatedBytesElement(fieldNumber, index, value) {
    this.kernel_.setRepeatedBytesElement(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!ByteString} value
   */
  addRepeatedBytesElement(fieldNumber, value) {
    this.kernel_.addRepeatedBytesElement(fieldNumber, value);
  }


  /**
   * @param {number} fieldNumber
   * @param {!Iterable<string>} values
   */
  setRepeatedStringIterable(fieldNumber, values) {
    this.kernel_.setRepeatedStringIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<string>} values
   */
  addRepeatedStringIterable(fieldNumber, values) {
    this.kernel_.addRepeatedStringIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {number} index
   * @param {string} value
   * @throws {!Error} if index is out of range when check mode is critical
   */
  setRepeatedStringElement(fieldNumber, index, value) {
    this.kernel_.setRepeatedStringElement(fieldNumber, index, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {string} value
   */
  addRepeatedStringElement(fieldNumber, value) {
    this.kernel_.addRepeatedStringElement(fieldNumber, value);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<T>} values
   * @template T
   */
  setRepeatedMessageIterable(fieldNumber, values) {
    this.kernel_.setRepeatedMessageIterable(fieldNumber, values);
  }

  /**
   * @param {number} fieldNumber
   * @param {!Iterable<T>} values
   * @param {function(!Kernel):T} instanceCreator
   * @template T
   */
  addRepeatedMessageIterable(fieldNumber, values, instanceCreator) {
    this.kernel_.addRepeatedMessageIterable(
        fieldNumber, values, instanceCreator);
  }

  /**
   * @param {number} fieldNumber
   * @param {T} value
   * @param {function(!Kernel):T} instanceCreator
   * @param {number} index
   * @throws {!Error} if index is out of range when check mode is critical
   * @template T
   */
  setRepeatedMessageElement(fieldNumber, value, instanceCreator, index) {
    this.kernel_.setRepeatedMessageElement(
        fieldNumber, value, instanceCreator, index);
  }

  /**
   * @param {number} fieldNumber
   * @param {T} value
   * @param {function(!Kernel):T} instanceCreator
   * @template T
   */
  addRepeatedMessageElement(fieldNumber, value, instanceCreator) {
    this.kernel_.addRepeatedMessageElement(fieldNumber, value, instanceCreator);
  }
}

exports = TestMessage;
