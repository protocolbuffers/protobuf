/**
 * @fileoverview Handwritten code of ConformanceResponse.
 */
goog.module('proto.conformance.ConformanceResponse');

const ByteString = goog.require('protobuf.ByteString');
const Kernel = goog.require('protobuf.runtime.Kernel');

/**
 * Handwritten code of conformance.ConformanceResponse.
 * This is used to send response from the conformance testee to the test runner.
 * Check //third_party/protobuf/testing/protobuf/conformance/conformance.proto
 * for more details.
 * @final
 */
class ConformanceResponse {
  /**
   * @param {!ArrayBuffer} bytes
   * @private
   */
  constructor(bytes) {
    /** @private @const {!Kernel} */
    this.accessor_ = Kernel.fromArrayBuffer(bytes);
  }

  /**
   * Create an empty response instance.
   * @return {!ConformanceResponse}
   */
  static createEmpty() {
    return new ConformanceResponse(new ArrayBuffer(0));
  }

  /**
   * Sets parse_error field.
   * @param {string} value
   */
  setParseError(value) {
    this.accessor_.setString(1, value);
  }

  /**
   * Sets runtime_error field.
   * @param {string} value
   */
  setRuntimeError(value) {
    this.accessor_.setString(2, value);
  }

  /**
   * Sets protobuf_payload field.
   * @param {!ArrayBuffer} value
   */
  setProtobufPayload(value) {
    const bytesString = ByteString.fromArrayBuffer(value);
    this.accessor_.setBytes(3, bytesString);
  }

  /**
   * Sets skipped field.
   * @param {string} value
   */
  setSkipped(value) {
    this.accessor_.setString(5, value);
  }

  /**
   * Serializes into binary data.
   * @return {!ArrayBuffer}
   */
  serialize() {
    return this.accessor_.serialize();
  }
}

exports = ConformanceResponse;
