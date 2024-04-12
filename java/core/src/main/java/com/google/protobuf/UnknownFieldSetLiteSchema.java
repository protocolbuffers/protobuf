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

package com.google.protobuf;

import java.io.IOException;

@CheckReturnValue
class UnknownFieldSetLiteSchema
    extends UnknownFieldSchema<UnknownFieldSetLite, UnknownFieldSetLite> {

  UnknownFieldSetLiteSchema() {}

  @Override
  boolean shouldDiscardUnknownFields(Reader reader) {
    // We never drop unknown fields in lite.
    return false;
  }

  @Override
  UnknownFieldSetLite newBuilder() {
    return UnknownFieldSetLite.newInstance();
  }

  @Override
  void addVarint(UnknownFieldSetLite fields, int number, long value) {
    fields.storeField(WireFormat.makeTag(number, WireFormat.WIRETYPE_VARINT), value);
  }

  @Override
  void addFixed32(UnknownFieldSetLite fields, int number, int value) {
    fields.storeField(WireFormat.makeTag(number, WireFormat.WIRETYPE_FIXED32), value);
  }

  @Override
  void addFixed64(UnknownFieldSetLite fields, int number, long value) {
    fields.storeField(WireFormat.makeTag(number, WireFormat.WIRETYPE_FIXED64), value);
  }

  @Override
  void addLengthDelimited(UnknownFieldSetLite fields, int number, ByteString value) {
    fields.storeField(WireFormat.makeTag(number, WireFormat.WIRETYPE_LENGTH_DELIMITED), value);
  }

  @Override
  void addGroup(UnknownFieldSetLite fields, int number, UnknownFieldSetLite subFieldSet) {
    fields.storeField(WireFormat.makeTag(number, WireFormat.WIRETYPE_START_GROUP), subFieldSet);
  }

  @Override
  UnknownFieldSetLite toImmutable(UnknownFieldSetLite fields) {
    fields.makeImmutable();
    return fields;
  }

  @Override
  void setToMessage(Object message, UnknownFieldSetLite fields) {
    ((GeneratedMessageLite<?, ?>) message).unknownFields = fields;
  }

  @Override
  UnknownFieldSetLite getFromMessage(Object message) {
    return ((GeneratedMessageLite<?, ?>) message).unknownFields;
  }

  @Override
  UnknownFieldSetLite getBuilderFromMessage(Object message) {
    UnknownFieldSetLite unknownFields = getFromMessage(message);
    // When parsing into a lite message object, its UnknownFieldSet is either the default instance
    // or mutable. It can't be in a state where it's immutable but not default instance.
    if (unknownFields == UnknownFieldSetLite.getDefaultInstance()) {
      unknownFields = UnknownFieldSetLite.newInstance();
      setToMessage(message, unknownFields);
    }
    return unknownFields;
  }

  @Override
  void setBuilderToMessage(Object message, UnknownFieldSetLite fields) {
    setToMessage(message, fields);
  }

  @Override
  void makeImmutable(Object message) {
    getFromMessage(message).makeImmutable();
  }

  @Override
  void writeTo(UnknownFieldSetLite fields, Writer writer) throws IOException {
    fields.writeTo(writer);
  }

  @Override
  void writeAsMessageSetTo(UnknownFieldSetLite fields, Writer writer) throws IOException {
    fields.writeAsMessageSetTo(writer);
  }

  @Override
  UnknownFieldSetLite merge(UnknownFieldSetLite target, UnknownFieldSetLite source) {
    if (UnknownFieldSetLite.getDefaultInstance().equals(source)) {
      return target;
    }
    if (UnknownFieldSetLite.getDefaultInstance().equals(target)) {
      return UnknownFieldSetLite.mutableCopyOf(target, source);
    }
    return target.mergeFrom(source);
  }

  @Override
  int getSerializedSize(UnknownFieldSetLite unknowns) {
    return unknowns.getSerializedSize();
  }

  @Override
  int getSerializedSizeAsMessageSet(UnknownFieldSetLite unknowns) {
    return unknowns.getSerializedSizeAsMessageSet();
  }
}
