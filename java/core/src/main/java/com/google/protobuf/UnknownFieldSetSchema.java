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

class UnknownFieldSetSchema extends UnknownFieldSchema<UnknownFieldSet, UnknownFieldSet.Builder> {

  public UnknownFieldSetSchema() {}

  @Override
  boolean shouldDiscardUnknownFields(Reader reader) {
    return reader.shouldDiscardUnknownFields();
  }

  @Override
  UnknownFieldSet.Builder newBuilder() {
    return UnknownFieldSet.newBuilder();
  }

  @Override
  void addVarint(UnknownFieldSet.Builder fields, int number, long value) {
    fields.mergeField(number, UnknownFieldSet.Field.newBuilder().addVarint(value).build());
  }

  @Override
  void addFixed32(UnknownFieldSet.Builder fields, int number, int value) {
    fields.mergeField(number, UnknownFieldSet.Field.newBuilder().addFixed32(value).build());
  }

  @Override
  void addFixed64(UnknownFieldSet.Builder fields, int number, long value) {
    fields.mergeField(number, UnknownFieldSet.Field.newBuilder().addFixed64(value).build());
  }

  @Override
  void addLengthDelimited(UnknownFieldSet.Builder fields, int number, ByteString value) {
    fields.mergeField(number, UnknownFieldSet.Field.newBuilder().addLengthDelimited(value).build());
  }

  @Override
  void addGroup(UnknownFieldSet.Builder fields, int number, UnknownFieldSet subFieldSet) {
    fields.mergeField(number, UnknownFieldSet.Field.newBuilder().addGroup(subFieldSet).build());
  }

  @Override
  UnknownFieldSet toImmutable(UnknownFieldSet.Builder fields) {
    return fields.build();
  }

  @Override
  void writeTo(UnknownFieldSet message, Writer writer) throws IOException {
    message.writeTo(writer);
  }

  @Override
  void writeAsMessageSetTo(UnknownFieldSet message, Writer writer) throws IOException {
    message.writeAsMessageSetTo(writer);
  }

  @Override
  UnknownFieldSet getFromMessage(Object message) {
    return ((GeneratedMessageV3) message).unknownFields;
  }

  @Override
  void setToMessage(Object message, UnknownFieldSet fields) {
    ((GeneratedMessageV3) message).unknownFields = fields;
  }

  @Override
  UnknownFieldSet.Builder getBuilderFromMessage(Object message) {
    return ((GeneratedMessageV3) message).unknownFields.toBuilder();
  }

  @Override
  void setBuilderToMessage(Object message, UnknownFieldSet.Builder builder) {
    ((GeneratedMessageV3) message).unknownFields = builder.build();
  }

  @Override
  void makeImmutable(Object message) {
    // Already immutable.
  }

  @Override
  UnknownFieldSet merge(UnknownFieldSet message, UnknownFieldSet other) {
    return message.toBuilder().mergeFrom(other).build();
  }

  @Override
  int getSerializedSize(UnknownFieldSet message) {
    return message.getSerializedSize();
  }

  @Override
  int getSerializedSizeAsMessageSet(UnknownFieldSet unknowns) {
    return unknowns.getSerializedSizeAsMessageSet();
  }
}
