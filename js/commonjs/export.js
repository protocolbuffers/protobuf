/**
 * @fileoverview Export symbols needed by generated code in CommonJS style.
 *
 * This effectively is our canonical list of what we publicly export from
 * the google-protobuf.js file that we build at distribution time.
 */

goog.require('goog.object');
goog.require('jspb.BinaryReader');
goog.require('jspb.BinaryWriter');
goog.require('jspb.ExtensionFieldBinaryInfo');
goog.require('jspb.ExtensionFieldInfo');
goog.require('jspb.Message');

exports.Message = jspb.Message;
exports.BinaryReader = jspb.BinaryReader;
exports.BinaryWriter = jspb.BinaryWriter;
exports.ExtensionFieldInfo = jspb.ExtensionFieldInfo;
exports.ExtensionFieldBinaryInfo = jspb.ExtensionFieldBinaryInfo;

// These are used by generated code but should not be used directly by clients.
exports.exportSymbol = goog.exportSymbol;
exports.inherits = goog.inherits;
exports.object = {extend: goog.object.extend};
