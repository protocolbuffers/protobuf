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

import com.google.protobuf.Internal.ProtobufList;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** Helper methods used by schemas. */
@ExperimentalApi
public final class SchemaUtil {
  private static final Class<?> UNMODIFIABLE_LIST_CLASS =
      Collections.unmodifiableList(new ArrayList<Integer>()).getClass();
  private static final Class<?> ABSTRACT_MESSAGE_CLASS = getAbstractMessageClass();
  private static final Class<?> GENERATED_MESSAGE_CLASS = getGeneratedMessageClass();
  private static final Class<?> UNKNOWN_FIELD_SET_CLASS = getUnknownFieldSetClass();
  private static final long UNKNOWN_FIELD_OFFSET = unknownFieldOffset();
  private static final long LITE_UNKNOWN_FIELD_OFFSET = unknownFieldLiteOffset();
  private static final Object DEFAULT_UNKNOWN_FIELD_SET = defaultUnknownFieldSet();
  private static final long MEMOIZED_SIZE_FIELD_OFFSET = getMemoizedSizeFieldOffset();
  private static final long LITE_MEMOIZED_SIZE_FIELD_OFFSET = getLiteMemoizedSizeFieldOffset();

  private SchemaUtil() {}

  /**
   * Initializes all of the base class fields for the given {@link GeneratedMessageLite} to their
   * default values.
   */
  public static void initLiteBaseClassFields(Object msg) {
    UnsafeUtil.putObject(msg, LITE_UNKNOWN_FIELD_OFFSET, UnknownFieldSetLite.getDefaultInstance());
    UnsafeUtil.putInt(msg, LITE_MEMOIZED_SIZE_FIELD_OFFSET, -1);
  }

  /**
   * Initializes all of the base class fields for the given {@link
   * com.google.protobuf.GeneratedMessage} to their default values.
   */
  public static void initBaseClassFields(Object msg) {
    UnsafeUtil.putObject(msg, UNKNOWN_FIELD_OFFSET, DEFAULT_UNKNOWN_FIELD_SET);
    UnsafeUtil.putInt(msg, MEMOIZED_SIZE_FIELD_OFFSET, -1);
  }

  /**
   * Requires that the given message extend {@link com.google.protobuf.GeneratedMessage} or {@link
   * GeneratedMessageLite}.
   */
  public static void requireGeneratedMessage(Class<?> messageType) {
    if (!GeneratedMessageLite.class.isAssignableFrom(messageType)
        && !GENERATED_MESSAGE_CLASS.isAssignableFrom(messageType)) {
      throw new IllegalArgumentException(
          "Message classes must extend GeneratedMessage or GeneratedMessageLite");
    }
  }

  /** Initializes the message's memoized size to the default value. */
  public static void initMemoizedSize(Object msg) {
    final long fieldOffset =
        (msg instanceof GeneratedMessageLite)
            ? LITE_MEMOIZED_SIZE_FIELD_OFFSET
            : MEMOIZED_SIZE_FIELD_OFFSET;

    if (fieldOffset < 0) {
      throw new IllegalArgumentException(
          "Unable to identify memoizedSize field offset for message of type: "
              + msg.getClass().getName());
    }

    UnsafeUtil.putInt(msg, fieldOffset, -1);
  }

  public static void writeDouble(int fieldNumber, double value, Writer writer) {
    if (Double.compare(value, 0.0) != 0) {
      writer.writeDouble(fieldNumber, value);
    }
  }

  public static void writeFloat(int fieldNumber, float value, Writer writer) {
    if (Float.compare(value, 0.0f) != 0) {
      writer.writeFloat(fieldNumber, value);
    }
  }

  public static void writeInt64(int fieldNumber, long value, Writer writer) {
    if (value != 0) {
      writer.writeInt64(fieldNumber, value);
    }
  }

  public static void writeUInt64(int fieldNumber, long value, Writer writer) {
    if (value != 0) {
      writer.writeUInt64(fieldNumber, value);
    }
  }

