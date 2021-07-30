goog.module('javascript.protobuf.conformance');

const ConformanceRequest = goog.require('proto.conformance.ConformanceRequest');
const ConformanceResponse = goog.require('proto.conformance.ConformanceResponse');
const TestAllTypesProto2 = goog.require('proto.conformance.TestAllTypesProto2');
const TestAllTypesProto3 = goog.require('proto.conformance.TestAllTypesProto3');
const WireFormat = goog.require('proto.conformance.WireFormat');
const base64 = goog.require('goog.crypt.base64');

/**
 * Creates a `proto.conformance.ConformanceResponse` response according to the
 * `proto.conformance.ConformanceRequest` request.
 * @param {!ConformanceRequest} request
 * @return {!ConformanceResponse} response
 */
function doTest(request) {
  const response = ConformanceResponse.createEmpty();

  if(request.getPayloadCase() === ConformanceRequest.PayloadCase.JSON_PAYLOAD) {
    response.setSkipped('Json is not supported as input format.');
    return response;
  }

  if(request.getPayloadCase() === ConformanceRequest.PayloadCase.TEXT_PAYLOAD) {
    response.setSkipped('Text format is not supported as input format.');
    return response;
  }

  if(request.getPayloadCase() === ConformanceRequest.PayloadCase.PAYLOAD_NOT_SET) {
    response.setRuntimeError('Request didn\'t have payload.');
    return response;
  }

  if(request.getPayloadCase() !== ConformanceRequest.PayloadCase.PROTOBUF_PAYLOAD) {
     throw new Error('Request didn\'t have accepted input format.');
  }

  if (request.getRequestedOutputFormat() === WireFormat.JSON) {
    response.setSkipped('Json is not supported as output format.');
    return response;
  }

  if (request.getRequestedOutputFormat() === WireFormat.TEXT_FORMAT) {
    response.setSkipped('Text format is not supported as output format.');
    return response;
  }

  if (request.getRequestedOutputFormat() === WireFormat.TEXT_FORMAT) {
    response.setRuntimeError('Unspecified output format');
    return response;
  }

  if (request.getRequestedOutputFormat() !== WireFormat.PROTOBUF) {
    throw new Error('Request didn\'t have accepted output format.');
  }

  if (request.getMessageType() === 'conformance.FailureSet') {
    response.setProtobufPayload(new ArrayBuffer(0));
  } else if (
      request.getMessageType() ===
      'protobuf_test_messages.proto2.TestAllTypesProto2') {
    try {
      const testMessage =
          TestAllTypesProto2.deserialize(request.getProtobufPayload());
      response.setProtobufPayload(testMessage.serialize());
    } catch (err) {
      response.setParseError(err.toString());
    }
  } else if (
      request.getMessageType() ===
      'protobuf_test_messages.proto3.TestAllTypesProto3') {
    try {
      const testMessage =
          TestAllTypesProto3.deserialize(request.getProtobufPayload());
      response.setProtobufPayload(testMessage.serialize());
    } catch (err) {
      response.setParseError(err.toString());
    }
  } else {
    throw new Error(
        `Payload message not supported: ${request.getMessageType()}.`);
  }

  return response;
}

/**
 * Same as doTest, but both request and response are in base64.
 * @param {string} base64Request
 * @return {string} response
 */
function runConformanceTest(base64Request) {
  const request =
      ConformanceRequest.deserialize(
          base64.decodeStringToUint8Array(base64Request).buffer);
  const response = doTest(request);
  return base64.encodeByteArray(new Uint8Array(response.serialize()));
}

// Needed for node test
exports.doTest = doTest;
// Needed for browser test
goog.exportSymbol('runConformanceTest', runConformanceTest);
