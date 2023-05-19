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

import com.google.protobuf.FieldSet.FieldDescriptorLite;
import com.google.protobuf.Internal.EnumLiteMap;
import com.google.protobuf.Internal.EnumVerifier;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.Iterator;
import java.util.List;
import java.util.RandomAccess;

/** Helper methods used by schemas. */
@ExperimentalApi
@CheckReturnValue
final class SchemaUtil {
  private static final Class<?> GENERATED_MESSAGE_CLASS = getGeneratedMessageClass();
  private static final UnknownFieldSchema<?, ?> UNKNOWN_FIELD_SET_FULL_SCHEMA =
      getUnknownFieldSetSchema();
  private static final UnknownFieldSchema<?, ?> UNKNOWN_FIELD_SET_LITE_SCHEMA =
      new UnknownFieldSetLiteSchema();

  private static final int DEFAULT_LOOK_UP_START_NUMBER = 40;

  private SchemaUtil() {}

  /**
   * Requires that the given message extend {@link com.google.protobuf.GeneratedMessageV3} or {@link
   * GeneratedMessageLite}.
   */
  public static void requireGeneratedMessage(Class<?> messageType) {
    // TODO(b/248560713) decide if we're keeping support for Full in schema classes and handle this
    // better.
    if (!GeneratedMessageLite.class.isAssignableFrom(messageType)
        && GENERATED_MESSAGE_CLASS != null
        && !GENERATED_MESSAGE_CLASS.isAssignableFrom(messageType)) {
      throw new IllegalArgumentException(
          "Message classes must extend GeneratedMessageV3 or GeneratedMessageLite");
    }
  }

  public static void writeDouble(int fieldNumber, double value, Writer writer) throws IOException {
    if (Double.doubleToRawLongBits(value) != 0) {
      writer.writeDouble(fieldNumber, value);
    }
  }

  public static void writeFloat(int fieldNumber, float value, Writer writer) throws IOException {
    if (Float.floatToRawIntBits(value) != 0) {
      writer.writeFloat(fieldNumber, value);
    }
  }

