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

import static com.google.protobuf.Proto3Manifest.FIELD_LENGTH;
import static com.google.protobuf.Proto3Manifest.offset;
import static com.google.protobuf.Proto3Manifest.type;

import java.util.List;

/** Abstract base class for all proto3-based schemas. */
abstract class AbstractProto3Schema<T> implements Schema<T> {
  protected final Proto3Manifest manifest;

  public AbstractProto3Schema(Proto3Manifest manifest) {
    this.manifest = manifest;
  }

  @SuppressWarnings("unchecked")
  @Override
  public final void writeTo(T message, Writer writer) {
    for (long pos = manifest.address; pos < manifest.limit; pos += FIELD_LENGTH) {
      final int typeAndOffset = manifest.typeAndOffsetAt(pos);

      // Benchmarks have shown that switching on a byte is faster than an enum.
      switch (type(typeAndOffset)) {
        case 0: //DOUBLE:
          SchemaUtil.writeDouble(
              manifest.numberAt(pos), UnsafeUtil.getDouble(message, offset(typeAndOffset)), writer);
          break;
        case 1: //FLOAT:
          SchemaUtil.writeFloat(
              manifest.numberAt(pos), UnsafeUtil.getFloat(message, offset(typeAndOffset)), writer);
          break;
        case 2: //INT64:
          SchemaUtil.writeInt64(
              manifest.numberAt(pos), UnsafeUtil.getLong(message, offset(typeAndOffset)), writer);
          break;
        case 3: //UINT64:
          SchemaUtil.writeUInt64(
              manifest.numberAt(pos), UnsafeUtil.getLong(message, offset(typeAndOffset)), writer);
          break;
        case 4: //INT32:
          SchemaUtil.writeInt32(
              manifest.numberAt(pos), UnsafeUtil.getInt(message, offset(typeAndOffset)), writer);
          break;
        case 5: //FIXED64:
          SchemaUtil.writeFixed64(
              manifest.numberAt(pos), UnsafeUtil.getLong(message, offset(typeAndOffset)), writer);
          break;
        case 6: //FIXED32:
          SchemaUtil.writeFixed32(
              manifest.numberAt(pos), UnsafeUtil.getInt(message, offset(typeAndOffset)), writer);
          break;
        case 7: //BOOL:
          SchemaUtil.writeBool(
              manifest.numberAt(pos),
              UnsafeUtil.getBoolean(message, offset(typeAndOffset)),
              writer);
          break;
        case 8: //STRING:
          SchemaUtil.writeString(
              manifest.numberAt(pos), UnsafeUtil.getObject(message, offset(typeAndOffset)), writer);
          break;
        case 9: //MESSAGE:
          SchemaUtil.writeMessage(
              manifest.numberAt(pos), UnsafeUtil.getObject(message, offset(typeAndOffset)), writer);
          break;
        case 10: //BYTES:
          SchemaUtil.writeBytes(
              manifest.numberAt(pos),
              (ByteString) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer);
          break;
        case 11: //UINT32:
          SchemaUtil.writeUInt32(
              manifest.numberAt(pos), UnsafeUtil.getInt(message, offset(typeAndOffset)), writer);
          break;
        case 12: //ENUM:
          SchemaUtil.writeEnum(
              manifest.numberAt(pos), UnsafeUtil.getInt(message, offset(typeAndOffset)), writer);
          break;
        case 13: //SFIXED32:
          SchemaUtil.writeSFixed32(
              manifest.numberAt(pos), UnsafeUtil.getInt(message, offset(typeAndOffset)), writer);
          break;
        case 14: //SFIXED64:
          SchemaUtil.writeSFixed64(
              manifest.numberAt(pos), UnsafeUtil.getLong(message, offset(typeAndOffset)), writer);
          break;
        case 15: //SINT32:
          SchemaUtil.writeSInt32(
              manifest.numberAt(pos), UnsafeUtil.getInt(message, offset(typeAndOffset)), writer);
          break;
        case 16: //SINT64:
          SchemaUtil.writeSInt64(
              manifest.numberAt(pos), UnsafeUtil.getLong(message, offset(typeAndOffset)), writer);
          break;
        case 17: //DOUBLE_LIST:
          SchemaUtil.writeDoubleList(
              manifest.numberAt(pos),
              (List<Double>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 18: //FLOAT_LIST:
          SchemaUtil.writeFloatList(
              manifest.numberAt(pos),
              (List<Float>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 19: //INT64_LIST:
          SchemaUtil.writeInt64List(
              manifest.numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 20: //UINT64_LIST:
          SchemaUtil.writeUInt64List(
              manifest.numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 21: //INT32_LIST:
          SchemaUtil.writeInt32List(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 22: //FIXED64_LIST:
          SchemaUtil.writeFixed64List(
              manifest.numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 23: //FIXED32_LIST:
          SchemaUtil.writeFixed32List(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 24: //BOOL_LIST:
          SchemaUtil.writeBoolList(
              manifest.numberAt(pos),
              (List<Boolean>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 25: //STRING_LIST:
          SchemaUtil.writeStringList(
              manifest.numberAt(pos),
              (List<String>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer);
          break;
        case 26: //MESSAGE_LIST:
          SchemaUtil.writeMessageList(
              manifest.numberAt(pos),
              (List<Double>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer);
          break;
        case 27: //BYTES_LIST:
          SchemaUtil.writeBytesList(
              manifest.numberAt(pos),
              (List<ByteString>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer);
          break;
        case 28: //UINT32_LIST:
          SchemaUtil.writeUInt32List(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 29: //ENUM_LIST:
          SchemaUtil.writeEnumList(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 30: //SFIXED32_LIST:
          SchemaUtil.writeSFixed32List(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 31: //SFIXED64_LIST:
          SchemaUtil.writeSFixed64List(
              manifest.numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 32: //SINT32_LIST:
          SchemaUtil.writeSInt32List(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 33: //SINT64_LIST:
          SchemaUtil.writeSInt64List(
              manifest.numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 34: //DOUBLE_LIST_PACKED:
          SchemaUtil.writeDoubleList(
              manifest.numberAt(pos),
              (List<Double>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 35: //FLOAT_LIST_PACKED:
          SchemaUtil.writeFloatList(
              manifest.numberAt(pos),
              (List<Float>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 36: //INT64_LIST_PACKED:
          SchemaUtil.writeInt64List(
              manifest.numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 37: //UINT64_LIST_PACKED:
          SchemaUtil.writeUInt64List(
              manifest.numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 38: //INT32_LIST_PACKED:
          SchemaUtil.writeInt32List(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 39: //FIXED64_LIST_PACKED:
          SchemaUtil.writeFixed64List(
              manifest.numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 40: //FIXED32_LIST_PACKED:
          SchemaUtil.writeFixed32List(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 41: //BOOL_LIST_PACKED:
          SchemaUtil.writeBoolList(
              manifest.numberAt(pos),
              (List<Boolean>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 42: //UINT32_LIST_PACKED:
          SchemaUtil.writeUInt32List(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 43: //ENUM_LIST_PACKED:
          SchemaUtil.writeEnumList(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 44: //SFIXED32_LIST_PACKED:
          SchemaUtil.writeSFixed32List(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 45: //SFIXED64_LIST_PACKED:
          SchemaUtil.writeSFixed64List(
              manifest.numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 46: //SINT32_LIST_PACKED:
          SchemaUtil.writeSInt32List(
              manifest.numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 47: //SINT64_LIST_PACKED:
          SchemaUtil.writeSInt64List(
              manifest.numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        default:
          // Assume it's an empty entry - just go to the next entry.
          break;
      }
    }
  }
}
