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

import static com.google.protobuf.MessageSchema.getMutableUnknownFields;

import com.google.protobuf.Internal.ProtobufList;
import java.io.IOException;

/**
 * Helper functions to decode protobuf wire format from a byte array.
 *
 * <p>Note that these functions don't do boundary check on the byte array but instead rely on Java
 * VM to check it. That means parsing rountines utilizing these functions must catch
 * IndexOutOfBoundsException and convert it to protobuf's InvalidProtocolBufferException when
 * crossing protobuf public API boundaries.
 */
final class ArrayDecoders {
  /**
   * A helper used to return multiple values in a Java function. Java doesn't natively support
   * returning multiple values in a function. Creating a new Object to hold the return values will
   * be too expensive. Instead, we pass a Registers instance to functions that want to return
   * multiple values and let the function set the return value in this Registers instance instead.
   *
   * <p>TODO(xiaofeng): This could be merged into CodedInputStream or CodedInputStreamReader which
   * is already being passed through all the parsing rountines.
   */
  static final class Registers {
    public int int1;
    public long long1;
    public Object object1;
    public final ExtensionRegistryLite extensionRegistry;

    Registers() {
      this.extensionRegistry = ExtensionRegistryLite.getEmptyRegistry();
    }

    Registers(ExtensionRegistryLite extensionRegistry) {
      if (extensionRegistry == null) {
        throw new NullPointerException();
      }
      this.extensionRegistry = extensionRegistry;
    }
  }

  /**
   * Decodes a varint. Returns the position after the varint. The decoded varint is stored in
   * registers.int1.
   */
  static int decodeVarint32(byte[] data, int position, Registers registers) {
    int value = data[position++];
    if (value >= 0) {
      registers.int1 = value;
      return position;
    }
    return decodeVarint32(value, data, position, registers);
  }

  /** Like decodeVarint32 except that the first byte is already read. */
  static int decodeVarint32(int firstByte, byte[] data, int position, Registers registers) {
    int value = firstByte & 0x7F;
    final byte b2 = data[position++];
    if (b2 >= 0) {
      registers.int1 = value | ((int) b2 << 7);
      return position;
    }
    value |= (b2 & 0x7F) << 7;

    final byte b3 = data[position++];
    if (b3 >= 0) {
      registers.int1 = value | ((int) b3 << 14);
      return position;
    }
    value |= (b3 & 0x7F) << 14;

    final byte b4 = data[position++];
    if (b4 >= 0) {
      registers.int1 = value | ((int) b4 << 21);
      return position;
    }
    value |= (b4 & 0x7F) << 21;

    final byte b5 = data[position++];
    if (b5 >= 0) {
      registers.int1 = value | ((int) b5 << 28);
      return position;
    }
    value |= (b5 & 0x7F) << 28;

    while (data[position++] < 0) {}

    registers.int1 = value;
    return position;
  }

  /**
   * Decodes a varint. Returns the position after the varint. The decoded varint is stored in
   * registers.long1.
   */
  static int decodeVarint64(byte[] data, int position, Registers registers) {
    long value = data[position++];
    if (value >= 0) {
      registers.long1 = value;
      return position;
    } else {
      return decodeVarint64(value, data, position, registers);
    }
  }

  /** Like decodeVarint64 except that the first byte is already read. */
  static int decodeVarint64(long firstByte, byte[] data, int position, Registers registers) {
    long value = firstByte & 0x7F;
    byte next = data[position++];
    int shift = 7;
    value |= (long) (next & 0x7F) << 7;
    while (next < 0) {
      next = data[position++];
      shift += 7;
      value |= (long) (next & 0x7F) << shift;
    }
    registers.long1 = value;
    return position;
  }

  /** Decodes and returns a fixed32 value. */
  static int decodeFixed32(byte[] data, int position) {
    return (data[position] & 0xff)
        | ((data[position + 1] & 0xff) << 8)
        | ((data[position + 2] & 0xff) << 16)
        | ((data[position + 3] & 0xff) << 24);
  }

