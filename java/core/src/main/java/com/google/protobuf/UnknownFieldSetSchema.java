// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
