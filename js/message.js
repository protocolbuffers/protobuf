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

/**
 * @fileoverview Definition of jspb.Message.
 *
 * @author mwr@google.com (Mark Rawling)
 */

goog.provide('jspb.ExtensionFieldInfo');
goog.provide('jspb.Message');

goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.crypt.base64');
goog.require('goog.json');

// Not needed in compilation units that have no protos with xids.
goog.forwardDeclare('xid.String');



/**
 * Stores information for a single extension field.
 *
 * For example, an extension field defined like so:
 *
 *     extend BaseMessage {
 *       optional MyMessage my_field = 123;
 *     }
 *
 * will result in an ExtensionFieldInfo object with these properties:
 *
 *     {
 *       fieldIndex: 123,
 *       fieldName: {my_field_renamed: 0},
 *       ctor: proto.example.MyMessage,
 *       toObjectFn: proto.example.MyMessage.toObject,
 *       isRepeated: 0
 *     }
 *
 * We include `toObjectFn` to allow the JSCompiler to perform dead-code removal
 * on unused toObject() methods.
 *
 * If an extension field is primitive, ctor and toObjectFn will be null.
 * isRepeated should be 0 or 1.
 *
 * binary{Reader,Writer}Fn and (if message type) binaryMessageSerializeFn are
 * always provided. binaryReaderFn and binaryWriterFn are references to the
 * appropriate methods on BinaryReader/BinaryWriter to read/write the value of
 * this extension, and binaryMessageSerializeFn is a reference to the message
 * class's .serializeBinary method, if available.
 *
 * @param {number} fieldNumber
 * @param {Object} fieldName This has the extension field name as a property.
 * @param {?function(new: jspb.Message, Array=)} ctor
 * @param {?function((boolean|undefined),!jspb.Message):!Object} toObjectFn
 * @param {number} isRepeated
 * @param {?function(number,?)=} opt_binaryReaderFn
 * @param {?function(number,?)|function(number,?,?,?,?,?)=} opt_binaryWriterFn
 * @param {?function(?,?)=} opt_binaryMessageSerializeFn
 * @param {?function(?,?)=} opt_binaryMessageDeserializeFn
 * @param {?boolean=} opt_isPacked
 * @constructor
 * @struct
 * @template T
 */
jspb.ExtensionFieldInfo = function(fieldNumber, fieldName, ctor, toObjectFn,
    isRepeated, opt_binaryReaderFn, opt_binaryWriterFn,
    opt_binaryMessageSerializeFn, opt_binaryMessageDeserializeFn,
    opt_isPacked) {
  /** @const */
  this.fieldIndex = fieldNumber;
  /** @const */
  this.fieldName = fieldName;
  /** @const */
  this.ctor = ctor;
  /** @const */
  this.toObjectFn = toObjectFn;
  /** @const */
  this.binaryReaderFn = opt_binaryReaderFn;
  /** @const */
  this.binaryWriterFn = opt_binaryWriterFn;
  /** @const */
  this.binaryMessageSerializeFn = opt_binaryMessageSerializeFn;
  /** @const */
  this.binaryMessageDeserializeFn = opt_binaryMessageDeserializeFn;
  /** @const */
  this.isRepeated = isRepeated;
  /** @const */
  this.isPacked = opt_isPacked;
};


/**
 * @return {boolean} Does this field represent a sub Message?
 */
jspb.ExtensionFieldInfo.prototype.isMessageType = function() {
  return !!this.ctor;
};


/**
 * Base class for all JsPb messages.
 * @constructor
 * @struct
 */
jspb.Message = function() {
};


/**
 * @define {boolean} Whether to generate toObject methods for objects. Turn
 *     this off, if you do not want toObject to be ever used in your project.
 *     When turning off this flag, consider adding a conformance test that bans
 *     calling toObject. Enabling this will disable the JSCompiler's ability to
 *     dead code eliminate fields used in protocol buffers that are never used
 *     in an application.
 */
goog.define('jspb.Message.GENERATE_TO_OBJECT', true);


/**
 * @define {boolean} Whether to generate fromObject methods for objects. Turn
 *     this off, if you do not want fromObject to be ever used in your project.
 *     When turning off this flag, consider adding a conformance test that bans
 *     calling fromObject. Enabling this might disable the JSCompiler's ability
 *     to dead code eliminate fields used in protocol buffers that are never
 *     used in an application.
 *     NOTE: By default no protos actually have a fromObject method. You need to
 *     add the jspb.generate_from_object options to the proto definition to
 *     activate the feature.
 *     By default this is enabled for test code only.
 */
goog.define('jspb.Message.GENERATE_FROM_OBJECT', !goog.DISALLOW_TEST_ONLY_CODE);


/**
 * @define {boolean} Turning on this flag does NOT change the behavior of JSPB
 *     and only affects private internal state. It may, however, break some
 *     tests that use naive deeply-equals algorithms, because using a proto
 *     mutates its internal state.
 *     Projects are advised to turn this flag always on.
 */
goog.define('jspb.Message.MINIMIZE_MEMORY_ALLOCATIONS', COMPILED);
// TODO(b/19419436) Turn this on by default.


/**
 * Does this browser support Uint8Aray typed arrays?
 * @type {boolean}
 * @private
 */
jspb.Message.SUPPORTS_UINT8ARRAY_ = (typeof Uint8Array == 'function');


/**
 * The internal data array.
 * @type {!Array}
 * @protected
 */
jspb.Message.prototype.array;


/**
 * Wrappers are the constructed instances of message-type fields. They are built
 * on demand from the raw array data. Includes message fields, repeated message
 * fields and extension message fields. Indexed by field number.
 * @type {Object}
 * @private
 */