  public static void writeSInt64(int fieldNumber, long value, Writer writer) {
    if (value != 0) {
      writer.writeSInt64(fieldNumber, value);
    }
  }

  public static void writeFixed64(int fieldNumber, long value, Writer writer) {
    if (value != 0) {
      writer.writeFixed64(fieldNumber, value);
    }
  }

  public static void writeSFixed64(int fieldNumber, long value, Writer writer) {
    if (value != 0) {
      writer.writeSFixed64(fieldNumber, value);
    }
  }

  public static void writeInt32(int fieldNumber, int value, Writer writer) {
    if (value != 0) {
      writer.writeInt32(fieldNumber, value);
    }
  }

  public static void writeUInt32(int fieldNumber, int value, Writer writer) {
    if (value != 0) {
      writer.writeUInt32(fieldNumber, value);
    }
  }

  public static void writeSInt32(int fieldNumber, int value, Writer writer) {
    if (value != 0) {
      writer.writeSInt32(fieldNumber, value);
    }
  }

  public static void writeFixed32(int fieldNumber, int value, Writer writer) {
    if (value != 0) {
      writer.writeFixed32(fieldNumber, value);
    }
  }

  public static void writeSFixed32(int fieldNumber, int value, Writer writer) {
    if (value != 0) {
      writer.writeSFixed32(fieldNumber, value);
    }
  }

  public static void writeEnum(int fieldNumber, int value, Writer writer) {
    if (value != 0) {
      writer.writeEnum(fieldNumber, value);
    }
  }

  public static void writeBool(int fieldNumber, boolean value, Writer writer) {
    if (value) {
      writer.writeBool(fieldNumber, true);
    }
  }

  public static void writeString(int fieldNumber, Object value, Writer writer) {
    if (value instanceof String) {
      writeStringInternal(fieldNumber, (String) value, writer);
    } else {
      writeBytes(fieldNumber, (ByteString) value, writer);
    }
  }

  private static void writeStringInternal(int fieldNumber, String value, Writer writer) {
    if (value != null && !value.isEmpty()) {
      writer.writeString(fieldNumber, value);
    }
  }

  public static void writeBytes(int fieldNumber, ByteString value, Writer writer) {
    if (value != null && !value.isEmpty()) {
      writer.writeBytes(fieldNumber, value);
    }
  }

  public static void writeMessage(int fieldNumber, Object value, Writer writer) {
    if (value != null) {
      writer.writeMessage(fieldNumber, value);
    }
  }

