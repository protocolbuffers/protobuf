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