jspb.Message.prototype.wrappers_;


/**
 * The object that contains extension fields, if any. This is an object that
 * maps from a proto field number to the field's value.
 * @type {Object}
 * @private
 */
jspb.Message.prototype.extensionObject_;


/**
 * Non-extension fields with a field number at or above the pivot are
 * stored in the extension object (in addition to all extension fields).
 * @type {number}
 * @private
 */
jspb.Message.prototype.pivot_;


/**
 * The JsPb message_id of this proto.
 * @type {string|undefined} the message id or undefined if this message
 *     has no id.
 * @private
 */
jspb.Message.prototype.messageId_;


/**
 * Repeated float or double fields which have been converted to include only
 * numbers and not strings holding "NaN", "Infinity" and "-Infinity".
 * @private {!Object<number,boolean>|undefined}
 */
jspb.Message.prototype.convertedFloatingPointFields_;


/**
 * The xid of this proto type (The same for all instances of a proto). Provides
 * a way to identify a proto by stable obfuscated name.
 * @see {xid}.
 * Available if {@link jspb.generate_xid} is added as a Message option to
 * a protocol buffer.
 * @const {!xid.String|undefined} The xid or undefined if message is
 *     annotated to generate the xid.
 */
jspb.Message.prototype.messageXid;



/**
 * Returns the JsPb message_id of this proto.
 * @return {string|undefined} the message id or undefined if this message
 *     has no id.
 */
jspb.Message.prototype.getJsPbMessageId = function() {
  return this.messageId_;
};


/**
 * An offset applied to lookups into this.array to account for the presence or
 * absence of a messageId at position 0. For response messages, this will be 0.
 * Otherwise, it will be -1 so that the first array position is not wasted.
 * @type {number}
 * @private
 */
jspb.Message.prototype.arrayIndexOffset_;


/**
 * Returns the index into msg.array at which the proto field with tag number
 * fieldNumber will be located.
 * @param {!jspb.Message} msg Message for which we're calculating an index.
 * @param {number} fieldNumber The field number.
 * @return {number} The index.
 * @private
 */
jspb.Message.getIndex_ = function(msg, fieldNumber) {
  return fieldNumber + msg.arrayIndexOffset_;
};


/**
 * Initializes a JsPb Message.
 * @param {!jspb.Message} msg The JsPb proto to modify.
 * @param {Array|undefined} data An initial data array.
 * @param {string|number} messageId For response messages, the message id or ''
 *     if no message id is specified. For non-response messages, 0.
 * @param {number} suggestedPivot The field number at which to start putting
 *     fields into the extension object. This is only used if data does not
 *     contain an extension object already. -1 if no extension object is
 *     required for this message type.
 * @param {Array<number>} repeatedFields The message's repeated fields.
 * @param {Array<!Array<number>>=} opt_oneofFields The fields belonging to
 *     each of the message's oneof unions.
 * @protected
 */
jspb.Message.initialize = function(
    msg, data, messageId, suggestedPivot, repeatedFields, opt_oneofFields) {
  msg.wrappers_ = jspb.Message.MINIMIZE_MEMORY_ALLOCATIONS ? null : {};
  if (!data) {
    data = messageId ? [messageId] : [];
  }
  msg.messageId_ = messageId ? String(messageId) : undefined;
  // If the messageId is 0, this message is not a response message, so we shift
  // array indices down by 1 so as not to waste the first position in the array,
  // which would otherwise go unused.
  msg.arrayIndexOffset_ = messageId === 0 ? -1 : 0;
  msg.array = data;
  jspb.Message.materializeExtensionObject_(msg, suggestedPivot);
  msg.convertedFloatingPointFields_ = {};

  if (repeatedFields) {
    for (var i = 0; i < repeatedFields.length; i++) {
      var fieldNumber = repeatedFields[i];
      if (fieldNumber < msg.pivot_) {
        var index = jspb.Message.getIndex_(msg, fieldNumber);
        msg.array[index] = msg.array[index] ||
            (jspb.Message.MINIMIZE_MEMORY_ALLOCATIONS ?
                jspb.Message.EMPTY_LIST_SENTINEL_ :
                []);
      } else {
        msg.extensionObject_[fieldNumber] =
            msg.extensionObject_[fieldNumber] ||
            (jspb.Message.MINIMIZE_MEMORY_ALLOCATIONS ?
                jspb.Message.EMPTY_LIST_SENTINEL_ :
                []);
      }
    }
  }

  if (opt_oneofFields && opt_oneofFields.length) {
    // Compute the oneof case for each union. This ensures only one value is
    // set in the union.
    goog.array.forEach(
        opt_oneofFields, goog.partial(jspb.Message.computeOneofCase, msg));
  }
};


/**
 * Used to mark empty repeated fields. Serializes to null when serialized
 * to JSON.
 * When reading a repeated field readers must check the return value against
 * this value and return and replace it with a new empty array if it is
 * present.
 * @private @const {!Object}
 */
jspb.Message.EMPTY_LIST_SENTINEL_ = goog.DEBUG && Object.freeze ?
    Object.freeze([]) :
    [];


/**
 * Ensures that the array contains an extension object if necessary.
 * If the array contains an extension object in its last position, then the
 * object is kept in place and its position is used as the pivot.  If not, then
 * create an extension object using suggestedPivot.  If suggestedPivot is -1,
 * we don't have an extension object at all, in which case all fields are stored
 * in the array.
 * @param {!jspb.Message} msg The JsPb proto to modify.
 * @param {number} suggestedPivot See description for initialize().
 * @private
 */
