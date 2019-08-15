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

goog.provide('jspb.Map');

goog.require('goog.asserts');

goog.requireType('jspb.BinaryReader');
goog.requireType('jspb.BinaryWriter');



/**
 * Constructs a new Map. A Map is a container that is used to implement map
 * fields on message objects. It closely follows the ES6 Map API; however,
 * it is distinct because we do not want to depend on external polyfills or
 * on ES6 itself.
 *
 * This constructor should only be called from generated message code. It is not
 * intended for general use by library consumers.
 *
 * @template K, V
 *
 * @param {!Array<!Array<?>>} arr
 *
 * @param {?function(new:V, ?=)=} opt_valueCtor
 *    The constructor for type V, if type V is a message type.
 *
 * @constructor
 * @struct
 */
jspb.Map = function(arr, opt_valueCtor) {
  /** @const @private */
  this.arr_ = arr;
  /** @const @private */
  this.valueCtor_ = opt_valueCtor;

  /** @type {!Object<string, !jspb.Map.Entry_<K,V>>} @private */
  this.map_ = {};

  /**
   * Is `this.arr_ updated with respect to `this.map_`?
   * @type {boolean}
   */
  this.arrClean = true;

  if (this.arr_.length > 0) {
    this.loadFromArray_();
  }
};


/**
 * Load initial content from underlying array.
 * @private
 */
jspb.Map.prototype.loadFromArray_ = function() {
  for (var i = 0; i < this.arr_.length; i++) {
    var record = this.arr_[i];
    var key = record[0];
    var value = record[1];
    this.map_[key.toString()] = new jspb.Map.Entry_(key, value);
  }
  this.arrClean = true;
};


/**
 * Synchronize content to underlying array, if needed, and return it.
 * @return {!Array<!Array<!Object>>}
 */
jspb.Map.prototype.toArray = function() {
  if (this.arrClean) {
    if (this.valueCtor_) {
      // We need to recursively sync maps in submessages to their arrays.
      var m = this.map_;
      for (var p in m) {
        if (Object.prototype.hasOwnProperty.call(m, p)) {
          var valueWrapper = /** @type {?jspb.Message} */ (m[p].valueWrapper);
          if (valueWrapper) {
            valueWrapper.toArray();
          }
        }
      }
    }
  } else {
    // Delete all elements.
    this.arr_.length = 0;
    var strKeys = this.stringKeys_();
    // Output keys in deterministic (sorted) order.
    strKeys.sort();
    for (var i = 0; i < strKeys.length; i++) {
      var entry = this.map_[strKeys[i]];
      var valueWrapper = /** @type {?jspb.Message} */ (entry.valueWrapper);
      if (valueWrapper) {
        valueWrapper.toArray();
      }
      this.arr_.push([entry.key, entry.value]);
    }
    this.arrClean = true;
  }
  return this.arr_;
};


/**
 * Returns the map formatted as an array of key-value pairs, suitable for the
 * toObject() form of a message.
 *
 * @param {boolean=} includeInstance Whether to include the JSPB instance for
 *    transitional soy proto support: http://goto/soy-param-migration
 * @param {function((boolean|undefined),V):!Object=} valueToObject
 *    The static toObject() method, if V is a message type.
 * @return {!Array<!Array<!Object>>}
 */
jspb.Map.prototype.toObject = function(includeInstance, valueToObject) {
  var rawArray = this.toArray();
  var entries = [];
  for (var i = 0; i < rawArray.length; i++) {
    var entry = this.map_[rawArray[i][0].toString()];
    this.wrapEntry_(entry);
    var valueWrapper = /** @type {V|undefined} */ (entry.valueWrapper);
    if (valueWrapper) {
      goog.asserts.assert(valueToObject);
      entries.push([entry.key, valueToObject(includeInstance, valueWrapper)]);
    } else {
      entries.push([entry.key, entry.value]);
    }
  }
  return entries;
};