  /** Decodes and returns a fixed64 value. */
  static long decodeFixed64(byte[] data, int position) {
    return (data[position] & 0xffL)
        | ((data[position + 1] & 0xffL) << 8)
        | ((data[position + 2] & 0xffL) << 16)
        | ((data[position + 3] & 0xffL) << 24)
        | ((data[position + 4] & 0xffL) << 32)
        | ((data[position + 5] & 0xffL) << 40)
        | ((data[position + 6] & 0xffL) << 48)
        | ((data[position + 7] & 0xffL) << 56);
  }

  /** Decodes and returns a double value. */
  static double decodeDouble(byte[] data, int position) {
    return Double.longBitsToDouble(decodeFixed64(data, position));
  }

  /** Decodes and returns a float value. */
  static float decodeFloat(byte[] data, int position) {
    return Float.intBitsToFloat(decodeFixed32(data, position));
  }

  /** Decodes a string value. */
  static int decodeString(byte[] data, int position, Registers registers)
      throws InvalidProtocolBufferException {
    position = decodeVarint32(data, position, registers);
    final int length = registers.int1;
    if (length < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    } else if (length == 0) {
      registers.object1 = "";
      return position;
    } else {
      registers.object1 = new String(data, position, length, Internal.UTF_8);
      return position + length;
    }
  }

  /** Decodes a string value with utf8 check. */
  static int decodeStringRequireUtf8(byte[] data, int position, Registers registers)
      throws InvalidProtocolBufferException {
    position = decodeVarint32(data, position, registers);
    final int length = registers.int1;
    if (length < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    } else if (length == 0) {
      registers.object1 = "";
      return position;
    } else {
      registers.object1 = Utf8.decodeUtf8(data, position, length);
      return position + length;
    }
  }

  /** Decodes a bytes value. */
  static int decodeBytes(byte[] data, int position, Registers registers)
      throws InvalidProtocolBufferException {
    position = decodeVarint32(data, position, registers);
    final int length = registers.int1;
    if (length < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    } else if (length > data.length - position) {
      throw InvalidProtocolBufferException.truncatedMessage();
    } else if (length == 0) {
      registers.object1 = ByteString.EMPTY;
      return position;
    } else {
      registers.object1 = ByteString.copyFrom(data, position, length);
      return position + length;
    }
  }

  /** Decodes a message value. */
  @SuppressWarnings({"unchecked", "rawtypes"})
  static int decodeMessageField(
      Schema schema, byte[] data, int position, int limit, Registers registers) throws IOException {
    int length = data[position++];
    if (length < 0) {
      position = decodeVarint32(length, data, position, registers);
      length = registers.int1;
    }
    if (length < 0 || length > limit - position) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    Object result = schema.newInstance();
    schema.mergeFrom(result, data, position, position + length, registers);
    schema.makeImmutable(result);
    registers.object1 = result;
    return position + length;
  }

  /** Decodes a group value. */
  @SuppressWarnings({"unchecked", "rawtypes"})
  static int decodeGroupField(
      Schema schema, byte[] data, int position, int limit, int endGroup, Registers registers)
      throws IOException {
    // A group field must has a MessageSchema (the only other subclass of Schema is MessageSetSchema
    // and it can't be used in group fields).
    final MessageSchema messageSchema = (MessageSchema) schema;
    Object result = messageSchema.newInstance();
    // It's OK to directly use parseProto2Message since proto3 doesn't have group.
    final int endPosition =
        messageSchema.parseProto2Message(result, data, position, limit, endGroup, registers);
    messageSchema.makeImmutable(result);
    registers.object1 = result;
    return endPosition;
  }