jspb.Message.materializeExtensionObject_ = function(msg, suggestedPivot) {
  if (msg.array.length) {
    var foundIndex = msg.array.length - 1;
    var obj = msg.array[foundIndex];
    // Normal fields are never objects, so we can be sure that if we find an
    // object here, then it's the extension object. However, we must ensure that
    // the object is not an array, since arrays are valid field values.
    // NOTE(lukestebbing): We avoid looking at .length to avoid a JIT bug
    // in Safari on iOS 8. See the description of CL/86511464 for details.
    if (obj && typeof obj == 'object' && !goog.isArray(obj)) {
      msg.pivot_ = foundIndex - msg.arrayIndexOffset_;
      msg.extensionObject_ = obj;
      return;
    }
  }
  // This complexity exists because we keep all extension fields in the
  // extensionObject_ regardless of proto field number. Changing this would
  // simplify the code here, but it would require changing the serialization
  // format from the server, which is not backwards compatible.
  // TODO(jshneier): Should we just treat extension fields the same as
  // non-extension fields, and select whether they appear in the object or in
  // the array purely based on tag number? This would allow simplifying all the
  // get/setExtension logic, but it would require the breaking change described
  // above.
  if (suggestedPivot > -1) {
    msg.pivot_ = suggestedPivot;
    var pivotIndex = jspb.Message.getIndex_(msg, suggestedPivot);
    if (!jspb.Message.MINIMIZE_MEMORY_ALLOCATIONS) {
      msg.extensionObject_ = msg.array[pivotIndex] = {};
    } else {
      // Initialize to null to avoid changing the shape of the proto when it
      // gets eventually set.
      msg.extensionObject_ = null;
    }
  } else {
    msg.pivot_ = Number.MAX_VALUE;
  }
};


/**
 * Creates an empty extensionObject_ if non exists.
 * @param {!jspb.Message} msg The JsPb proto to modify.
 * @private
 */
jspb.Message.maybeInitEmptyExtensionObject_ = function(msg) {
  var pivotIndex = jspb.Message.getIndex_(msg, msg.pivot_);
  if (!msg.array[pivotIndex]) {
    msg.extensionObject_ = msg.array[pivotIndex] = {};
  }
};


/**
 * Converts a JsPb repeated message field into an object list.
 * @param {!Array<T>} field The repeated message field to be
 *     converted.
 * @param {?function(boolean=): Object|
 *     function((boolean|undefined),T): Object} toObjectFn The toObject
 *     function for this field.  We need to pass this for effective dead code
 *     removal.
 * @param {boolean=} opt_includeInstance Whether to include the JSPB instance
 *     for transitional soy proto support: http://goto/soy-param-migration
 * @return {!Array<Object>} An array of converted message objects.
 * @template T
 */
jspb.Message.toObjectList = function(field, toObjectFn, opt_includeInstance) {
  // Not using goog.array.map in the generated code to keep it small.
  // And not using it here to avoid a function call.
  var result = [];
  for (var i = 0; i < field.length; i++) {
    result[i] = toObjectFn.call(field[i], opt_includeInstance,
      /** @type {!jspb.Message} */ (field[i]));
  }
  return result;
};


/**
 * Adds a proto's extension data to a Soy rendering object.
 * @param {!jspb.Message} proto The proto whose extensions to convert.
 * @param {!Object} obj The Soy object to add converted extension data to.
 * @param {!Object} extensions The proto class' registered extensions.
 * @param {function(this:?, jspb.ExtensionFieldInfo) : *} getExtensionFn
 *     The proto class' getExtension function. Passed for effective dead code
 *     removal.
 * @param {boolean=} opt_includeInstance Whether to include the JSPB instance
 *     for transitional soy proto support: http://goto/soy-param-migration
 */
jspb.Message.toObjectExtension = function(proto, obj, extensions,
    getExtensionFn, opt_includeInstance) {
  for (var fieldNumber in extensions) {
    var fieldInfo = extensions[fieldNumber];
    var value = getExtensionFn.call(proto, fieldInfo);
    if (value) {
      for (var name in fieldInfo.fieldName) {
        if (fieldInfo.fieldName.hasOwnProperty(name)) {
          break; // the compiled field name
        }
      }
      if (!fieldInfo.toObjectFn) {
        obj[name] = value;
      } else {
        if (fieldInfo.isRepeated) {
          obj[name] = jspb.Message.toObjectList(
              /** @type {!Array<jspb.Message>} */ (value),
              fieldInfo.toObjectFn, opt_includeInstance);
        } else {
          obj[name] = fieldInfo.toObjectFn(opt_includeInstance, value);
        }
      }
    }
  }
};


/**
 * Writes a proto's extension data to a binary-format output stream.
 * @param {!jspb.Message} proto The proto whose extensions to convert.
 * @param {*} writer The binary-format writer to write to.
 * @param {!Object} extensions The proto class' registered extensions.
 * @param {function(jspb.ExtensionFieldInfo) : *} getExtensionFn The proto
 *     class' getExtension function. Passed for effective dead code removal.
 */