/**
 * Returns a Map from the given array of key-value pairs when the values are of
 * message type. The values in the array must match the format returned by their
 * message type's toObject() method.
 *
 * @template K, V
 * @param {!Array<!Array<!Object>>} entries
 * @param {function(new:V,?=)} valueCtor
 *    The constructor for type V.
 * @param {function(!Object):V} valueFromObject
 *    The fromObject function for type V.
 * @return {!jspb.Map<K, V>}
 */
jspb.Map.fromObject = function(entries, valueCtor, valueFromObject) {
  var result = new jspb.Map([], valueCtor);
  for (var i = 0; i < entries.length; i++) {
    var key = entries[i][0];
    var value = valueFromObject(entries[i][1]);
    result.set(key, value);
  }
  return result;
};


/**
 * Helper: an IteratorIterable over an array.
 * @template T
 * @param {!Array<T>} arr the array
 * @implements {IteratorIterable<T>}
 * @constructor @struct
 * @private
 */
jspb.Map.ArrayIteratorIterable_ = function(arr) {
  /** @type {number} @private */
  this.idx_ = 0;

  /** @const @private */
  this.arr_ = arr;
};


/** @override @final */
jspb.Map.ArrayIteratorIterable_.prototype.next = function() {
  if (this.idx_ < this.arr_.length) {
    return {done: false, value: this.arr_[this.idx_++]};
  } else {
    return {done: true, value: undefined};
  }
};

if (typeof(Symbol) != 'undefined') {
  /** @override */
  jspb.Map.ArrayIteratorIterable_.prototype[Symbol.iterator] = function() {
    return this;
  };
}


/**
 * Returns the map's length (number of key/value pairs).
 * @return {number}
 */
jspb.Map.prototype.getLength = function() {
  return this.stringKeys_().length;
};


/**
 * Clears the map.
 */
jspb.Map.prototype.clear = function() {
  this.map_ = {};
  this.arrClean = false;
};


/**
 * Deletes a particular key from the map.
 * N.B.: differs in name from ES6 Map's `delete` because IE8 does not support
 * reserved words as property names.
 * @this {jspb.Map}
 * @param {K} key
 * @return {boolean} Whether any entry with this key was deleted.
 */
jspb.Map.prototype.del = function(key) {
  var keyValue = key.toString();
  var hadKey = this.map_.hasOwnProperty(keyValue);
  delete this.map_[keyValue];
  this.arrClean = false;
  return hadKey;
};


/**
 * Returns an array of [key, value] pairs in the map.
 *
 * This is redundant compared to the plain entries() method, but we provide this
 * to help out Angular 1.x users.  Still evaluating whether this is the best
 * option.
 *
 * @return {!Array<!Array<K|V>>}
 */
jspb.Map.prototype.getEntryList = function() {
  var entries = [];
  var strKeys = this.stringKeys_();
  strKeys.sort();
  for (var i = 0; i < strKeys.length; i++) {
    var entry = this.map_[strKeys[i]];
    entries.push([entry.key, entry.value]);
  }
  return entries;
};


/**
 * Returns an iterator-iterable over [key, value] pairs in the map.
 * Closure compiler sadly doesn't support tuples, ie. Iterator<[K,V]>.
 * @return {!IteratorIterable<!Array<K|V>>} The iterator-iterable.
 */
jspb.Map.prototype.entries = function() {
  var entries = [];
  var strKeys = this.stringKeys_();
  strKeys.sort();
  for (var i = 0; i < strKeys.length; i++) {
    var entry = this.map_[strKeys[i]];
    entries.push([entry.key, this.wrapEntry_(entry)]);
  }
  return new jspb.Map.ArrayIteratorIterable_(entries);
};


/**
 * Returns an iterator-iterable over keys in the map.
 * @return {!IteratorIterable<K>} The iterator-iterable.
 */
