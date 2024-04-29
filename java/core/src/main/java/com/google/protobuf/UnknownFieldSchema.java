// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.io.IOException;

@ExperimentalApi
@CheckReturnValue
abstract class UnknownFieldSchema<T, B> {

  /** Whether unknown fields should be dropped. */
  abstract boolean shouldDiscardUnknownFields(Reader reader);

  /** Adds a varint value to the unknown fields. */
  abstract void addVarint(B fields, int number, long value);

  /** Adds a fixed32 value to the unknown fields. */
  abstract void addFixed32(B fields, int number, int value);

  /** Adds a fixed64 value to the unknown fields. */
  abstract void addFixed64(B fields, int number, long value);

  /** Adds a length delimited value to the unknown fields. */
  abstract void addLengthDelimited(B fields, int number, ByteString value);

  /** Adds a group value to the unknown fields. */
  abstract void addGroup(B fields, int number, T subFieldSet);

  /** Create a new builder for unknown fields. */
  abstract B newBuilder();

  /** Returns an immutable instance of the field container. */
  abstract T toImmutable(B fields);

  /**
   * Sets the unknown fields into the message. Caller must take care of the mutability of the
   * fields.
   */
  abstract void setToMessage(Object message, T fields);

  /** Get the unknown fields from the message. */
  abstract T getFromMessage(Object message);

  /** Returns a builder for unknown fields in the message. */
  abstract B getBuilderFromMessage(Object message);

  /** Sets an unknown field builder into the message. */
  abstract void setBuilderToMessage(Object message, B builder);

  /** Marks unknown fields as immutable. */
  abstract void makeImmutable(Object message);

  /** Merges one field into the unknown fields. */
  final boolean mergeOneFieldFrom(B unknownFields, Reader reader) throws IOException {
    int tag = reader.getTag();
    int fieldNumber = WireFormat.getTagFieldNumber(tag);
    switch (WireFormat.getTagWireType(tag)) {
      case WireFormat.WIRETYPE_VARINT:
        addVarint(unknownFields, fieldNumber, reader.readInt64());
        return true;
      case WireFormat.WIRETYPE_FIXED32:
        addFixed32(unknownFields, fieldNumber, reader.readFixed32());
        return true;
      case WireFormat.WIRETYPE_FIXED64:
        addFixed64(unknownFields, fieldNumber, reader.readFixed64());
        return true;
      case WireFormat.WIRETYPE_LENGTH_DELIMITED:
        addLengthDelimited(unknownFields, fieldNumber, reader.readBytes());
        return true;
      case WireFormat.WIRETYPE_START_GROUP:
        final B subFields = newBuilder();
        int endGroupTag = WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP);
        mergeFrom(subFields, reader);
        if (endGroupTag != reader.getTag()) {
          throw InvalidProtocolBufferException.invalidEndTag();
        }
        addGroup(unknownFields, fieldNumber, toImmutable(subFields));
        return true;
      case WireFormat.WIRETYPE_END_GROUP:
        return false;
      default:
        throw InvalidProtocolBufferException.invalidWireType();
    }
  }

  final void mergeFrom(B unknownFields, Reader reader) throws IOException {
    while (true) {
      if (reader.getFieldNumber() == Reader.READ_DONE
          || !mergeOneFieldFrom(unknownFields, reader)) {
        break;
      }
    }
  }

  abstract void writeTo(T unknownFields, Writer writer) throws IOException;

  abstract void writeAsMessageSetTo(T unknownFields, Writer writer) throws IOException;

  /** Merges {@code source} into {@code destination} and returns the merged instance. */
  abstract T merge(T destination, T source);

  /** Get the serialized size for message set serialization. */
  abstract int getSerializedSizeAsMessageSet(T message);

  abstract int getSerializedSize(T unknowns);
}