jspb.Message.serializeBinaryExtensions = function(proto, writer, extensions,
    getExtensionFn) {
  for (var fieldNumber in extensions) {
    var fieldInfo = extensions[fieldNumber];
    // The old codegen doesn't add the extra fields to ExtensionFieldInfo, so we
    // need to gracefully error-out here rather than produce a null dereference
    // below.
    if (!fieldInfo.binaryWriterFn) {
      throw new Error('Message extension present that was generated ' +
                      'without binary serialization support');
    }
    var value = getExtensionFn.call(proto, fieldInfo);
    if (value) {
      if (fieldInfo.isMessageType()) {
        // If the message type of the extension was generated without binary
        // support, there may not be a binary message serializer function, and
        // we can't know when we codegen the extending message that the extended
        // message may require binary support, so we can *only* catch this error
        // here, at runtime (and this decoupled codegen is the whole point of
        // extensions!).
        if (fieldInfo.binaryMessageSerializeFn) {
          fieldInfo.binaryWriterFn.call(writer, fieldInfo.fieldIndex,
              value, fieldInfo.binaryMessageSerializeFn);
        } else {
          throw new Error('Message extension present holding submessage ' +
                          'without binary support enabled, and message is ' +
                          'being serialized to binary format');
        }
      } else {
        fieldInfo.binaryWriterFn.call(writer, fieldInfo.fieldIndex, value);
      }
    }
  }
};


/**
 * Reads an extension field from the given reader and, if a valid extension,
 * sets the extension value.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {{skipField:function(),getFieldNumber:function():number}} reader
 * @param {!Object} extensions The extensions object.
 * @param {function(jspb.ExtensionFieldInfo)} getExtensionFn
 * @param {function(jspb.ExtensionFieldInfo, ?)} setExtensionFn
 */
jspb.Message.readBinaryExtension = function(msg, reader, extensions,
    getExtensionFn, setExtensionFn) {
  var fieldInfo = extensions[reader.getFieldNumber()];
  if (!fieldInfo) {
    reader.skipField();
    return;
  }
  if (!fieldInfo.binaryReaderFn) {
    throw new Error('Deserializing extension whose generated code does not ' +
                    'support binary format');
  }

  var value;
  if (fieldInfo.isMessageType()) {
    value = new fieldInfo.ctor();
    fieldInfo.binaryReaderFn.call(
        reader, value, fieldInfo.binaryMessageDeserializeFn);
  } else {
    // All other types.
    value = fieldInfo.binaryReaderFn.call(reader);
  }

  if (fieldInfo.isRepeated && !fieldInfo.isPacked) {
    var currentList = getExtensionFn.call(msg, fieldInfo);
    if (!currentList) {
      setExtensionFn.call(msg, fieldInfo, [value]);
    } else {
      currentList.push(value);
    }
  } else {
    setExtensionFn.call(msg, fieldInfo, value);
  }
};


/**
 * Gets the value of a non-extension field.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {number} fieldNumber The field number.
 * @return {string|number|boolean|Uint8Array|Array|null|undefined}
 * The field's value.
 * @protected
 */
jspb.Message.getField = function(msg, fieldNumber) {
  if (fieldNumber < msg.pivot_) {
    var index = jspb.Message.getIndex_(msg, fieldNumber);
    var val = msg.array[index];
    if (val === jspb.Message.EMPTY_LIST_SENTINEL_) {
      return msg.array[index] = [];
    }
    return val;
  } else {
    var val = msg.extensionObject_[fieldNumber];
    if (val === jspb.Message.EMPTY_LIST_SENTINEL_) {
      return msg.extensionObject_[fieldNumber] = [];
    }
    return val;
  }
};


/**
 * Gets the value of an optional float or double field.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {number} fieldNumber The field number.
 * @return {?number|undefined} The field's value.
 * @protected
 */
jspb.Message.getOptionalFloatingPointField = function(msg, fieldNumber) {
  var value = jspb.Message.getField(msg, fieldNumber);
  // Converts "NaN", "Infinity" and "-Infinity" to their corresponding numbers.
  return value == null ? value : +value;
};


/**
 * Gets the value of a repeated float or double field.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {number} fieldNumber The field number.
 * @return {!Array<number>} The field's value.
 * @protected
 */
jspb.Message.getRepeatedFloatingPointField = function(msg, fieldNumber) {
  var values = jspb.Message.getField(msg, fieldNumber);
  if (!msg.convertedFloatingPointFields_) {
    msg.convertedFloatingPointFields_ = {};
  }
  if (!msg.convertedFloatingPointFields_[fieldNumber]) {
    for (var i = 0; i < values.length; i++) {
      // Converts "NaN", "Infinity" and "-Infinity" to their corresponding
      // numbers.
      values[i] = +values[i];
    }
    msg.convertedFloatingPointFields_[fieldNumber] = true;
  }
  return /** @type {!Array<number>} */ (values);
};


/**
 * Coerce a 'bytes' field to a base 64 string.
 * @param {string|Uint8Array|null} value
 * @return {?string} The field's coerced value.
 */
jspb.Message.bytesAsB64 = function(value) {
  if (value == null || goog.isString(value)) {
    return value;
  }
  if (jspb.Message.SUPPORTS_UINT8ARRAY_ && value instanceof Uint8Array) {
    return goog.crypt.base64.encodeByteArray(value);
  }
  goog.asserts.fail('Cannot coerce to b64 string: ' + goog.typeOf(value));
  return null;
};


/**
 * Coerce a 'bytes' field to a Uint8Array byte buffer.
 * Note that Uint8Array is not supported on IE versions before 10 nor on Opera
 * Mini. @see http://caniuse.com/Uint8Array
 * @param {string|Uint8Array|null} value
 * @return {?Uint8Array} The field's coerced value.
 */
jspb.Message.bytesAsU8 = function(value) {
  if (value == null || value instanceof Uint8Array) {
    return value;
  }
  if (goog.isString(value)) {
    return goog.crypt.base64.decodeStringToUint8Array(value);
  }
  goog.asserts.fail('Cannot coerce to Uint8Array: ' + goog.typeOf(value));
  return null;
};