jspb.Map.prototype.keys = function() {
  var keys = [];
  var strKeys = this.stringKeys_();
  strKeys.sort();
  for (var i = 0; i < strKeys.length; i++) {
    var entry = this.map_[strKeys[i]];
    keys.push(entry.key);
  }
  return new jspb.Map.ArrayIteratorIterable_(keys);
};


/**
 * Returns an iterator-iterable over values in the map.
 * @return {!IteratorIterable<V>} The iterator-iterable.
 */
jspb.Map.prototype.values = function() {
  var values = [];
  var strKeys = this.stringKeys_();
  strKeys.sort();
  for (var i = 0; i < strKeys.length; i++) {
    var entry = this.map_[strKeys[i]];
    values.push(this.wrapEntry_(entry));
  }
  return new jspb.Map.ArrayIteratorIterable_(values);
};


/**
 * Iterates over entries in the map, calling a function on each.
 * @template T
 * @param {function(this:T, V, K, ?jspb.Map<K, V>)} cb
 * @param {T=} opt_thisArg
 */
jspb.Map.prototype.forEach = function(cb, opt_thisArg) {
  var strKeys = this.stringKeys_();
  strKeys.sort();
  for (var i = 0; i < strKeys.length; i++) {
    var entry = this.map_[strKeys[i]];
    cb.call(opt_thisArg, this.wrapEntry_(entry), entry.key, this);
  }
};


/**
 * Sets a key in the map to the given value.
 * @param {K} key The key
 * @param {V} value The value
 * @return {!jspb.Map<K,V>}
 */
jspb.Map.prototype.set = function(key, value) {
  var entry = new jspb.Map.Entry_(key);
  if (this.valueCtor_) {
    entry.valueWrapper = value;
    // .toArray() on a message returns a reference to the underlying array
    // rather than a copy.
    entry.value = value.toArray();
  } else {
    entry.value = value;
  }
  this.map_[key.toString()] = entry;
  this.arrClean = false;
  return this;
};


/**
 * Helper: lazily construct a wrapper around an entry, if needed, and return the
 * user-visible type.
 * @param {!jspb.Map.Entry_<K,V>} entry
 * @return {V}
 * @private
 */
jspb.Map.prototype.wrapEntry_ = function(entry) {
  if (this.valueCtor_) {
    if (!entry.valueWrapper) {
      entry.valueWrapper = new this.valueCtor_(entry.value);
    }
    return /** @type {V} */ (entry.valueWrapper);
  } else {
    return entry.value;
  }
};


/**
 * Gets the value corresponding to a key in the map.
 * @param {K} key
 * @return {V|undefined} The value, or `undefined` if key not present
 */
jspb.Map.prototype.get = function(key) {
  var keyValue = key.toString();
  var entry = this.map_[keyValue];
  if (entry) {
    return this.wrapEntry_(entry);
  } else {
    return undefined;
  }
};


/**
 * Determines whether the given key is present in the map.
 * @param {K} key
 * @return {boolean} `true` if the key is present
 */
jspb.Map.prototype.has = function(key) {
  var keyValue = key.toString();
  return (keyValue in this.map_);
};


/**
 * Write this Map field in wire format to a BinaryWriter, using the given field
 * number.
 * @param {number} fieldNumber
 * @param {!jspb.BinaryWriter} writer
 * @param {function(this:jspb.BinaryWriter,number,K)} keyWriterFn
 *     The method on BinaryWriter that writes type K to the stream.
 * @param {function(this:jspb.BinaryWriter,number,V,?=)|
 *          function(this:jspb.BinaryWriter,number,V,?)} valueWriterFn
 *     The method on BinaryWriter that writes type V to the stream.  May be
 *     writeMessage, in which case the second callback arg form is used.
 * @param {function(V,!jspb.BinaryWriter)=} opt_valueWriterCallback
 *    The BinaryWriter serialization callback for type V, if V is a message
 *    type.
 */