  /** Decodes a repeated 32-bit varint field. Returns the position after all read values. */
  static int decodeVarint32List(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers) {
    final IntArrayList output = (IntArrayList) list;
    position = decodeVarint32(data, position, registers);
    output.addInt(registers.int1);
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      position = decodeVarint32(data, nextPosition, registers);
      output.addInt(registers.int1);
    }
    return position;
  }

  /** Decodes a repeated 64-bit varint field. Returns the position after all read values. */
  static int decodeVarint64List(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers) {
    final LongArrayList output = (LongArrayList) list;
    position = decodeVarint64(data, position, registers);
    output.addLong(registers.long1);
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      position = decodeVarint64(data, nextPosition, registers);
      output.addLong(registers.long1);
    }
    return position;
  }

  /** Decodes a repeated fixed32 field. Returns the position after all read values. */
  static int decodeFixed32List(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers) {
    final IntArrayList output = (IntArrayList) list;
    output.addInt(decodeFixed32(data, position));
    position += 4;
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      output.addInt(decodeFixed32(data, nextPosition));
      position = nextPosition + 4;
    }
    return position;
  }

  /** Decodes a repeated fixed64 field. Returns the position after all read values. */
  static int decodeFixed64List(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers) {
    final LongArrayList output = (LongArrayList) list;
    output.addLong(decodeFixed64(data, position));
    position += 8;
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      output.addLong(decodeFixed64(data, nextPosition));
      position = nextPosition + 8;
    }
    return position;
  }

  /** Decodes a repeated float field. Returns the position after all read values. */
  static int decodeFloatList(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers) {
    final FloatArrayList output = (FloatArrayList) list;
    output.addFloat(decodeFloat(data, position));
    position += 4;
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      output.addFloat(decodeFloat(data, nextPosition));
      position = nextPosition + 4;
    }
    return position;
  }

  /** Decodes a repeated double field. Returns the position after all read values. */
  static int decodeDoubleList(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers) {
    final DoubleArrayList output = (DoubleArrayList) list;
    output.addDouble(decodeDouble(data, position));
    position += 8;
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      output.addDouble(decodeDouble(data, nextPosition));
      position = nextPosition + 8;
    }
    return position;
  }

  /** Decodes a repeated boolean field. Returns the position after all read values. */
  static int decodeBoolList(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers) {
    final BooleanArrayList output = (BooleanArrayList) list;
    position = decodeVarint64(data, position, registers);
    output.addBoolean(registers.long1 != 0);
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      position = decodeVarint64(data, nextPosition, registers);
      output.addBoolean(registers.long1 != 0);
    }
    return position;
  }

  /** Decodes a repeated sint32 field. Returns the position after all read values. */
  static int decodeSInt32List(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers) {
    final IntArrayList output = (IntArrayList) list;
    position = decodeVarint32(data, position, registers);
    output.addInt(CodedInputStream.decodeZigZag32(registers.int1));
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      position = decodeVarint32(data, nextPosition, registers);
      output.addInt(CodedInputStream.decodeZigZag32(registers.int1));
    }
    return position;
  }

  /** Decodes a repeated sint64 field. Returns the position after all read values. */
  static int decodeSInt64List(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers) {
    final LongArrayList output = (LongArrayList) list;
    position = decodeVarint64(data, position, registers);
    output.addLong(CodedInputStream.decodeZigZag64(registers.long1));
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      position = decodeVarint64(data, nextPosition, registers);
      output.addLong(CodedInputStream.decodeZigZag64(registers.long1));
    }
    return position;
  }

  /** Decodes a packed 32-bit varint field. Returns the position after all read values. */
  static int decodePackedVarint32List(
      byte[] data, int position, ProtobufList<?> list, Registers registers) throws IOException {
    final IntArrayList output = (IntArrayList) list;
    position = decodeVarint32(data, position, registers);
    final int fieldLimit = position + registers.int1;
    while (position < fieldLimit) {
      position = decodeVarint32(data, position, registers);
      output.addInt(registers.int1);
    }
    if (position != fieldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return position;
  }

  /** Decodes a packed 64-bit varint field. Returns the position after all read values. */
  static int decodePackedVarint64List(
      byte[] data, int position, ProtobufList<?> list, Registers registers) throws IOException {
    final LongArrayList output = (LongArrayList) list;
    position = decodeVarint32(data, position, registers);
    final int fieldLimit = position + registers.int1;
    while (position < fieldLimit) {
      position = decodeVarint64(data, position, registers);
      output.addLong(registers.long1);
    }
    if (position != fieldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return position;
  }

  /** Decodes a packed fixed32 field. Returns the position after all read values. */
  static int decodePackedFixed32List(
      byte[] data, int position, ProtobufList<?> list, Registers registers) throws IOException {
    final IntArrayList output = (IntArrayList) list;
    position = decodeVarint32(data, position, registers);
    final int fieldLimit = position + registers.int1;
    while (position < fieldLimit) {
      output.addInt(decodeFixed32(data, position));
      position += 4;
    }
    if (position != fieldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return position;
  }

  /** Decodes a packed fixed64 field. Returns the position after all read values. */
  static int decodePackedFixed64List(
      byte[] data, int position, ProtobufList<?> list, Registers registers) throws IOException {
    final LongArrayList output = (LongArrayList) list;
    position = decodeVarint32(data, position, registers);
    final int fieldLimit = position + registers.int1;
    while (position < fieldLimit) {
      output.addLong(decodeFixed64(data, position));
      position += 8;
    }
    if (position != fieldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return position;
  }

  /** Decodes a packed float field. Returns the position after all read values. */
  static int decodePackedFloatList(
      byte[] data, int position, ProtobufList<?> list, Registers registers) throws IOException {
    final FloatArrayList output = (FloatArrayList) list;
    position = decodeVarint32(data, position, registers);
    final int fieldLimit = position + registers.int1;
    while (position < fieldLimit) {
      output.addFloat(decodeFloat(data, position));
      position += 4;
    }
    if (position != fieldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return position;
  }

  /** Decodes a packed double field. Returns the position after all read values. */
  static int decodePackedDoubleList(
      byte[] data, int position, ProtobufList<?> list, Registers registers) throws IOException {
    final DoubleArrayList output = (DoubleArrayList) list;
    position = decodeVarint32(data, position, registers);
    final int fieldLimit = position + registers.int1;
    while (position < fieldLimit) {
      output.addDouble(decodeDouble(data, position));
      position += 8;
    }
    if (position != fieldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return position;
  }

  /** Decodes a packed boolean field. Returns the position after all read values. */
  static int decodePackedBoolList(
      byte[] data, int position, ProtobufList<?> list, Registers registers) throws IOException {
    final BooleanArrayList output = (BooleanArrayList) list;
    position = decodeVarint32(data, position, registers);
    final int fieldLimit = position + registers.int1;
    while (position < fieldLimit) {
      position = decodeVarint64(data, position, registers);
      output.addBoolean(registers.long1 != 0);
    }
    if (position != fieldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return position;
  }

  /** Decodes a packed sint32 field. Returns the position after all read values. */
  static int decodePackedSInt32List(
      byte[] data, int position, ProtobufList<?> list, Registers registers) throws IOException {
    final IntArrayList output = (IntArrayList) list;
    position = decodeVarint32(data, position, registers);
    final int fieldLimit = position + registers.int1;
    while (position < fieldLimit) {
      position = decodeVarint32(data, position, registers);
      output.addInt(CodedInputStream.decodeZigZag32(registers.int1));
    }
    if (position != fieldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return position;
  }

  /** Decodes a packed sint64 field. Returns the position after all read values. */
  @SuppressWarnings("unchecked")
  static int decodePackedSInt64List(
      byte[] data, int position, ProtobufList<?> list, Registers registers) throws IOException {
    final LongArrayList output = (LongArrayList) list;
    position = decodeVarint32(data, position, registers);
    final int fieldLimit = position + registers.int1;
    while (position < fieldLimit) {
      position = decodeVarint64(data, position, registers);
      output.addLong(CodedInputStream.decodeZigZag64(registers.long1));
    }
    if (position != fieldLimit) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    return position;
  }

  /** Decodes a repeated string field. Returns the position after all read values. */
  @SuppressWarnings("unchecked")
  static int decodeStringList(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers)
      throws InvalidProtocolBufferException {
    final ProtobufList<String> output = (ProtobufList<String>) list;
    position = decodeVarint32(data, position, registers);
    final int length = registers.int1;
    if (length < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    } else if (length == 0) {
      output.add("");
    } else {
      String value = new String(data, position, length, Internal.UTF_8);
      output.add(value);
      position += length;
    }
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      position = decodeVarint32(data, nextPosition, registers);
      final int nextLength = registers.int1;
      if (nextLength < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      } else if (nextLength == 0) {
        output.add("");
      } else {
        String value = new String(data, position, nextLength, Internal.UTF_8);
        output.add(value);
        position += nextLength;
      }
    }
    return position;
  }

  /**
   * Decodes a repeated string field with utf8 check. Returns the position after all read values.
   */
  @SuppressWarnings("unchecked")
  static int decodeStringListRequireUtf8(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers)
      throws InvalidProtocolBufferException {
    final ProtobufList<String> output = (ProtobufList<String>) list;
    position = decodeVarint32(data, position, registers);
    final int length = registers.int1;
    if (length < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    } else if (length == 0) {
      output.add("");
    } else {
      if (!Utf8.isValidUtf8(data, position, position + length)) {
        throw InvalidProtocolBufferException.invalidUtf8();
      }
      String value = new String(data, position, length, Internal.UTF_8);
      output.add(value);
      position += length;
    }
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      position = decodeVarint32(data, nextPosition, registers);
      final int nextLength = registers.int1;
      if (nextLength < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      } else if (nextLength == 0) {
        output.add("");
      } else {
        if (!Utf8.isValidUtf8(data, position, position + nextLength)) {
          throw InvalidProtocolBufferException.invalidUtf8();
        }
        String value = new String(data, position, nextLength, Internal.UTF_8);
        output.add(value);
        position += nextLength;
      }
    }
    return position;
  }

  /** Decodes a repeated bytes field. Returns the position after all read values. */
  @SuppressWarnings("unchecked")
  static int decodeBytesList(
      int tag, byte[] data, int position, int limit, ProtobufList<?> list, Registers registers)
      throws InvalidProtocolBufferException {
    final ProtobufList<ByteString> output = (ProtobufList<ByteString>) list;
    position = decodeVarint32(data, position, registers);
    final int length = registers.int1;
    if (length < 0) {
      throw InvalidProtocolBufferException.negativeSize();
    } else if (length > data.length - position) {
      throw InvalidProtocolBufferException.truncatedMessage();
    } else if (length == 0) {
      output.add(ByteString.EMPTY);
    } else {
      output.add(ByteString.copyFrom(data, position, length));
      position += length;
    }
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      position = decodeVarint32(data, nextPosition, registers);
      final int nextLength = registers.int1;
      if (nextLength < 0) {
        throw InvalidProtocolBufferException.negativeSize();
      } else if (nextLength > data.length - position) {
        throw InvalidProtocolBufferException.truncatedMessage();
      } else if (nextLength == 0) {
        output.add(ByteString.EMPTY);
      } else {
        output.add(ByteString.copyFrom(data, position, nextLength));
        position += nextLength;
      }
    }
    return position;
  }

  /**
   * Decodes a repeated message field
   *
   * @return The position of after read all messages
   */
  @SuppressWarnings({"unchecked"})
  static int decodeMessageList(
      Schema<?> schema,
      int tag,
      byte[] data,
      int position,
      int limit,
      ProtobufList<?> list,
      Registers registers)
      throws IOException {
    final ProtobufList<Object> output = (ProtobufList<Object>) list;
    position = decodeMessageField(schema, data, position, limit, registers);
    output.add(registers.object1);
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      position = decodeMessageField(schema, data, nextPosition, limit, registers);
      output.add(registers.object1);
    }
    return position;
  }

  /**
   * Decodes a repeated group field
   *
   * @return The position of after read all groups
   */
  @SuppressWarnings({"unchecked", "rawtypes"})
  static int decodeGroupList(
      Schema schema,
      int tag,
      byte[] data,
      int position,
      int limit,
      ProtobufList<?> list,
      Registers registers)
      throws IOException {
    final ProtobufList<Object> output = (ProtobufList<Object>) list;
    final int endgroup = (tag & ~0x7) | WireFormat.WIRETYPE_END_GROUP;
    position = decodeGroupField(schema, data, position, limit, endgroup, registers);
    output.add(registers.object1);
    while (position < limit) {
      int nextPosition = decodeVarint32(data, position, registers);
      if (tag != registers.int1) {
        break;
      }
      position = decodeGroupField(schema, data, nextPosition, limit, endgroup, registers);
      output.add(registers.object1);
    }
    return position;
  }

  static int decodeExtensionOrUnknownField(
      int tag, byte[] data, int position, int limit,
      Object message,
      MessageLite defaultInstance,
      UnknownFieldSchema<UnknownFieldSetLite, UnknownFieldSetLite> unknownFieldSchema,
      Registers registers)
      throws IOException {
    final int number = tag >>> 3;
    GeneratedMessageLite.GeneratedExtension extension =
        registers.extensionRegistry.findLiteExtensionByNumber(defaultInstance, number);
    if (extension == null) {
      return decodeUnknownField(
          tag, data, position, limit, getMutableUnknownFields(message), registers);
    } else  {
      ((GeneratedMessageLite.ExtendableMessage<?, ?>) message).ensureExtensionsAreMutable();
      return decodeExtension(
          tag, data, position, limit, (GeneratedMessageLite.ExtendableMessage) message,
          extension, unknownFieldSchema, registers);
    }
  }

  static int decodeExtension(
      int tag,
      byte[] data,
      int position,
      int limit,
      GeneratedMessageLite.ExtendableMessage<?, ?> message,
      GeneratedMessageLite.GeneratedExtension<?, ?> extension,
      UnknownFieldSchema<UnknownFieldSetLite, UnknownFieldSetLite> unknownFieldSchema,
      Registers registers)
      throws IOException {
    final FieldSet<GeneratedMessageLite.ExtensionDescriptor> extensions = message.extensions;
    final int fieldNumber = tag >>> 3;
    if (extension.descriptor.isRepeated() && extension.descriptor.isPacked()) {
      switch (extension.getLiteType()) {
        case DOUBLE:
        {
          DoubleArrayList list = new DoubleArrayList();
          position = decodePackedDoubleList(data, position, list, registers);
          extensions.setField(extension.descriptor, list);
          break;
        }
        case FLOAT:
        {
          FloatArrayList list = new FloatArrayList();
          position = decodePackedFloatList(data, position, list, registers);
          extensions.setField(extension.descriptor, list);
          break;
        }
        case INT64:
        case UINT64:
        {
          LongArrayList list = new LongArrayList();
          position = decodePackedVarint64List(data, position, list, registers);
          extensions.setField(extension.descriptor, list);
          break;
        }
        case INT32:
        case UINT32:
        {
          IntArrayList list = new IntArrayList();
          position = decodePackedVarint32List(data, position, list, registers);
          extensions.setField(extension.descriptor, list);
          break;
        }
        case FIXED64:
        case SFIXED64:
        {
          LongArrayList list = new LongArrayList();
          position = decodePackedFixed64List(data, position, list, registers);
          extensions.setField(extension.descriptor, list);
          break;
        }
        case FIXED32:
        case SFIXED32:
        {
          IntArrayList list = new IntArrayList();
          position = decodePackedFixed32List(data, position, list, registers);
          extensions.setField(extension.descriptor, list);
          break;
        }
        case BOOL:
        {
          BooleanArrayList list = new BooleanArrayList();
          position = decodePackedBoolList(data, position, list, registers);
          extensions.setField(extension.descriptor, list);
          break;
        }
        case SINT32:
        {
          IntArrayList list = new IntArrayList();
          position = decodePackedSInt32List(data, position, list, registers);
          extensions.setField(extension.descriptor, list);
          break;
        }
        case SINT64:
        {
          LongArrayList list = new LongArrayList();
          position = decodePackedSInt64List(data, position, list, registers);
          extensions.setField(extension.descriptor, list);
          break;
        }
        case ENUM:
        {
          IntArrayList list = new IntArrayList();
          position = decodePackedVarint32List(data, position, list, registers);
          UnknownFieldSetLite unknownFields = message.unknownFields;
          if (unknownFields == UnknownFieldSetLite.getDefaultInstance()) {
            unknownFields = null;
          }
          unknownFields =
              SchemaUtil.filterUnknownEnumList(
                  fieldNumber,
                  list,
                  extension.descriptor.getEnumType(),
                  unknownFields,
                  unknownFieldSchema);
          if (unknownFields != null) {
            message.unknownFields = unknownFields;
          }
          extensions.setField(extension.descriptor, list);
          break;
        }
        default:
          throw new IllegalStateException(
              "Type cannot be packed: " + extension.descriptor.getLiteType());
      }
    } else {
      Object value = null;
      // Enum is a special case becasue unknown enum values will be put into UnknownFieldSetLite.
      if (extension.getLiteType() == WireFormat.FieldType.ENUM) {
        position = decodeVarint32(data, position, registers);
        Object enumValue = extension.descriptor.getEnumType().findValueByNumber(registers.int1);
        if (enumValue == null) {
          UnknownFieldSetLite unknownFields = ((GeneratedMessageLite) message).unknownFields;
          if (unknownFields == UnknownFieldSetLite.getDefaultInstance()) {
            unknownFields = UnknownFieldSetLite.newInstance();
            ((GeneratedMessageLite) message).unknownFields = unknownFields;
          }
          SchemaUtil.storeUnknownEnum(
              fieldNumber, registers.int1, unknownFields, unknownFieldSchema);
          return position;
        }
        // Note, we store the integer value instead of the actual enum object in FieldSet.
        // This is also different from full-runtime where we store EnumValueDescriptor.
        value = registers.int1;
      } else {
        switch (extension.getLiteType()) {
          case DOUBLE:
            value = decodeDouble(data, position);
            position += 8;
            break;
          case FLOAT:
            value = decodeFloat(data, position);
            position += 4;
            break;
          case INT64:
          case UINT64:
            position = decodeVarint64(data, position, registers);
            value = registers.long1;
            break;
          case INT32:
          case UINT32:
            position = decodeVarint32(data, position, registers);
            value = registers.int1;
            break;
          case FIXED64:
          case SFIXED64:
            value = decodeFixed64(data, position);
            position += 8;
            break;
          case FIXED32:
          case SFIXED32:
            value = decodeFixed32(data, position);
            position += 4;
            break;
          case BOOL:
            position = decodeVarint64(data, position, registers);
            value = (registers.long1 != 0);
            break;
          case BYTES:
            position = decodeBytes(data, position, registers);
            value = registers.object1;
            break;
          case SINT32:
            position = decodeVarint32(data, position, registers);
            value = CodedInputStream.decodeZigZag32(registers.int1);
            break;
          case SINT64:
            position = decodeVarint64(data, position, registers);
            value = CodedInputStream.decodeZigZag64(registers.long1);
            break;
          case STRING:
            position = decodeString(data, position, registers);
            value = registers.object1;
            break;
          case GROUP:
            final int endTag = (fieldNumber << 3) | WireFormat.WIRETYPE_END_GROUP;
            position = decodeGroupField(
                Protobuf.getInstance().schemaFor(extension.getMessageDefaultInstance().getClass()),
                data, position, limit, endTag, registers);
            value = registers.object1;
            break;

          case MESSAGE:
            position = decodeMessageField(
                Protobuf.getInstance().schemaFor(extension.getMessageDefaultInstance().getClass()),
                data, position, limit, registers);
            value = registers.object1;
            break;

          case ENUM:
            throw new IllegalStateException("Shouldn't reach here.");
        }
      }
      if (extension.isRepeated()) {
        extensions.addRepeatedField(extension.descriptor, value);
      } else {
        switch (extension.getLiteType()) {
          case MESSAGE:
          case GROUP:
            Object oldValue = extensions.getField(extension.descriptor);
            if (oldValue != null) {
              value = Internal.mergeMessage(oldValue, value);
            }
            break;
          default:
            break;
        }
        extensions.setField(extension.descriptor, value);
      }
    }
    return position;
  }

  /** Decodes an unknown field. */
  static int decodeUnknownField(
      int tag,
      byte[] data,
      int position,
      int limit,
      UnknownFieldSetLite unknownFields,
      Registers registers)
      throws InvalidProtocolBufferException {
    if (WireFormat.getTagFieldNumber(tag) == 0) {
      throw InvalidProtocolBufferException.invalidTag();
    }
    switch (WireFormat.getTagWireType(tag)) {
      case WireFormat.WIRETYPE_VARINT:
        position = decodeVarint64(data, position, registers);
        unknownFields.storeField(tag, registers.long1);
        return position;
      case WireFormat.WIRETYPE_FIXED32:
        unknownFields.storeField(tag, decodeFixed32(data, position));
        return position + 4;
      case WireFormat.WIRETYPE_FIXED64:
        unknownFields.storeField(tag, decodeFixed64(data, position));
        return position + 8;
      case WireFormat.WIRETYPE_LENGTH_DELIMITED:
        position = decodeVarint32(data, position, registers);
        final int length = registers.int1;
        if (length < 0) {
          throw InvalidProtocolBufferException.negativeSize();
        } else if (length > data.length - position) {
          throw InvalidProtocolBufferException.truncatedMessage();
        } else if (length == 0) {
          unknownFields.storeField(tag, ByteString.EMPTY);
        } else {
          unknownFields.storeField(tag, ByteString.copyFrom(data, position, length));
        }
        return position + length;
      case WireFormat.WIRETYPE_START_GROUP:
        final UnknownFieldSetLite child = UnknownFieldSetLite.newInstance();
        final int endGroup = (tag & ~0x7) | WireFormat.WIRETYPE_END_GROUP;
        int lastTag = 0;
        while (position < limit) {
          position = decodeVarint32(data, position, registers);
          lastTag = registers.int1;
          if (lastTag == endGroup) {
            break;
          }
          position = decodeUnknownField(lastTag, data, position, limit, child, registers);
        }
        if (position > limit || lastTag != endGroup) {
          throw InvalidProtocolBufferException.parseFailure();
        }
        unknownFields.storeField(tag, child);
        return position;
      default:
        throw InvalidProtocolBufferException.invalidTag();
    }
  }

  /** Skips an unknown field. */
  static int skipField(int tag, byte[] data, int position, int limit, Registers registers)
      throws InvalidProtocolBufferException {
    if (WireFormat.getTagFieldNumber(tag) == 0) {
      throw InvalidProtocolBufferException.invalidTag();
    }
    switch (WireFormat.getTagWireType(tag)) {
      case WireFormat.WIRETYPE_VARINT:
        position = decodeVarint64(data, position, registers);
        return position;
      case WireFormat.WIRETYPE_FIXED32:
        return position + 4;
      case WireFormat.WIRETYPE_FIXED64:
        return position + 8;
      case WireFormat.WIRETYPE_LENGTH_DELIMITED:
        position = decodeVarint32(data, position, registers);
        return position + registers.int1;
      case WireFormat.WIRETYPE_START_GROUP:
        final int endGroup = (tag & ~0x7) | WireFormat.WIRETYPE_END_GROUP;
        int lastTag = 0;
        while (position < limit) {
          position = decodeVarint32(data, position, registers);
          lastTag = registers.int1;
          if (lastTag == endGroup) {
            break;
          }
          position = skipField(lastTag, data, position, limit, registers);
        }
        if (position > limit || lastTag != endGroup) {
          throw InvalidProtocolBufferException.parseFailure();
        }
        return position;
      default:
        throw InvalidProtocolBufferException.invalidTag();
    }
  }
}