/**
 * Coerce a repeated 'bytes' field to an array of base 64 strings.
 * Note: the returned array should be treated as immutable.
 * @param {!Array<string>|!Array<!Uint8Array>} value
 * @return {!Array<string?>} The field's coerced value.
 */
jspb.Message.bytesListAsB64 = function(value) {
  jspb.Message.assertConsistentTypes_(value);
  if (!value.length || goog.isString(value[0])) {
    return /** @type {!Array<string>} */ (value);
  }
  return goog.array.map(value, jspb.Message.bytesAsB64);
};


/**
 * Coerce a repeated 'bytes' field to an array of Uint8Array byte buffers.
 * Note: the returned array should be treated as immutable.
 * Note that Uint8Array is not supported on IE versions before 10 nor on Opera
 * Mini. @see http://caniuse.com/Uint8Array
 * @param {!Array<string>|!Array<!Uint8Array>} value
 * @return {!Array<Uint8Array?>} The field's coerced value.
 */
jspb.Message.bytesListAsU8 = function(value) {
  jspb.Message.assertConsistentTypes_(value);
  if (!value.length || value[0] instanceof Uint8Array) {
    return /** @type {!Array<!Uint8Array>} */ (value);
  }
  return goog.array.map(value, jspb.Message.bytesAsU8);
};


/**
 * Asserts that all elements of an array are of the same type.
 * @param {Array?} array The array to test.
 * @private
 */
jspb.Message.assertConsistentTypes_ = function(array) {
  if (goog.DEBUG && array && array.length > 1) {
    var expected = goog.typeOf(array[0]);
    goog.array.forEach(array, function(e) {
      if (goog.typeOf(e) != expected) {
        goog.asserts.fail('Inconsistent type in JSPB repeated field array. ' +
            'Got ' + goog.typeOf(e) + ' expected ' + expected);
      }
    });
  }
};


/**
 * Gets the value of a non-extension primitive field, with proto3 (non-nullable
 * primitives) semantics. Returns `defaultValue` if the field is not otherwise
 * set.
 * @template T
 * @param {!jspb.Message} msg A jspb proto.
 * @param {number} fieldNumber The field number.
 * @param {T} defaultValue The default value.
 * @return {T} The field's value.
 * @protected
 */
jspb.Message.getFieldProto3 = function(msg, fieldNumber, defaultValue) {
  var value = jspb.Message.getField(msg, fieldNumber);
  if (value == null) {
    return defaultValue;
  } else {
    return value;
  }
};


/**
 * Sets the value of a non-extension field.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {number} fieldNumber The field number.
 * @param {string|number|boolean|Uint8Array|Array|undefined} value New value
 * @protected
 */
jspb.Message.setField = function(msg, fieldNumber, value) {
  if (fieldNumber < msg.pivot_) {
    msg.array[jspb.Message.getIndex_(msg, fieldNumber)] = value;
  } else {
    msg.extensionObject_[fieldNumber] = value;
  }
};


/**
 * Sets the value of a field in a oneof union and clears all other fields in
 * the union.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {number} fieldNumber The field number.
 * @param {!Array<number>} oneof The fields belonging to the union.
 * @param {string|number|boolean|Uint8Array|Array|undefined} value New value
 * @protected
 */
jspb.Message.setOneofField = function(msg, fieldNumber, oneof, value) {
  var currentCase = jspb.Message.computeOneofCase(msg, oneof);
  if (currentCase && currentCase !== fieldNumber && value !== undefined) {
    if (msg.wrappers_ && currentCase in msg.wrappers_) {
      msg.wrappers_[currentCase] = undefined;
    }
    jspb.Message.setField(msg, currentCase, undefined);
  }
  jspb.Message.setField(msg, fieldNumber, value);
};


/**
 * Computes the selection in a oneof group for the given message, ensuring
 * only one field is set in the process.
 *
 * According to the protobuf language guide (
 * https://developers.google.com/protocol-buffers/docs/proto#oneof), "if the
 * parser encounters multiple members of the same oneof on the wire, only the
 * last member seen is used in the parsed message." Since JSPB serializes
 * messages to a JSON array, the "last member seen" will always be the field
 * with the greatest field number (directly corresponding to the greatest
 * array index).
 *
 * @param {!jspb.Message} msg A jspb proto.
 * @param {!Array<number>} oneof The field numbers belonging to the union.
 * @return {number} The field number currently set in the union, or 0 if none.
 * @protected
 */
jspb.Message.computeOneofCase = function(msg, oneof) {
  var oneofField;
  var oneofValue;

  goog.array.forEach(oneof, function(fieldNumber) {
    var value = jspb.Message.getField(msg, fieldNumber);
    if (goog.isDefAndNotNull(value)) {
      oneofField = fieldNumber;
      oneofValue = value;
      jspb.Message.setField(msg, fieldNumber, undefined);
    }
  });

  if (oneofField) {
    // NB: We know the value is unique, so we can call jspb.Message.setField
    // directly instead of jpsb.Message.setOneofField. Also, setOneofField
    // calls this function.
    jspb.Message.setField(msg, oneofField, oneofValue);
    return oneofField;
  }

  return 0;
};


/**
 * Gets and wraps a proto field on access.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {function(new:jspb.Message, Array)} ctor Constructor for the field.
 * @param {number} fieldNumber The field number.
 * @param {number=} opt_required True (1) if this is a required field.
 * @return {jspb.Message} The field as a jspb proto.
 * @protected
 */
