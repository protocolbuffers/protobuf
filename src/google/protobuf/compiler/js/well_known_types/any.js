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
 * google/protobuf/any.proto. */

/**
 * Returns the type name contained in this instance, if any.
 * @return {string|undefined}
 */
proto.google.protobuf.Any.prototype.getTypeName = function() {
  return this.getTypeUrl().split('/').pop();
};


/**
 * Packs the given message instance into this Any.
 * @param {!Uint8Array} serialized The serialized data to pack.
 * @param {string} name The type name of this message object.
 * @param {string=} opt_typeUrlPrefix the type URL prefix.
 */
proto.google.protobuf.Any.prototype.pack = function(serialized, name,
                                                    opt_typeUrlPrefix) {
  if (!opt_typeUrlPrefix) {
    opt_typeUrlPrefix = 'type.googleapis.com/';
  }

  if (opt_typeUrlPrefix.substr(-1) != '/') {
    this.setTypeUrl(opt_typeUrlPrefix + '/' + name);
  } else {
    this.setTypeUrl(opt_typeUrlPrefix + name);
  }

  this.setValue(serialized);
};


/**
 * @template T
 * Unpacks this Any into the given message object.
 * @param {function(Uint8Array):T} deserialize Function that will deserialize
 *     the binary data properly.
 * @param {string} name The expected type name of this message object.
 * @return {?T} If the name matched the expected name, returns the deserialized
 *     object, otherwise returns null.
 */
proto.google.protobuf.Any.prototype.unpack = function(deserialize, name) {
  if (this.getTypeName() == name) {
    return deserialize(this.getValue_asU8());
  } else {
    return null;
  }
};