jspb.Map.prototype.serializeBinary = function(
    fieldNumber, writer, keyWriterFn, valueWriterFn, opt_valueWriterCallback) {
  var strKeys = this.stringKeys_();
  strKeys.sort();
  for (var i = 0; i < strKeys.length; i++) {
    var entry = this.map_[strKeys[i]];
    writer.beginSubMessage(fieldNumber);
    keyWriterFn.call(writer, 1, entry.key);
    if (this.valueCtor_) {
      valueWriterFn.call(writer, 2, this.wrapEntry_(entry),
                         opt_valueWriterCallback);
    } else {
      /** @type {function(this:jspb.BinaryWriter,number,?)} */ (valueWriterFn)
          .call(writer, 2, entry.value);
    }
    writer.endSubMessage();
  }
};


/**
 * Read one key/value message from the given BinaryReader. Compatible as the
 * `reader` callback parameter to jspb.BinaryReader.readMessage, to be called
 * when a key/value pair submessage is encountered. If the Key is undefined,
 * we should default it to 0.
 * @template K, V
 * @param {!jspb.Map} map
 * @param {!jspb.BinaryReader} reader
 * @param {function(this:jspb.BinaryReader):K} keyReaderFn
 *     The method on BinaryReader that reads type K from the stream.
 *
 * @param {function(this:jspb.BinaryReader):V|
 *          function(this:jspb.BinaryReader,V,
 *                  function(V,!jspb.BinaryReader))} valueReaderFn
 *    The method on BinaryReader that reads type V from the stream. May be
 *    readMessage, in which case the second callback arg form is used.
 *
 * @param {?function(V,!jspb.BinaryReader)=} opt_valueReaderCallback
 *    The BinaryReader parsing callback for type V, if V is a message type
 *
 * @param {K=} opt_defaultKey
 *    The default value for the type of map keys. Accepting map
 *    entries with unset keys is required for maps to be backwards compatible
 *    with the repeated message representation described here: goo.gl/zuoLAC
 *
 */
jspb.Map.deserializeBinary = function(map, reader, keyReaderFn, valueReaderFn,
                                      opt_valueReaderCallback, opt_defaultKey) {
  var key = opt_defaultKey;
  var value = undefined;

  while (reader.nextField()) {
    if (reader.isEndGroup()) {
      break;
    }
    var field = reader.getFieldNumber();

    if (field == 1) {
      // Key.
      key = keyReaderFn.call(reader);
    } else if (field == 2) {
      // Value.
      if (map.valueCtor_) {
        goog.asserts.assert(opt_valueReaderCallback);
        value = new map.valueCtor_();
        valueReaderFn.call(reader, value, opt_valueReaderCallback);
      } else {
        value =
            (/** @type {function(this:jspb.BinaryReader):?} */ (valueReaderFn))
                .call(reader);
      }
    }
  }

  goog.asserts.assert(key != undefined);
  goog.asserts.assert(value != undefined);
  map.set(key, value);
};


/**
 * Helper: compute the list of all stringified keys in the underlying Object
 * map.
 * @return {!Array<string>}
 * @private
 */
jspb.Map.prototype.stringKeys_ = function() {
  var m = this.map_;
  var ret = [];
  for (var p in m) {
    if (Object.prototype.hasOwnProperty.call(m, p)) {
      ret.push(p);
    }
  }
  return ret;
};



/**
 * @param {K} key The entry's key.
 * @param {V=} opt_value The entry's value wrapper.
 * @constructor
 * @struct
 * @template K, V
 * @private
 */
jspb.Map.Entry_ = function(key, opt_value) {
  /** @const {K} */
  this.key = key;

  // The JSPB-serializable value.  For primitive types this will be of type V.
  // For message types it will be an array.
  /** @type {V} */
  this.value = opt_value;

  // Only used for submessage values.
  /** @type {V} */
  this.valueWrapper = undefined;
};