jspb.Message.getWrapperField = function(msg, ctor, fieldNumber, opt_required) {
  // TODO(mwr): Consider copying data and/or arrays.
  if (!msg.wrappers_) {
    msg.wrappers_ = {};
  }
  if (!msg.wrappers_[fieldNumber]) {
    var data = /** @type {Array} */ (jspb.Message.getField(msg, fieldNumber));
    if (opt_required || data) {
      // TODO(mwr): Remove existence test for always valid default protos.
      msg.wrappers_[fieldNumber] = new ctor(data);
    }
  }
  return /** @type {jspb.Message} */ (msg.wrappers_[fieldNumber]);
};


/**
 * Gets and wraps a repeated proto field on access.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {function(new:jspb.Message, Array)} ctor Constructor for the field.
 * @param {number} fieldNumber The field number.
 * @return {Array<!jspb.Message>} The repeated field as an array of protos.
 * @protected
 */
jspb.Message.getRepeatedWrapperField = function(msg, ctor, fieldNumber) {
  if (!msg.wrappers_) {
    msg.wrappers_ = {};
  }
  if (!msg.wrappers_[fieldNumber]) {
    var data = jspb.Message.getField(msg, fieldNumber);
    for (var wrappers = [], i = 0; i < data.length; i++) {
      wrappers[i] = new ctor(data[i]);
    }
    msg.wrappers_[fieldNumber] = wrappers;
  }
  var val = msg.wrappers_[fieldNumber];
  if (val == jspb.Message.EMPTY_LIST_SENTINEL_) {
    val = msg.wrappers_[fieldNumber] = [];
  }
  return /** @type {Array<!jspb.Message>} */ (val);
};


/**
 * Sets a proto field and syncs it to the backing array.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {number} fieldNumber The field number.
 * @param {jspb.Message|undefined} value A new value for this proto field.
 * @protected
 */
jspb.Message.setWrapperField = function(msg, fieldNumber, value) {
  if (!msg.wrappers_) {
    msg.wrappers_ = {};
  }
  var data = value ? value.toArray() : value;
  msg.wrappers_[fieldNumber] = value;
  jspb.Message.setField(msg, fieldNumber, data);
};


/**
 * Sets a proto field in a oneof union and syncs it to the backing array.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {number} fieldNumber The field number.
 * @param {!Array<number>} oneof The fields belonging to the union.
 * @param {jspb.Message|undefined} value A new value for this proto field.
 * @protected
 */
jspb.Message.setOneofWrapperField = function(msg, fieldNumber, oneof, value) {
  if (!msg.wrappers_) {
    msg.wrappers_ = {};
  }
  var data = value ? value.toArray() : value;
  msg.wrappers_[fieldNumber] = value;
  jspb.Message.setOneofField(msg, fieldNumber, oneof, data);
};


/**
 * Sets a repeated proto field and syncs it to the backing array.
 * @param {!jspb.Message} msg A jspb proto.
 * @param {number} fieldNumber The field number.
 * @param {Array<!jspb.Message>|undefined} value An array of protos.
 * @protected
 */
jspb.Message.setRepeatedWrapperField = function(msg, fieldNumber, value) {
  if (!msg.wrappers_) {
    msg.wrappers_ = {};
  }
  value = value || [];
  for (var data = [], i = 0; i < value.length; i++) {
    data[i] = value[i].toArray();
  }
  msg.wrappers_[fieldNumber] = value;
  jspb.Message.setField(msg, fieldNumber, data);
};


/**
 * Converts a JsPb repeated message field into a map. The map will contain
 * protos unless an optional toObject function is given, in which case it will
 * contain objects suitable for Soy rendering.
 * @param {!Array<T>} field The repeated message field to be
 *     converted.
 * @param {function() : string?} mapKeyGetterFn The function to get the key of
 *     the map.
 * @param {?function(boolean=): Object|
 *     function((boolean|undefined),T): Object} opt_toObjectFn The
 *     toObject function for this field. We need to pass this for effective
 *     dead code removal.
 * @param {boolean=} opt_includeInstance Whether to include the JSPB instance
 *     for transitional soy proto support: http://goto/soy-param-migration
 * @return {!Object.<string, Object>} A map of proto or Soy objects.
 * @template T
 */
jspb.Message.toMap = function(
    field, mapKeyGetterFn, opt_toObjectFn, opt_includeInstance) {
  var result = {};
  for (var i = 0; i < field.length; i++) {
    result[mapKeyGetterFn.call(field[i])] = opt_toObjectFn ?
        opt_toObjectFn.call(field[i], opt_includeInstance,
            /** @type {!jspb.Message} */ (field[i])) : field[i];
  }
  return result;
};


/**
 * Returns the internal array of this proto.
 * <p>Note: If you use this array to construct a second proto, the content
 * would then be partially shared between the two protos.
 * @return {!Array} The proto represented as an array.
 */
jspb.Message.prototype.toArray = function() {
  return this.array;
};




/**
 * Creates a string representation of the internal data array of this proto.
 * <p>NOTE: This string is *not* suitable for use in server requests.
 * @return {string} A string representation of this proto.
 * @override
 */
jspb.Message.prototype.toString = function() {
  return this.array.toString();
};


/**
 * Gets the value of the extension field from the extended object.
 * @param {jspb.ExtensionFieldInfo.<T>} fieldInfo Specifies the field to get.
 * @return {T} The value of the field.
 * @template T
 */