  public static void writeInt64(int fieldNumber, long value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeInt64(fieldNumber, value);
    }
  }

  public static void writeUInt64(int fieldNumber, long value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeUInt64(fieldNumber, value);
    }
  }

  public static void writeSInt64(int fieldNumber, long value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeSInt64(fieldNumber, value);
    }
  }

  public static void writeFixed64(int fieldNumber, long value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeFixed64(fieldNumber, value);
    }
  }

  public static void writeSFixed64(int fieldNumber, long value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeSFixed64(fieldNumber, value);
    }
  }

  public static void writeInt32(int fieldNumber, int value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeInt32(fieldNumber, value);
    }
  }

  public static void writeUInt32(int fieldNumber, int value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeUInt32(fieldNumber, value);
    }
  }

  public static void writeSInt32(int fieldNumber, int value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeSInt32(fieldNumber, value);
    }
  }

  public static void writeFixed32(int fieldNumber, int value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeFixed32(fieldNumber, value);
    }
  }

  public static void writeSFixed32(int fieldNumber, int value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeSFixed32(fieldNumber, value);
    }
  }

  public static void writeEnum(int fieldNumber, int value, Writer writer) throws IOException {
    if (value != 0) {
      writer.writeEnum(fieldNumber, value);
    }
  }

  public static void writeBool(int fieldNumber, boolean value, Writer writer) throws IOException {
    if (value) {
      writer.writeBool(fieldNumber, true);
    }
  }

  public static void writeString(int fieldNumber, Object value, Writer writer) throws IOException {
    if (value instanceof String) {
      writeStringInternal(fieldNumber, (String) value, writer);
    } else {
      writeBytes(fieldNumber, (ByteString) value, writer);
    }
  }

  private static void writeStringInternal(int fieldNumber, String value, Writer writer)
      throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeString(fieldNumber, value);
    }
  }

  public static void writeBytes(int fieldNumber, ByteString value, Writer writer)
      throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeBytes(fieldNumber, value);
    }
  }

  public static void writeMessage(int fieldNumber, Object value, Writer writer) throws IOException {
    if (value != null) {
      writer.writeMessage(fieldNumber, value);
    }
  }

  public static void writeDoubleList(
      int fieldNumber, List<Double> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeDoubleList(fieldNumber, value, packed);
    }
  }

  public static void writeFloatList(
      int fieldNumber, List<Float> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeFloatList(fieldNumber, value, packed);
    }
  }

  public static void writeInt64List(
      int fieldNumber, List<Long> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeInt64List(fieldNumber, value, packed);
    }
  }

  public static void writeUInt64List(
      int fieldNumber, List<Long> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeUInt64List(fieldNumber, value, packed);
    }
  }

  public static void writeSInt64List(
      int fieldNumber, List<Long> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeSInt64List(fieldNumber, value, packed);
    }
  }

  public static void writeFixed64List(
      int fieldNumber, List<Long> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeFixed64List(fieldNumber, value, packed);
    }
  }

  public static void writeSFixed64List(
      int fieldNumber, List<Long> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeSFixed64List(fieldNumber, value, packed);
    }
  }

  public static void writeInt32List(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeInt32List(fieldNumber, value, packed);
    }
  }

  public static void writeUInt32List(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeUInt32List(fieldNumber, value, packed);
    }
  }

  public static void writeSInt32List(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeSInt32List(fieldNumber, value, packed);
    }
  }

  public static void writeFixed32List(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeFixed32List(fieldNumber, value, packed);
    }
  }

  public static void writeSFixed32List(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeSFixed32List(fieldNumber, value, packed);
    }
  }

  public static void writeEnumList(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeEnumList(fieldNumber, value, packed);
    }
  }

  public static void writeBoolList(
      int fieldNumber, List<Boolean> value, Writer writer, boolean packed) throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeBoolList(fieldNumber, value, packed);
    }
  }

  public static void writeStringList(int fieldNumber, List<String> value, Writer writer)
      throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeStringList(fieldNumber, value);
    }
  }

  public static void writeBytesList(int fieldNumber, List<ByteString> value, Writer writer)
      throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeBytesList(fieldNumber, value);
    }
  }

  public static void writeMessageList(int fieldNumber, List<?> value, Writer writer)
      throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeMessageList(fieldNumber, value);
    }
  }

  public static void writeMessageList(int fieldNumber, List<?> value, Writer writer, Schema schema)
      throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeMessageList(fieldNumber, value, schema);
    }
  }

  public static void writeLazyFieldList(int fieldNumber, List<?> value, Writer writer)
      throws IOException {
    if (value != null && !value.isEmpty()) {
      for (Object item : value) {
        ((LazyFieldLite) item).writeTo(writer, fieldNumber);
      }
    }
  }

  public static void writeGroupList(int fieldNumber, List<?> value, Writer writer)
      throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeGroupList(fieldNumber, value);
    }
  }

  public static void writeGroupList(int fieldNumber, List<?> value, Writer writer, Schema schema)
      throws IOException {
    if (value != null && !value.isEmpty()) {
      writer.writeGroupList(fieldNumber, value, schema);
    }
  }

  static int computeSizeInt64ListNoTag(List<Long> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }

    int size = 0;

    if (list instanceof LongArrayList) {
      final LongArrayList primitiveList = (LongArrayList) list;
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeInt64SizeNoTag(primitiveList.getLong(i));
      }
    } else {
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeInt64SizeNoTag(list.get(i));
      }
    }
    return size;
  }

  static int computeSizeInt64List(int fieldNumber, List<Long> list, boolean packed) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = computeSizeInt64ListNoTag(list);

    if (packed) {
      return CodedOutputStream.computeTagSize(fieldNumber)
          + CodedOutputStream.computeLengthDelimitedFieldSize(size);
    } else {
      return size + (list.size() * CodedOutputStream.computeTagSize(fieldNumber));
    }
  }

  static int computeSizeUInt64ListNoTag(List<Long> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }

    int size = 0;

    if (list instanceof LongArrayList) {
      final LongArrayList primitiveList = (LongArrayList) list;
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeUInt64SizeNoTag(primitiveList.getLong(i));
      }
    } else {
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeUInt64SizeNoTag(list.get(i));
      }
    }
    return size;
  }

  static int computeSizeUInt64List(int fieldNumber, List<Long> list, boolean packed) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = computeSizeUInt64ListNoTag(list);

    if (packed) {
      return CodedOutputStream.computeTagSize(fieldNumber)
          + CodedOutputStream.computeLengthDelimitedFieldSize(size);
    } else {
      return size + (length * CodedOutputStream.computeTagSize(fieldNumber));
    }
  }

  static int computeSizeSInt64ListNoTag(List<Long> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }

    int size = 0;

    if (list instanceof LongArrayList) {
      final LongArrayList primitiveList = (LongArrayList) list;
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeSInt64SizeNoTag(primitiveList.getLong(i));
      }
    } else {
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeSInt64SizeNoTag(list.get(i));
      }
    }
    return size;
  }

  static int computeSizeSInt64List(int fieldNumber, List<Long> list, boolean packed) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = computeSizeSInt64ListNoTag(list);

    if (packed) {
      return CodedOutputStream.computeTagSize(fieldNumber)
          + CodedOutputStream.computeLengthDelimitedFieldSize(size);
    } else {
      return size + (length * CodedOutputStream.computeTagSize(fieldNumber));
    }
  }

  static int computeSizeEnumListNoTag(List<Integer> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }

    int size = 0;

    if (list instanceof IntArrayList) {
      final IntArrayList primitiveList = (IntArrayList) list;
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeEnumSizeNoTag(primitiveList.getInt(i));
      }
    } else {
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeEnumSizeNoTag(list.get(i));
      }
    }
    return size;
  }

  static int computeSizeEnumList(int fieldNumber, List<Integer> list, boolean packed) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = computeSizeEnumListNoTag(list);

    if (packed) {
      return CodedOutputStream.computeTagSize(fieldNumber)
          + CodedOutputStream.computeLengthDelimitedFieldSize(size);
    } else {
      return size + (length * CodedOutputStream.computeTagSize(fieldNumber));
    }
  }

  static int computeSizeInt32ListNoTag(List<Integer> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }

    int size = 0;

    if (list instanceof IntArrayList) {
      final IntArrayList primitiveList = (IntArrayList) list;
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeInt32SizeNoTag(primitiveList.getInt(i));
      }
    } else {
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeInt32SizeNoTag(list.get(i));
      }
    }
    return size;
  }

  static int computeSizeInt32List(int fieldNumber, List<Integer> list, boolean packed) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = computeSizeInt32ListNoTag(list);

    if (packed) {
      return CodedOutputStream.computeTagSize(fieldNumber)
          + CodedOutputStream.computeLengthDelimitedFieldSize(size);
    } else {
      return size + (length * CodedOutputStream.computeTagSize(fieldNumber));
    }
  }

  static int computeSizeUInt32ListNoTag(List<Integer> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }

    int size = 0;

    if (list instanceof IntArrayList) {
      final IntArrayList primitiveList = (IntArrayList) list;
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeUInt32SizeNoTag(primitiveList.getInt(i));
      }
    } else {
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeUInt32SizeNoTag(list.get(i));
      }
    }
    return size;
  }

  static int computeSizeUInt32List(int fieldNumber, List<Integer> list, boolean packed) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = computeSizeUInt32ListNoTag(list);

    if (packed) {
      return CodedOutputStream.computeTagSize(fieldNumber)
          + CodedOutputStream.computeLengthDelimitedFieldSize(size);
    } else {
      return size + (length * CodedOutputStream.computeTagSize(fieldNumber));
    }
  }

  static int computeSizeSInt32ListNoTag(List<Integer> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }

    int size = 0;

    if (list instanceof IntArrayList) {
      final IntArrayList primitiveList = (IntArrayList) list;
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeSInt32SizeNoTag(primitiveList.getInt(i));
      }
    } else {
      for (int i = 0; i < length; i++) {
        size += CodedOutputStream.computeSInt32SizeNoTag(list.get(i));
      }
    }
    return size;
  }

  static int computeSizeSInt32List(int fieldNumber, List<Integer> list, boolean packed) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }

    int size = computeSizeSInt32ListNoTag(list);

    if (packed) {
      return CodedOutputStream.computeTagSize(fieldNumber)
          + CodedOutputStream.computeLengthDelimitedFieldSize(size);
    } else {
      return size + (length * CodedOutputStream.computeTagSize(fieldNumber));
    }
  }

  static int computeSizeFixed32ListNoTag(List<?> list) {
    return list.size() * WireFormat.FIXED32_SIZE;
  }

  static int computeSizeFixed32List(int fieldNumber, List<?> list, boolean packed) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    if (packed) {
      int dataSize = length * WireFormat.FIXED32_SIZE;
      return CodedOutputStream.computeTagSize(fieldNumber)
          + CodedOutputStream.computeLengthDelimitedFieldSize(dataSize);
    } else {
      return length * CodedOutputStream.computeFixed32Size(fieldNumber, 0);
    }
  }

  static int computeSizeFixed64ListNoTag(List<?> list) {
    return list.size() * WireFormat.FIXED64_SIZE;
  }

  static int computeSizeFixed64List(int fieldNumber, List<?> list, boolean packed) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    if (packed) {
      final int dataSize = length * WireFormat.FIXED64_SIZE;
      return CodedOutputStream.computeTagSize(fieldNumber)
          + CodedOutputStream.computeLengthDelimitedFieldSize(dataSize);
    } else {
      return length * CodedOutputStream.computeFixed64Size(fieldNumber, 0);
    }
  }

  static int computeSizeBoolListNoTag(List<?> list) {
    // bools are 1 byte varints
    return list.size();
  }

  static int computeSizeBoolList(int fieldNumber, List<?> list, boolean packed) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    if (packed) {
      // bools are 1 byte varints
      return CodedOutputStream.computeTagSize(fieldNumber)
          + CodedOutputStream.computeLengthDelimitedFieldSize(length);
    } else {
      return length * CodedOutputStream.computeBoolSize(fieldNumber, true);
    }
  }

  static int computeSizeStringList(int fieldNumber, List<?> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = length * CodedOutputStream.computeTagSize(fieldNumber);
    if (list instanceof LazyStringList) {
      LazyStringList lazyList = ((LazyStringList) list);
      for (int i = 0; i < length; i++) {
        Object value = lazyList.getRaw(i);
        if (value instanceof ByteString) {
          size += CodedOutputStream.computeBytesSizeNoTag((ByteString) value);
        } else {
          size += CodedOutputStream.computeStringSizeNoTag((String) value);
        }
      }
    } else {
      for (int i = 0; i < length; i++) {
        Object value = list.get(i);
        if (value instanceof ByteString) {
          size += CodedOutputStream.computeBytesSizeNoTag((ByteString) value);
        } else {
          size += CodedOutputStream.computeStringSizeNoTag((String) value);
        }
      }
    }
    return size;
  }

  static int computeSizeMessage(int fieldNumber, Object value, Schema schema) {
    if (value instanceof LazyFieldLite) {
      return CodedOutputStream.computeLazyFieldSize(fieldNumber, (LazyFieldLite) value);
    } else {
      return CodedOutputStream.computeMessageSize(fieldNumber, (MessageLite) value, schema);
    }
  }

  static int computeSizeMessageList(int fieldNumber, List<?> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = length * CodedOutputStream.computeTagSize(fieldNumber);
    for (int i = 0; i < length; i++) {
      Object value = list.get(i);
      if (value instanceof LazyFieldLite) {
        size += CodedOutputStream.computeLazyFieldSizeNoTag((LazyFieldLite) value);
      } else {
        size += CodedOutputStream.computeMessageSizeNoTag((MessageLite) value);
      }
    }
    return size;
  }

  static int computeSizeMessageList(int fieldNumber, List<?> list, Schema schema) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = length * CodedOutputStream.computeTagSize(fieldNumber);
    for (int i = 0; i < length; i++) {
      Object value = list.get(i);
      if (value instanceof LazyFieldLite) {
        size += CodedOutputStream.computeLazyFieldSizeNoTag((LazyFieldLite) value);
      } else {
        size += CodedOutputStream.computeMessageSizeNoTag((MessageLite) value, schema);
      }
    }
    return size;
  }

  static int computeSizeByteStringList(int fieldNumber, List<ByteString> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = length * CodedOutputStream.computeTagSize(fieldNumber);
    for (int i = 0; i < list.size(); i++) {
      size += CodedOutputStream.computeBytesSizeNoTag(list.get(i));
    }
    return size;
  }

  static int computeSizeGroupList(int fieldNumber, List<MessageLite> list) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = 0;
    for (int i = 0; i < length; i++) {
      size += CodedOutputStream.computeGroupSize(fieldNumber, list.get(i));
    }
    return size;
  }

  static int computeSizeGroupList(int fieldNumber, List<MessageLite> list, Schema schema) {
    final int length = list.size();
    if (length == 0) {
      return 0;
    }
    int size = 0;
    for (int i = 0; i < length; i++) {
      size += CodedOutputStream.computeGroupSize(fieldNumber, list.get(i), schema);
    }
    return size;
  }

  /**
   * Determines whether to issue tableswitch or lookupswitch for the mergeFrom method.
   *
   * @see #shouldUseTableSwitch(int, int, int)
   */
  public static boolean shouldUseTableSwitch(FieldInfo[] fields) {
    // Determine whether to issue a tableswitch or a lookupswitch
    // instruction.
    if (fields.length == 0) {
      return false;
    }

    int lo = fields[0].getFieldNumber();
    int hi = fields[fields.length - 1].getFieldNumber();
    return shouldUseTableSwitch(lo, hi, fields.length);
  }

  /**
   * Determines whether to issue tableswitch or lookupswitch for the mergeFrom method. This is based
   * on the <a href=
   * "http://hg.openjdk.java.net/jdk8/jdk8/langtools/file/30db5e0aaf83/src/share/classes/com/sun/tools/javac/jvm/Gen.java#l1159">
   * logic in the JDK</a>.
   *
   * @param lo the lowest fieldNumber contained within the message.
   * @param hi the highest fieldNumber contained within the message.
   * @param numFields the total number of fields in the message.
   * @return {@code true} if tableswitch should be used, rather than lookupswitch.
   */
  public static boolean shouldUseTableSwitch(int lo, int hi, int numFields) {
    if (hi < DEFAULT_LOOK_UP_START_NUMBER) {
      return true;
    }
    long tableSpaceCost = ((long) hi - lo + 1); // words
    long tableTimeCost = 3; // comparisons
    long lookupSpaceCost = 3 + 2 * (long) numFields;
    long lookupTimeCost = 3 + (long) numFields;
    return tableSpaceCost + 3 * tableTimeCost <= lookupSpaceCost + 3 * lookupTimeCost;
  }

  public static UnknownFieldSchema<?, ?> unknownFieldSetFullSchema() {
    return UNKNOWN_FIELD_SET_FULL_SCHEMA;
  }

  public static UnknownFieldSchema<?, ?> unknownFieldSetLiteSchema() {
    return UNKNOWN_FIELD_SET_LITE_SCHEMA;
  }

  private static UnknownFieldSchema<?, ?> getUnknownFieldSetSchema() {
    try {
      Class<?> clz = getUnknownFieldSetSchemaClass();
      if (clz == null) {
        return null;
      }
      return (UnknownFieldSchema) clz.getConstructor().newInstance();
    } catch (Throwable t) {
      return null;
    }
  }

  private static Class<?> getGeneratedMessageClass() {
    try {
      // TODO(b/248560713) decide if we're keeping support for Full in schema classes and handle
      // this better.
      return Class.forName("com.google.protobuf.GeneratedMessageV3");
    } catch (Throwable e) {
      return null;
    }
  }

  private static Class<?> getUnknownFieldSetSchemaClass() {
    try {
      return Class.forName("com.google.protobuf.UnknownFieldSetSchema");
    } catch (Throwable e) {
      return null;
    }
  }

  static Object getMapDefaultEntry(Class<?> clazz, String name) {
    try {
      Class<?> holder =
          Class.forName(clazz.getName() + "$" + toCamelCase(name, true) + "DefaultEntryHolder");
      Field[] fields = holder.getDeclaredFields();
      if (fields.length != 1) {
        throw new IllegalStateException(
            "Unable to look up map field default entry holder class for "
                + name
                + " in "
                + clazz.getName());
      }
      return UnsafeUtil.getStaticObject(fields[0]);
    } catch (Throwable t) {
      throw new RuntimeException(t);
    }
  }

  static String toCamelCase(String name, boolean capNext) {
    StringBuilder sb = new StringBuilder();
    for (int i = 0; i < name.length(); ++i) {
      char c = name.charAt(i);
      // Matches protoc field name function:
      if ('a' <= c && c <= 'z') {
        if (capNext) {
          sb.append((char) (c + ('A' - 'a')));
        } else {
          sb.append(c);
        }
        capNext = false;
      } else if ('A' <= c && c <= 'Z') {
        if (i == 0 && !capNext) {
          // Force first letter to lower-case unless explicitly told to capitalize it.
          sb.append((char) (c - ('A' - 'a')));
        } else {
          sb.append(c);
        }
        capNext = false;
      } else if ('0' <= c && c <= '9') {
        sb.append(c);
        capNext = true;
      } else {
        capNext = true;
      }
    }
    return sb.toString();
  }

  /** Returns true if both are null or both are {@link Object#equals}. */
  static boolean safeEquals(Object a, Object b) {
    return a == b || (a != null && a.equals(b));
  }

  static <T> void mergeMap(MapFieldSchema mapFieldSchema, T message, T o, long offset) {
    Object merged =
        mapFieldSchema.mergeFrom(
            UnsafeUtil.getObject(message, offset), UnsafeUtil.getObject(o, offset));
    UnsafeUtil.putObject(message, offset, merged);
  }

  static <T, FT extends FieldDescriptorLite<FT>> void mergeExtensions(
      ExtensionSchema<FT> schema, T message, T other) {
    FieldSet<FT> otherExtensions = schema.getExtensions(other);
    if (!otherExtensions.isEmpty()) {
      FieldSet<FT> messageExtensions = schema.getMutableExtensions(message);
      messageExtensions.mergeFrom(otherExtensions);
    }
  }

  static <T, UT, UB> void mergeUnknownFields(
      UnknownFieldSchema<UT, UB> schema, T message, T other) {
    UT messageUnknowns = schema.getFromMessage(message);
    UT otherUnknowns = schema.getFromMessage(other);
    UT merged = schema.merge(messageUnknowns, otherUnknowns);
    schema.setToMessage(message, merged);
  }

  /** Filters unrecognized enum values in a list. */
  @CanIgnoreReturnValue
  static <UT, UB> UB filterUnknownEnumList(
      Object containerMessage,
      int number,
      List<Integer> enumList,
      EnumLiteMap<?> enumMap,
      UB unknownFields,
      UnknownFieldSchema<UT, UB> unknownFieldSchema) {
    if (enumMap == null) {
      return unknownFields;
    }
    // TODO(dweis): Specialize for IntArrayList to avoid boxing.
    if (enumList instanceof RandomAccess) {
      int writePos = 0;
      int size = enumList.size();
      for (int readPos = 0; readPos < size; ++readPos) {
        int enumValue = enumList.get(readPos);
        if (enumMap.findValueByNumber(enumValue) != null) {
          if (readPos != writePos) {
            enumList.set(writePos, enumValue);
          }
          ++writePos;
        } else {
          unknownFields =
              storeUnknownEnum(
                  containerMessage, number, enumValue, unknownFields, unknownFieldSchema);
        }
      }
      if (writePos != size) {
        enumList.subList(writePos, size).clear();
      }
    } else {
      for (Iterator<Integer> it = enumList.iterator(); it.hasNext(); ) {
        int enumValue = it.next();
        if (enumMap.findValueByNumber(enumValue) == null) {
          unknownFields =
              storeUnknownEnum(
                  containerMessage, number, enumValue, unknownFields, unknownFieldSchema);
          it.remove();
        }
      }
    }
    return unknownFields;
  }

  /** Filters unrecognized enum values in a list. */
  @CanIgnoreReturnValue
  static <UT, UB> UB filterUnknownEnumList(
      Object containerMessage,
      int number,
      List<Integer> enumList,
      EnumVerifier enumVerifier,
      UB unknownFields,
      UnknownFieldSchema<UT, UB> unknownFieldSchema) {
    if (enumVerifier == null) {
      return unknownFields;
    }
    // TODO(dweis): Specialize for IntArrayList to avoid boxing.
    if (enumList instanceof RandomAccess) {
      int writePos = 0;
      int size = enumList.size();
      for (int readPos = 0; readPos < size; ++readPos) {
        int enumValue = enumList.get(readPos);
        if (enumVerifier.isInRange(enumValue)) {
          if (readPos != writePos) {
            enumList.set(writePos, enumValue);
          }
          ++writePos;
        } else {
          unknownFields =
              storeUnknownEnum(
                  containerMessage, number, enumValue, unknownFields, unknownFieldSchema);
        }
      }
      if (writePos != size) {
        enumList.subList(writePos, size).clear();
      }
    } else {
      for (Iterator<Integer> it = enumList.iterator(); it.hasNext(); ) {
        int enumValue = it.next();
        if (!enumVerifier.isInRange(enumValue)) {
          unknownFields =
              storeUnknownEnum(
                  containerMessage, number, enumValue, unknownFields, unknownFieldSchema);
          it.remove();
        }
      }
    }
    return unknownFields;
  }

  /** Stores an unrecognized enum value as an unknown value. */
  @CanIgnoreReturnValue
  static <UT, UB> UB storeUnknownEnum(
      Object containerMessage,
      int number,
      int enumValue,
      UB unknownFields,
      UnknownFieldSchema<UT, UB> unknownFieldSchema) {
    if (unknownFields == null) {
      unknownFields = unknownFieldSchema.getBuilderFromMessage(containerMessage);
    }
    unknownFieldSchema.addVarint(unknownFields, number, enumValue);
    return unknownFields;
  }
}
