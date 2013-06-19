// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

package com.google.protobuf.nano;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * This class is used internally by the Protocol Buffer library and generated
 * message implementations.  It is public only because those generated messages
 * do not reside in the {@code protobuf} package.  Others should not use this
 * class directly.
 *
 * This class contains constants and helper functions useful for dealing with
 * the Protocol Buffer wire format.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class WireFormatNano {
  // Do not allow instantiation.
  private WireFormatNano() {}

  static final int WIRETYPE_VARINT           = 0;
  static final int WIRETYPE_FIXED64          = 1;
  static final int WIRETYPE_LENGTH_DELIMITED = 2;
  static final int WIRETYPE_START_GROUP      = 3;
  static final int WIRETYPE_END_GROUP        = 4;
  static final int WIRETYPE_FIXED32          = 5;

  static final int TAG_TYPE_BITS = 3;
  static final int TAG_TYPE_MASK = (1 << TAG_TYPE_BITS) - 1;

  /** Given a tag value, determines the wire type (the lower 3 bits). */
  static int getTagWireType(final int tag) {
    return tag & TAG_TYPE_MASK;
  }

  /** Given a tag value, determines the field number (the upper 29 bits). */
  public static int getTagFieldNumber(final int tag) {
    return tag >>> TAG_TYPE_BITS;
  }

  /** Makes a tag value given a field number and wire type. */
  static int makeTag(final int fieldNumber, final int wireType) {
    return (fieldNumber << TAG_TYPE_BITS) | wireType;
  }

  // Field numbers for feilds in MessageSet wire format.
  static final int MESSAGE_SET_ITEM    = 1;
  static final int MESSAGE_SET_TYPE_ID = 2;
  static final int MESSAGE_SET_MESSAGE = 3;

  // Tag numbers.
  static final int MESSAGE_SET_ITEM_TAG =
    makeTag(MESSAGE_SET_ITEM, WIRETYPE_START_GROUP);
  static final int MESSAGE_SET_ITEM_END_TAG =
    makeTag(MESSAGE_SET_ITEM, WIRETYPE_END_GROUP);
  static final int MESSAGE_SET_TYPE_ID_TAG =
    makeTag(MESSAGE_SET_TYPE_ID, WIRETYPE_VARINT);
  static final int MESSAGE_SET_MESSAGE_TAG =
    makeTag(MESSAGE_SET_MESSAGE, WIRETYPE_LENGTH_DELIMITED);

  public static final int EMPTY_INT_ARRAY[] = {};
  public static final long EMPTY_LONG_ARRAY[] = {};
  public static final float EMPTY_FLOAT_ARRAY[] = {};
  public static final double EMPTY_DOUBLE_ARRAY[] = {};
  public static final boolean EMPTY_BOOLEAN_ARRAY[] = {};
  public static final String EMPTY_STRING_ARRAY[] = {};
  public static final byte[] EMPTY_BYTES_ARRAY[] = {};
  public static final byte[] EMPTY_BYTES = {};

  /**
   * Parses an unknown field. This implementation skips the field.
   *
   * <p>Generated messages will call this for unknown fields if the store_unknown_fields
   * option is off.
   *
   * @return {@literal true} unless the tag is an end-group tag.
   */
  public static boolean parseUnknownField(
      final CodedInputByteBufferNano input,
      final int tag) throws IOException {
    return input.skipField(tag);
  }

  /**
   * Stores the binary data of an unknown field.
   *
   * <p>Generated messages will call this for unknown fields if the store_unknown_fields
   * option is on.
   *
   * @param data a Collection in which to store the data.
   * @param input the input buffer.
   * @param tag the tag of the field.

   * @return {@literal true} unless the tag is an end-group tag.
   */
  public static boolean storeUnknownField(
      final List<UnknownFieldData> data,
      final CodedInputByteBufferNano input,
      final int tag) throws IOException {
    int startPos = input.getPosition();
    boolean skip = input.skipField(tag);
    int endPos = input.getPosition();
    byte[] bytes = input.getData(startPos, endPos - startPos);
    data.add(new UnknownFieldData(tag, bytes));
    return skip;
  }

  /**
   * Computes the array length of a repeated field. We assume that in the common case repeated
   * fields are contiguously serialized but we still correctly handle interspersed values of a
   * repeated field (but with extra allocations).
   *
   * Rewinds to current input position before returning.
   *
   * @param input stream input, pointing to the byte after the first tag
   * @param tag repeated field tag just read
   * @return length of array
   * @throws IOException
   */
  public static final int getRepeatedFieldArrayLength(
      final CodedInputByteBufferNano input,
      final int tag) throws IOException {
    int arrayLength = 1;
    int startPos = input.getPosition();
    input.skipField(tag);
    while (input.getBytesUntilLimit() > 0) {
      int thisTag = input.readTag();
      if (thisTag != tag) {
        break;
      }
      input.skipField(tag);
      arrayLength++;
    }
    input.rewindToPosition(startPos);
    return arrayLength;
  }

  /**
   * Decodes the value of an extension.
   */
  public static <T> T getExtension(Extension<T> extension, List<UnknownFieldData> unknownFields) {
    if (unknownFields == null) {
      return null;
    }
    List<UnknownFieldData> dataForField = new ArrayList<UnknownFieldData>();
    for (UnknownFieldData data : unknownFields) {
      if (getTagFieldNumber(data.tag) == extension.fieldNumber) {
        dataForField.add(data);
      }
    }
    if (dataForField.isEmpty()) {
      return null;
    }

    if (extension.isRepeatedField) {
      List<Object> result = new ArrayList<Object>(dataForField.size());
      for (UnknownFieldData data : dataForField) {
        result.add(readData(extension.fieldType, data.bytes));
      }
      return extension.listType.cast(result);
    }

    // Normal fields. Note that the protobuf docs require us to handle multiple instances
    // of the same field even for fields that are not repeated.
    UnknownFieldData lastData = dataForField.get(dataForField.size() - 1);
    return readData(extension.fieldType, lastData.bytes);
  }

  /**
   * Reads (extension) data of the specified type from the specified byte array.
   *
   * @throws IllegalArgumentException if an error occurs while reading the data.
   */
  private static <T> T readData(Class<T> clazz, byte[] data) {
    if (data.length == 0) {
      return null;
    }
    CodedInputByteBufferNano buffer = CodedInputByteBufferNano.newInstance(data);
    try {
      if (clazz == String.class) {
        return clazz.cast(buffer.readString());
      } else if (clazz == Integer.class) {
        return clazz.cast(buffer.readInt32());
      } else if (clazz == Long.class) {
        return clazz.cast(buffer.readInt64());
      } else if (clazz == Boolean.class) {
        return clazz.cast(buffer.readBool());
      } else if (clazz == Float.class) {
        return clazz.cast(buffer.readFloat());
      } else if (clazz == Double.class) {
        return clazz.cast(buffer.readDouble());
      } else if (clazz == byte[].class) {
        return clazz.cast(buffer.readBytes());
      } else if (MessageNano.class.isAssignableFrom(clazz)) {
        try {
          MessageNano message = (MessageNano) clazz.newInstance();
          buffer.readMessage(message);
          return clazz.cast(message);
        } catch (IllegalAccessException e) {
          throw new IllegalArgumentException("Error creating instance of class " + clazz, e);
        } catch (InstantiationException e) {
          throw new IllegalArgumentException("Error creating instance of class " + clazz, e);
        }
      } else {
        throw new IllegalArgumentException("Unhandled extension field type: " + clazz);
      }
    } catch (IOException e) {
      throw new IllegalArgumentException("Error reading extension field", e);
    }
  }

  public static <T> void setExtension(Extension<T> extension, T value,
      List<UnknownFieldData> unknownFields) {
    // First, remove all unknown fields with this tag.
    for (Iterator<UnknownFieldData> i = unknownFields.iterator(); i.hasNext();) {
      UnknownFieldData data = i.next();
      if (extension.fieldNumber == getTagFieldNumber(data.tag)) {
        i.remove();
      }
    }
    if (value == null) {
      return;
    }
    // Repeated field.
    if (value instanceof List) {
      for (Object item : (List<?>) value) {
        unknownFields.add(write(extension.fieldNumber, item));
      }
    } else {
      unknownFields.add(write(extension.fieldNumber, value));
    }
  }

  /**
   * Writes extension data and returns an {@link UnknownFieldData} containing
   * bytes and a tag.
   *
   * @throws IllegalArgumentException if an error occurs while writing.
   */
  private static UnknownFieldData write(int fieldNumber, Object object) {
    byte[] data;
    int tag;
    Class<?> clazz = object.getClass();
    try {
      if (clazz == String.class) {
        String str = (String) object;
        data = new byte[CodedOutputByteBufferNano.computeStringSizeNoTag(str)];
        CodedOutputByteBufferNano.newInstance(data).writeStringNoTag(str);
        tag = makeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
      } else if (clazz == Integer.class) {
        Integer integer = (Integer) object;
        data = new byte[CodedOutputByteBufferNano.computeInt32SizeNoTag(integer)];
        CodedOutputByteBufferNano.newInstance(data).writeInt32NoTag(integer);
        tag = makeTag(fieldNumber, WIRETYPE_VARINT);
      } else if (clazz == Long.class) {
        Long longValue = (Long) object;
        data = new byte[CodedOutputByteBufferNano.computeInt64SizeNoTag(longValue)];
        CodedOutputByteBufferNano.newInstance(data).writeInt64NoTag(longValue);
        tag = makeTag(fieldNumber, WIRETYPE_VARINT);
      } else if (clazz == Boolean.class) {
        Boolean boolValue = (Boolean) object;
        data = new byte[CodedOutputByteBufferNano.computeBoolSizeNoTag(boolValue)];
        CodedOutputByteBufferNano.newInstance(data).writeBoolNoTag(boolValue);
        tag = makeTag(fieldNumber, WIRETYPE_VARINT);
      } else if (clazz == Float.class) {
        Float floatValue = (Float) object;
        data = new byte[CodedOutputByteBufferNano.computeFloatSizeNoTag(floatValue)];
        CodedOutputByteBufferNano.newInstance(data).writeFloatNoTag(floatValue);
        tag = makeTag(fieldNumber, WIRETYPE_FIXED32);
      } else if (clazz == Double.class) {
        Double doubleValue = (Double) object;
        data = new byte[CodedOutputByteBufferNano.computeDoubleSizeNoTag(doubleValue)];
        CodedOutputByteBufferNano.newInstance(data).writeDoubleNoTag(doubleValue);
        tag = makeTag(fieldNumber, WIRETYPE_FIXED64);
      } else if (clazz == byte[].class) {
        byte[] byteArrayValue = (byte[]) object;
        data = new byte[CodedOutputByteBufferNano.computeByteArraySizeNoTag(byteArrayValue)];
        CodedOutputByteBufferNano.newInstance(data).writeByteArrayNoTag(byteArrayValue);
        tag = makeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
      } else if (MessageNano.class.isAssignableFrom(clazz)) {
        MessageNano messageValue = (MessageNano) object;

        int messageSize = messageValue.getSerializedSize();
        int delimiterSize = CodedOutputByteBufferNano.computeRawVarint32Size(messageSize);
        data = new byte[messageSize + delimiterSize];
        CodedOutputByteBufferNano buffer = CodedOutputByteBufferNano.newInstance(data);
        buffer.writeRawVarint32(messageSize);
        buffer.writeRawBytes(MessageNano.toByteArray(messageValue));
        tag = makeTag(fieldNumber, WIRETYPE_LENGTH_DELIMITED);
      } else {
        throw new IllegalArgumentException("Unhandled extension field type: " + clazz);
      }
    } catch (IOException e) {
      throw new IllegalArgumentException(e);
    }
    return new UnknownFieldData(tag, data);
  }

  /**
   * Given a set of unknown field data, compute the wire size.
   */
  public static int computeWireSize(List<UnknownFieldData> unknownFields) {
    if (unknownFields == null) {
      return 0;
    }
    int size = 0;
    for (UnknownFieldData unknownField : unknownFields) {
      size += CodedOutputByteBufferNano.computeRawVarint32Size(unknownField.tag);
      size += unknownField.bytes.length;
    }
    return size;
  }

  /**
   * Write unknown fields.
   */
  public static void writeUnknownFields(List<UnknownFieldData> unknownFields,
      CodedOutputByteBufferNano outBuffer) throws IOException {
    if (unknownFields == null) {
      return;
    }
    for (UnknownFieldData data : unknownFields) {
      outBuffer.writeTag(getTagFieldNumber(data.tag), getTagWireType(data.tag));
      outBuffer.writeRawBytes(data.bytes);
    }
  }
}
