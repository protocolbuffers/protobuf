/**
 * @fileoverview Handwritten code of WireFormat.
 */
goog.module('proto.conformance.WireFormat');

/**
 * @enum {number}
 */
const WireFormat = {
  UNSPECIFIED: 0,
  PROTOBUF: 1,
  JSON: 2,
  TEXT_FORMAT: 4,
};

exports = WireFormat;
