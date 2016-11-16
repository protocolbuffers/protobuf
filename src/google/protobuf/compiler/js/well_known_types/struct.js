// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/* This code will be inserted into generated code for
 * google/protobuf/struct.proto. */

/**
 * Typedef representing plain JavaScript values that can go into a
 *     Struct.
 * @typedef {null|number|string|boolean|Array|Object}
 */
proto.google.protobuf.JavaScriptValue;


/**
 * Converts this Value object to a plain JavaScript value.
 * @return {?proto.google.protobuf.JavaScriptValue} a plain JavaScript
 *     value representing this Struct.
 */
proto.google.protobuf.Value.prototype.toJavaScript = function() {
  var kindCase = proto.google.protobuf.Value.KindCase;
  switch (this.getKindCase()) {
    case kindCase.NULL_VALUE:
      return null;
    case kindCase.NUMBER_VALUE:
      return this.getNumberValue();
    case kindCase.STRING_VALUE:
      return this.getStringValue();
    case kindCase.BOOL_VALUE:
      return this.getBoolValue();
    case kindCase.STRUCT_VALUE:
      return this.getStructValue().toJavaScript();
    case kindCase.LIST_VALUE:
      return this.getListValue().toJavaScript();
    default:
      throw new Error('Unexpected struct type');
  }
};


/**
 * Converts this JavaScript value to a new Value proto.
 * @param {!proto.google.protobuf.JavaScriptValue} value The value to
 *     convert.
 * @return {!proto.google.protobuf.Value} The newly constructed value.
 */
proto.google.protobuf.Value.fromJavaScript = function(value) {
  var ret = new proto.google.protobuf.Value();
  switch (goog.typeOf(value)) {
    case 'string':
      ret.setStringValue(/** @type {string} */ (value));
      break;
    case 'number':
      ret.setNumberValue(/** @type {number} */ (value));
      break;
    case 'boolean':
      ret.setBoolValue(/** @type {boolean} */ (value));
      break;
    case 'null':
      ret.setNullValue(proto.google.protobuf.NullValue.NULL_VALUE);
      break;
    case 'array':
      ret.setListValue(proto.google.protobuf.ListValue.fromJavaScript(
          /** @type{!Array} */ (value)));
      break;
    case 'object':
      ret.setStructValue(proto.google.protobuf.Struct.fromJavaScript(
          /** @type{!Object} */ (value)));
      break;
    default:
      throw new Error('Unexpected struct type.');
  }

  return ret;
};


/**
 * Converts this ListValue object to a plain JavaScript array.
 * @return {!Array} a plain JavaScript array representing this List.
 */
proto.google.protobuf.ListValue.prototype.toJavaScript = function() {
  var ret = [];
  var values = this.getValuesList();

  for (var i = 0; i < values.length; i++) {
    ret[i] = values[i].toJavaScript();
  }

  return ret;
};


/**
 * Constructs a ListValue protobuf from this plain JavaScript array.
 * @param {!Array} array a plain JavaScript array
 * @return {proto.google.protobuf.ListValue} a new ListValue object
 */
proto.google.protobuf.ListValue.fromJavaScript = function(array) {
  var ret = new proto.google.protobuf.ListValue();

  for (var i = 0; i < array.length; i++) {
    ret.addValues(proto.google.protobuf.Value.fromJavaScript(array[i]));
  }

  return ret;
};


/**
 * Converts this Struct object to a plain JavaScript object.
 * @return {!Object<string, !proto.google.protobuf.JavaScriptValue>} a plain
 *     JavaScript object representing this Struct.
 */
proto.google.protobuf.Struct.prototype.toJavaScript = function() {
  var ret = {};

  this.getFieldsMap().forEach(function(value, key) {
    ret[key] = value.toJavaScript();
  });

  return ret;
};


/**
 * Constructs a Struct protobuf from this plain JavaScript object.
 * @param {!Object} obj a plain JavaScript object
 * @return {proto.google.protobuf.Struct} a new Struct object
 */
proto.google.protobuf.Struct.fromJavaScript = function(obj) {
  var ret = new proto.google.protobuf.Struct();
  var map = ret.getFieldsMap();

  for (var property in obj) {
    var val = obj[property];
    map.set(property, proto.google.protobuf.Value.fromJavaScript(val));
  }

  return ret;
};