jspb.Message.prototype.getExtension = function(fieldInfo) {
  if (!this.extensionObject_) {
    return undefined;
  }
  if (!this.wrappers_) {
    this.wrappers_ = {};
  }
  var fieldNumber = fieldInfo.fieldIndex;
  if (fieldInfo.isRepeated) {
    if (fieldInfo.isMessageType()) {
      if (!this.wrappers_[fieldNumber]) {
        this.wrappers_[fieldNumber] =
            goog.array.map(this.extensionObject_[fieldNumber] || [],
                function(arr) {
                  return new fieldInfo.ctor(arr);
                });
      }
      return this.wrappers_[fieldNumber];
    } else {
      return this.extensionObject_[fieldNumber];
    }
  } else {
    if (fieldInfo.isMessageType()) {
      if (!this.wrappers_[fieldNumber] && this.extensionObject_[fieldNumber]) {
        this.wrappers_[fieldNumber] = new fieldInfo.ctor(
            /** @type {Array|undefined} */ (
                this.extensionObject_[fieldNumber]));
      }
      return this.wrappers_[fieldNumber];
    } else {
      return this.extensionObject_[fieldNumber];
    }
  }
};


/**
 * Sets the value of the extension field in the extended object.
 * @param {jspb.ExtensionFieldInfo} fieldInfo Specifies the field to set.
 * @param {jspb.Message|string|Uint8Array|number|boolean|Array?} value The value
 *     to set.
 */
jspb.Message.prototype.setExtension = function(fieldInfo, value) {
  if (!this.wrappers_) {
    this.wrappers_ = {};
  }
  jspb.Message.maybeInitEmptyExtensionObject_(this);
  var fieldNumber = fieldInfo.fieldIndex;
  if (fieldInfo.isRepeated) {
    value = value || [];
    if (fieldInfo.isMessageType()) {
      this.wrappers_[fieldNumber] = value;
      this.extensionObject_[fieldNumber] = goog.array.map(
          /** @type {Array<jspb.Message>} */ (value), function(msg) {
        return msg.toArray();
      });
    } else {
      this.extensionObject_[fieldNumber] = value;
    }
  } else {
    if (fieldInfo.isMessageType()) {
      this.wrappers_[fieldNumber] = value;
      this.extensionObject_[fieldNumber] = value ? value.toArray() : value;
    } else {
      this.extensionObject_[fieldNumber] = value;
    }
  }
};


/**
 * Creates a difference object between two messages.
 *
 * The result will contain the top-level fields of m2 that differ from those of
 * m1 at any level of nesting. No data is cloned, the result object will
 * share its top-level elements with m2 (but not with m1).
 *
 * Note that repeated fields should not have null/undefined elements, but if
 * they do, this operation will treat repeated fields of different length as
 * the same if the only difference between them is due to trailing
 * null/undefined values.
 *
 * @param {!jspb.Message} m1 The first message object.
 * @param {!jspb.Message} m2 The second message object.
 * @return {!jspb.Message} The difference returned as a proto message.
 *     Note that the returned message may be missing required fields. This is
 *     currently tolerated in Js, but would cause an error if you tried to
 *     send such a proto to the server. You can access the raw difference
 *     array with result.toArray().
 * @throws {Error} If the messages are responses with different types.
 */
jspb.Message.difference = function(m1, m2) {
  if (!(m1 instanceof m2.constructor)) {
    throw new Error('Messages have different types.');
  }
  var arr1 = m1.toArray();
  var arr2 = m2.toArray();
  var res = [];
  var start = 0;
  var length = arr1.length > arr2.length ? arr1.length : arr2.length;
  if (m1.getJsPbMessageId()) {
    res[0] = m1.getJsPbMessageId();
    start = 1;
  }
  for (var i = start; i < length; i++) {
    if (!jspb.Message.compareFields(arr1[i], arr2[i])) {
      res[i] = arr2[i];
    }
  }
  return new m1.constructor(res);
};


/**
 * Tests whether two messages are equal.
 * @param {jspb.Message|undefined} m1 The first message object.
 * @param {jspb.Message|undefined} m2 The second message object.
 * @return {boolean} true if both messages are null/undefined, or if both are
 *     of the same type and have the same field values.
 */
jspb.Message.equals = function(m1, m2) {
  return m1 == m2 || (!!(m1 && m2) && (m1 instanceof m2.constructor) &&
      jspb.Message.compareFields(m1.toArray(), m2.toArray()));
};


/**
 * Compares two message extension fields recursively.
 * @param {!Object} extension1 The first field.
 * @param {!Object} extension2 The second field.
 * @return {boolean} true if the extensions are null/undefined, or otherwise
 *     equal.
 */
jspb.Message.compareExtensions = function(extension1, extension2) {
  extension1 = extension1 || {};
  extension2 = extension2 || {};

  var keys = {};
  for (var name in extension1) {
    keys[name] = 0;
  }
  for (var name in extension2) {
    keys[name] = 0;
  }
  for (name in keys) {
    if (!jspb.Message.compareFields(extension1[name], extension2[name])) {
      return false;
    }
  }
  return true;
};


/**
 * Compares two message fields recursively.
 * @param {*} field1 The first field.
 * @param {*} field2 The second field.
 * @return {boolean} true if the fields are null/undefined, or otherwise equal.
 */
