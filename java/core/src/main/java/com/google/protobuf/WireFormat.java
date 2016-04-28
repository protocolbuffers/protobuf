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
public final class WireFormat {
  // Do not allow instantiation.
  private WireFormat() {}

  public static final int WIRETYPE_VARINT           = 0;
  public static final int WIRETYPE_FIXED64          = 1;
  public static final int WIRETYPE_LENGTH_DELIMITED = 2;
  public static final int WIRETYPE_START_GROUP      = 3;
  public static final int WIRETYPE_END_GROUP        = 4;
  public static final int WIRETYPE_FIXED32          = 5;

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
   * Lite equivalent to {@link Descriptors.FieldDescriptor.JavaType}.  This is
   * only here to support the lite runtime and should not be used by users.
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

    /**
     * The default default value for fields of this type, if it's a primitive
     * type.
     */
    Object getDefaultDefault() {
      return defaultDefault;
    }

    private final Object defaultDefault;
  }

  /**
   * Lite equivalent to {@link Descriptors.FieldDescriptor.Type}.  This is
   * only here to support the lite runtime and should not be used by users.
   */
  public enum FieldType {
    DOUBLE  (JavaType.DOUBLE     , WIRETYPE_FIXED64         ),
    FLOAT   (JavaType.FLOAT      , WIRETYPE_FIXED32         ),
    INT64   (JavaType.LONG       , WIRETYPE_VARINT          ),
    UINT64  (JavaType.LONG       , WIRETYPE_VARINT          ),
    INT32   (JavaType.INT        , WIRETYPE_VARINT          ),
    FIXED64 (JavaType.LONG       , WIRETYPE_FIXED64         ),
    FIXED32 (JavaType.INT        , WIRETYPE_FIXED32         ),
    BOOL    (JavaType.BOOLEAN    , WIRETYPE_VARINT          ),
    STRING  (JavaType.STRING     , WIRETYPE_LENGTH_DELIMITED) {
      @Override
      public boolean isPackable() {
        return false; }
    },
    GROUP   (JavaType.MESSAGE    , WIRETYPE_START_GROUP     ) {
      @Override
      public boolean isPackable() {
        return false; }
    },
    MESSAGE (JavaType.MESSAGE    , WIRETYPE_LENGTH_DELIMITED) {
      @Override
      public boolean isPackable() {
        return false; }
    },
    BYTES   (JavaType.BYTE_STRING, WIRETYPE_LENGTH_DELIMITED) {
      @Override
      public boolean isPackable() {
        return false; }
    },
    UINT32  (JavaType.INT        , WIRETYPE_VARINT          ),
    ENUM    (JavaType.ENUM       , WIRETYPE_VARINT          ),
    SFIXED32(JavaType.INT        , WIRETYPE_FIXED32         ),
    SFIXED64(JavaType.LONG       , WIRETYPE_FIXED64         ),
    SINT32  (JavaType.INT        , WIRETYPE_VARINT          ),
    SINT64  (JavaType.LONG       , WIRETYPE_VARINT          );

    FieldType(final JavaType javaType, final int wireType) {
      this.javaType = javaType;
      this.wireType = wireType;
    }

    private final JavaType javaType;
    private final int wireType;

    public JavaType getJavaType() { return javaType; }
    public int getWireType() { return wireType; }

    public boolean isPackable() { return true; }
  }

  // Field numbers for fields in MessageSet wire format.
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

  /**
   * Validation level for handling incoming string field data which potentially
   * contain non-UTF8 bytes.
   */
  enum Utf8Validation {
    /** Eagerly parses to String; silently accepts invalid UTF8 bytes. */
    LOOSE {
      @Override
      Object readString(CodedInputStream input) throws IOException {
        return input.readString();
      }
    },
    /** Eagerly parses to String; throws an IOException on invalid bytes. */
    STRICT {
      @Override
      Object readString(CodedInputStream input) throws IOException {
        return input.readStringRequireUtf8();
      }
    },
    /** Keep data as ByteString; validation/conversion to String is lazy. */
    LAZY {
      @Override
      Object readString(CodedInputStream input) throws IOException {
        return input.readBytes();
      }
    };

    /** Read a string field from the input with the proper UTF8 validation. */
    abstract Object readString(CodedInputStream input) throws IOException;
  }

  /**
   * Read a field of any primitive type for immutable messages from a
   * CodedInputStream. Enums, groups, and embedded messages are not handled by
   * this method.
   *
   * @param input The stream from which to read.
   * @param type Declared type of the field.
   * @param utf8Validation Different string UTF8 validation level for handling
   *                       string fields.
   * @return An object representing the field's value, of the exact
   *         type which would be returned by
   *         {@link Message#getField(Descriptors.FieldDescriptor)} for
   *         this field.
   */
  static Object readPrimitiveField(
      CodedInputStream input,
      FieldType type,
      Utf8Validation utf8Validation) throws IOException {
    switch (type) {
      case DOUBLE  : return input.readDouble  ();
      case FLOAT   : return input.readFloat   ();
      case INT64   : return input.readInt64   ();
      case UINT64  : return input.readUInt64  ();
      case INT32   : return input.readInt32   ();
      case FIXED64 : return input.readFixed64 ();
      case FIXED32 : return input.readFixed32 ();
      case BOOL    : return input.readBool    ();
      case BYTES   : return input.readBytes   ();
      case UINT32  : return input.readUInt32  ();
      case SFIXED32: return input.readSFixed32();
      case SFIXED64: return input.readSFixed64();
      case SINT32  : return input.readSInt32  ();
      case SINT64  : return input.readSInt64  ();

      case STRING  : return utf8Validation.readString(input);
      case GROUP:
        throw new IllegalArgumentException(
          "readPrimitiveField() cannot handle nested groups.");
      case MESSAGE:
        throw new IllegalArgumentException(
          "readPrimitiveField() cannot handle embedded messages.");
      case ENUM:
        // We don't handle enums because we don't know what to do if the
        // value is not recognized.
        throw new IllegalArgumentException(
          "readPrimitiveField() cannot handle enums.");
    }

    throw new RuntimeException(
      "There is no way to get here, but the compiler thinks otherwise.");
  }
}
