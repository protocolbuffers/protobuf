/**
 * @fileoverview Handwritten code of ConformanceRequest.
 */
goog.module('proto.conformance.ConformanceRequest');

const Kernel = goog.require('protobuf.runtime.Kernel');
const WireFormat = goog.require('proto.conformance.WireFormat');

/**
 * Handwritten code of conformance.ConformanceRequest.
 * This is used to send request from the conformance test runner to the testee.
 * Check //third_party/protobuf/testing/protobuf/conformance/conformance.proto
 * for more details.
 * @final
 */
class ConformanceRequest {
  /**
   * @param {!ArrayBuffer} bytes
   * @private
   */
  constructor(bytes) {
    /** @private @const {!Kernel} */
    this.accessor_ = Kernel.fromArrayBuffer(bytes);
  }

  /**
   * Create a request instance with the given bytes data.
   * @param {!ArrayBuffer} bytes
   * @return {!ConformanceRequest}
   */
  static deserialize(bytes) {
    return new ConformanceRequest(bytes);
  }

  /**
   * Gets the protobuf_payload.
   * @return {!ArrayBuffer}
   */
  getProtobufPayload() {
    return this.accessor_.getBytesWithDefault(1).toArrayBuffer();
  }

  /**
   * Gets the requested_output_format.
   * @return {!WireFormat}
   */
  getRequestedOutputFormat() {
    return /** @type {!WireFormat} */ (this.accessor_.getInt32WithDefault(3));
  }

  /**
   * Gets the message_type.
   * @return {string}
   */
  getMessageType() {
    return this.accessor_.getStringWithDefault(4);
  }

  /**
   * Gets the oneof case for payload field.
   * This implementation assumes only one field in a oneof group is set.
   * @return {!ConformanceRequest.PayloadCase}
   */
  getPayloadCase() {
    if (this.accessor_.hasFieldNumber(1)) {
      return /** @type {!ConformanceRequest.PayloadCase} */ (
          ConformanceRequest.PayloadCase.PROTOBUF_PAYLOAD);
    } else if (this.accessor_.hasFieldNumber(2)) {
      return /** @type {!ConformanceRequest.PayloadCase} */ (
          ConformanceRequest.PayloadCase.JSON_PAYLOAD);
    } else if (this.accessor_.hasFieldNumber(8)) {
      return /** @type {!ConformanceRequest.PayloadCase} */ (
          ConformanceRequest.PayloadCase.TEXT_PAYLOAD);
    } else {
      return /** @type {!ConformanceRequest.PayloadCase} */ (
          ConformanceRequest.PayloadCase.PAYLOAD_NOT_SET);
    }
  }
}

/**
 * @enum {number}
 */
ConformanceRequest.PayloadCase = {
  PAYLOAD_NOT_SET: 0,
  PROTOBUF_PAYLOAD: 1,
  JSON_PAYLOAD: 2,
  TEXT_PAYLOAD: 8,
};

exports = ConformanceRequest;