jspb.Message.compareFields = function(field1, field2) {
  // If the fields are trivially equal, they're equal.
  if (field1 == field2) return true;

  // If the fields aren't trivially equal and one of them isn't an object,
  // they can't possibly be equal.
  if (!goog.isObject(field1) || !goog.isObject(field2)) {
    return false;
  }

  // We have two objects. If they're different types, they're not equal.
  field1 = /** @type {!Object} */(field1);
  field2 = /** @type {!Object} */(field2);
  if (field1.constructor != field2.constructor) return false;

  // If both are Uint8Arrays, compare them element-by-element.
  if (jspb.Message.SUPPORTS_UINT8ARRAY_ && field1.constructor === Uint8Array) {
    var bytes1 = /** @type {!Uint8Array} */(field1);
    var bytes2 = /** @type {!Uint8Array} */(field2);
    if (bytes1.length != bytes2.length) return false;
    for (var i = 0; i < bytes1.length; i++) {
      if (bytes1[i] != bytes2[i]) return false;
    }
    return true;
  }

  // If they're both Arrays, compare them element by element except for the
  // optional extension objects at the end, which we compare separately.
  if (field1.constructor === Array) {
    var extension1 = undefined;
    var extension2 = undefined;

    var length = Math.max(field1.length, field2.length);
    for (var i = 0; i < length; i++) {
      var val1 = field1[i];
      var val2 = field2[i];

      if (val1 && (val1.constructor == Object)) {
        goog.asserts.assert(extension1 === undefined);
        goog.asserts.assert(i === field1.length - 1);
        extension1 = val1;
        val1 = undefined;
      }

      if (val2 && (val2.constructor == Object)) {
        goog.asserts.assert(extension2 === undefined);
        goog.asserts.assert(i === field2.length - 1);
        extension2 = val2;
        val2 = undefined;
      }

      if (!jspb.Message.compareFields(val1, val2)) {
        return false;
      }
    }

    if (extension1 || extension2) {
      extension1 = extension1 || {};
      extension2 = extension2 || {};
      return jspb.Message.compareExtensions(extension1, extension2);
    }

    return true;
  }

  // If they're both plain Objects (i.e. extensions), compare them as
  // extensions.
  if (field1.constructor === Object) {
    return jspb.Message.compareExtensions(field1, field2);
  }

  throw new Error('Invalid type in JSPB array');
};


/**
 * Static clone function. NOTE: A type-safe method called "cloneMessage" exists
 * on each generated JsPb class. Do not call this function directly.
 * @param {!jspb.Message} msg A message to clone.
 * @return {!jspb.Message} A deep clone of the given message.
 */
jspb.Message.clone = function(msg) {
  // Although we could include the wrappers, we leave them out here.
  return jspb.Message.cloneMessage(msg);
};


/**
 * @param {!jspb.Message} msg A message to clone.
 * @return {!jspb.Message} A deep clone of the given message.
 * @protected
 */
jspb.Message.cloneMessage = function(msg) {
  // Although we could include the wrappers, we leave them out here.
  return new msg.constructor(jspb.Message.clone_(msg.toArray()));
};


/**
 * Takes 2 messages of the same type and copies the contents of the first
 * message into the second. After this the 2 messages will equals in terms of
 * value semantics but share no state. All data in the destination message will
 * be overridden.
 *
 * @param {MESSAGE} fromMessage Message that will be copied into toMessage.
 * @param {MESSAGE} toMessage Message which will receive a copy of fromMessage
 *     as its contents.
 * @template MESSAGE
 */
jspb.Message.copyInto = function(fromMessage, toMessage) {
  goog.asserts.assertInstanceof(fromMessage, jspb.Message);
  goog.asserts.assertInstanceof(toMessage, jspb.Message);
  goog.asserts.assert(fromMessage.constructor == toMessage.constructor,
      'Copy source and target message should have the same type.');
  var copyOfFrom = jspb.Message.clone(fromMessage);

  var to = toMessage.toArray();
  var from = copyOfFrom.toArray();

  // Empty destination in case it has more values at the end of the array.
  to.length = 0;
  // and then copy everything from the new to the existing message.
  for (var i = 0; i < from.length; i++) {
    to[i] = from[i];
  }

  // This is either null or empty for a fresh copy.
  toMessage.wrappers_ = copyOfFrom.wrappers_;
  // Just a reference into the shared array.
  toMessage.extensionObject_ = copyOfFrom.extensionObject_;
};


/**
 * Helper for cloning an internal JsPb object.
 * @param {!Object} obj A JsPb object, eg, a field, to be cloned.
 * @return {!Object} A clone of the input object.
 * @private
 */
jspb.Message.clone_ = function(obj) {
  var o;
  if (goog.isArray(obj)) {
    // Allocate array of correct size.
    var clonedArray = new Array(obj.length);
    // Use array iteration where possible because it is faster than for-in.
    for (var i = 0; i < obj.length; i++) {
      if ((o = obj[i]) != null) {
        clonedArray[i] = typeof o == 'object' ? jspb.Message.clone_(o) : o;
      }
    }
    return clonedArray;
  }
  var clone = {};
  for (var key in obj) {
    if ((o = obj[key]) != null) {
      clone[key] = typeof o == 'object' ? jspb.Message.clone_(o) : o;
    }
  }
  return clone;
};


/**
 * Registers a JsPb message type id with its constructor.
 * @param {string} id The id for this type of message.
 * @param {Function} constructor The message constructor.
 */
jspb.Message.registerMessageType = function(id, constructor) {
  jspb.Message.registry_[id] = constructor;
  // This is needed so we can later access messageId directly on the contructor,
  // otherwise it is not available due to 'property collapsing' by the compiler.
  constructor.messageId = id;
};


/**
 * The registry of message ids to message constructors.
 * @private
 */
jspb.Message.registry_ = {};


/**
 * The extensions registered on MessageSet. This is a map of extension
 * field number to field info object. This should be considered as a
 * private API.
 *
 * This is similar to [jspb class name].extensions object for
 * non-MessageSet. We special case MessageSet so that we do not need
 * to goog.require MessageSet from classes that extends MessageSet.
 *
 * @type {!Object.<number, jspb.ExtensionFieldInfo>}
 */
jspb.Message.messageSetExtensions = {};
