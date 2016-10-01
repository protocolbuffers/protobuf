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

import static com.google.protobuf.Proto2Manifest.offset;
import static com.google.protobuf.Proto2Manifest.type;

import java.io.IOException;

/**
 * A generic, lookup-based schema that can be used with any standard (i.e. non-lite) proto3 message
 * class. The message class must extend {@link GeneratedMessage}.
 */
final class Proto2LookupSchema<T> extends AbstractProto2StandardSchema<T> {
  private final Int2ObjectHashMap<Class<?>> messageFieldClassMap;

  Proto2LookupSchema(Class<T> messageClass, MessageInfo descriptor) {
    super(messageClass, Proto2Manifest.newLookupManifest(descriptor));
    this.messageFieldClassMap = descriptor.messageFieldClassMap();
  }

  @Override
  public void mergeFrom(T message, Reader reader) throws IOException {
    while (true) {
      final int fieldNumber = reader.getFieldNumber();
      final long pos = manifest.lookupPositionForFieldNumber(fieldNumber);
      if (pos >= 0L) {
        final int typeAndOffset = manifest.typeAndOffsetAt(pos);
        try {
          switch (type(typeAndOffset)) {
            case 0: //DOUBLE:
              UnsafeUtil.putDouble(message, offset(typeAndOffset), reader.readDouble());
              manifest.setFieldPresent(message, pos);
              continue;
            case 1: //FLOAT:
              UnsafeUtil.putFloat(message, offset(typeAndOffset), reader.readFloat());
              manifest.setFieldPresent(message, pos);
              continue;
            case 2: //INT64:
              UnsafeUtil.putLong(message, offset(typeAndOffset), reader.readInt64());
              manifest.setFieldPresent(message, pos);
              continue;
            case 3: //UINT64:
              UnsafeUtil.putLong(message, offset(typeAndOffset), reader.readUInt64());
              manifest.setFieldPresent(message, pos);
              continue;
            case 4: //INT32:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readInt32());
              manifest.setFieldPresent(message, pos);
              continue;
            case 5: //FIXED64:
              UnsafeUtil.putLong(message, offset(typeAndOffset), reader.readFixed64());
              manifest.setFieldPresent(message, pos);
              continue;
            case 6: //FIXED32:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readFixed32());
              manifest.setFieldPresent(message, pos);
              continue;
            case 7: //BOOL:
              UnsafeUtil.putBoolean(message, offset(typeAndOffset), reader.readBool());
              manifest.setFieldPresent(message, pos);
              continue;
            case 8: //STRING:
              UnsafeUtil.putObject(message, offset(typeAndOffset), reader.readString());
              manifest.setFieldPresent(message, pos);
              continue;
            case 9: //MESSAGE:
              UnsafeUtil.putObject(
                  message,
                  offset(typeAndOffset),
                  reader.readMessage(messageFieldClassMap.get(fieldNumber)));
              manifest.setFieldPresent(message, pos);
              continue;
            case 10: //BYTES:
              UnsafeUtil.putObject(message, offset(typeAndOffset), reader.readBytes());
              manifest.setFieldPresent(message, pos);
              continue;
            case 11: //UINT32:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readUInt32());
              manifest.setFieldPresent(message, pos);
              continue;
            case 12: //ENUM:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readEnum());
              manifest.setFieldPresent(message, pos);
              continue;
            case 13: //SFIXED32:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readSFixed32());
              manifest.setFieldPresent(message, pos);
              continue;
            case 14: //SFIXED64:
              UnsafeUtil.putLong(message, offset(typeAndOffset), reader.readSFixed64());
              manifest.setFieldPresent(message, pos);
              continue;
            case 15: //SINT32:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readSInt32());
              manifest.setFieldPresent(message, pos);
              continue;
            case 16: //SINT64:
              UnsafeUtil.putLong(message, offset(typeAndOffset), reader.readSInt64());
              manifest.setFieldPresent(message, pos);
              continue;
            case 17: //DOUBLE_LIST:
              reader.readDoubleList(
                  SchemaUtil.<Double>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 18: //FLOAT_LIST:
              reader.readFloatList(SchemaUtil.<Float>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 19: //INT64_LIST:
              reader.readInt64List(SchemaUtil.<Long>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 20: //UINT64_LIST:
              reader.readUInt64List(SchemaUtil.<Long>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 21: //INT32_LIST:
              reader.readInt32List(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 22: //FIXED64_LIST:
              reader.readFixed64List(
                  SchemaUtil.<Long>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 23: //FIXED32_LIST:
              reader.readFixed32List(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 24: //BOOL_LIST:
              reader.readBoolList(
                  SchemaUtil.<Boolean>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 25: //STRING_LIST:
              reader.readStringList(
                  SchemaUtil.<String>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 26: //MESSAGE_LIST:
              SchemaUtil.readMessageList(
                  message, offset(typeAndOffset), reader, messageFieldClassMap.get(fieldNumber));
              continue;
            case 27: //BYTES_LIST:
              reader.readBytesList(
                  SchemaUtil.<ByteString>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 28: //UINT32_LIST:
              reader.readUInt32List(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 29: //ENUM_LIST:
              reader.readEnumList(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 30: //SFIXED32_LIST:
              reader.readSFixed32List(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 31: //SFIXED64_LIST:
              reader.readSFixed64List(
                  SchemaUtil.<Long>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 32: //SINT32_LIST:
              reader.readSInt32List(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 33: //SINT64_LIST:
              reader.readSInt64List(SchemaUtil.<Long>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 34: //DOUBLE_LIST_PACKED:
              reader.readDoubleList(
                  SchemaUtil.<Double>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 35: //FLOAT_LIST_PACKED:
              reader.readFloatList(SchemaUtil.<Float>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 36: //INT64_LIST_PACKED:
              reader.readInt64List(SchemaUtil.<Long>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 37: //UINT64_LIST_PACKED:
              reader.readUInt64List(SchemaUtil.<Long>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 38: //INT32_LIST_PACKED:
              reader.readInt32List(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 39: //FIXED64_LIST_PACKED:
              reader.readFixed64List(
                  SchemaUtil.<Long>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 40: //FIXED32_LIST_PACKED:
              reader.readFixed32List(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 41: //BOOL_LIST_PACKED:
              reader.readBoolList(
                  SchemaUtil.<Boolean>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 42: //UINT32_LIST_PACKED:
              reader.readUInt32List(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 43: //ENUM_LIST_PACKED:
              reader.readEnumList(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 44: //SFIXED32_LIST_PACKED:
              reader.readSFixed32List(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 45: //SFIXED64_LIST_PACKED:
              reader.readSFixed64List(
                  SchemaUtil.<Long>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 46: //SINT32_LIST_PACKED:
              reader.readSInt32List(
                  SchemaUtil.<Integer>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case 47: //SINT64_LIST_PACKED:
              reader.readSInt64List(SchemaUtil.<Long>mutableListAt(message, offset(typeAndOffset)));
              continue;
            case -4: //GROUP (actually should be 252, but byte is [-128, 127])
              UnsafeUtil.putObject(
                  message,
                  offset(typeAndOffset),
                  reader.readGroup(messageFieldClassMap.get(fieldNumber)));
              manifest.setFieldPresent(message, pos);
              continue;
            case -3: //GROUP_LIST (actually should be 253, but byte is [-128, 127])
              SchemaUtil.readGroupList(
                  message, offset(typeAndOffset), reader, messageFieldClassMap.get(fieldNumber));
              continue;
            default:
              // Unknown field type - break out of loop and skip the field.
              break;
          }
        } catch (InvalidProtocolBufferException.InvalidWireTypeException e) {
          // Treat it as an unknown field - same as the default case.
        }
      }

      // Unknown field.
      if (!reader.skipField()) {
        // Done reading.
        return;
      }
    }
  }
}