  public static void writeDoubleList(
      int fieldNumber, List<Double> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeDoubleList(fieldNumber, value, packed);
    }
  }

  public static void writeFloatList(
      int fieldNumber, List<Float> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeFloatList(fieldNumber, value, packed);
    }
  }

  public static void writeInt64List(
      int fieldNumber, List<Long> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeInt64List(fieldNumber, value, packed);
    }
  }

  public static void writeUInt64List(
      int fieldNumber, List<Long> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeUInt64List(fieldNumber, value, packed);
    }
  }

  public static void writeSInt64List(
      int fieldNumber, List<Long> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeSInt64List(fieldNumber, value, packed);
    }
  }

  public static void writeFixed64List(
      int fieldNumber, List<Long> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeFixed64List(fieldNumber, value, packed);
    }
  }

  public static void writeSFixed64List(
      int fieldNumber, List<Long> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeSFixed64List(fieldNumber, value, packed);
    }
  }

  public static void writeInt32List(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeInt32List(fieldNumber, value, packed);
    }
  }

  public static void writeUInt32List(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeUInt32List(fieldNumber, value, packed);
    }
  }

  public static void writeSInt32List(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeSInt32List(fieldNumber, value, packed);
    }
  }

  public static void writeFixed32List(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeFixed32List(fieldNumber, value, packed);
    }
  }

  public static void writeSFixed32List(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeSFixed32List(fieldNumber, value, packed);
    }
  }

  public static void writeEnumList(
      int fieldNumber, List<Integer> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeEnumList(fieldNumber, value, packed);
    }
  }

  public static void writeBoolList(
      int fieldNumber, List<Boolean> value, Writer writer, boolean packed) {
    if (value != null && !value.isEmpty()) {
      writer.writeBoolList(fieldNumber, value, packed);
    }
  }

  public static void writeStringList(int fieldNumber, List<String> value, Writer writer) {
    if (value != null && !value.isEmpty()) {
      writer.writeStringList(fieldNumber, value);
    }
  }

  public static void writeBytesList(int fieldNumber, List<ByteString> value, Writer writer) {
    if (value != null && !value.isEmpty()) {
      writer.writeBytesList(fieldNumber, value);
    }
  }

  public static void writeMessageList(int fieldNumber, List<?> value, Writer writer) {
    if (value != null && !value.isEmpty()) {
      writer.writeMessageList(fieldNumber, value);
    }
  }

  public static void writeGroupList(int fieldNumber, List<?> value, Writer writer) {
    if (value != null && !value.isEmpty()) {
      writer.writeGroupList(fieldNumber, value);
    }
  }

  /**
   * Used for protobuf-lite (i.e. messages that extend {@link GeneratedMessageLite}). Reads in a
   * list of messages into a target {@link ProtobufList} field in the message.
   */
  public static final <T> void readProtobufMessageList(
      Object message, long offset, Reader reader, Class<T> targetType) throws IOException {
    reader.readMessageList(SchemaUtil.<T>mutableProtobufListAt(message, offset), targetType);
  }

  /**
   * Used for standard protobuf messages (i.e. messages that extend {@link
   * com.google.protobuf.GeneratedMessage}). Reads in a list of messages into a target list field in
   * the message.
   */
  public static <T> void readMessageList(
      Object message, long offset, Reader reader, Class<T> targetType) throws IOException {
    reader.readMessageList(SchemaUtil.<T>mutableListAt(message, offset), targetType);
  }

  /**
   * Used for group types. Reads in a list of group messages into a target list field in the
   * message.
   */
  public static <T> void readGroupList(
      Object message, long offset, Reader reader, Class<T> targetType) throws IOException {
    reader.readGroupList(SchemaUtil.<T>mutableListAt(message, offset), targetType);
  }

  /**
   * Used for protobuf-lite (i.e. messages that extend GeneratedMessageLite). Converts the {@link
   * ProtobufList} at the given field position in the message into a mutable list (if it isn't
   * already).
   */
  @SuppressWarnings("unchecked")
  public static <L> ProtobufList<L> mutableProtobufListAt(Object message, long pos) {
    ProtobufList<L> list = (ProtobufList<L>) UnsafeUtil.getObject(message, pos);
    if (!list.isModifiable()) {
      int size = list.size();
      list = list.mutableCopyWithCapacity(
          size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
      UnsafeUtil.putObject(message, pos, list);
    }
    return list;
  }

  /** Converts the list at the given field location in the message into a mutable list. */
  @SuppressWarnings("unchecked")
  public static <L> List<L> mutableListAt(Object message, long pos) {
    List<L> list = (List<L>) UnsafeUtil.getObject(message, pos);
    if (list.isEmpty()) {
      list =
          (list instanceof LazyStringList)
              ? (List<L>) new LazyStringArrayList()
              : new ArrayList<L>();
      UnsafeUtil.putObject(message, pos, list);
    } else if (UNMODIFIABLE_LIST_CLASS.isAssignableFrom(list.getClass())) {
      list = new ArrayList<L>(list);
      UnsafeUtil.putObject(message, pos, list);
    } else if (list instanceof UnmodifiableLazyStringList) {
      // Convert the list to a mutable list.
      list = (List<L>) new LazyStringArrayList((List<String>) list);
      UnsafeUtil.putObject(message, pos, list);
    }
    return list;
  }

  /**
   * Determines whether to issue tableswitch or lookupswitch for the mergeFrom method.
   *
   * @see #shouldUseTableSwitch(int, int, int)
   */
  public static boolean shouldUseTableSwitch(List<FieldInfo> fields) {
    // Determine whether to issue a tableswitch or a lookupswitch
    // instruction.
    if (fields.isEmpty()) {
      return false;
    }

    int lo = fields.get(0).getFieldNumber();
    int hi = fields.get(fields.size() - 1).getFieldNumber();
    return shouldUseTableSwitch(lo, hi, fields.size());
  }

  /**
   * Determines whether to issue tableswitch or lookupswitch for the mergeFrom method. This is based
   * on the <a href=
   * "http://hg.openjdk.java.net/jdk8/jdk8/langtools/file/30db5e0aaf83/src/share/classes/com/sun/tools/javac/jvm/Gen.java#l1159">
   * logic in the JDK</a>.
   *
   * @param lo the lowest fieldNumber contained within the message.
   * @param hi the higest fieldNumber contained within the message.
   * @param numFields the total number of fields in the message.
   * @return {@code true} if tableswitch should be used, rather than lookupswitch.
   */
  public static boolean shouldUseTableSwitch(int lo, int hi, int numFields) {
    long tableSpaceCost = 4 + ((long) hi - lo + 1); // words
    long tableTimeCost = 3; // comparisons
    long lookupSpaceCost = 3 + 2 * (long) numFields;
    long lookupTimeCost = numFields;
    return tableSpaceCost + 3 * tableTimeCost <= lookupSpaceCost + 3 * lookupTimeCost;
  }

  private static Class<?> getAbstractMessageClass() {
    try {
      return Class.forName("com.google.protobuf.AbstractMessage");
    } catch (Throwable e) {
      return null;
    }
  }

  private static Class<?> getGeneratedMessageClass() {
    try {
      return Class.forName("com.google.protobuf.GeneratedMessage");
    } catch (Throwable e) {
      return null;
    }
  }

  private static Class<?> getUnknownFieldSetClass() {
    try {
      return Class.forName("com.google.protobuf.UnknownFieldSet");
    } catch (Throwable e) {
      return null;
    }
  }

  private static long unknownFieldOffset() {
    try {
      if (GENERATED_MESSAGE_CLASS != null) {
        Field field = GENERATED_MESSAGE_CLASS.getDeclaredField("unknownFields");
        return UnsafeUtil.objectFieldOffset(field);
      }
    } catch (Throwable e) {
      // Do nothing.
    }
    return -1;
  }

  private static long unknownFieldLiteOffset() {
    try {
      Field field = GeneratedMessageLite.class.getDeclaredField("unknownFields");
      return UnsafeUtil.objectFieldOffset(field);
    } catch (Throwable e) {
      // Do nothing.
    }
    return -1;
  }

  private static Object defaultUnknownFieldSet() {
    try {
      if (UNKNOWN_FIELD_SET_CLASS != null) {
        Method method = UNKNOWN_FIELD_SET_CLASS.getDeclaredMethod("getDefaultInstance");
        return method.invoke(null);
      }
    } catch (Throwable e) {
      // Do nothing.
    }
    return null;
  }

  private static long getMemoizedSizeFieldOffset() {
    try {
      if (ABSTRACT_MESSAGE_CLASS != null) {
        Field field = ABSTRACT_MESSAGE_CLASS.getDeclaredField("memoizedSize");
        return UnsafeUtil.objectFieldOffset(field);
      }
    } catch (Throwable e) {
      // Do nothing.
    }
    return -1;
  }

  private static long getLiteMemoizedSizeFieldOffset() {
    try {
      Field field = GeneratedMessageLite.class.getDeclaredField("memoizedSerializedSize");
      return UnsafeUtil.objectFieldOffset(field);
    } catch (Throwable e) {
      // Do nothing.
    }
    return -1;
  }
}
