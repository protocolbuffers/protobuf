// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * This class is used internally by the Protocol Buffer library and generated message
 * implementations. It is public only because those generated messages do not reside in the {@code
 * protobuf} package. Others should not use this class directly.
 *
 * <p>This class contains constants and helper functions useful for dealing with the Protocol Buffer
 * wire format.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class WireFormat {
  // Do not allow instantiation.
  private WireFormat() {}

  static final int FIXED32_SIZE = 4;
  static final int FIXED64_SIZE = 8;
  static final int MAX_VARINT32_SIZE = 5;
  static final int MAX_VARINT64_SIZE = 10;
  static final int MAX_VARINT_SIZE = 10;

  public static final int WIRETYPE_VARINT = 0;
  public static final int WIRETYPE_FIXED64 = 1;
  public static final int WIRETYPE_LENGTH_DELIMITED = 2;
  public static final int WIRETYPE_START_GROUP = 3;
  public static final int WIRETYPE_END_GROUP = 4;
  public static final int WIRETYPE_FIXED32 = 5;

  static final int TAG_TYPE_BITS = 3;
  static final int TAG_TYPE_MASK = (1 << TAG_TYPE_BITS) - 1;

  /** Given a tag value, determines the wire type (the lower 3 bits). */
  public static int getTagWireType(final int tag) {
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

  /**
   * Lite equivalent to {@link Descriptors.FieldDescriptor.JavaType}. This is only here to support
   * the lite runtime and should not be used by users.
   */
  public enum JavaType {
    INT(0),
    LONG(0L),
    FLOAT(0F),
    DOUBLE(0D),
    BOOLEAN(false),
    STRING(""),
    BYTE_STRING(ByteString.EMPTY),
    ENUM(null),
    MESSAGE(null);

    JavaType(final Object defaultDefault) {
      this.defaultDefault = defaultDefault;
    }

    /** The default default value for fields of this type, if it's a primitive type. */
    Object getDefaultDefault() {
      return defaultDefault;
    }

    private final Object defaultDefault;
  }

  /**
   * Lite equivalent to {@link Descriptors.FieldDescriptor.Type}. This is only here to support the
   * lite runtime and should not be used by users.
   */
  public enum FieldType {
    DOUBLE(JavaType.DOUBLE, WIRETYPE_FIXED64),
    FLOAT(JavaType.FLOAT, WIRETYPE_FIXED32),
    INT64(JavaType.LONG, WIRETYPE_VARINT),
    UINT64(JavaType.LONG, WIRETYPE_VARINT),
    INT32(JavaType.INT, WIRETYPE_VARINT),
    FIXED64(JavaType.LONG, WIRETYPE_FIXED64),
    FIXED32(JavaType.INT, WIRETYPE_FIXED32),
    BOOL(JavaType.BOOLEAN, WIRETYPE_VARINT),
    STRING(JavaType.STRING, WIRETYPE_LENGTH_DELIMITED) {
      @Override
      public boolean isPackable() {
        return false;
      }
    },
    GROUP(JavaType.MESSAGE, WIRETYPE_START_GROUP) {
      @Override
      public boolean isPackable() {
        return false;
      }
    },
    MESSAGE(JavaType.MESSAGE, WIRETYPE_LENGTH_DELIMITED) {
      @Override
      public boolean isPackable() {
        return false;
      }
    },
    BYTES(JavaType.BYTE_STRING, WIRETYPE_LENGTH_DELIMITED) {
      @Override
      public boolean isPackable() {
        return false;
      }
    },
    UINT32(JavaType.INT, WIRETYPE_VARINT),
    ENUM(JavaType.ENUM, WIRETYPE_VARINT),
    SFIXED32(JavaType.INT, WIRETYPE_FIXED32),
    SFIXED64(JavaType.LONG, WIRETYPE_FIXED64),
    SINT32(JavaType.INT, WIRETYPE_VARINT),
    SINT64(JavaType.LONG, WIRETYPE_VARINT);

    FieldType(final JavaType javaType, final int wireType) {
      this.javaType = javaType;
      this.wireType = wireType;
    }

    private final JavaType javaType;
    private final int wireType;

    public JavaType getJavaType() {
      return javaType;
    }

    public int getWireType() {
      return wireType;
    }

    public boolean isPackable() {
      return true;
    }
  }

  // Field numbers for fields in MessageSet wire format.
  static final int MESSAGE_SET_ITEM = 1;
  static final int MESSAGE_SET_TYPE_ID = 2;
  static final int MESSAGE_SET_MESSAGE = 3;

  // Tag numbers.
  static final int MESSAGE_SET_ITEM_TAG = makeTag(MESSAGE_SET_ITEM, WIRETYPE_START_GROUP);
  static final int MESSAGE_SET_ITEM_END_TAG = makeTag(MESSAGE_SET_ITEM, WIRETYPE_END_GROUP);
  static final int MESSAGE_SET_TYPE_ID_TAG = makeTag(MESSAGE_SET_TYPE_ID, WIRETYPE_VARINT);
  static final int MESSAGE_SET_MESSAGE_TAG =
      makeTag(MESSAGE_SET_MESSAGE, WIRETYPE_LENGTH_DELIMITED);

  /**
   * Validation level for handling incoming string field data which potentially contain non-UTF8
   * bytes.
   */
  enum Utf8Validation {
    /** Eagerly parses to String; silently accepts invalid UTF8 bytes. */
    LOOSE,
    /** Eagerly parses to String; throws an IOException on invalid bytes. */
    STRICT,
    /** Keep data as ByteString; validation/conversion to String is lazy. */
    LAZY;
  }
}
